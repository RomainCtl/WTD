//// ce script fournit des fonctions utilitaires pour les programmes
//// du livre Synthèse d'images à l'aide d'OpenGL

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <FrameBufferObject.h>

FrameBufferObject::FrameBufferObject()
{
    init(0,0);
}


void FrameBufferObject::init(int width, int height)
{
    // variables d'instance
    m_FBO = 0;
    m_DepthBufferId = 0;
    m_Width = width;
    m_Height = height;
    m_PrecFBO = 0;
}


/**
 * constructeur de la classe FrameBufferObject
 * @param width largeur du FBO (nombre de pixels)
 * @param height hauteur du FBO
 * @param color : fournir GL_NONE si aucun, GL_TEXTURE_2D si on veut un buffer de type texture, GL_RENDERBUFFER si c'est un renderbuffer.
 * @param depth : fournir GL_NONE si aucun, GL_TEXTURE_2D si on veut un buffer de type texture, GL_RENDERBUFFER si c'est un renderbuffer, NB: il faut impérativement un depth buffer dans un FBO destiné à être rendu
 * @param colorsnb : nombre de color buffer supplémentaires pour faire du dessin différé (MRT), et affecter plusieurs valeurs de glFragData[i]
 * @param filtering : filtrage des textures, mettre GL_NEAREST ou GL_LINEAR (valeur par défaut)
 */
FrameBufferObject::FrameBufferObject(int width, int height, GLenum color, GLenum depth, int colorsnb, GLenum filtering)
{
    // test sur les paramètres pour éviter des bizarreries
    if (colorsnb > 0 && color == GL_NONE) {
        throw std::invalid_argument("FrameBufferObject: colorsnb>0 but no main color buffer");
    }

    // variables d'instance
    init(width, height);
    std::vector<GLenum> drawBuffers;

    const GLfloat borderColor[] = {1.0,1.0,1.0,0.0};

    // créer le FBO
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // identifiant du buffer courant
    GLuint bufferId = 0;

    // créer le buffer de couleur principal, rempli par glFragColor ou glFragData[0]
    switch (color) {

    case GL_TEXTURE_2D:
        // créer une texture 2D pour recevoir les dessins faits via le FBO
        glGenTextures(1, &bufferId);
        glBindTexture(GL_TEXTURE_2D, bufferId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,  width, height, 0, GL_RGBA, GL_FLOAT, 0);

        // configurer la texture
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);

        // attacher la texture au FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferId, 0);
        m_ColorBufferIds.push_back(bufferId);
        m_DrawBufferNames.push_back(GL_COLOR_ATTACHMENT0);
        break;

    case GL_RENDERBUFFER:
        // lui ajouter un color buffer de type render buffer
        glGenRenderbuffers(1, &bufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, bufferId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, width, height);

        // attacher le render buffer au FBO
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, bufferId);
        m_ColorBufferIds.push_back(bufferId);
        m_DrawBufferNames.push_back(GL_COLOR_ATTACHMENT0);
        break;

    case GL_NONE:
        break;

    default:
        std::cerr << "FrameBufferObject: color is not among GL_NONE, GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, GL_RENDERBUFFER" << std::endl;
    }

    // créer le buffer de profondeur, rempli automatiquement par gl_FragDepth
    switch (depth) {

    case GL_TEXTURE_2D:
        // lui ajouter un depth buffer de type texture
        glGenTextures(1, &m_DepthBufferId);
        glBindTexture(GL_TEXTURE_2D, m_DepthBufferId);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

        // configurer la texture
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        // couleur du bord
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // attacher le depth buffer au FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthBufferId, 0);
        break;

    case GL_RENDERBUFFER:
        // lui ajouter un depth buffer de type render buffer
        glGenRenderbuffers(1, &m_DepthBufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBufferId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

        // attacher le depth buffer au FBO
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBufferId);
        break;

    case GL_NONE:
        // voir la remarque dans https://www.opengl.org/wiki/Framebuffer_Object
        // Even if you don't plan on reading from this depth_attachment, an off screen buffer that will be rendered to should have a depth attachment.
        break;

    default:
        std::cerr << "FrameBufferObject: depth is not among GL_NONE, GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, GL_RENDERBUFFER" << std::endl;
    }

    // créer des buffers supplémentaires si c'est demandé
    for (int i=0; i<colorsnb; i++) {

        // créer une texture 2D pour recevoir les dessins (voir glFragData dans les shaders)
        glGenTextures(1, &bufferId);
        glBindTexture(GL_TEXTURE_2D, bufferId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);

        // configurer la texture
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);

        // attacher la texture au FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1+i, GL_TEXTURE_2D, bufferId, 0);
        m_ColorBufferIds.push_back(bufferId);
        m_DrawBufferNames.push_back(GL_COLOR_ATTACHMENT1+i);
    }

    // indiquer quels sont les buffers utilisés pour le dessin (glFragData valides)
    glDrawBuffers(m_DrawBufferNames.size(), &m_DrawBufferNames[0]);

    // vérifier l'état des lieux
    checkStatus();

    // désactiver le FBO pour l'instant
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


