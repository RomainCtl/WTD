#ifndef SCENE_H
#define SCENE_H

// Définition de la classe Scene

#include <gl-matrix.h>

#include "Light.h"

#include "Object.h"
#include "Ground.h"
#include "Compass.h"
#include "CompassNeedle.h"
#include "commons.h"


class Scene
{
private:

    // objets de la scène
    std::map<unsigned int, std::pair<Object*, bool> > m_Objects;
    Ground* m_Ground;
    Compass* m_Compass;
    CompassNeedle* m_CompassNeedle;

    // lampes
    Light* m_Light;

    // matrices de transformation des objets de la scène
    mat4 m_MatP;
    mat4 m_MatV;
    mat4 m_MatVM;
    mat4 m_MatTMP;

    // caméra table tournante
    float m_Azimut;
    float m_Elevation;
    float m_Distance;
    vec3 m_Center;

    // For Third-Person perpective
    Object *lego;

    // souris
    bool m_Clicked;
    double m_MousePrecX;
    double m_MousePrecY;

    vec3 m_lastPlayerPosition;


public:

    /**
     * constructeur, crée les objets 3D à dessiner
     */
    Scene();

    /** destructeur, libère les ressources */
    ~Scene();

    /**
     * appelée quand la taille de la vue OpenGL change
     * @param width : largeur en nombre de pixels de la fenêtre
     * @param height : hauteur en nombre de pixels de la fenêtre
     */
    void onSurfaceChanged(int width, int height);


    /**
     * appelée quand on enfonce un bouton de la souris
     * @param btn : GLFW_MOUSE_BUTTON_LEFT pour le bouton gauche
     * @param x coordonnée horizontale relative à la fenêtre
     * @param y coordonnée verticale relative à la fenêtre
     */
    void onMouseDown(int btn, double x, double y);

    /**
     * appelée quand on relache un bouton de la souris
     * @param btn : GLFW_MOUSE_BUTTON_LEFT pour le bouton gauche
     * @param x coordonnée horizontale relative à la fenêtre
     * @param y coordonnée verticale relative à la fenêtre
     */
    void onMouseUp(int btn, double x, double y);

    /**
     * appelée quand on bouge la souris
     * @param x coordonnée horizontale relative à la fenêtre
     * @param y coordonnée verticale relative à la fenêtre
     */
    void onMouseMove(double x, double y);

    /**
     * appelée quand on appuie sur une touche du clavier
     * @param code : touche enfoncée
     */
    void onKeyDown(unsigned char code);

    /** Dessine l'image courante */
    void onDrawFrame();

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
    void addObject(unsigned int id, ObjectType type, double pos_x, double pos_y, double pos_z, double dir_x, double dir_y, double dir_z);
};

#endif
