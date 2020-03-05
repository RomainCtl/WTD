// Définition de la classe MaterialTexture

#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>

#include <utils.h>

#include <MaterialTexture.h>


/**
 * constructeur
 * @param filename : nom du fichier contenant l'image à charger
 * @param filtering : mettre GL_LINEAR ou gl.NEAREST ou GL_LINEAR_MIPMAP_LINEAR (mipmaps)
 * @param repetition : mettre GL_CLAMP_TO_EDGE ou GL_REPEAT
 */
MaterialTexture::MaterialTexture(std::string filename, GLenum filtering, GLenum repetition) : Material("MaterialTexture")
{
    /** définir le shader */

    // vertex shader
    std::string srcVertexShader =
        "#version 300 es\n"
        "// matrices de transformation\n"
        "uniform mat4 matP;\n"
        "uniform mat4 matVM;\n"
        "uniform mat3 matN;\n"
        "\n"
        "// informations des sommets (VBO)\n"
        "in vec3 glVertex;\n"
        "in vec3 glNormal;\n"
        "in vec2 glTexCoords;\n"
        "\n"
        "// calculs allant vers le fragment shader\n"
        "out vec3 frgN;              // normale du fragment en coordonnées caméra\n"
        "out vec4 frgPosition;       // position du fragment en coordonnées caméra\n"
        "out vec2 frgTexCoords;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    frgPosition = matVM * vec4(glVertex, 1.0);\n"
        "    gl_Position = matP * frgPosition;\n"
        "    frgN = matN * glNormal;\n"
        "    frgTexCoords = glTexCoords;\n"
        "}";

    // fragment shader
    std::string srcFragmentShader =
        "#version 300 es\n"
        "precision mediump float;\n"
        "// couleur du matériau donnée par la texture\n"
        "uniform sampler2D txColor;\n"
        "\n"
        "// paramètres du shader : caractéristiques de la lampe\n"
        "uniform vec3 LightColor;        // couleur de la lampe\n"
        "uniform vec4 LightPosition;     // position ou direction d'une lampe positionnelle ou directionnelle\n"
        "uniform vec4 LightDirection;    // direction du cône pour une lampe spot\n"
        "uniform float cosmaxangle;\n"
        "uniform float cosminangle;\n"
        "\n"
        "// informations venant du vertex shader\n"
        "in vec3 frgN;              // normale du fragment en coordonnées caméra\n"
        "in vec4 frgPosition;       // position du fragment en coordonnées caméra\n"
        "in vec2 frgTexCoords;\n"
        "\n"
        "// sortie du shader\n"
        "out vec4 glFragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    // couleur diffuse\n"
        "    vec3 Kd = texture(txColor, frgTexCoords).rgb;\n"
        "\n"
        "    // éclairement ambiant : 20%\n"
        "    vec3 amb = 0.2 * Kd;\n"
        "\n"
        "    // vecteur normal normalisé\n"
        "    vec3 N = normalize(frgN);\n"
        "\n"
        "    // direction de la lumière dans le repère caméra\n"
        "    vec3 L = LightPosition.xyz - frgPosition.xyz * LightPosition.w;\n"
        "    float dist = length(L);\n"
        "    L /= dist;\n"
        "\n"
        "    // présence dans le cône du spot\n"
        "    float visib = smoothstep(cosmaxangle, cosminangle, dot(-L, LightDirection.xyz));\n"
        "\n"
        "    // diminution de l'intensité à cause de la distance\n"
        "    visib /= dist*dist;\n"
        "\n"
        "    // éclairement diffus de Lambert\n"
        "    float dotNL = clamp(dot(N, L), 0.0, 1.0);\n"
        "    vec3 dif = visib * LightColor * Kd * dotNL;\n"
        "\n"
        "    // couleur finale = diffus + ambiant\n"
        "    glFragColor = vec4(dif + amb, 1.0);\n"
        "}";

    setShaders(srcVertexShader, srcFragmentShader);

    // emplacement des variables uniform spécifiques
    m_LightColorLoc     = glGetUniformLocation(m_ShaderId, "LightColor");
    m_LightPositionLoc  = glGetUniformLocation(m_ShaderId, "LightPosition");
    m_LightDirectionLoc = glGetUniformLocation(m_ShaderId, "LightDirection");
    m_CosMaxAngleLoc    = glGetUniformLocation(m_ShaderId, "cosmaxangle");
    m_CosMinAngleLoc    = glGetUniformLocation(m_ShaderId, "cosminangle");

    /** charger la texture */
    m_TextureLoc = glGetUniformLocation(m_ShaderId, "txColor");
    m_Texture = new Texture2D(filename, filtering, repetition);
}


/**
 * définit la lampe
 * @param light : instance de Light spécifiant les caractéristiques de la lampe
 */
void MaterialTexture::setLight(Light* light)
{
    // activer le shader
    glUseProgram(m_ShaderId);

    // fournir les infos de la lampe au shader
    vec3::glUniform(m_LightColorLoc,     light->getColor());
    vec4::glUniform(m_LightPositionLoc,  light->getPosition());
    vec4::glUniform(m_LightDirectionLoc, light->getDirection());
    glUniform1f(m_CosMinAngleLoc,        light->getCosMinAngle());
    glUniform1f(m_CosMaxAngleLoc,        light->getCosMaxAngle());
}


void MaterialTexture::select(Mesh* mesh, const mat4& matP, const mat4& matVM)
{
    // méthode de la superclasse (active le shader)
    Material::select(mesh, matP, matVM);

    // activer la texture sur l'unité 0
    m_Texture->setTextureUnit(GL_TEXTURE0, m_TextureLoc);
}


void MaterialTexture::deselect()
{
    // libérer le sampler
    m_Texture->setTextureUnit(GL_TEXTURE0);

    // méthode de la superclasse (désactive le shader)
    Material::deselect();
}


MaterialTexture::~MaterialTexture()
{
    delete m_Texture;
}

