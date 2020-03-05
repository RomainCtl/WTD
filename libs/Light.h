#ifndef LIBS_LIGHT_H
#define LIBS_LIGHT_H

// Définition de la classe Light


#include <gl-matrix.h>
#include <utils.h>


class Light
{
protected:

    // caractéristiques de base
    vec3 m_LightColor;
    vec4 m_LightPositionScene;
    vec4 m_LightDirectionScene;

    // pour une lampe spot
    float m_LightMinAngle;
    float m_LightMaxAngle;

    // position et direction de la lampe relativement à la caméra
    vec4 m_LightPositionCamera;
    vec4 m_LightDirectionCamera;


public:

    /**
     * constructeur : initialise une lampe, utiliser les setters pour la définir
     */
    Light();

    /**
     * définit la couleur de la lampe, c'est à dire l'intensité
     */
    Light* setColor(vec3 rgb);
    Light* setColor(float r, float g, float b);
    Light* setColor(double r, double g, double b);

    /**
     * retourne la couleur de la lampe
     * @return vec3 couleur
     */
    vec3& getColor()
    {
        return m_LightColor;
    }

    /**
     * définit la position de la lampe par rapport à la scène
     */
    Light* setPosition(vec4 xyzw);
    Light* setPosition(float x, float y, float z, float w);
    Light* setPosition(double x, double y, double z, double w);

    /**
     * retourne la position de la lampe par rapport à la caméra
     * @return vec4 position caméra
     */
    vec4& getPosition()
    {
        return m_LightPositionCamera;
    }

    /**
     * définit la couleur de la lampe, c'est à dire l'intensité
     * @param color : vec3 donnant la couleur
     */
    Light* setDirection(vec4 xyzw);
    Light* setDirection(float x, float y, float z, float w);
    Light* setDirection(double x, double y, double z, double w);

    /**
     * retourne la direction de la lampe par rapport à la caméra
     * @return vec4 direction caméra
     */
    vec4& getDirection()
    {
        return m_LightDirectionCamera;
    }

    /**
     * calcule la position et la direction en coordonnées caméra
     * @param matV : mat4 matrice de vue caméra
     */
    void transform(mat4 matV);

    /**
     * définit la couleur de la lampe, c'est à dire l'intensité
     * @param color : vec3 donnant la couleur
     */
    Light* setAngles(float minangle, float maxangle);
    Light* setAngles(double minangle, double maxangle);

    /**
     * retourne le cosinus de l'angle de pleine lumière
     * @return float cos(minangle)
     */
    float getCosMinAngle();

    /**
     * retourne le cosinus de l'angle d'extinction
     * @return float cos(maxangle)
     */
    float getCosMaxAngle();
};

#endif
