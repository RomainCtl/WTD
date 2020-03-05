// Définition de la classe abstraite Material, superclasse de tous les matériaux

#include <GL/glew.h>
#include <GL/gl.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include <utils.h>
#include <Light.h>

/**
 * constructeur : initialise une lampe, utiliser les setters pour la définir
 */
Light::Light()
{
    // caractéristiques de base
    m_LightColor          = vec3::create();
    m_LightPositionScene  = vec4::create();
    m_LightDirectionScene = vec4::create();

    // pour une lampe spot
    m_LightMinAngle = 20.0;
    m_LightMaxAngle = 30.0;

    // position et direction de la lampe relativement à la caméra
    m_LightPositionCamera  = vec4::create();
    m_LightDirectionCamera = vec4::create();
}


/**
 * définit la couleur de la lampe, c'est à dire l'intensité
 */
Light* Light::setColor(vec3 rgb)
{
    vec3::copy(m_LightColor, rgb);
    return this;
}
Light* Light::setColor(float r, float g, float b)
{
    vec3::set(m_LightColor, r,g,b);
    return this;
}
Light* Light::setColor(double r, double g, double b)
{
    vec3::set(m_LightColor, r,g,b);
    return this;
}


/**
 * retourne la couleur de la lampe
 * @return vec3 couleur
 */
//vec3& Light::getColor();


/**
 * définit la position de la lampe par rapport à la scène
 */
Light* Light::setPosition(vec4 xyzw)
{
    vec4::copy(m_LightPositionScene, xyzw);
    return this;
}
Light* Light::setPosition(float x, float y, float z, float w)
{
    vec4::set(m_LightPositionScene, x,y,z,w);
    return this;
}
Light* Light::setPosition(double x, double y, double z, double w)
{
    vec4::set(m_LightPositionScene, x,y,z,w);
    return this;
}


/**
 * retourne la position de la lampe par rapport à la caméra
 * @return vec4 position caméra
 */
//vec4& Light::getPosition();


/**
 * définit la couleur de la lampe, c'est à dire l'intensité
 * @param color : vec3 donnant la couleur
 */
Light* Light::setDirection(vec4 xyzw)
{
    vec4::copy(m_LightDirectionScene, xyzw);
    return this;
}
Light* Light::setDirection(float x, float y, float z, float w)
{
    vec4::set(m_LightDirectionScene, x,y,z,w);
    return this;
}
Light* Light::setDirection(double x, double y, double z, double w)
{
    vec4::set(m_LightDirectionScene, x,y,z,w);
    return this;
}


/**
 * retourne la direction de la lampe par rapport à la caméra
 * @return vec4 direction caméra
 */
//vec4& Light::getDirection();


/**
 * calcule la position et la direction en coordonnées caméra
 * @param matV : mat4 matrice de vue caméra
 */
void Light::transform(mat4 matV)
{
    vec4::transformMat4(m_LightPositionCamera,  m_LightPositionScene,  matV);
    vec4::transformMat4(m_LightDirectionCamera, m_LightDirectionScene, matV);
    vec4::normalize(m_LightDirectionCamera, m_LightDirectionCamera);
}


/**
 * définit la couleur de la lampe, c'est à dire l'intensité
 * @param color : vec3 donnant la couleur
 */
Light* Light::setAngles(float minangle, float maxangle)
{
    m_LightMinAngle = minangle;
    m_LightMaxAngle = maxangle;
    return this;
}
Light* Light::setAngles(double minangle, double maxangle)
{
    m_LightMinAngle = minangle;
    m_LightMaxAngle = maxangle;
    return this;
}


/**
 * retourne le cosinus de l'angle de pleine lumière
 * @return float cos(minangle)
 */
float Light::getCosMinAngle()
{
    return cos(Utils::radians(m_LightMinAngle));
}


/**
 * retourne le cosinus de l'angle d'extinction
 * @return float cos(maxangle)
 */
float Light::getCosMaxAngle()
{
    return cos(Utils::radians(m_LightMaxAngle));
}
