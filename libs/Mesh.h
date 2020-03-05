#ifndef LIBS_MESH_H
#define LIBS_MESH_H

// Définition de la classe Mesh

// IMPORTANT: cette représentation des mesh (point-triangle) est très inefficace et ne peut pas convenir à un projet important
// Consulter le livre Synthèse d'images avec OpenGL ES de Pierre Nerzic pour une meilleure modélisation (half-edge)


#include <vector>
#include <map>
#include <list>
#include <string>

#include <gl-matrix.h>
#include <utils.h>


// pré-déclarations
class Mesh;
namespace mesh {
    class Vertex;
    class Triangle;
}
class Material;

#include <Material.h>
#include <MeshVertex.h>
#include <MeshTriangle.h>


using namespace mesh;


/**
 * Cette classe représente l'ensemble du maillage : listes des sommets et des triangles, avec une méthode de dessin
 */
class Mesh
{
private:

    /// nom du maillage
    std::string m_Name;

    /// liste des sommets (accès direct => vector)
    std::vector<Vertex*> m_VertexList;

    /// liste des triangles (accès direct => vector)
    std::vector<Triangle*> m_TriangleList;

    // si true, les VBOS seront refaits au prochain dessin
    bool m_UpdateVBOs;

    // identifiants des VBOs
    GLint m_VertexBufferId;
    GLint m_ColorBufferId;
    GLint m_TexCoordsBufferId;
    GLint m_NormalBufferId;
    GLint m_TangentBufferId;
    GLint m_FacesIndexBufferId;
    GLint m_EdgesIndexBufferId;

    // types des VBOS d'index
    GLint m_FacesIndexBufferType;
    GLint m_EdgesIndexBufferType;

    // matériaux, l'un peut être null
    Material* m_FacesMaterial;
    Material* m_EdgesMaterial;

public:

    /**
     * constructeur. On lui fournit au moins un matériau (sous-classe de Material), pour les triangles et/ou les arêtes.
     * @param name : nom du maillage (pour la mise au point)
     * @param facesmaterial : sous-classe de Material pour dessiner les facettes
     * @param edgesmaterial : sous-classe de Material pour dessiner les arêtes
     */
    Mesh(std::string name, Material* facesmaterial=nullptr, Material* edgesmaterial=nullptr);

    /** destructeur */
    virtual ~Mesh();


    /**
     * affecte les matériaux après création de l'instance
     * @param facesmaterial : sous-classe de Material pour dessiner les facettes
     * @param edgesmaterial : sous-classe de Material pour dessiner les arêtes
     */
     void setMaterials(Material* facesmaterial=nullptr, Material* edgesmaterial=nullptr);


    /**
     * retourne le nom du maillage
     * @return nom du maillage
     */
    std::string getName()
    {
        return m_Name;
    }


    /**
     * retourne la liste des sommets du maillage
     * @return liste des sommets
     */
    std::vector<Vertex*>& getVertexList()
    {
        return m_VertexList;
    }


    /**
     * retourne la liste des triangles du maillage
     * @return liste des triangles
     */
    std::vector<Triangle*>& getTriangleList()
    {
        return m_TriangleList;
    }


    /**
     * retourne le nombre de sommets
     * @return nombre de sommets
     */
    int getVertexCount()
    {
        return m_VertexList.size();
    }


    /**
     * retourne le nombre de triangles
     * @return nombre de triangles
     */
    int getTriangleCount()
    {
        return m_TriangleList.size();
    }

    /**
     * retourne le triangle n°i (0..) du maillage, ou nullptr si i n'est pas correct
     * @param i : numéro 0..NT-1 du triangle
     * @return le Triangle() demandé ou nullptr si i n'est pas dans les bornes
     */
    Vertex* getVertex(int i);

    /**
     * retourne le triangle n°i (0..) du maillage, ou nullptr si i n'est pas correct
     * @param i : numéro 0..NT-1 du triangle
     * @return le Triangle() demandé ou nullptr si i n'est pas dans les bornes
     */
    Triangle* getTriangle(int i);

    /**
     * affiche le nombre de sommets et de triangles sur stdout
     */
    void info();

    /**
     * Cette méthode ajoute le sommet fourni à la fin de la liste
     * @param vertex sommet à rajouter
     */
    void pushVertex(Vertex* vertex);

    /**
     * Cette méthode enlève le sommet fourni de la liste
     * NB: la méthode ne le supprime pas (voir son destructeur pour cela)
     * @see #delVertex
     * @param vertex sommet à enlever
     */
    void popVertex(Vertex* vertex);

    /**
     * Cette méthode ajoute le triangle fourni à la fin de la liste
     * @param triangle à rajouter
     */
    void pushTriangle(Triangle* triangle);

