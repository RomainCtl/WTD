#include <GL/glew.h>
#include <GL/gl.h>

#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <iterator>
#include <vector>
#include <stdexcept>

#include <utils.h>
#include <Mesh.h>

// IMPORTANT: cette représentation des mesh inefficace ne peut pas convenir à un projet important
// Consulter le livre Synthèse d'images avec OpenGL ES de Pierre Nerzic pour une meilleure modélisation (half-edge)

using namespace mesh;


/**
 * constructeur. On lui fournit au moins un matériau (sous-classe de Material), pour les triangles et/ou les arêtes.
 * @param name : nom du maillage (pour la mise au point)
 * @param facesmaterial : sous-classe de Material pour dessiner les facettes
 * @param edgesmaterial : sous-classe de Material pour dessiner les arêtes
 */
Mesh::Mesh(std::string name, Material* facesmaterial, Material* edgesmaterial)
{
    m_Name = name;

    // identifiants des VBOs
    m_VertexBufferId     = -1;
    m_ColorBufferId      = -1;
    m_TexCoordsBufferId  = -1;
    m_NormalBufferId     = -1;
    m_TangentBufferId    = -1;
    m_FacesIndexBufferId = -1;
    m_EdgesIndexBufferId = -1;

    // matériaux, l'un peut être null
    m_FacesMaterial = facesmaterial;
    m_EdgesMaterial = edgesmaterial;

    // refaire les VBOs
    m_UpdateVBOs = true;
}


/**
 * affecte les matériaux après création de l'instance
 * @param facesmaterial : sous-classe de Material pour dessiner les facettes
 * @param edgesmaterial : sous-classe de Material pour dessiner les arêtes
 */
void Mesh::setMaterials(Material* facesmaterial, Material* edgesmaterial)
{
    // matériaux, l'un peut être null
    m_FacesMaterial = facesmaterial;
    m_EdgesMaterial = edgesmaterial;
}


/**
 * Cette méthode ajoute le sommet fourni à la fin de la liste
 * @param vertex sommet à rajouter
 */
void Mesh::pushVertex(Vertex* vertex)
{
    m_VertexList.push_back(vertex);

    // refaire les VBOs
    m_UpdateVBOs = true;
}


/**
 * Cette méthode enlève le sommet fourni de la liste
 * NB: la méthode ne le supprime pas (voir son destructeur pour cela)
 * @see #delVertex
 * @param vertex sommet à enlever
 */
void Mesh::popVertex(Vertex* vertex)
{
    m_VertexList.erase(
        std::remove_if(
            m_VertexList.begin(),
            m_VertexList.end(),
            [vertex](Vertex* v){ return v == vertex;}),
        m_VertexList.end());

    // refaire les VBOs
    m_UpdateVBOs = true;
}


/**
 * Cette méthode ajoute le triangle fourni à la fin de la liste
 * @param triangle à rajouter
 */
void Mesh::pushTriangle(Triangle* triangle)
{
    m_TriangleList.push_back(triangle);

    // refaire les VBOs
    m_UpdateVBOs = true;
}


/**
 * Cette méthode enlève le triangle fourni de la liste
 * NB: la méthode ne le supprime pas (voir son destructeur pour cela)
 * @see #delTriangle
 * @param triangle à enlever
 */
void Mesh::popTriangle(Triangle* triangle)
{
    m_TriangleList.erase(
        std::remove_if(
            m_TriangleList.begin(),
            m_TriangleList.end(),
            [triangle](Triangle* t){ return t == triangle;}),
        m_TriangleList.end());

    // refaire les VBOs
    m_UpdateVBOs = true;
}


/**
 * Cette méthode crée et rajoute un triangle au maillage.
 * NB: l'ordre de rotation des sommets est crucial pour le calcul des normales.
 * Il faut tourner dans le sens trigonométrique, comme dans OpenGL.
 * @param v1 : le premier coin du triangle
 * @param v2 : le deuxième coin du triangle
 * @param v3 : le troisième coin du triangle
 * @return le nouveau triangle, ajouté au maillage
 */
