#ifndef LIBS_VERTEX_H
#define LIBS_VERTEX_H

// Définition de la classe Vertex dans le namespace mesh

#include <vector>
#include <map>
#include <list>
#include <string>

#include <gl-matrix.h>
#include <utils.h>

#include <Mesh.h>


namespace mesh {


    /**
     * Cette classe représente un sommet dans le maillage.
     */
    class Vertex
    {
    private:

        /// numéro du sommet dans les VBOs d'indices
        long m_Index;

        /// attributs du sommet
        vec3 m_Coords;
        vec3 m_Color;
        vec2 m_TexCoords;
        vec3 m_Normal;
        vec3 m_Tangent;

        /// maillage d'appartenance de ce sommet
        Mesh* m_Mesh;



    public:

        /**
         * Constructeur de la classe Vertex qui représente un sommet dans
         * le maillage. Initialise un sommet non connecté au maillage, employer
         * Maillage.addVertex pour l'insérer puis new Triangle(...) pour le mettre
         * dans un triangle, setCoord et setColor pour lui donner des coordonnées et
         * des couleurs.
         * @param mesh : maillage d'appartenance de ce sommet
         * @param nom : nom du sommet à créer
         */
        Vertex(Mesh* mesh, vec3 xyz);
        Vertex(Mesh* mesh, float x, float y, float z);
        Vertex(Mesh* mesh, double x, double y, double z);

        /**
         * Destructeur de la classe Vertex
         */
        virtual ~Vertex();

        /**
         * réaffecte le mesh de ce sommet
         */
        void setMesh(Mesh* mesh);

        /**
         * réaffecte le numéro de ce sommet
         */
        void setIndex(long index);

        /**
         * retourne le numéro de ce sommet
         */
        long getIndex()
        {
            return m_Index;
        }

        /**
         * définit les coordonnées du sommet
         * @param xyz coordonnées
         * @return this pour pouvoir chaîner les affectations
         */
        Vertex* setCoords(vec3 xyz);
        Vertex* setCoords(float x, float y, float z);
        Vertex* setCoords(double x, double y, double z);

        /**
         * retourne les coordonnées du sommet
         * @return coordonnées 3D du sommet
         */
        vec3& getCoords()
        {
            return m_Coords;
        }


        /**
         * définit la couleur du sommet
         * @param rgb couleur (r,g,b)
         * @return this pour pouvoir chaîner les affectations
         */
        Vertex* setColor(vec3 rgb);
        Vertex* setColor(float r, float g, float b);
        Vertex* setColor(double r, double g, double b);

        /**
         * retourne la couleur du sommet
         * @return couleur (r,g,b)
         */
        vec3& getColor()
        {
            return m_Color;
        }

        /**
         * définit les coordonnées de la normale du sommet
         * @param normal normale à affecter
         * @return this pour pouvoir chaîner les affectations
         */
        Vertex* setNormal(vec3 normal);
        Vertex* setNormal(float x, float y, float z);
        Vertex* setNormal(double x, double y, double z);

        /**
         * retourne la normale du sommet
         * @return normale
         */
        vec3& getNormal()
        {
            return m_Normal;
        }

        /**
         * retourne la tangente du sommet
         * @return tangente
         */
        vec3& getTangent()
        {
            return m_Tangent;
        }


        /**
         * définit les coordonnées de texture du sommet
         * @param uv coordonnées de texture
         * @return this pour pouvoir chaîner les affectations
         */
        Vertex* setTexCoords(vec2 uv);
        Vertex* setTexCoords(float u, float v);
        Vertex* setTexCoords(double u, double v);

        /**
         * retourne les coordonnées de texture du sommet
         * @return coordonnées de texture
         */
        vec2& getTexCoords()
        {
            return m_TexCoords;
        }

        /**
         * Cette méthode calcule la normale du sommet = moyenne des normales des
         * triangles contenant ce sommet.
         */
        void computeNormal();

        /**
         * Cette méthode calcule la tangente du sommet = moyenne des tangentes des
         * triangles contenant ce sommet.
         */
        void computeTangent();
    };
}

#endif
