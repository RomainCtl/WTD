#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>

#include <MaterialRectangle.h>

/**
 * constructor
 * @param filename
 * @param filtering
 * @param repetition
*/
MaterialRectangle::MaterialRectangle(std::string filename) {
    // vertex shader
    std::string srcVertexShader =
        "#version 300 es\n"
        "\n"
        "// informations des sommets (VBO)\n"
        "in vec3 glVertex;\n"
        "in vec2 glTexCoords;\n"
        "out vec2 frgTexCoords;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(glVertex, 1.0);\n"
        "    frgTexCoords = glTexCoords;\n"
        "}";

    // fragment shader
    std::string srcFragmentShader =
        "#version 300 es\n"
        "precision mediump float;\n"
        "\n"
        "// couleur du matériau donnée par une texture\n"
        "uniform sampler2D txColor;\n"
        "\n"
        "// informations venant du vertex shader\n"
        "in vec2 frgTexCoords;\n"
        "\n"
        "// sortie du shader\n"
        "out vec4 glFragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    glFragColor = texture(txColor, frgTexCoords);\n"
        "}\n";

    setShaders(srcVertexShader, srcFragmentShader);
    /** charger la texture */
    m_TextureLoc = glGetUniformLocation(m_ShaderId, "txColor");
    m_Texture = new Texture2D(filename, GL_LINEAR);
}

void MaterialRectangle::select(Mesh* mesh, const mat4& matP, const mat4& matVM) {
    // méthode de la superclasse (active le shader)
    Material::select(mesh, matP, matVM);

    // activer la texture sur l'unité 0
    m_Texture->setTextureUnit(GL_TEXTURE0, m_TextureLoc);
}


void MaterialRectangle::deselect() {
    // libérer le sampler
    m_Texture->setTextureUnit(GL_TEXTURE0);

    // méthode de la superclasse (désactive le shader)
    Material::deselect();
}


MaterialRectangle::~MaterialRectangle() {
    delete m_Texture;
}
