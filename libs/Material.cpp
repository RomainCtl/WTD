// Définition de la classe abstraite Material, superclasse de tous les matériaux

#include <GL/glew.h>
#include <GL/gl.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include <utils.h>
#include <Material.h>


/**
 * constructeur
 * @param srcVertexShader : nom du matériau
 * @param srcFragmentShader : nom du matériau
 * @param name : nom du matériau
 */
Material::Material(std::string srcVertexShader, std::string srcFragmentShader, std::string name)
{
    // nom du matériau pour la mise au point
    m_Name = name;

    // compiler le shader
    setShaders(srcVertexShader, srcFragmentShader);

    // matrice normale
    m_MatN = mat3::create();
}


/**
 * constructeur quand les shaders ne peuvent pas être écrits lors de la construction
 * @param name : nom du matériau
 */
Material::Material(std::string name)
{
    // nom du matériau pour la mise au point
    m_Name = name;

    // matrice normale
    m_MatN = mat3::create();
}


void Material::setShaders(std::string srcVertexShader, std::string srcFragmentShader)
{
    // test des paramètres
    if (srcVertexShader=="" || srcFragmentShader=="") {
        throw "Missing shader source for material subclass "+m_Name;
    }

    // compiler le shader
    m_ShaderId = Utils::makeShaderProgram(srcVertexShader, srcFragmentShader, m_Name);

    // déterminer où sont les variables uniform (paramètres du matériau)
    m_MatPLoc   = glGetUniformLocation(m_ShaderId, "matP");
    m_MatVMLoc  = glGetUniformLocation(m_ShaderId, "matVM");
    m_MatNLoc   = glGetUniformLocation(m_ShaderId, "matN");
    m_TimeLoc   = glGetUniformLocation(m_ShaderId, "time");

    // déterminer où sont les variables attribute (associées aux VBO)
    m_VertexLoc    = glGetAttribLocation(m_ShaderId, "glVertex");
    m_ColorLoc     = glGetAttribLocation(m_ShaderId, "glColor");
    m_NormalLoc    = glGetAttribLocation(m_ShaderId, "glNormal");
    m_TangentLoc   = glGetAttribLocation(m_ShaderId, "glTangent");
    m_TexCoordsLoc = glGetAttribLocation(m_ShaderId, "glTexCoords");

    // tests de validité minimaux
    if (m_VertexLoc < 0) {
        throw std::runtime_error("Vertex shader of "+m_Name+" uses another name for coordinates instead of attribute vec3 glVertex;");
    }
    if (m_MatPLoc  < 0) std::cerr << "no uniform mat4 matP; in "<<m_Name<<" vertex shader ?"<<std::endl;
    if (m_MatVMLoc < 0) std::cerr << "no uniform mat4 matVM; in "<<m_Name<<" vertex shader ?"<<std::endl;
}


/**
 * active le matériau : son shader et lie les variables uniform communes
 * @param mesh : maillage pour lequel on active ce matériau
 * @param matP : matrice de projection perpective
 * @param matVM : matrice de transformation de l'objet par rapport à la caméra
 */
void Material::select(Mesh* mesh, const mat4& matP, const mat4& matVM)
{
    // activer le shader
    glUseProgram(m_ShaderId);

    // fournir les matrices P et VM au shader
    mat4::glUniformMatrix(m_MatPLoc, matP);
    mat4::glUniformMatrix(m_MatVMLoc, matVM);

    // fournir le temps (il n'est pas forcément utilisé par le shader)
    glUniform1f(m_TimeLoc, Utils::Time);

    // calcul de la matrice normale si elle est utilisée
    if (m_MatNLoc >= 0) {
        mat3::fromMat4(m_MatN, matVM);
        mat3::transpose(m_MatN, m_MatN);
        mat3::invert(m_MatN, m_MatN);
        mat3::glUniformMatrix(m_MatNLoc, m_MatN);
    }

    // activer et lier le buffer contenant les coordonnées, attention ce sont des vec3 obligatoirement
    GLint vertexBufferId = mesh->getVertexBufferId();
    if (vertexBufferId <= 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    glEnableVertexAttribArray(m_VertexLoc);
    glVertexAttribPointer(m_VertexLoc, Utils::VEC3, GL_FLOAT, GL_FALSE, 0, 0);

    // activer et lier le buffer contenant les couleurs s'il est utilisé dans le shader
    if (m_ColorLoc >= 0) {
        GLint colorBufferId = mesh->getColorBufferId();
        if (colorBufferId >= 0) {
            glBindBuffer(GL_ARRAY_BUFFER, colorBufferId);
            glEnableVertexAttribArray(m_ColorLoc);
            glVertexAttribPointer(m_ColorLoc, Utils::VEC3, GL_FLOAT, GL_FALSE, 0, 0);
        }
    }

    // activer et lier le buffer contenant les normales s'il est utilisé dans le shader
    if (m_NormalLoc >= 0) {
        GLint normalBufferId = mesh->getNormalBufferId();
        if (normalBufferId >= 0) {
            glBindBuffer(GL_ARRAY_BUFFER, normalBufferId);
            glEnableVertexAttribArray(m_NormalLoc);
            glVertexAttribPointer(m_NormalLoc, Utils::VEC3, GL_FLOAT, GL_FALSE, 0, 0);
        }
    }

    // activer et lier le buffer contenant les tangentes s'il est utilisé dans le shader
    if (m_TangentLoc >= 0) {
        GLint tangentBufferId = mesh->getTangentBufferId();
        if (tangentBufferId >= 0) {
            glBindBuffer(GL_ARRAY_BUFFER, tangentBufferId);
            glEnableVertexAttribArray(m_TangentLoc);
            glVertexAttribPointer(m_TangentLoc, Utils::VEC3, GL_FLOAT, GL_FALSE, 0, 0);
        }
    }

    // activer et lier le buffer contenant les coordonnées de textures s'il est utilisé dans le shader
    if (m_TexCoordsLoc >= 0) {
        GLint texcoordsBufferId = mesh->getTexCoordsBufferId();
        if (texcoordsBufferId >= 0) {
            glBindBuffer(GL_ARRAY_BUFFER, texcoordsBufferId);
            glEnableVertexAttribArray(m_TexCoordsLoc);
            glVertexAttribPointer(m_TexCoordsLoc, Utils::VEC2, GL_FLOAT, GL_FALSE, 0, 0);
        }
    }
}


/**
 * désactive le matériau
 */
void Material::deselect()
{
    // désactiver les buffers s'ils sont utilisés
    glDisableVertexAttribArray(m_VertexLoc);
    if (m_ColorLoc >= 0) {
        glDisableVertexAttribArray(m_ColorLoc);
    }
    if (m_NormalLoc >= 0) {
        glDisableVertexAttribArray(m_NormalLoc);
    }
    if (m_TangentLoc >= 0) {
        glDisableVertexAttribArray(m_TangentLoc);
    }
    if (m_TexCoordsLoc >= 0) {
        glDisableVertexAttribArray(m_TexCoordsLoc);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // désactiver le shader
    glUseProgram(0);
}


/**
 * supprime toutes les ressources allouées dans le constructeur
 */
Material::~Material()
{
    // supprimer le shader
    Utils::deleteShaderProgram(m_ShaderId);
}

