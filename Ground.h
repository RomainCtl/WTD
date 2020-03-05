#ifndef GROUND_H
#define GROUND_H

// Définition de la classe Ground

#include <Mesh.h>
#include <Light.h>
#include <MaterialTexture.h>
#include <gl-matrix.h>

class Ground: public Mesh
{
private:

    /** matériau */
    MaterialTexture* m_Material;


public:

    Ground();

    virtual ~Ground();

    /**
     * définit la lampe
     * @param light : instance de Light spécifiant les caractéristiques de la lampe
     */
    void setLight(Light* light);
};

#endif
