#include <GL/glew.h>
#include <GL/gl.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <stdexcept>

#include <utils.h>
#include <MeshVertex.h>

using namespace mesh;



/**
 * Constructeur de la classe Vertex qui représente un sommet dans
 * le maillage. Initialise un sommet non connecté au maillage, employer
 * Mesh.addVertex pour l'insérer puis new Triangle(...) pour le mettre
 * dans un triangle, setCoords et setColor pour lui donner des coordonnées et
 * des couleurs.
 * @param mesh : maillage d'appartenance de ce sommet
 * @param name : nom du sommet à créer
 */
Vertex::Vertex(Mesh* mesh, vec3 xyz)
{
    // attributs de sommet
    m_Index     = -1;
    vec3::copy(m_Coords, xyz);
    m_Color     = vec3::fromValues(1, 0, 1);
    m_TexCoords = vec2::create();
    m_Normal    = vec3::create();
    m_Tangent   = vec3::create();

    // liens entre sommet et mesh
    mesh->pushVertex(this);
    m_Mesh = mesh;
}
Vertex::Vertex(Mesh* mesh, float x, float y, float z)
{
    // attributs de sommet
    m_Index     = -1;
    m_Coords    = vec3::fromValues(x,y,z);
    m_Color     = vec3::fromValues(1, 0, 1);
    m_TexCoords = vec2::create();
    m_Normal    = vec3::create();
    m_Tangent   = vec3::create();

    // liens entre sommet et mesh
    mesh->pushVertex(this);
    m_Mesh = mesh;
}
Vertex::Vertex(Mesh* mesh, double x, double y, double z)
{
    // attributs de sommet
    m_Index     = -1;
    m_Coords    = vec3::fromValues(x,y,z);
    m_Color     = vec3::fromValues(1, 0, 1);
    m_TexCoords = vec2::create();
    m_Normal    = vec3::create();
    m_Tangent   = vec3::create();

    // liens entre sommet et mesh
    mesh->pushVertex(this);
    m_Mesh = mesh;
}


/** destructeur */
Vertex::~Vertex()
{
    // supprimer ce sommet de la liste du maillage
    if (m_Mesh != nullptr) m_Mesh->popVertex(this);
    m_Mesh = nullptr;
}


/**
 * réaffecte le mesh de ce sommet
 */
void Vertex::setMesh(Mesh* mesh)
{
    m_Mesh = mesh;
}


/**
 * réaffecte le numéro de ce sommet
 */
void Vertex::setIndex(long index)
{
    m_Index = index;
}


/**
 * définit les coordonnées du sommet
 * @param xyz coordonnées
 * @return this pour pouvoir chaîner les affectations
 */
Vertex* Vertex::setCoords(vec3 xyz)
{
    vec3::copy(m_Coords, xyz);
    return this;
}
Vertex* Vertex::setCoords(float x, float y, float z)
{
    m_Coords[0] = x; m_Coords[1] = y; m_Coords[2] = z;
    return this;
}
Vertex* Vertex::setCoords(double x, double y, double z)
{
    m_Coords[0] = x; m_Coords[1] = y; m_Coords[2] = z;
    return this;
}


/**
 * retourne les coordonnées du sommet
 * @return coordonnées 3D du sommet
 */
//vec3& Vertex::getCoords();


/**
 * définit la couleur du sommet
 * @param rgba couleur (r,g,b,a)
 * @return this pour pouvoir chaîner les affectations
 */
Vertex* Vertex::setColor(vec3 rgba)
{
    vec3::copy(m_Color, rgba);
    return this;
}
Vertex* Vertex::setColor(float r, float g, float b)
{
    m_Color[0] = r; m_Color[1] = g; m_Color[2] = b;
    return this;
}
Vertex* Vertex::setColor(double r, double g, double b)
{
    m_Color[0] = r; m_Color[1] = g; m_Color[2] = b;
    return this;
}


/**
 * retourne la couleur du sommet
 * @return couleur (r,g,b)
 */
//vec3& Vertex::getColor();


/**
 * définit les coordonnées de la normale du sommet
 * @param normal : normale à affecter
 * @return this pour pouvoir chaîner les affectations
 */
Vertex* Vertex::setNormal(vec3 normal)
{
    vec3::copy(m_Normal, normal);
    return this;
}
Vertex* Vertex::setNormal(float x, float y, float z)
{
    m_Normal[0] = x; m_Normal[1] = y; m_Normal[2] = z;
    return this;
}
Vertex* Vertex::setNormal(double x, double y, double z)
{
    m_Normal[0] = x; m_Normal[1] = y; m_Normal[2] = z;
    return this;
}


/**
 * retourne la normale du sommet
 * @return normale
 */
//vec3& Vertex::getNormal();


/**
 * retourne la tangente du sommet
 * @return tangente
 */
//vec3& Vertex::getTangent();


/**
 * définit les coordonnées de texture du sommet
 * @param uv coordonnées de texture
 * @return this pour pouvoir chaîner les affectations
 */
Vertex* Vertex::setTexCoords(vec2 uv)
{
    vec2::copy(m_TexCoords, uv);
    return this;
}
Vertex* Vertex::setTexCoords(float u, float v)
{
    m_TexCoords[0] = u; m_TexCoords[1] = v;
    return this;
}
Vertex* Vertex::setTexCoords(double u, double v)
{
    m_TexCoords[0] = u; m_TexCoords[1] = v;
    return this;
}


/**
 * retourne les coordonnées de texture du sommet
 * @return coordonnées de texture
 */
//vec2& Vertex::getTexCoords();


/**
 * Cette méthode calcule la normale du sommet = moyenne des normales des
 * triangles contenant ce sommet.
 */
void Vertex::computeNormal()
{
    // calculer la moyenne des normales des triangles contenant ce sommet
    vec3::zero(m_Normal);

    // parcourir tous les triangles du maillage et prendre en compte ceux qui contiennent this
    for (Triangle* triangle: m_Mesh->getTriangleList()) {
        if (triangle->containsVertex(this)) {
            // ajouter la normale du triangle courant, elle tient compte de la surface
            vec3::add(m_Normal, m_Normal, triangle->getNormal());
        }
    }

    // normaliser le résultat
    vec3::normalize(m_Normal, m_Normal);
}


/**
 * Cette méthode calcule la tangente du sommet = moyenne des tangentes des
 * triangles contenant ce sommet.
 */
void Vertex::computeTangent()
{
    // calculer la moyenne des tangentes des triangles contenant ce sommet
    vec3::zero(m_Tangent);

    // parcourir tous les triangles du maillage et prendre en compte ceux qui contiennent this
    for (Triangle* triangle: m_Mesh->getTriangleList()) {
        if (triangle->containsVertex(this)) {
            // ajouter la normale du triangle courant, elle tient compte de la surface
            vec3::add(m_Tangent, m_Tangent, triangle->getTangent());
        }
    }

    // normaliser le résultat
    vec3::normalize(m_Tangent, m_Tangent);
}
