#ifndef DUCK_H
#define DUCK_H

// Définition de la classe Duck

#include <Mesh.h>
#include <Light.h>
#include <MaterialTexture.h>
#include <gl-matrix.h>


class Duck: public Mesh
{
private:

    /** matériau */
    MaterialTexture* m_Material;


    /** buffers pour la gestion du son */
    ALuint buffer, source;

    /** position 3D du cube */
    vec3 m_Position;

    /** position 3D du cube */
    vec3 m_Orientation;

    bool m_Draw, m_Sound;

public:

    /** constructeur, crée le maillage */
    Duck();

    /** destructeur, libère le maillage et l'audio */
    ~Duck();

    /**
     * dessiner le canard
     * @param matP : matrice de projection
     * @param matMV : matrice view*model (caméra * position objet)
     */
    void onRender(const mat4& matP, const mat4& matMV);

    /**
     * retourne la position % scèce du cube
     * @return vec3 position
     */
    vec3& getPosition();

    /**
     * affecte la position % scène du cube
     * @param vec3 pos position
     */
    void setPosition(vec3 pos);

    /**
     * retourne la position % scèce du cube
     * @return vec3 position
     */
    vec3& getOrientation();

    /**
     * affecte la position % scène du cube
     * @param vec3 pos position
     */
    void setOrientation(vec3 ori);

    /**
     * modifie la propriete d'affichage
=     */
    void setDraw(bool b);

    /**
     * modifie la propriete de son
=     */
    void setSound(bool b);

    /**
     * définit la lampe
     * @param light : instance de Light spécifiant les caractéristiques de la lampe
     */
    void setLight(Light* light);
};

#endif