/**
 * vérifie l'état du FBO
 */
void FrameBufferObject::checkStatus()
{
    GLuint code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    const char* status;
    switch (code) {
        case GL_FRAMEBUFFER_COMPLETE:
            status="FRAMEBUFFER_COMPLETE";
            break;
        case GL_FRAMEBUFFER_UNDEFINED:
            status="FRAMEBUFFER_UNDEFINED";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            status="FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            status="FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            status="FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            status="FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            status="FRAMEBUFFER_UNSUPPORTED";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            status="FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            status="FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
            break;
        default:
            status="FRAMEBUFFER_??";
            break;
    }
    if (code != GL_FRAMEBUFFER_COMPLETE) {
        // signaler une erreur
        std::cout << "FrameBufferObject : creating FBO " << m_Width<<"x"<<m_Height << " : " << status << std::endl;
        throw std::invalid_argument("creating FBO");
    }
}


/**
 * supprime ce FBO
 */
FrameBufferObject::~FrameBufferObject()
{
    // déterminer quels sont les types des attachements
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    GLint color = GL_NONE;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &color);
    GLint depth = GL_NONE;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &depth);

    // supprimer le FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &m_FBO);

    // libérer le color buffer s'il y en a un
    switch (color) {
    case GL_TEXTURE:
        glDeleteTextures(1, &m_ColorBufferIds[0]);
        break;
    case GL_RENDERBUFFER:
        glDeleteRenderbuffers(1, &m_ColorBufferIds[0]);
        break;
    }

    // libérer le depth buffer s'il y en a un
    switch (depth) {
    case GL_TEXTURE:
        glDeleteTextures(1, &m_DepthBufferId);
        break;
    case GL_RENDERBUFFER:
        glDeleteRenderbuffers(1, &m_DepthBufferId);
        break;
    }

    // libérer les autres buffers s'il y en a
    for (int i=1; i<m_ColorBufferIds.size(); i++) {
        glDeleteTextures(1, &m_ColorBufferIds[i]);
    }
}


/**
 * redirige tous les tracés suivants vers le FBO
 */
void FrameBufferObject::enable()
{
    // sauver le viewport
    glGetIntegerv(GL_VIEWPORT, m_Viewport);

    // redéfinir le viewport pour correspondre au FBO
    glViewport(0, 0, m_Width, m_Height);

    // enregistrer le précédent FBO actif
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_PrecFBO);

    // activer le FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
}


/**
 * redirige tous les tracés suivants vers l'écran à nouveau
 */
void FrameBufferObject::disable()
{
    // désactiver le FBO, remettre le précédent en place
    glBindFramebuffer(GL_FRAMEBUFFER, m_PrecFBO);

    // remettre le viewport à ce qu'il était avant
    glViewport(m_Viewport[0], m_Viewport[1], m_Viewport[2], m_Viewport[3]);
}


/**
 * retourne l'identifiant du Color Buffer
 * @param num = 0 pour l'attachement COLOR_ATTACHMENT0, 1 pour COLOR_ATTACHMENT1...
 * @return color buffer de ce FBO
 */
GLuint FrameBufferObject::getColorBuffer(int num)
{
    return m_ColorBufferIds[num];
}


/**
 * retourne l'identifiant du Depth Buffer
 * @return depth buffer de ce FBO
 */
GLuint FrameBufferObject::getDepthBuffer()
{
    return m_DepthBufferId;
}


/**
 * retourne la largeur de ce FBO
 * @return largeur
 */
GLuint FrameBufferObject::getWidth()
{
    return m_Width;
}


/**
 * retourne la hauteur de ce FBO
 * @return hauteur
 */
GLuint FrameBufferObject::getHeight()
{
    return m_Height;
}


/**
 * cette fonction associe l'une des textures de ce FBO à une unité pour un shader
 * NB : le shader concerné doit être actif
 * @param unit : unité de texture concernée, par exemple GL_TEXTURE0
 * @param locSampler : emplacement de la variable uniform sampler* de cette texture dans le shader ou -1 pour désactiver la texture
 * @param bufferId : l'un des buffers retourné par getColorBuffer ou getDepthBuffer ou 0 pour désactiver
 */
void FrameBufferObject::setTextureUnit(GLint unit, GLint locSampler, GLuint bufferId)
{
    /*****DEBUG*****/
    if (unit < GL_TEXTURE0 || unit > GL_TEXTURE7) {
        throw std::invalid_argument("FrameBufferObject::setTextureUnit: first parameter, unit is not GL_TEXTURE0 ... GL_TEXTURE7");
    }
    /*****DEBUG*****/
    glActiveTexture(unit);
    if (locSampler < 0 || bufferId <= 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, bufferId);
        glUniform1i(locSampler, unit-GL_TEXTURE0);
    }
}
