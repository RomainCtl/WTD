#include <signal.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <jsoncpp/json/json.h>
#include <thread>
#include <mutex>
#include <regex>
#include <future>

#define N_CHAR 1024UL
// To delimite each socket msgs
const std::string MSG_DELIMITER = "$";

// Player structure definition
struct Player {
    unsigned int id;
    std::string name;
    unsigned int nb_objects_found;
};

// Game status
enum Status {
    WAITING, // waiting players (client connection authorized)
    STARTING, // to send start msg to each player
    IN_PROGRESS, // game in progress (client can not initialize new connection)
    COMPLETED // we have a winner ! (reload all data)
};

// function definition
void stop_server();
void close_sockets();
void end(Player *winner);
void deal_with_socket(u_int16_t port, std::future<void> exit_signal);
void deal_with_client(int socket, unsigned int id, std::future<void> exit_signal);
void deal_with_game(std::future<void> exit_signal);


/*******************************
 * Global variables definitions
*******************************/
std::mutex mtx_main; // to allow passive lock for main
std::thread connection_dealer;
std::thread game_dealer;
std::promise<void> connection_dealer_exit_signal;
std::promise<void> game_dealer_exit_signal;

// Clients list
std::map<unsigned int, std::tuple<std::thread, Player, std::promise<void> > > clients;
std::vector<unsigned int> client_to_remove; // thread can not remove itself
std::vector<unsigned int> new_player_has_join;
std::vector<std::pair<unsigned int, unsigned int> > player_find_object;
std::mutex mtx_clients;

// Game status
Status current_status = COMPLETED;
std::mutex mtx_status;

// Who is the room leader (only he can start the game)
int leader = -1;
std::mutex mtx_leader;

// Config file
Json::Value config;
std::mutex mtx_config;



/**
 * Catch kill signal
 *
 * @param s signal
 */
void exit_handler(int s) {
    std::cout << "Caught signal " << s << std::endl;
    stop_server();
    std::cout << "Shutdown server..." << std::endl;
    exit(EXIT_SUCCESS);
}

/**
 * Stop the server and terminate threads
 */
void stop_server() {
    connection_dealer_exit_signal.set_value();
    game_dealer_exit_signal.set_value();
    connection_dealer.detach();
    game_dealer.detach();
    close_sockets();
}

/**
 * Close all clients connections
 */
void close_sockets() {
    mtx_clients.lock();
    for (auto &i : clients) {
        std::get<2>(i.second).set_value(); // send exit
        close(std::get<1>(i.second).id); // stop socket (from id)
    }
    for (auto &i : clients) {
        std::get<0>(i.second).detach(); // stop thread
    }
    clients.clear(); // clear player list
    client_to_remove.clear();
    mtx_clients.unlock();
}

/**
 * Tell to each client that a player win, and reload data
 *
 * @param winner the winner
 */
void end(Player *winner) {
    mtx_status.lock();
    current_status = COMPLETED;
    mtx_status.unlock();

    // Announce
    std::string msg = "WIN=";
    msg += std::to_string(winner->id);
    msg += MSG_DELIMITER;

    // Server
    std::cout << msg << std::endl;

    // for each player
    mtx_clients.lock();
    for (auto &i : clients) {
        send((int) std::get<1>(i.second).id, msg.c_str(), msg.length(), 0);
    }
    mtx_clients.unlock();

    close_sockets();

    mtx_status.lock();
    current_status = WAITING;
    mtx_status.unlock();
}

/**
 * Main program function
 *
 * @param argc number of arguments
 * @param argv pointer to the first element of an array (arguments list)
 */
int main(int argc, char* argv[]) {
    // create handler
    signal(SIGTERM, exit_handler);
    signal(SIGINT, exit_handler);

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << "<port> <json_config_file>" << std::endl;
        return EXIT_FAILURE;
    }

    uint16_t port = (u_int16_t) atoi(argv[1]);
    std::ifstream config_file(argv[2]);

    Json::CharReaderBuilder rbuilder;
    std::string errs;

    // Read config file
    mtx_config.lock();
    bool result = Json::parseFromStream(rbuilder, config_file, &config, &errs);
    mtx_config.unlock();

    if (!result) {
        std::cerr << "Failed to load configuration '" << argv[2] << "'\n" << errs;
        return EXIT_FAILURE;
    }

    // Display Server name
    mtx_config.lock();
    std::cout << "Server: [" << config["name"].asString() << "] loaded..." << std::endl;
    mtx_config.unlock();

    // Start the connection catch thread
    connection_dealer = std::thread(deal_with_socket, port, std::move(connection_dealer_exit_signal.get_future()));

    // Start the game dealer
    game_dealer = std::thread(deal_with_game, std::move(game_dealer_exit_signal.get_future()));

    // infinite loop, the server will shutdown on kill (or Ctrl-C)
    while (true) {
        mtx_main.lock(); // PASSIVE LOCK
    }
}

