#include <GL/glew.h>
#include <GL/gl.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <stdexcept>

#include <utils.h>
#include <MeshTriangle.h>

using namespace mesh;



/**
 * Constructeur de la classe Triangle à partir de trois sommets
 * NB: l'ordre de rotation des sommets est crucial pour le calcul des normales.
 * Il faut tourner dans le sens trigonométrique, comme dans OpenGL.
 * @param mesh : maillage dans lequel on rajoute ce triangle
 * @param v0 : l'un des coins du triangle, nullptr si création manuelle
 * @param v1 : l'un des coins du triangle, nullptr si création manuelle
 * @param v2 : l'un des coins du triangle, nullptr si création manuelle
 */
Triangle::Triangle(Mesh* mesh, Vertex* v0, Vertex* v1, Vertex* v2)
{
    // tableau des sommets
    m_Vertices[0] = v0;
    m_Vertices[1] = v1;
    m_Vertices[2] = v2;

    // liens entre triangle et mesh
    if (mesh != nullptr) mesh->pushTriangle(this);
    m_Mesh = mesh;

    // vecteur normal non normalisé, il est proportionnel à la surface du triangle
    m_Normal = vec3::create();

    // vecteur tangent, idem
    m_Tangent = vec3::create();
}


/**
 * Cette méthode supprime ce triangle du maillage en mettant à jour toutes
 * les listes. Cela peut supprimer des arêtes et rendre des sommets isolés.
 */
Triangle::~Triangle()
{
    // supprimer ce triangle de la liste du maillage
    if (m_Mesh != nullptr) m_Mesh->popTriangle(this);
    m_Mesh = nullptr;
}


/**
 * réaffecte le mesh de ce triangle
 */
void Triangle::setMesh(Mesh* mesh)
{
    m_Mesh = mesh;
}


/**
 * retourne le sommet n°n (0..2) du triangle, ou nullptr si n n'est pas correct
 * @param n : numéro 0..2 du sommet
 * @return le sommet demandé ou nullptr si n n'est pas dans les bornes
 */
Vertex* Triangle::getVertex(int n)
{
    if (n < 0 || n > 2) return nullptr;
    return m_Vertices[n];
}


/**
 * retourne la valeur de la normale
 * @see #calcNormale pour la calculer auparavant
 * @return normale du triangle
 */
//vec3 Triangle::getNormal();


/**
 * recalcule les informations géométriques du triangle : centre, normale, surface...
 */
void Triangle::computeNormal()
{
    // les trois sommets
    Vertex* A = m_Vertices[0];
    Vertex* B = m_Vertices[1];
    Vertex* C = m_Vertices[2];

    // les coordonnées des trois sommets
    vec3 cA = A->getCoords();
    vec3 cB = B->getCoords();
    vec3 cC = C->getCoords();

    // vecteurs AB et AC
    vec3 cAB = vec3::create();
    vec3::subtract(cAB, cB, cA);
    vec3 cAC = vec3::create();
    vec3::subtract(cAC, cC, cA);

    // calculer le vecteur normal
    vec3::cross(m_Normal, cAB, cAC);
}


/**
 * retourne la valeur de la tangente
 * @see #calcTangente pour la calculer auparavant
 * @return tangente du triangle
 */
//vec3 Triangle::getTangent();


/**
 * recalcule la tangente du triangle à l'aide de la normale et des coordonnées de texture
 */
void Triangle::computeTangent()
{
    // les trois sommets
    Vertex* A = m_Vertices[0];
    Vertex* B = m_Vertices[1];
    Vertex* C = m_Vertices[2];

    // les coordonnées des trois sommets
    vec3 cA = A->getCoords();
    vec3 cB = B->getCoords();
    vec3 cC = C->getCoords();

    // vecteurs AB et AC
    vec3 cAB = vec3::create();
    vec3::subtract(cAB, cB, cA);
    vec3 cAC = vec3::create();
    vec3::subtract(cAC, cC, cA);

    // récupération de leur 2e coordonnée de texture
    float tA = A->getTexCoords()[1];
    float tB = B->getTexCoords()[1];
    float tC = C->getTexCoords()[1];

    // vecteurs dans l'espace (s,t), et uniquement la coordonnée t
    float tAB = tB - tA;
    float tAC = tC - tA;

    // TODO s'il n'y a pas de coordonnées de texture, alors tAB et tAC sont nuls, les remplacer par AB et AC

    // calcul de la tangente
    vec3::scale(cAB, cAB, tAC);
    vec3::scale(cAC, cAC, tAB);
    vec3::subtract(m_Tangent, cAB, cAC);

    // normalisation
    vec3::normalize(m_Tangent, m_Tangent);
}


/**
 * Cette méthode indique si le triangle this contient le sommet indiqué
 * @param vertex : sommet dont il faut vérifier l'appartenance à this
 * @return true si ok, false si le sommet est absent du triangle
 */
bool Triangle::containsVertex(Vertex* vertex)
{
    if (m_Vertices[0] == vertex) return true;
    if (m_Vertices[1] == vertex) return true;
    if (m_Vertices[2] == vertex) return true;
    return false;
}
