#ifndef LIBS_MATERIAL_H
#define LIBS_MATERIAL_H

// Définition de la classe abstraite Material, superclasse de tous les matériaux


#include <gl-matrix.h>
#include <utils.h>

#include <Mesh.h>


class Material
{
protected:

    /**
     * constructeur
     * @param srcVertexShader : vertex shader
     * @param srcFragmentShader : fragment shader
     * @param name : nom du matériau
     */
    Material(std::string srcVertexShader, std::string srcFragmentShader, std::string name="undefined");

    /**
     * constructeur quand les shaders ne peuvent pas être écrits lors de la construction
     * @param name : nom du matériau
     */
    Material(std::string name="undefined");


public:

    /** destructeur */
    virtual ~Material();


    /**
     * définit les shaders
     * @param srcVertexShader : vertex shader
     * @param srcFragmentShader : fragment shader
     */
    void setShaders(std::string srcVertexShader, std::string srcFragmentShader);

    /**
     * active le matériau : son shader et lie les variables uniform communes
     * @param mesh : maillage pour lequel on active ce matériau
     * @param matP : matrice de projection perpective
     * @param matVM : matrice de transformation de l'objet par rapport à la caméra
     */
    virtual void select(Mesh* mesh, const mat4& matP, const mat4& matVM);

    /**
     * Cette méthode désactive le matériau
     */
    virtual void deselect();


protected:

    /** nom du matériau **/
    std::string m_Name;

    /** identifiants liés au shader */
    GLint m_ShaderId;
    GLint m_MatPLoc;
    GLint m_MatVMLoc;
    GLint m_MatNLoc;
    GLint m_TimeLoc;
    GLint m_VertexLoc;
    GLint m_ColorLoc;
    GLint m_NormalLoc;
    GLint m_TangentLoc;
    GLint m_TexCoordsLoc;

    /** matrice normale */
    mat3 m_MatN;

};

#endif