/**
 * Thread that will wait forever for new connections
 *
 * @param port for socket
 */
void deal_with_socket(uint16_t port, std::future<void> exit_signal) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor  (IPv4 protocol, TCP : reliable &  connection oriented)
    if ( (server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0 ) {
        std::cerr << "Socket failed " << __FILE__ << " " << __LINE__ << std::endl;
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port <port>
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "setsockopt" << __FILE__ << " " << __LINE__ << std::endl;
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET; // IPv4 protocol
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );

    // Binds the socket to the address and port number
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))) {
        std::cerr << "Bind failed " << __FILE__ << " " << __LINE__ << std::endl;
        exit(EXIT_FAILURE);
    }

    // Puts the server socket in a passive mode (it waits for the client to approach the server to make a connection)
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen " << __FILE__ << " " << __LINE__ << std::endl;
        exit(EXIT_FAILURE);
    }

    // Get max_player from config
    mtx_config.lock();
    unsigned int max_player = config["max_player"].asUInt();
    mtx_config.unlock();

    unsigned int next_id;

    // Let's change current status
    mtx_status.lock();
    current_status = WAITING;
    mtx_status.unlock();

    while(exit_signal.wait_for(std::chrono::nanoseconds(1)) == std::future_status::timeout) {
        int new_socket;

        // A new client connection... PASSIVE WAIT
        if ( (new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept " << __FILE__ << " " << __LINE__ << std::endl;
            exit(EXIT_FAILURE);
        }

        if (clients.size() < max_player && current_status == WAITING) {
            next_id = (unsigned int) new_socket;
            mtx_clients.lock();
            if (clients.empty()) {
                mtx_clients.unlock();

                // set leader
                mtx_leader.lock();
                leader = new_socket;
                mtx_leader.unlock();

                mtx_clients.lock();
            }

            std::cout << "Client " << next_id << " connected"<<std::endl;

            Player p = {next_id, "", 0};
            std::promise<void> exitSignal;
            clients[next_id] = std::make_tuple(
                std::thread(deal_with_client, new_socket, next_id, std::move(exitSignal.get_future())),
                p,
                std::move(exitSignal)
            );
            mtx_clients.unlock();
        } else {
            std::cout << "out" << std::endl;
            close(new_socket);
            // Maximum number of players already reached
            // Or game already in progress
        }

        mtx_main.unlock(); // tells main about a new connection...
    }
}

/**
 * Thread that will manage the game (new player, send data to all clients...)
 */
