#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <utils.h>

#include "Scene.h"


/** constructeur */
Scene::Scene() {
    m_Ground = new Ground();
    m_Compass = new Compass();
    m_CompassNeedle = new CompassNeedle();

    // caractéristiques de la lampe
    m_Light = new Light();
    m_Light->setColor(500.0, 500.0, 500.0);
    m_Light->setPosition(0.0,  16.0,  13.0, 1.0);
    m_Light->setDirection(0.0, -1.0, -1.0, 0.0);
    m_Light->setAngles(30.0, 40.0);

    // couleur du fond : gris foncé
    glClearColor(0.4, 0.4, 0.4, 0.0);

    // activer le depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // initialiser les matrices
    m_MatP = mat4::create();
    m_MatV = mat4::create();
    m_MatVM = mat4::create();
    m_MatTMP = mat4::create();

    // gestion vue et souris
    m_Azimut    = 20.0;
    m_Elevation = 20.0;
    // distance of 0 -> First-Person perspective
    // distance of 5 (example) -> Third-Person perspective (but we do not implement player obj, so we do not see him, it is just strange)
    m_Distance  = 0.0;
    m_Center    = vec3::create();
    m_Clicked   = false;

    lego = new Object(_THIRD_PERSON);
    lego->setPosition(vec3::fromValues(0, 0, 0));
    lego->setOrientation(vec3::fromValues(0, 0, 0));
    lego->setDraw(false);
    lego->setSound(false);

    // init mouse position
    m_MousePrecX = 0.0;
    m_MousePrecY = 0.0;

    m_lastPlayerPosition = vec3::fromValues(0, 0, 0);
}


/**
 * appelée quand la taille de la vue OpenGL change
 * @param width : largeur en nombre de pixels de la fenêtre
 * @param height : hauteur en nombre de pixels de la fenêtre
 */
void Scene::onSurfaceChanged(int width, int height)
{
    // met en place le viewport
    glViewport(0, 0, width, height);

    // matrice de projection (champ de vision)
    mat4::perspective(m_MatP, Utils::radians(25.0), (float)width / height, 0.1, 100.0);
}


/**
 * appelée quand on enfonce un bouton de la souris
 * @param btn : GLFW_MOUSE_BUTTON_LEFT pour le bouton gauche
 * @param x coordonnée horizontale relative à la fenêtre
 * @param y coordonnée verticale relative à la fenêtre
 */
void Scene::onMouseDown(int btn, double x, double y)
{
    if (btn != GLFW_MOUSE_BUTTON_LEFT) return;
    m_Clicked = true;
    m_MousePrecX = x;
    m_MousePrecY = y;
}


/**
 * appelée quand on relache un bouton de la souris
 * @param btn : GLFW_MOUSE_BUTTON_LEFT pour le bouton gauche
 * @param x coordonnée horizontale relative à la fenêtre
 * @param y coordonnée verticale relative à la fenêtre
 */
void Scene::onMouseUp(int btn, double x, double y)
{
    m_Clicked = false;
}


/**
 * appelée quand on bouge la souris
 * @param x coordonnée horizontale relative à la fenêtre
 * @param y coordonnée verticale relative à la fenêtre
 */
void Scene::onMouseMove(double x, double y)
{
    if (! m_Clicked) return;
    m_Azimut  += (x - m_MousePrecX) * 0.1;
    m_Elevation += (y - m_MousePrecY) * 0.1;
    if (m_Elevation >  90.0) m_Elevation =  90.0;
    if (m_Elevation < -90.0) m_Elevation = -90.0;
    m_MousePrecX = x;
    m_MousePrecY = y;
}


/**
 * appelée quand on appuie sur une touche du clavier
 * @param code : touche enfoncée
 */
void Scene::onKeyDown(unsigned char code)
{
    // construire la matrice inverse de l'orientation de la vue à la souris
    mat4::identity(m_MatTMP);
    mat4::rotateY(m_MatTMP, m_MatTMP, Utils::radians(-m_Azimut));
    mat4::rotateX(m_MatTMP, m_MatTMP, Utils::radians(-m_Elevation));

    // vecteur indiquant le décalage à appliquer au pivot de la rotation
    vec3 offset = vec3::create();
    switch (code) {
    case GLFW_KEY_W: // avant
//        m_Distance *= exp(-0.01);
        vec3::transformMat4(offset, vec3::fromValues(0, 0, +0.1), m_MatTMP);
        break;
    case GLFW_KEY_S: // arrière
//        m_Distance *= exp(+0.01);
        vec3::transformMat4(offset, vec3::fromValues(0, 0, -0.1), m_MatTMP);
        break;
    case GLFW_KEY_A: // droite
        vec3::transformMat4(offset, vec3::fromValues(+0.1, 0, 0), m_MatTMP);
        break;
    case GLFW_KEY_D: // gauche
        vec3::transformMat4(offset, vec3::fromValues(-0.1, 0, 0), m_MatTMP);
        break;
    case GLFW_KEY_Q: // haut
        vec3::transformMat4(offset, vec3::fromValues(0, -0.1, 0), m_MatTMP);
        break;
    case GLFW_KEY_Z: // bas
        vec3::transformMat4(offset, vec3::fromValues(0, +0.1, 0), m_MatTMP);
        break;
    case GLFW_KEY_P: // Switch between third-person and first-person perspective
        if (m_Distance == 0) {
            m_Distance = 5.0;
            lego->setDraw(true);
        } else {
            m_Distance = 0;
            lego->setDraw(false);
        }
        break;
    default:
        return;
    }

    // appliquer le décalage au centre de la rotation
    vec3::add(m_Center, m_Center, offset);
}