    /**
     * Cette méthode enlève le triangle fourni de la liste
     * NB: la méthode ne le supprime pas (voir son destructeur pour cela)
     * @see #delTriangle
     * @param triangle à enlever
     */
    void popTriangle(Triangle* triangle);

    /**
     * Cette méthode crée et rajoute un triangle au maillage.
     * NB: l'ordre de rotation des sommets est crucial pour le calcul des normales.
     * Il faut tourner dans le sens trigonométrique, comme dans OpenGL.
     * @param v1 : le premier coin du triangle
     * @param v2 : le deuxième coin du triangle
     * @param v3 : le troisième coin du triangle
     * @return le nouveau triangle, ajouté au maillage
     */
    Triangle* addTriangle(Vertex* v1, Vertex* v2, Vertex* v3);

    /**
     * Cette méthode crée et rajoute un quadrilatère au maillage. En fait, cela
     * revient à créer deux triangles, (v1, v2, s4) et (s4, v2, v3).
     * NB: l'ordre de rotation des sommets est crucial pour le calcul des normales.
     * Il faut tourner dans le sens trigonométrique, comme dans OpenGL.
     * NB : les triangles sont définis dans l'ordre qui facilite la création de rubans
     * @param v1 : l'un des coins du quad
     * @param v2 : l'un des coins du quad
     * @param v3 : l'un des coins du quad
     * @param s4 : l'un des coins du quad
     */
    void addQuad(Vertex* v1, Vertex* v2, Vertex* v3, Vertex* s4);

    /**
     * Cette méthode supprime le triangle du maillage en mettant à jour toutes
     * les listes
     * @param triangle : celui qu'il faut supprimer
     */
    void delTriangle(Triangle* triangle);

    /**
     * Cette méthode supprime le sommet du maillage en mettant à jour toutes
     * les listes : tous les triangles qui le contiennent sont aussi supprimés.
     * @param vertex : celui qu'il faut supprimer
     */
    void delVertex(Vertex* vertex);


    /**
     * Cette méthode recalcule les normales des triangles et sommets.
     * Les normales des triangles sont calculées d'après leurs côtés.
     * Les normales des sommets sont les moyennes des normales des triangles
     * auxquels ils appartiennent.
     */
    void computeNormals();

    /**
     * Cette méthode recalcule les tangentes des triangles et sommets.
     * Les tangentes des triangles sont calculées d'après leurs côtés et les coordonnées de texture.
     * Les tangentes des sommets sont les moyennes des tangentes des triangles
     * auxquels ils appartiennent.
     */
    void computeTangents();


    /**
     * Cette méthode lit le fichier indiqué, il contient un maillage au format OBJ
     * @param filename : nom complet du fichier à lire
     */
    void loadObj(std::string filename);


    /**
     * Cette méthode retourne l'identifiant du VBO contenant les coordonnées 3D des sommets.
     * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
     * Cette méthode met aussi à jour tous les indices m_Index des sommets
     * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des coordonnées
     */
    GLint getVertexBufferId();

    /**
     * Cette méthode retourne l'identifiant du VBO contenant les couleurs des sommets.
     * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
     * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des couleurs
     */
    GLint getColorBufferId();

    /**
     * Cette méthode retourne l'identifiant du VBO contenant les coordonnées de texture des sommets.
     * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
     * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des coordonnées de texture 2D
     */
    GLint getTexCoordsBufferId();

    /**
     * Cette méthode retourne l'identifiant du VBO contenant les normales des sommets.
     * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
     * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des normales
     */
    GLint getNormalBufferId();

    /**
     * Cette méthode retourne l'identifiant du VBO contenant les tangentes des sommets.
     * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
     * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des tangentes
     */
    GLint getTangentBufferId();

    /**
     * Cette méthode retourne l'identifiant du VBO contenant les indices pour dessiner les triangles en primitives indexées.
     * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
     * Il faut avoir appelé getVertexBufferId auparavant pour que les indices soient corrects et cohérents
     * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des indices de triangles
     */
    GLint getFacesIndexBufferId();

    /**
     * Cette méthode retourne l'identifiant du VBO contenant les indices pour dessiner les arêtes en primitives indexées.
     * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
     * Il faut avoir appelé getVertexBufferId auparavant pour que les indices soient corrects et cohérents
     * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des indices de lignes
     */
    GLint getEdgesIndexBufferId();

    /**
     * dessiner le maillage s'il est prêt. S'il y a un matériau pour les faces, elles sont dessinées, pareil pour les arêtes.
     * @param matP : matrice de projection perpective
     * @param matVM : matrice de transformation de l'objet par rapport à la caméra
     */
    void onDraw(const mat4& matP, const mat4& matVM);

    /**
     * modifie les coordonnées des sommets par la matrice indiquée
     * @param matT mat4 qui est appliquée sur chaque sommet
     */
    void transform(mat4 matT);
};



#endif
