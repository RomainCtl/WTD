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

#define N_CHAR 1024UL

// Player structure definition
struct Player {
    unsigned int id;
    char name[N_CHAR];
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
std::vector<std::pair<std::thread,Player>> clients;
std::mutex mtx_clients;

// Game status
Status current_status = COMPLETED;
std::mutex mtx_status;

// Who is the room leader (only he can start the game)
unsigned int leader = -1;
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
        i.first.detach(); // stop thread
        close(i.second.id); // stop socket (from id)
    }
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
        send(i.second.id, msg.c_str(), msg.length(), 0);
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

    uint16_t port = atoi(argv[1]);
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
    int max_player = config["max_player"].asInt();
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

        if (clients.size() < max_player && current_status == WAITING) {
            mtx_clients.lock();
            if (clients.empty()) {
                next_id = 0;
                mtx_clients.unlock();

                // set leader
                mtx_leader.lock();
                leader = next_id;
                mtx_leader.unlock();

                mtx_clients.lock();
            } else {
                next_id = clients.back().second.id + 1;
            }

            std::cout << "Client " << next_id << "connected"<<std::endl;

            clients.emplace_back(
                std::thread(deal_with_client, new_socket, next_id),
                new_socket
            ); // emplace_back creates the std::pair...
            mtx_clients.unlock();
        } else {
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
    //
}