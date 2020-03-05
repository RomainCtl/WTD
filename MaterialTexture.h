#ifndef MATERIALTEXTURE_H
#define MATERIALTEXTURE_H

// Définition de la classe MaterialTexture

#include <Mesh.h>
#include <Light.h>
#include <Texture2D.h>
#include <gl-matrix.h>


class MaterialTexture: public Material
{
private:

    // texture
    GLint m_TextureLoc;
    Texture2D* m_Texture;

    // variables uniform du shader
    int m_LightColorLoc;
    int m_LightPositionLoc;
    int m_LightDirectionLoc;
    int m_CosMaxAngleLoc;
    int m_CosMinAngleLoc;


public:

    /**
     * constructeur
     * @param filename : nom du fichier contenant l'image à charger
     * @param filtering : mettre GL_LINEAR ou gl.NEAREST ou GL_LINEAR_MIPMAP_LINEAR (mipmaps)
     * @param repetition : mettre GL_CLAMP_TO_EDGE ou GL_REPEAT
     */
    MaterialTexture(std::string filename, GLenum filtering=GL_LINEAR, GLenum repetition=GL_CLAMP_TO_EDGE);


    /**
     * définit la lampe
     * @param light : instance de Light spécifiant les caractéristiques de la lampe
     */
    virtual void setLight(Light* light);


    virtual void select(Mesh* mesh, const mat4& matP, const mat4& matVM);


    virtual void deselect();


    virtual ~MaterialTexture();
};

#endif
