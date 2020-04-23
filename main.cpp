#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <regex>
#include <future>
#include <signal.h>
#include <cstdlib>

#include <utils.h>
#include "commons.h"
#include "Scene.h"

/** Global variable */
std::string username;
int userid = -1;

std::mutex mtx_players;
std::map<unsigned int, Player> players;

std::mutex mtx_objects;
std::map<unsigned int, ObjectDef> objects;

std::thread interface_dealer;
std::promise<void> interface_dealer_exit_signal;

std::thread keypress_dealer;
std::promise<void> keypress_dealer_exit_signal;

std::mutex mtx_status;
Status current_status = Status::WAITING;
int client_socket = 0; // it is global var

bool third_person_perspective = false;

/**
 * Scène à dessiner
 * NB: son constructeur doit être appelé après avoir initialisé OpenGL
 **/
Scene* scene = nullptr;

/**
 * Callback pour GLFW : prendre en compte la taille de la vue OpenGL
 **/
static void onSurfaceChanged(GLFWwindow* window, int width, int height)
{
    if (scene == nullptr) return;
    scene->onSurfaceChanged(width, height);
}

/**
 * Callback pour GLFW : redessiner la vue OpenGL
 **/
static void onDrawRequest(GLFWwindow* window)
{
    if (scene == nullptr) return;
    Utils::UpdateTime();
    scene->onDrawFrame();
    static bool premiere = true;
    if (premiere) {
        // copie écran automatique
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        Utils::ScreenShotPPM("image.ppm", width, height);
        premiere = false;
    }

    // afficher le back buffer
    glfwSwapBuffers(window);
}


static void onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    if (scene == nullptr) return;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    if (action == GLFW_PRESS) {
        scene->onMouseDown(button, x,y);
    } else {
        scene->onMouseUp(button, x,y);
    }
}


static void onMouseMove(GLFWwindow* window, double x, double y)
{
    if (scene == nullptr) return;
    scene->onMouseMove(x,y);
}


static void onKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) return;
    if (scene == nullptr) return;
        scene->onKeyDown(key);
}


void onExit()
{
    // libération des ressources demandées par la scène
    if (scene != nullptr) delete scene;
    scene = nullptr;

    // terminaison de GLFW
    glfwTerminate();

    // libération des ressources openal
    alutExit();

    // retour à la ligne final
    std::cout << std::endl;
}


/** appelée en cas d'erreur */
void error_callback(int error, const char* description)
{
    std::cerr << "GLFW error : " << description << std::endl;
}

/**
 * Thread to manage interface
*/
void deal_with_interface(std::future<void> exit_signal) {
    // initialisation de GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }
    glfwSetErrorCallback(error_callback);

    // caractéristiques de la fenêtre à ouvrir
    //glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing

    // initialisation de la fenêtre
    GLFWwindow* window = glfwCreateWindow(640,480, "Livre OpenGL", NULL, NULL);
    if (window == nullptr) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowPos(window, 200, 200);
    glfwSetWindowTitle(window, "Cameras - TurnTable");

    // initialisation de glew
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Unable to initialize Glew : " << glewGetErrorString(err) << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // pour spécifier ce qu'il faut impérativement faire à la sortie
    atexit(onExit);

    // initialisation de la bibliothèque de gestion du son
    alutInit(0, NULL);
    alGetError();

    // position de la caméra qui écoute
    alListener3f(AL_POSITION, 0, 0, 0);
    alListener3f(AL_VELOCITY, 0, 0, 0);

    // création de la scène => création des objets...
    scene = new Scene(third_person_perspective);
    //debugGLFatal("new Scene()");

    mtx_objects.lock();
    for (auto &o : objects) {
        scene->addObject(
            std::get<1>(o).id,
            std::get<1>(o).type,
            std::get<1>(o).pos_x,
            std::get<1>(o).pos_y,
            std::get<1>(o).pos_z,
            std::get<1>(o).dir_x,
            std::get<1>(o).dir_y,
            std::get<1>(o).dir_z
        );
    }
    mtx_objects.unlock();

    // enregistrement des fonctions callbacks
    glfwSetFramebufferSizeCallback(window, onSurfaceChanged);
    glfwSetCursorPosCallback(window, onMouseMove);
    glfwSetMouseButtonCallback(window, onMouseButton);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, onKeyboard);

    // affichage du mode d'emploi
    std::cout << "Usage:" << std::endl;
    std::cout << "Left button to rotate object" << std::endl;
    std::cout << "Q,D (axis x) A,W (axis y) Z,S (axis z) keys to move" << std::endl;

    // boucle principale
    onSurfaceChanged(window, 640,480);
    do {
        // dessiner
        onDrawRequest(window);
        // attendre les événements
        glfwPollEvents();
    } while (
        glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        !glfwWindowShouldClose(window) &&
        exit_signal.wait_for(std::chrono::nanoseconds(1)) == std::future_status::timeout &&
        current_status == Status::IN_PROGRESS
    );

    onExit(); // normal exit
}

