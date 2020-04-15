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

// function definition
void stop_server();
void deal_with_socket(u_int16_t port);
void deal_with_client(int socket, unsigned int id);


// Player structure definition
struct Player {
    unsigned int id;
    char name[N_CHAR];
    unsigned int nb_objects_found;
};


/*******************************
 * Global variables definitions
*******************************/
std::mutex mtx_main; // to allow passive lock for main
std::thread connection_dealer;

// Clients list
std::vector<std::pair<std::thread,Player>> clients;
std::mutex mtx_clients;

// To define winner FIXME (see line 114)
int winner_id = -1;
std::mutex mtx_winner_id;

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
    mtx_clients.lock();
    for (auto &i : clients) {
        i.first.detach(); // stop thread
        close(i.second.id); // stop socket (from id)
    }
    mtx_clients.unlock();
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

    // FIXME le server doit-il vraiment s'eteindre lorsqu'un joueur gagne ?
    mtx_winner_id.lock();
    while (winner_id == -1) {
        mtx_winner_id.unlock();
        mtx_main.lock(); // PASSIVE LOCK
        mtx_winner_id.lock();
    }
    mtx_winner_id.unlock();

    mtx_winner_id.lock();
    std::cout << "Player " << winner_id << " wins !" << std::endl;
    mtx_winner_id.unlock();

    // THE END
    stop_server();
    std::cout << "Shutdown server..." << std::endl;
    return EXIT_SUCCESS;
}

/**
 * Thread that will wait forever for new connections
 *
 * @param port for socket
 */
void deal_with_socket(uint16_t port) {
    //
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