Triangle* Mesh::addTriangle(Vertex* v1, Vertex* v2, Vertex* v3)
{
    return new Triangle(this, v1, v2, v3);
}


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
void Mesh::addQuad(Vertex* v1, Vertex* v2, Vertex* v3, Vertex* s4)
{
    addTriangle(v1, v2, s4);
    addTriangle(s4, v2, v3);
}


/**
 * Cette méthode supprime le triangle du maillage en mettant à jour toutes
 * les listes
 * @param triangle : celui qu'il faut supprimer
 */
void Mesh::delTriangle(Triangle* triangle)
{
    delete triangle;
}


/**
 * Cette méthode supprime le sommet du maillage en mettant à jour toutes
 * les listes : tous les triangles qui le contiennent sont aussi supprimés.
 * @param vertex : celui qu'il faut supprimer
 */
void Mesh::delVertex(Vertex* vertex)
{
    delete vertex;
}


/**
 * Cette méthode recalcule les normales des triangles et sommets.
 * Les normales des triangles sont calculées d'après leurs côtés.
 * Les normales des sommets sont les moyennes des normales des triangles
 * auxquels ils appartiennent.
 */
void Mesh::computeNormals()
{
    // calculer les normales des triangles
    for (Triangle* triangle: m_TriangleList) {
        triangle->computeNormal();
    }

    // calculer les normales des sommets
    int iv = 0;
    for (Vertex* vertex: m_VertexList) {
        // renuméroter le sommet (numéro dans les VBOs)
        vertex->setIndex(iv);
        iv++;
        // recalculer la normale de ce sommet
        vertex->computeNormal();
    }
}


/**
 * Cette méthode recalcule les tangentes des triangles et sommets.
 * Les tangentes des triangles sont calculées d'après leurs côtés et les coordonnées de texture.
 * Les tangentes des sommets sont les moyennes des tangentes des triangles
 * auxquels ils appartiennent.
 */
void Mesh::computeTangents()
{
    // calculer les tangentes des triangles
    for (Triangle* triangle: m_TriangleList) {
        triangle->computeTangent();
    }

    // calculer les tangentes des sommets
    for (Vertex* vertex: m_VertexList) {
        // recalculer la tangente de ce sommet
        vertex->computeTangent();
    }
}


static Vertex* findOrCreateVertex(
    Mesh* mesh,
    char* nvntnn,
    std::map<int, std::list<Vertex*>> &vertexlist,
    std::vector<vec3> &coordlist,
    std::vector<vec2> &texcoordlist,
    std::vector<vec3> &normallist)
{
    // indices des coordonnées 3D, des coordonnées de texture et de la normale
    int nv = 0;
    int nt = 0;
    int nn = 0;
    sscanf(nvntnn, "%d", &nv);
    sscanf(nvntnn, "%d//%d", &nv, &nn);
    sscanf(nvntnn, "%d/%d", &nv, &nt);
    sscanf(nvntnn, "%d/%d/%d", &nv, &nt, &nn);
    if (nv == 0) return nullptr;
    if (nv < 0) nv = coordlist.size() + nv; else nv = nv - 1;
    if (nt < 0) nt = texcoordlist.size() + nt; else nt = nt - 1;
    if (nt >= texcoordlist.size()) nt = -1;
    if (nn < 0) nn = normallist.size() + nn; else nn = nn - 1;
    if (nn >= normallist.size()) nn = -1;

    // identifiant du sommet courant
    int index = (nv*1000000 + nt)*1000000 + nn;

    // parcourir les sommets de la liste nv et voir s'il y a le même
    std::list<Vertex*>& siblings = vertexlist[nv];
    for (Vertex* other: siblings) {
        // si les noms sont égaux, c'est le même sommet
        if (index == other->getIndex()) return other;
    }

    // il faut créer un nouveau sommet car soit nouveau, soit un peu différent des autres
    Vertex* vertex = new Vertex(mesh, coordlist[nv]);
    vertex->setIndex(index);
    if (nt >= 0) vertex->setTexCoords(texcoordlist[nt]);
    if (nn >= 0) vertex->setNormal(normallist[nn]);

    // on ajoute ce sommet dans la liste de ceux qui ont le même numéro nv
    siblings.push_front(vertex);
    return vertex;
}


