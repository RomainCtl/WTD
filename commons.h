#ifndef COMMONS_H
#define COMMONS_H

#include <sys/socket.h>
#include <stdlib.h>
#include <string>
#include <map>

// List of object type
enum ObjectType {
    DUCK,
    CAT,
    HORSE,
    LION,
    PENGUIN,
    MONKEY,
    _THIRD_PERSON
};

struct ObjectConfigType {
    std::string sound_file;
    std::string obj_file;
    std::string diffuse_img;
    float inner_angle;
    float outer_gain;
    float outer_angle;
    float max_distance;
    double ratio;
    int rotation_x;
    int rotation_y;
    int rotation_z;
};

extern std::map<ObjectType, ObjectConfigType> objects_config;

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
    double pos_x;
    double pos_y;
    double pos_z;
    double dir_x;
    double dir_y;
    double dir_z;
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
