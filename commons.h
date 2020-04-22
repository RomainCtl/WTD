#ifndef COMMONS_H
#define COMMONS_H

#include <sys/socket.h>
#include <stdlib.h>
#include <string>

// List of object type
enum ObjectType {
    DUCK
};

#define N_CHAR 1024UL

// Player structure definition
struct Player {
    unsigned int id;
    std::string name;
    unsigned int nb_objects_found;
};

// ObjectDef structure definition
struct ObjectDef {
    unsigned int id;
    ObjectType type;
    std::string sound;
    double pos_x;
    double pos_y;
    double pos_z;
    double dir_x;
    double dir_y;
    double dir_z;
    bool is_found;
};

// Game status
enum Status {
    WAITING,
    IN_PROGRESS,
    COMPLETED
};

// Socket
extern int client_socket;
// To delimite each socket msgs
const std::string MSG_DELIMITER = "$";

#endif