/**
 * Cette méthode lit le fichier indiqué, il contient un maillage au format OBJ
 * @param filename : nom complet du fichier à lire
 */
void Mesh::loadObj(std::string filename)
{
    // tableaux des sommets qu'on va extraire du fichier obj, ils sont groupés par indice nv
    std::map<int, std::list<Vertex*>> vertexlist;

    // tableau des coordonnées, des textures et des normales
    std::vector<vec3> coordlist;
    std::vector<vec2> texcoordlist;
    std::vector<vec3> normallist;

    // ouverture du fichier
    std::ifstream inputStream;
    inputStream.open(filename.c_str(), std::ifstream::in);
    if (! inputStream.is_open()) {
        std::cerr << "Error : \"" << filename << "\" cannot be loaded, check pathname and permissions." << std::endl;
        return;
    }

    // parcourir le fichier obj ligne par ligne
    char* word = NULL;
    char* saveptr_mot = NULL;
    int line_number = 0;
    char line[180];
    while (inputStream.getline(line, sizeof(line))) {
        line_number++;
        // extraire le premier mot de la ligne
        word = strtok_r(line," \t", &saveptr_mot);
        if (word == NULL) continue;
        // mettre le mot en minuscules
        for (char* c=word; *c!='\0'; ++c) *c = tolower(*c);

        if (strcmp(word,"f") == 0) {
            // lire les numéros du premier point
            if (! (word = strtok_r(NULL, " \t", &saveptr_mot))) continue;
            Vertex* v1 = findOrCreateVertex(this, word, vertexlist, coordlist, texcoordlist, normallist);
            // lire les numéros du deuxième point
            if (! (word = strtok_r(NULL, " \t", &saveptr_mot))) continue;
            Vertex* v2 = findOrCreateVertex(this, word, vertexlist, coordlist, texcoordlist, normallist);
            // lire et traiter les points suivants
            while ((word = strtok_r(NULL, " \t", &saveptr_mot))) {
                Vertex* v3 = findOrCreateVertex(this, word, vertexlist, coordlist, texcoordlist, normallist);
                // ajouter un triangle v1,v2,v3
                if (v1 != nullptr && v2 != nullptr && v3 != nullptr) {
                    addTriangle(v1,v2,v3);
                }
                // préparer le passage au triangle suivant
                v2 = v3;
            }
        } else
        if (strcmp(word,"v") == 0) {
            // coordonnées du sommet
            float x = atof(strtok_r(NULL, " \t", &saveptr_mot));
            float y = atof(strtok_r(NULL, " \t", &saveptr_mot));
            float z = atof(strtok_r(NULL, " \t", &saveptr_mot));
            vec3 coords = vec3::fromValues(x,y,z);
            coordlist.push_back(coords);
        } else
        if (strcmp(word,"vt") == 0) {
            // coordonnées de texture
            float u = atof(strtok_r(NULL, " \t", &saveptr_mot));
            float v = atof(strtok_r(NULL, " \t", &saveptr_mot));
            texcoordlist.push_back(vec2::fromValues(u,v));
        } else
        if (strcmp(word,"vn") == 0) {
            // coordonnées de la normale
            float nx = atof(strtok_r(NULL, " \t", &saveptr_mot));
            float ny = atof(strtok_r(NULL, " \t", &saveptr_mot));
            float nz = atof(strtok_r(NULL, " \t", &saveptr_mot));
            normallist.push_back(vec3::fromValues(nx,ny,nz));
        }
    }

    // message
    std::cout<<m_Name<<" : obj loaded,"<<m_VertexList.size()<<" vertices,"<<m_TriangleList.size()<<" triangles"<<std::endl;
}


