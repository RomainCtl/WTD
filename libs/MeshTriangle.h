#ifndef LIBS_TRIANGLE_H
#define LIBS_TRIANGLE_H

// Définition de la classe Triangle dans le namespace mesh

#include <iterator>
#include <vector>
#include <map>
#include <list>
#include <string>

#include <gl-matrix.h>
#include <utils.h>

#include <Mesh.h>

namespace mesh {

    /**
     * Cette classe représente l'un des triangles d'un maillage
     */
    class Triangle
    {
    private:

        /// maillage d'appartenance du triangle
        Mesh* m_Mesh;

        // sommets
        Vertex* m_Vertices[3];

        // attributs de triangle
        vec3 m_Normal;
        vec3 m_Tangent;


    public:

         /**
         * Constructeur de la classe Triangle à partir de trois sommets
         * NB: l'ordre de rotation des sommets est crucial pour le calcul des normales.
         * Il faut tourner dans le sens trigonométrique, comme dans OpenGL.
         * @param maillage : maillage dans lequel on rajoute ce triangle
         * @param v0 : l'un des coins du triangle, nullptr si création manuelle
         * @param v1 : l'un des coins du triangle, nullptr si création manuelle
         * @param v2 : l'un des coins du triangle, nullptr si création manuelle
         */
        Triangle(Mesh* mesh, Vertex* v0, Vertex* v1, Vertex* v2);

        /**
         * Cette méthode supprime ce triangle du maillage en mettant à jour toutes
         * les listes. Cela peut supprimer des arêtes et rendre des sommets isolés.
         */
        virtual ~Triangle();

        /**
         * retourne le sommet n°n (0..2) du triangle, ou nullptr si n n'est pas correct
         * @param n : numéro 0..2 du sommet
         * @return le Vertex() demandé ou nullptr si n n'est pas dans les bornes
         */
        Vertex* getVertex(int n);

        /**
         * réaffecte le mesh de ce triangle
         */
        void setMesh(Mesh* mesh);

        /**
         * retourne la valeur de la normale
         * @see #calcNormal pour la calculer auparavant
         * @return normale du triangle
         */
        vec3 getNormal()
        {
            return m_Normal;
        }

        /**
         * recalcule les informations géométriques du triangle : centre, normale, surface...
         */
        void computeNormal();

        /**
         * retourne la valeur de la tangente
         * @see #calcTangente pour la calculer auparavant
         * @return tangente du triangle
         */
        vec3 getTangent()
        {
            return m_Tangent;
        }

        /**
         * recalcule la tangente du triangle à l'aide de la normale et des coordonnées de texture
         */
        void computeTangent();

        /**
         * Cette méthode indique si le triangle this contient le sommet indiqué
         * @param vertex : sommet dont il faut vérifier l'appartenance à this
         * @return true si ok, false si le sommet est absent du triangle
         */
        bool containsVertex(Vertex* vertex);
    };
}

#endif