void deal_with_game(std::future<void> exit_signal) {
    while (exit_signal.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout) {
        // remove lost clients
        if (client_to_remove.size() > 0) {
            mtx_clients.lock();
            for(auto &i : client_to_remove) {
                std::map<unsigned int, std::tuple<std::thread, Player, std::promise<void>> >::iterator it = clients.find(i);
                std::get<2>(it->second).set_value();
                std::get<0>(it->second).detach(); // stop thread
                clients.erase( it );
            }
            // Tell to other player
            for(auto &i : client_to_remove) {
                std::string msg = "PLAYERLEFT=";
                msg += std::to_string(i);
                msg += MSG_DELIMITER;
                for (auto &c : clients) {
                    send((int) std::get<1>(c.second).id, msg.c_str(), msg.length(), 0);
                }
            }
            client_to_remove.clear();

            // Reset status if all players have left
            if (clients.empty()) {
                mtx_clients.unlock();
                mtx_status.lock();
                current_status = WAITING;
                mtx_status.unlock();
            } else {
                mtx_clients.unlock();
            }
        }

        // Send start msg to all players
        if (current_status == STARTING) {
            std::string msg = "START";
            msg += MSG_DELIMITER;
            mtx_clients.lock();
            for (auto &i : clients) {
                send((int) std::get<1>(i.second).id, msg.c_str(), msg.length(), 0);
            }
            mtx_clients.unlock();
            std::cout << msg << std::endl; // server

            mtx_status.lock();
            current_status = IN_PROGRESS;
            mtx_status.unlock();
        }

        // Send new player msg to all players
        if (new_player_has_join.size() > 0) {
            std::string msg;
            mtx_clients.lock();
            for (auto &i : new_player_has_join) {
                std::map<unsigned int, std::tuple<std::thread, Player, std::promise<void>> >::iterator it = clients.find(i);
                msg = "PLAYER=";
                msg += std::to_string( std::get<1>(it->second).id ) + ":";
                msg += std::get<1>(it->second).name + ":";
                msg += std::to_string( std::get<1>(it->second).nb_objects_found );
                msg += MSG_DELIMITER;

                for (auto &i : clients) {
                    if (std::get<1>(i.second).id != std::get<1>(it->second).id)
                        send((int) std::get<1>(i.second).id, msg.c_str(), msg.length(), 0);
                }
            }
            new_player_has_join.clear();
            mtx_clients.unlock();
        }

        // Send player find object msg to all players
        if (player_find_object.size() > 0) {
            mtx_config.lock();
            int nb_object = config["objects"].size();
            mtx_config.unlock();

            std::string msg;
            mtx_clients.lock();
            for (auto &i : player_find_object) {
                std::map<unsigned int, std::tuple<std::thread, Player, std::promise<void> > >::iterator it = clients.find(i.first);
                msg = "PLAYERFIND=";
                msg += std::to_string( std::get<1>(it->second).id) + ":";
                msg += std::to_string(i.second);
                msg += MSG_DELIMITER;

                std::cout << msg << std::endl;

                for (auto &i : clients) {
                    send((int) std::get<1>(i.second).id, msg.c_str(), msg.length(), 0);
                }

                // check if he win
                if ((int) std::get<1>(it->second).nb_objects_found == nb_object) {
                    mtx_clients.unlock();
                    end(&(std::get<1>(it->second)));
                    mtx_clients.lock();
                    break; // Stop loop (to prevent multi-win)
                }
            }
            player_find_object.clear();
            mtx_clients.unlock();
        }
    }
}

/**
 * Thread that will get message from his client
 *
 * @param socket
 * @param id client id (= player id)
 */