/**
 * Cette méthode retourne l'identifiant du VBO contenant les coordonnées 3D des sommets.
 * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
 * Cette méthode met aussi à jour tous les indices m_Index des sommets
 * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des coordonnées
 */
GLint Mesh::getVertexBufferId()
{
    // faut-il refaire le VBO ?
    if (m_UpdateVBOs && m_VertexBufferId >= 0) {
        Utils::deleteVBO(m_VertexBufferId);
        m_VertexBufferId = -1;
    }

    // créer le VBO s'il n'a pas été déjà créé
    if (m_VertexBufferId < 0) {

        std::vector<float> array;
        int num = 0;
        for (Vertex* v: m_VertexList) {
            v->setIndex(num);
            num++;
            vec3& data = v->getCoords();
            array.push_back(data[0]); array.push_back(data[1]); array.push_back(data[2]);
        }
        m_VertexBufferId = Utils::makeFloatVBO(array, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    }

    // retourner l'identifiant du VBO
    return m_VertexBufferId;
}


/**
 * Cette méthode retourne l'identifiant du VBO contenant les couleurs des sommets.
 * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
 * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des couleurs
 */
GLint Mesh::getColorBufferId()
{
    // faut-il refaire le VBO ?
    if (m_UpdateVBOs && m_ColorBufferId >= 0) {
        Utils::deleteVBO(m_ColorBufferId);
        m_ColorBufferId = -1;
    }

    // créer le VBO s'il n'a pas été déjà créé
    if (m_ColorBufferId < 0) {

        std::vector<float> array;
        for (Vertex* v: m_VertexList) {
            vec3& data = v->getColor();
            array.push_back(data[0]); array.push_back(data[1]); array.push_back(data[2]);
        }
        m_ColorBufferId = Utils::makeFloatVBO(array, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    }

    // retourner l'identifiant du VBO
    return m_ColorBufferId;
}


/**
 * Cette méthode retourne l'identifiant du VBO contenant les coordonnées de texture des sommets.
 * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
 * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des coordonnées de texture 2D
 */
GLint Mesh::getTexCoordsBufferId()
{
    // faut-il refaire le VBO ?
    if (m_UpdateVBOs && m_TexCoordsBufferId >= 0) {
        Utils::deleteVBO(m_TexCoordsBufferId);
        m_TexCoordsBufferId = -1;
    }

    // créer le VBO s'il n'a pas été déjà créé
    if (m_TexCoordsBufferId < 0) {

        std::vector<float> array;
        for (Vertex* v: m_VertexList) {
            vec2& data = v->getTexCoords();
            array.push_back(data[0]); array.push_back(data[1]);
        }
        m_TexCoordsBufferId = Utils::makeFloatVBO(array, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    }

    // retourner l'identifiant du VBO
    return m_TexCoordsBufferId;
}


/**
 * Cette méthode retourne l'identifiant du VBO contenant les normales des sommets.
 * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
 * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des normales
 */
GLint Mesh::getNormalBufferId()
{
    // faut-il refaire le VBO ?
    if (m_UpdateVBOs && m_NormalBufferId >= 0) {
        Utils::deleteVBO(m_NormalBufferId);
        m_NormalBufferId = -1;
    }

    // créer le VBO s'il n'a pas été déjà créé
    if (m_NormalBufferId < 0) {

        std::vector<float> array;
        for (Vertex* v: m_VertexList) {
            vec3& data = v->getNormal();
            array.push_back(data[0]); array.push_back(data[1]); array.push_back(data[2]);
        }
        m_NormalBufferId = Utils::makeFloatVBO(array, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    }

    // retourner l'identifiant du VBO
    return m_NormalBufferId;
}


/**
 * Cette méthode retourne l'identifiant du VBO contenant les tangentes des sommets.
 * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
 * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des tangentes
 */
GLint Mesh::getTangentBufferId()
{
    // faut-il refaire le VBO ?
    if (m_UpdateVBOs && m_TangentBufferId >= 0) {
        Utils::deleteVBO(m_TangentBufferId);
        m_TangentBufferId = -1;
    }

    // créer le VBO s'il n'a pas été déjà créé
    if (m_TangentBufferId < 0) {

        std::vector<float> array;
        for (Vertex* v: m_VertexList) {
            vec3& data = v->getTangent();
            array.push_back(data[0]); array.push_back(data[1]); array.push_back(data[2]);
        }
        m_TangentBufferId = Utils::makeFloatVBO(array, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    }

    // retourner l'identifiant du VBO
    return m_TangentBufferId;
}


/**
 * Cette méthode retourne l'identifiant du VBO contenant les indices pour dessiner les triangles en primitives indexées.
 * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
 * Il faut avoir appelé getVertexBufferId auparavant pour que les indices soient corrects et cohérents
 * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des indices de triangles
 */
GLint Mesh::getFacesIndexBufferId()
{
    // faut-il refaire le VBO ?
    if (m_UpdateVBOs && m_FacesIndexBufferId >= 0) {
        Utils::deleteVBO(m_FacesIndexBufferId);
        m_FacesIndexBufferId = -1;
    }

    // créer le VBO s'il n'a pas été déjà créé
    if (m_FacesIndexBufferId < 0) {

        // selon le nombre d'indices : entiers 32 bits ou shorts 16 bits
        if (m_TriangleList.size() > 65532/3) {
            // créer le VBO des indices int pour dessiner les triangles
            std::vector<GLuint> indexlist;
            for (Triangle* t: m_TriangleList) {
                indexlist.push_back(t->getVertex(0)->getIndex());
                indexlist.push_back(t->getVertex(1)->getIndex());
                indexlist.push_back(t->getVertex(2)->getIndex());
            }
            m_FacesIndexBufferId = Utils::makeIntVBO(indexlist, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
            m_FacesIndexBufferType = GL_UNSIGNED_INT;
        } else {
            // créer le VBO des indices short pour dessiner les triangles
            std::vector<GLushort> indexlist;
            for (Triangle* t: m_TriangleList) {
                indexlist.push_back(t->getVertex(0)->getIndex());
                indexlist.push_back(t->getVertex(1)->getIndex());
                indexlist.push_back(t->getVertex(2)->getIndex());
            }
            m_FacesIndexBufferId = Utils::makeShortVBO(indexlist, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
            m_FacesIndexBufferType = GL_UNSIGNED_SHORT;
        }
    }

    // retourner l'identifiant du VBO
    return m_FacesIndexBufferId;
}


/**
 * Cette méthode retourne l'identifiant du VBO contenant les indices pour dessiner les arêtes en primitives indexées.
 * Elle construit ce VBO s'il n'est pas encore créé mais que le maillage est complet
 * Il faut avoir appelé getVertexBufferId auparavant pour que les indices soient corrects et cohérents
 * @return null si le maillage n'est pas prêt, sinon c'est l'identifiant WebGL du VBO des indices de lignes
 */
GLint Mesh::getEdgesIndexBufferId()
{
    // faut-il refaire le VBO ?
    if (m_UpdateVBOs && m_EdgesIndexBufferId >= 0) {
        Utils::deleteVBO(m_EdgesIndexBufferId);
        m_EdgesIndexBufferId = -1;
    }

    // créer le VBO s'il n'a pas été déjà créé
    if (m_EdgesIndexBufferId < 0) {

        // selon le nombre d'indices : entiers 32 bits ou shorts 16 bits
        if (m_TriangleList.size() > 65532/6) {
            // VBO des indices des arêtes
            std::vector<GLuint> indexlist;
            for (Triangle* t: m_TriangleList) {
                indexlist.push_back(t->getVertex(0)->getIndex());
                indexlist.push_back(t->getVertex(1)->getIndex());

                indexlist.push_back(t->getVertex(1)->getIndex());
                indexlist.push_back(t->getVertex(2)->getIndex());

                indexlist.push_back(t->getVertex(2)->getIndex());
                indexlist.push_back(t->getVertex(0)->getIndex());
            }
            m_EdgesIndexBufferId = Utils::makeIntVBO(indexlist, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
            m_EdgesIndexBufferType = GL_UNSIGNED_INT;
        } else {
            // VBO des indices des arêtes
            std::vector<GLushort> indexlist;
            for (Triangle* t: m_TriangleList) {
                indexlist.push_back(t->getVertex(0)->getIndex());
                indexlist.push_back(t->getVertex(1)->getIndex());

                indexlist.push_back(t->getVertex(1)->getIndex());
                indexlist.push_back(t->getVertex(2)->getIndex());

                indexlist.push_back(t->getVertex(2)->getIndex());
                indexlist.push_back(t->getVertex(0)->getIndex());
            }
            m_EdgesIndexBufferId = Utils::makeShortVBO(indexlist, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
            m_EdgesIndexBufferType = GL_UNSIGNED_SHORT;
        }
    }

    // retourner l'identifiant du VBO
    return m_EdgesIndexBufferId;
}


/**
 * dessiner le maillage s'il est prêt. S'il y a un matériau pour les faces, elles sont dessinées, pareil pour les arêtes.
 * @param matP : matrice de projection perpective
 * @param matVM : matrice de transformation de l'objet par rapport à la caméra
 */
void Mesh::onDraw(const mat4& matP, const mat4& matVM)
{
    // le matériau des facettes est-il défini ?
    if (m_FacesMaterial != nullptr) {

        // décalage des polygones s'il y a aussi les arêtespushVertex
        if (m_EdgesMaterial != nullptr) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0, 1.0);
        }

        // activer le matériau des triangles
        m_FacesMaterial->select(this, matP, matVM);

        // activer et lier le buffer contenant les indices
        int facesindexbufferid = getFacesIndexBufferId();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, facesindexbufferid);

        // les VBOs sont à jour
        m_UpdateVBOs = false;

        // dessiner les triangles
        glDrawElements(GL_TRIANGLES, m_TriangleList.size() * 3, m_FacesIndexBufferType, 0);

        // désactiver le matériau
        m_FacesMaterial->deselect();

        // désactiver le VBO des indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // fin du décalage des polygones s'il y a les arêtes
        if (m_EdgesMaterial != nullptr) {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    // le matériau des arêtes est-il défini ?
    if (m_EdgesMaterial != nullptr) {

        // activer le matériau des arêtes
        m_EdgesMaterial->select(this, matP, matVM);

        // activer et lier le buffer contenant les indices
        int edgesindexbufferid = getEdgesIndexBufferId();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgesindexbufferid);

        // dessiner les triangles
        glDrawElements(GL_LINES, m_TriangleList.size() * 6, m_EdgesIndexBufferType, 0);

        // désactiver le matériau
        m_EdgesMaterial->deselect();

        // désactiver le VBO des indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}


/**
 * modifie les coordonnées des sommets par la matrice indiquée
 * @param matT mat4 qui est appliquée sur chaque sommet
 */
void Mesh::transform(mat4 matT)
{
    for (Vertex* vertex: m_VertexList) {
        vec3::transformMat4(vertex->getCoords(), vertex->getCoords(), matT);
    }
}




/**
 * destructeur
 */
Mesh::~Mesh()
{
    // supprimer les triangles
    for (Triangle* triangle: m_TriangleList) {
        triangle->setMesh(nullptr);
        delete triangle;
    }

    // supprimer les sommets
    for (Vertex* vertex: m_VertexList) {
        vertex->setMesh(nullptr);
        delete vertex;
    }

    // supprimer les VBOs (le shader n'est pas créé ici)
    Utils::deleteVBO(m_VertexBufferId);
    Utils::deleteVBO(m_ColorBufferId);
    Utils::deleteVBO(m_TexCoordsBufferId);
    Utils::deleteVBO(m_NormalBufferId);
    Utils::deleteVBO(m_FacesIndexBufferId);
    Utils::deleteVBO(m_EdgesIndexBufferId);
}

