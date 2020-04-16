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

#define N_CHAR 1024UL

// Player structure definition
struct Player {
    unsigned int id;
    std::string name;
    unsigned int nb_objects_found;
};

// Game status
enum Status {
    WAITING, // waiting players (client connection authorized)
    IN_PROGRESS, // game in progress (client can not initialize new connection)
    COMPLETED // we have a winner ! (reload all data)
};

// function definition
void stop_server();
void close_sockets();
void end(Player *winner);
void deal_with_socket(u_int16_t port);
void deal_with_client(int socket, unsigned int id);


/*******************************
 * Global variables definitions
*******************************/
std::mutex mtx_main; // to allow passive lock for main
std::thread connection_dealer;

// Clients list
std::map<unsigned int, std::pair<std::thread,Player>> clients;
std::vector<unsigned int> client_to_remove; // thread can not remove itself
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
    connection_dealer.detach();
    close_sockets();
}

/**
 * Close all clients connections
 */
void close_sockets() {
    mtx_clients.lock();
    for (auto &i : clients) {
        i.second.first.detach(); // stop thread
        close(i.second.second.id); // stop socket (from id)
    }
    clients.clear(); // clear player list
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
    std::string msg = "Player ";
    msg += winner->name;
    msg += " (id: ";
    msg += std::to_string(winner->id);
    msg += ") wins !";

    // Server
    std::cout << msg << std::endl;

    // for each player
    mtx_clients.lock();
    for (auto &i : clients) {
        send(i.second.second.id, msg.c_str(), msg.length(), 0);
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
    signal(SIGILL, exit_handler);
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
    connection_dealer = std::thread(deal_with_socket, port);

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
void deal_with_socket(uint16_t port) {
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

    while(true) {
        int new_socket;

        // A new client connection... PASSIVE WAIT
        if ( (new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept " << __FILE__ << " " << __LINE__ << std::endl;
            exit(EXIT_FAILURE);
        }

        // remove lost clients
        if (client_to_remove.size() > 0) {
            mtx_clients.lock();
            for(auto &i : client_to_remove) {
                std::map<unsigned int, std::pair<std::thread, Player> >::iterator it = clients.find(i);
                it->second.first.detach(); // stop thread
                clients.erase( it );
            }
            client_to_remove.clear();
            mtx_clients.unlock();
        }

        if (clients.size() < max_player && current_status == WAITING) {
            mtx_clients.lock();
            if (clients.empty()) {
                next_id = 0;
                mtx_clients.unlock();

                // set leader
                mtx_leader.lock();
                leader = (int) next_id;
                mtx_leader.unlock();

                mtx_clients.lock();
            } else {
                next_id = clients.rbegin()->second.second.id + 1;
            }

            std::cout << "Client " << next_id << " connected"<<std::endl;

            Player p = {next_id, "", 0};
            clients[next_id] = std::make_pair(
                std::thread(deal_with_client, new_socket, next_id),
                p
            );
            mtx_clients.unlock();
        } else {
            std::cout << "out" << std::endl;
            close(new_socket);
            // Maximum number of players already reached
            // Or game already in progress
        }

        mtx_main.unlock(); // tells main about a new event...
    }
}

/**
 * Thread that will get message from his client
 *
 * @param socket
 * @param id client id (= player id)
 */
void deal_with_client(int socket, unsigned int id) {
    // TODO start game, object_found, new_player_has_join, player_find_object, send data (objects, player, id)

    char buffer[N_CHAR];
    size_t valread;

    bool is_registred = false; // if not, player can not ask for start, send position or request data

    // Get objects list (from config file)
    mtx_config.lock();
    const Json::Value &objs = config["objects"]; // array of objects
    mtx_config.unlock();

    //
    do {
        valread = read( socket , buffer, N_CHAR); // PASSIVE WAIT
        buffer[valread-1] = '\0'; // FIXME : buffer overflow may occurs

        std::string message(buffer);

        // define commands regex
        std::regex username("USERNAME=[A-Za-z0-9]+");
        std::regex position("POSITION=[-0-9]+:[-0-9]+:[-0-9]+");

        // match regex
        if (regex_match(message, username)) {
            std::cout << "Received username" << std::endl;

            mtx_clients.lock();
            std::map<unsigned int, std::pair<std::thread, Player> >::iterator it = clients.find(id);

            // overwrite
            it->second.second.name = message.substr(9); // 9 is size of "USERNAME="

            mtx_clients.unlock();

            is_registred = true;

            // TODO send data (object list, player list, client id)
        }
        else if (regex_match(message, position) && is_registred) {
            std::string _pos = message.substr(9);
            std::tuple<int, int, int> coor;
            int x, y, z;
            size_t p = 0;

            // X
            p = _pos.find(":");
            x = stoi( _pos.substr(0, p) );
            _pos.erase(0, p+1); // 1 for sizeof ":"
            // Y
            p = _pos.find(":");
            y = stoi( _pos.substr(0, p) );
            _pos.erase(0, p+1); // 1 for sizeof ":"
            // Z
            z = stoi(_pos);

            coor = std::make_tuple(x, y, z);
            // TODO do something with coor
            std::cout << "position " << std::get<0>(coor) << ":" << std::get<1>(coor) << ":" << std::get<2>(coor) << std::endl;
        }

    } while(valread != 0);

    std::cout << "Connection ended with "<< id <<std::endl; // this may not appear as a race condition is running between main and this thread...

    // if it was the leader
    // TODO if this thread was killed for any reason, assure that a leader is present in clients list (How ?)
    if ((int) id == leader) {
        if (clients.size() > 1) {
            mtx_leader.unlock();

            mtx_clients.lock();
            unsigned int _id = 999;
            for (std::map<unsigned int, std::pair<std::thread, Player>>::iterator it=clients.begin() ; it != clients.end() ; ++it) {
                if (_id > it->second.second.id && it->second.second.id != id) {
                    _id = it->second.second.id;
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

    // remove myself
    mtx_clients.lock();
    client_to_remove.push_back(id);
    mtx_clients.unlock();
}