void deal_with_client(int socket, unsigned int id, std::future<void> exit_signal) {
    char buffer[N_CHAR];
    size_t valread;

    bool is_registred = false; // if not, player can not ask for start, send position or request data
    bool is_an_exist_from_client = false;

    // Get objects list (from config file)
    mtx_config.lock();
    const Json::Value &objs = config["objects"]; // array of objects
    mtx_config.unlock();

    // define commands regex
    std::regex username("USERNAME=[A-Za-z0-9]+");
    std::regex position("POSITION=(-)?[0-9.]+:(-)?[0-9.]+:(-)?[0-9.]+");
    std::regex askstart("ASKSTART");
    std::regex objectfound("FOUND=[0-9]+");

    std::string messages;

    //
    while(exit_signal.wait_for(std::chrono::nanoseconds(1)) == std::future_status::timeout) {
        valread = read(socket , buffer, N_CHAR); // PASSIVE WAIT
        buffer[valread] = '\0';

        if (valread == 0) {
            is_an_exist_from_client = true;
            break;
        }
        if (exit_signal.wait_for(std::chrono::nanoseconds(1)) != std::future_status::timeout) break;

        messages += buffer;

        // Split buffer in real messages
        size_t pos = 0;
        std::string current_msg;
        while ( (pos = messages.find(MSG_DELIMITER)) != std::string::npos ) {
            current_msg = messages.substr(0, pos);
            messages.erase(0, pos + MSG_DELIMITER.length());

            // match regex
            if (regex_match(current_msg, username) && !is_registred) {
                std::cout << "Received username: " << current_msg << std::endl;

                mtx_clients.lock();
                std::map<unsigned int, std::tuple<std::thread, Player, std::promise<void>> >::iterator it = clients.find(id);

                // overwrite
                std::get<1>(it->second).name = current_msg.substr(9); // 9 is size of "USERNAME="

                mtx_clients.unlock();

                is_registred = true;

                /* Send data */
                std::string msg = "ID=" + std::to_string(id);
                msg += MSG_DELIMITER;

                // Send client id
                send(socket, msg.c_str(), msg.length(), 0);

                // Send every objects
                for (unsigned int i = 0 ; i < objs.size() ; i++) {
                    mtx_config.lock();
                    msg = "OBJECT=";
                    msg += std::to_string( i+1 ) + ":";
                    msg += objs[i]["type"].asString() + ":";
                    msg += objs[i]["sound"].asString() + ":";
                    msg += objs[i]["position"]["x"].asString() + ":";
                    msg += objs[i]["position"]["y"].asString() + ":";
                    msg += objs[i]["position"]["z"].asString() +":";
                    msg += objs[i]["direction"]["x"].asString() + ":";
                    msg += objs[i]["direction"]["y"].asString() + ":";
                    msg += objs[i]["direction"]["z"].asString();
                    mtx_config.unlock();
                    msg += MSG_DELIMITER;

                    send(socket, msg.c_str(), msg.length(), 0);
                }

                // Send player list
                mtx_clients.lock();
                for (std::map<unsigned int, std::tuple<std::thread, Player, std::promise<void>>>::iterator it=clients.begin() ; it != clients.end() ; ++it) {
                    msg = "PLAYER=";
                    msg += std::to_string( std::get<1>(it->second).id) + ":";
                    msg += std::get<1>(it->second).name + ":";
                    msg += std::to_string( std::get<1>(it->second).nb_objects_found );
                    mtx_clients.unlock();
                    msg += MSG_DELIMITER;

                    send(socket, msg.c_str(), msg.length(), 0);
                    mtx_clients.lock();
                }

                new_player_has_join.push_back(id); // add to queue
                mtx_clients.unlock();
            }
            else if (regex_match(current_msg, position) && is_registred && current_status == IN_PROGRESS) {
                std::string _pos = current_msg.substr(9);
                std::tuple<double, double, double> coor;
                double x, y, z;
                size_t p = 0;

                // X
                p = _pos.find(":");
                x = stod( _pos.substr(0, p) );
                _pos.erase(0, p+1); // 1 for sizeof ":"
                // Y
                p = _pos.find(":");
                y = stod( _pos.substr(0, p) );
                _pos.erase(0, p+1); // 1 for sizeof ":"
                // Z
                z = stod(_pos);

                coor = std::make_tuple(x, y, z);
                // TODO do something with coor
                std::cout << "position " << std::get<0>(coor) << ":" << std::get<1>(coor) << ":" << std::get<2>(coor) << std::endl;
            }
            else if (regex_match(current_msg, askstart) && is_registred && current_status == WAITING) {
                if ((int) id == leader) {
                    std::cout << "Leader asks to start" << std::endl;
                    mtx_status.lock();
                    current_status = STARTING;
                    mtx_status.unlock();
                } else {
                    std::string msg = "Vous n'êtes pas le leader, vous ne pouvez pas lancer la partie.";
                    msg += MSG_DELIMITER;
                    send(socket, msg.c_str(), msg.length(), 0);
                }
            }
            else if (regex_match(current_msg, objectfound) && is_registred && current_status == IN_PROGRESS) {
                unsigned int object_id = stoi(current_msg.substr(6));

                // check object exist
                mtx_config.lock();
                if (objs[object_id].isObject()) {
                    mtx_config.unlock();

                    mtx_clients.lock();
                    std::map<unsigned int, std::tuple<std::thread, Player, std::promise<void>> >::iterator it = clients.find(id);
                    // overwrite
                    std::get<1>(it->second).nb_objects_found++;

                    player_find_object.push_back( std::make_pair(id, object_id)); // add to queue
                    mtx_clients.unlock();
                } else {
                    mtx_config.unlock();
                }
            }
            else {
                // default
                std::cout << "Client (id: " << id << ") sent an unknown message" << std::endl;
                std::string msg = "Commande inconnu...";
                msg += MSG_DELIMITER;
                send(socket, msg.c_str(), msg.length(), 0);
            }
        }

    }

    std::cout << "Connection ended with "<< id <<std::endl;

    // if it was the leader
    if ((int) id == leader) {
        if (clients.size() > 1) {
            mtx_leader.unlock();

            mtx_clients.lock();
            unsigned int _id = 999;
            for (std::map<unsigned int, std::tuple<std::thread, Player, std::promise<void>>>::iterator it=clients.begin() ; it != clients.end() ; ++it) {
                if (_id > std::get<1>(it->second).id && std::get<1>(it->second).id != id) {
                    _id = std::get<1>(it->second).id;
                }
            }
            mtx_clients.unlock();

            mtx_leader.lock();
            if (_id == 999) leader = -1;
            else leader = (int) _id; // the first one
        } else {
            mtx_leader.lock();
            leader = -1;
        }
        mtx_leader.unlock();
        std::cout << "New leader : "<<leader<<std::endl;
    }

    if (is_an_exist_from_client) {
        // remove myself
        mtx_clients.lock();
        client_to_remove.push_back(id);
        mtx_clients.unlock();
    }
}