/**
 * Thread to manage keypress event (to ask to start game)
*/
void deal_with_keyevent(std::future<void> exit_signal) {
    std::string c;
    std::cout << "Press 's' to start... (work only if you are the leader of this game)" << std::endl;
    while (exit_signal.wait_for(std::chrono::nanoseconds(1)) == std::future_status::timeout) {
        std::cin >> c;

        if (c == "s" || c == "S") {
            std::cout << "Ask start" << std::endl;

            std::string msg = "ASKSTART" + MSG_DELIMITER;
            send(client_socket, msg.c_str(), msg.length(), 0);
        }
    }
}

/**
 * Shutdown client
*/
void stop() {
    // stop interface thread if exist
    try {
        interface_dealer_exit_signal.set_value();
        keypress_dealer_exit_signal.set_value();
    } catch (std::exception const& e) {}
    if (interface_dealer.joinable())
        interface_dealer.join();
    if (keypress_dealer.joinable())
        keypress_dealer.join();
}

/**
 * Catch kill signal
 *
 * @param s signal
 */
void exit_handler(int s) {
    std::cout << "Caught signal " << s << std::endl;
    stop();
    exit(EXIT_SUCCESS);
}

/** point d'entrée du programme **/
int main(int argc, char *argv[]) {
    // Get params
    if (argc < 3 && argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip_address> <server_port> [-p]" << std::endl << "\t-p  :  active third-person perspective" << std::endl;
        return EXIT_FAILURE;
    }
    char* addr = argv[1];
    uint16_t port = (u_int16_t) atoi(argv[2]);

    if (argc == 4) {
        if (strcmp(argv[3], "-p") == 0)
            third_person_perspective = true;
        else {
            std::cerr << "Usage: " << argv[0] << " <server_ip_address> <server_port> [-p]" << std::endl << "\t-p  :  active third-person perspective" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Client enter his/her name
    std::cout << "Enter your USERNAME: ";
    std::cin >> username;

    std::regex accepted_username("^[A-Za-z]+$");
    if (!regex_match(username, accepted_username)) {
        std::cerr << "Username must contain only upper or lower case letters! " << std::endl;
        return EXIT_FAILURE;
    }

    // Socket stuff
    size_t valread;
    struct sockaddr_in serv_addr;

    if ( (client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        std::cout << "Socket creation Error !" << std::endl;
        return EXIT_FAILURE;
    }

    serv_addr.sin_family = AF_INET; // IPv4 protocol
    serv_addr.sin_port = htons( port );

    // Convert IPv4 or IPv6 addr from text to binary form
    if (inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address/ Address not supported !" << std::endl;
        return EXIT_FAILURE;
    }

    if ( connect(client_socket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0 ) {
        std::cout << "Connection failed !" << std::endl;
        return EXIT_FAILURE;
    }

    // Sign up
    std::string msg = "USERNAME=";
    msg += username;
    msg += MSG_DELIMITER;
    send(client_socket, msg.c_str(), msg.length(), 0);

    keypress_dealer = std::thread(deal_with_keyevent, std::move(keypress_dealer_exit_signal.get_future()));

    // define commands regex
    std::regex id("ID=[0-9]+");
    std::regex player("PLAYER=[0-9]+:[A-Za-z0-9]+:[0-9]+");
    std::regex object("OBJECT=[0-9]+:[a-z]+:[0-9-.]+:[0-9-.]+:[0-9-.]+:[0-9-.]+:[0-9-.]+:[0-9-.]+");
    std::regex playerleft("PLAYERLEFT=[0-9]+");
    std::regex start("START");
    std::regex playerfind("PLAYERFIND=[0-9]+:[0-9]+");
    std::regex end("WIN=[0-9]+");

    std::string messages;
    char buffer[N_CHAR];

    do {
        valread = read(client_socket , buffer, N_CHAR); // PASSIVE WAIT
        buffer[valread] = '\0';

        if (valread == 0) break; // avoid unnecessary treatments

        messages += buffer;

        // Split buffer in real messages
        size_t pos = 0;
        while ( (pos = messages.find(MSG_DELIMITER)) != std::string::npos ) {
            std::string current_msg(messages.substr(0, pos));
            messages.erase(0, pos + MSG_DELIMITER.length());

            if (regex_match(current_msg, id) && current_status == Status::WAITING && userid == -1) {
                std::string _id = current_msg.substr(3);
                userid = stoi(_id);
                std::cout << "My ID: " << std::to_string(userid) << std::endl;
            }
            else if (regex_match(current_msg, object) && current_status == Status::WAITING && userid != -1) {
                std::string _o = current_msg.substr(7);
                unsigned int objectid;
                double pos_x, pos_y, pos_z, dir_x, dir_y, dir_z;
                std::string str_type;
                ObjectType type;

                std::cout << "New object: " << _o << std::endl;

                objectid = stoi( _o.substr(0, _o.find(":") ) );
                _o.erase(0, _o.find(":") + 1);

                str_type = _o.substr(0, _o.find(":") );
                if (str_type == "duck")
                    type = ObjectType::DUCK;
                else if (str_type == "cat")
                    type = ObjectType::CAT;
                else if (str_type == "horse")
                    type = ObjectType::HORSE;
                else if (str_type == "lion")
                    type = ObjectType::LION;
                else if (str_type == "penguin")
                    type = ObjectType::PENGUIN;
                else if (str_type == "monkey")
                    type = ObjectType::MONKEY;
                else {
                    std::cerr << "Unknown object !, exit..." << std::endl;
                    stop();
                    return EXIT_FAILURE;
                }

                // position
                _o.erase(0, _o.find(":") + 1);
                pos_x = stod( _o.substr(0, _o.find(":") ) );
                _o.erase(0, _o.find(":") + 1);
                pos_y = stod( _o.substr(0, _o.find(":") ) );
                _o.erase(0, _o.find(":") + 1);
                pos_z = stod( _o.substr(0, _o.find(":") ) );

                // direction
                _o.erase(0, _o.find(":") + 1);
                dir_x = stod( _o.substr(0, _o.find(":") ) );
                _o.erase(0, _o.find(":") + 1);
                dir_y = stod( _o.substr(0, _o.find(":") ) );
                _o.erase(0, _o.find(":") + 1);
                dir_z = stod( _o.substr(0, _o.find(":") ) );

                mtx_objects.lock();
                objects[objectid] = {objectid, type, pos_x, pos_y, pos_z, dir_x, dir_y, dir_z};
                mtx_objects.unlock();
            }
            else if (regex_match(current_msg, player) && current_status == Status::WAITING && userid != -1) {
                std::string _p = current_msg.substr(7);
                unsigned int playerid, nb_object_found;
                std::string name;

                std::cout << "New player: " << _p << std::endl;

                playerid = stoi( _p.substr(0, _p.find(":") ) );
                _p.erase(0, _p.find(":") + 1);
                name = _p.substr(0, _p.find(":") );
                _p.erase(0, _p.find(":") + 1);
                nb_object_found = stoi( _p.substr(0, _p.find(":") ) );

                mtx_players.lock();
                players[playerid] = {playerid, name, nb_object_found};
                mtx_players.unlock();
            }
            else if (regex_match(current_msg, playerleft) && userid != -1) {
                std::string _pid = current_msg.substr(11);
                int playerid = stoi(_pid);

                mtx_players.lock();
                std::map<unsigned int, Player>::iterator it = players.find(playerid);
                if (it != players.end()) {
                    std::cout << "Player '" << it->second.name << "' leave" << std::endl;
                    players.erase(it);
                }
                mtx_players.unlock();

            }
            else if (regex_match(current_msg, start) && current_status == Status::WAITING && userid != -1) {
                mtx_status.lock();
                current_status = Status::IN_PROGRESS;
                mtx_status.unlock();

                std::cout << "Let's go !" << std::endl;

                interface_dealer = std::thread(deal_with_interface, std::move(interface_dealer_exit_signal.get_future()));
                signal(SIGTERM, exit_handler);
                signal(SIGINT, exit_handler);

                keypress_dealer_exit_signal.set_value();
                keypress_dealer.detach();
            }
            else if (regex_match(current_msg, playerfind) && current_status == Status::IN_PROGRESS && userid != -1) {
                std::string _pid = current_msg.substr(11);
                int playerid = stoi(_pid);

                current_msg.erase(0, current_msg.find(":") + 1);
                std::string _oid = current_msg;
                int objectid = stoi(_oid);

                mtx_players.lock();
                std::map<unsigned int, Player>::iterator it = players.find(playerid);
                if (it != players.end()) {
                    it->second.nb_objects_found += 1;

                    std::cout << "Player '" << it->second.name << "' found object id: " << std::to_string(objectid) << std::endl;
                }
                mtx_players.unlock();
            }
            else if (regex_match(current_msg, end) && current_status == Status::IN_PROGRESS && userid != -1) {
                mtx_status.lock();
                current_status = Status::COMPLETED;
                mtx_status.unlock();

                std::string _id = current_msg.substr(4);
                int winnerid = stoi(_id);

                if (winnerid == userid) {
                    std::cout << "$$$$$$$$$$$$$$$" << std::endl;
                    std::cout << "$$ YOU WIN ! $$" << std::endl;
                    std::cout << "$$$$$$$$$$$$$$$" << std::endl << std::endl;
                }

                // Score board
                mtx_players.lock();
                std::cout << "========= ScoreBoard =========" << std::endl;
                for (auto &user : players) {
                    Player p = std::get<1>(user);
                    std::string msg = "";

                    if (p.id == userid) msg += "*";
                    msg += p.name + " (id: " + std::to_string(p.id) +") = " + std::to_string(p.nb_objects_found);
                    if (p.id == winnerid) msg += " WINNER";

                    std::cout << msg << std::endl;
                }
                std::cout << "==============================" << std::endl;
                mtx_players.unlock();
            }
        }
    } while (valread != 0 && current_status != Status::COMPLETED);

    stop();
    return EXIT_SUCCESS;
}