/**
 * Dessine l'image courante
 */
void Scene::onDrawFrame()
{
    /** préparation des matrices **/

    // positionner la caméra
    mat4::identity(m_MatV);

    // éloignement de la scène
    mat4::translate(m_MatV, m_MatV, vec3::fromValues(0.0, 0.0, -m_Distance));

    // rotation demandée par la souris
    mat4::rotateX(m_MatV, m_MatV, Utils::radians(m_Elevation));
    mat4::rotateY(m_MatV, m_MatV, Utils::radians(m_Azimut));

    // centre des rotations
    mat4::translate(m_MatV, m_MatV, m_Center);

    vec3 player_pos, object_pos;
    vec3::multiply(player_pos, m_Center, vec3::fromValues(-1, -1, -1));

    for (auto &object : m_Objects) {
        object_pos = std::get<1>(object).first->getPosition();
        double distance = std::sqrt(
            std::pow(object_pos[0] - player_pos[0], 2.0) +
            std::pow(object_pos[1] - player_pos[1], 2.0) +
            std::pow(object_pos[2] - player_pos[2], 2.0)
        );
        if (distance < 5) {
            if (!std::get<1>(object).second) {
                std::get<1>(object).first->setDraw(true);
                std::get<1>(object).first->setSound(false);

                // send msg to server
                std::string msg = "FOUND=";
                msg += std::to_string( std::get<0>(object) );
                msg += MSG_DELIMITER;
                send(client_socket, msg.c_str(), msg.length(), 0);
                std::get<1>(object).second = true;
            }
        }
    }

    if (!vec3::equals(player_pos, m_lastPlayerPosition)) {
        // Send position to server
        std::string msg = "POSITION=";
        msg += std::to_string( player_pos[0] ) + ":"; // x
        msg += std::to_string( player_pos[1] ) + ":"; // y
        msg += std::to_string( player_pos[2] );       // z
        msg += MSG_DELIMITER;
        // std::cout << msg << std::endl; // TODO remove me
        send(client_socket, msg.c_str(), msg.length(), 0);

        m_lastPlayerPosition = player_pos; // update last position

        lego->setPosition(m_lastPlayerPosition);
    }

    // set third-person orientation (TODO take care of Elevation ?)
    lego->setOrientation(vec3::fromValues(0, -Utils::radians( m_Azimut ), 0));

    /** gestion des lampes **/

    // calculer la position et la direction de la lampe par rapport à la scène
    m_Light->transform(m_MatV);

    // fournir position et direction en coordonnées caméra aux objets éclairés
    m_Ground->setLight(m_Light);
    for (auto &object : m_Objects) {
        std::get<1>(object).first->setLight(m_Light);
    }

    // enlighten lego
    lego->setLight(m_Light);

    /** dessin de l'image **/

    // effacer l'écran
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // dessiner le sol
    m_Ground->onDraw(m_MatP, m_MatV);

    // dessiner le canard en mouvement
    for (auto &object : m_Objects) {
        std::get<1>(object).first->onRender(m_MatP, m_MatV);
    }

    // Third person
    lego->onRender(m_MatP, m_MatV);

    // Draw compass
    glDisable(GL_DEPTH_TEST); // to allow superposition
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    m_Compass->onDraw(m_MatP, m_MatV);

    m_CompassNeedle->rotate(m_Azimut);
    m_CompassNeedle->onDraw(m_MatP, m_MatV);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}


/** supprime tous les objets de cette scène */
Scene::~Scene() {
    for (auto &object : m_Objects) {
        delete std::get<1>(object).first;
    }
    m_Objects.clear();
    delete m_Ground;
    delete m_Compass;
}

/**
 * To add an object to the scene
 *
 * @param id object unique id
 * @param type object type
 * @param pos_x X position coordinate
 * @param pos_y Y position coordinate
 * @param pos_z Z position coordinate
 * @param dir_x X direction coordinate
 * @param dir_y Y direction coordinate
 * @param dir_z Z direction coordinate
 */
void Scene::addObject(unsigned int id, ObjectType type, double pos_x, double pos_y, double pos_z, double dir_x, double dir_y, double dir_z) {
    Object *tmp = new Object(type);
    tmp->setPosition(vec3::fromValues(pos_x, pos_y, pos_z));
    tmp->setOrientation(vec3::fromValues(dir_x, Utils::radians(dir_y), dir_z));
    tmp->setDraw(false);
    tmp->setSound(true);
    m_Objects[id] = std::make_pair(tmp, false);
}
