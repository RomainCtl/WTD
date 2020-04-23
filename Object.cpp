// Définition de la classe Duck

#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <utils.h>

#include <Object.h>
#include <Light.h>
#include <Texture2D.h>

using namespace mesh;

std::map<ObjectType, ObjectConfigType> objects_config = {
    {
        DUCK,
        {"Duck-quacking-sound.wav", "10602_Rubber_Duck_v1_L3.obj", "10602_Rubber_Duck_v1_diffuse.jpg", 20.0f, 0.0f, 65.0f, 20.0f, 0.1, -90, 0, 0}
    },
    {
        CAT,
        {"cat_y.wav", "12221_Cat_v1_l3.obj", "Cat_diffuse.jpg", 30.0f, 0.0f, 80.0f, 20.0f, 0.04, -90, 0, 0}
    },
    {
        HORSE,
        {"horse2.wav", "Horse.obj", "horse_diffuse.jpg", 20.0f, 0.0f, 65.0f, 20.0f, 0.005, 0, 90, 0}
    },
    {
        LION,
        {"lion_growl.wav", "12273_Lion_v1_l3.obj", "12273_Lion_Diffuse.jpg", 20.0f, 0.0f, 65.0f, 20.0f, 0.01, -90, 0, 0}
    },
    {
        PENGUIN,
        {"penguin_sounds_6639_loops.wav", "PenguinBaseMesh.obj", "Penguin_Diffuse_Color.png", 20.0f, 0.0f, 65.0f, 20.0f, 1, 0, 0, 0}
    },
    {
        MONKEY,
        {"monkey2.wav", "12958_Spider_Monkey_v1_l2.obj", "12958_Spider_Monkey_diff.jpg", 20.0f, 0.0f, 65.0f, 20.0f, 0.015, -90, 0, 0}
    },
    {
        _THIRD_PERSON,
        {"white_noise.wav", "LegoMan.obj", "lego_diffuse.png", 0, 0, 0, 0, 0.2, 0, 180, 0}
    }
};



/**
 * constructeur, crée le maillage
 *
 * @param type object type
 */
Object::Object(ObjectType type): Mesh("Object") {
    std::map<ObjectType, ObjectConfigType>::iterator it = objects_config.find(type);
    if (it == objects_config.end()) {
        std::cerr << "Unable to find this object type..." << std::endl;
        alGetError();
        throw std::runtime_error("Unknown object type");
    }
    ObjectConfigType conf = it->second;

    // matériaux
    m_Material = new MaterialTexture("data/"+conf.diffuse_img);
    setMaterials(m_Material);
    m_Draw = false;
    m_Sound = false;

    // charger le fichier obj
    loadObj("data/"+conf.obj_file);

    // mise à l'échelle et rotation de l'objet (si son .obj est mal orienté et trop grand/petit)
    mat4 correction = mat4::create();
    mat4::identity(correction);
    mat4::scale(correction, correction, vec3::fromValues(conf.ratio, conf.ratio, conf.ratio));
    mat4::rotateX(correction, correction, Utils::radians(conf.rotation_x));
    mat4::rotateY(correction, correction, Utils::radians(conf.rotation_y));
    mat4::rotateZ(correction, correction, Utils::radians(conf.rotation_z));
    transform(correction);

    // recalcul des normales
    computeNormals();

    // ouverture du flux audio à placer dans le buffer
    std::string soundpathname = "data/"+conf.sound_file;
    buffer = alutCreateBufferFromFile(soundpathname.c_str());
    if (buffer == AL_NONE) {
        std::cerr << "unable to open file " << soundpathname << std::endl;
        alGetError();
        throw std::runtime_error("file not found or not readable");
    }

    // lien buffer -> source
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);

    // propriétés de la source à l'origine
    alSource3f(source, AL_POSITION, 0, 0, 0); // on positionne la source à (0,0,0) par défaut
    alSource3f(source, AL_VELOCITY, 0, 0, 0);
    alSourcei(source, AL_LOOPING, AL_TRUE);
    // dans un cone d'angle [-inner/2,inner/2] il n'y a pas d'attenuation
    alSourcef(source, AL_CONE_INNER_ANGLE, conf.inner_angle);
    // dans un cone d'angle [-outer/2,outer/2] il y a une attenuation linéaire entre 0 et le gain
    alSourcef(source, AL_CONE_OUTER_GAIN, conf.outer_gain);
    alSourcef(source, AL_CONE_OUTER_ANGLE, conf.outer_angle);
    // à l'extérieur de [-outer/2,outer/2] il y a une attenuation totale

    // atténuation linéaire du son selon la distance
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    alSourcef(source, AL_MAX_DISTANCE, conf.max_distance);
}


/**
 * définit la lampe
 * @param light : instance de Light spécifiant les caractéristiques de la lampe
 */
void Object::setLight(Light* light)
{
    m_Material->setLight(light);
}

void Object::setDraw(bool b)
{
	m_Draw = b;
}

void Object::setSound(bool b)
{
	if (m_Sound && !b) alSourceStop(source);
	if (!m_Sound && b) alSourcePlay(source);
	m_Sound = b;
}

/**
     * dessiner le cube
     * @param matP : matrice de projection
     * @param matMV : matrice view*model (caméra * position objet)
 */
void Object::onRender(const mat4& matP, const mat4& matVM)
{
   	/** dessin OpenGL **/
   	mat4 local_vm;
  	mat4::translate(local_vm, matVM, m_Position);
   	mat4::rotateX(local_vm, local_vm, m_Orientation[0]);
   	mat4::rotateY(local_vm, local_vm, m_Orientation[1]);//-Utils::Time * 0.8);
   	mat4::rotateZ(local_vm, local_vm, m_Orientation[2]);

    if (m_Draw)
    {
	    onDraw(matP, local_vm);
	}

    /** sonorisation OpenAL **/

    if (m_Sound)
    {
	    // obtenir la position relative à la caméra
	    vec4 pos = vec4::fromValues(0,0,0,1);   // point en (0,0,0)
	    vec4::transformMat4(pos, pos, local_vm);
	    //std::cout << "Position = " << vec4::str(pos);
	    alSource3f(source, AL_POSITION, pos[0], pos[1], pos[2]);

	    // obtenir la direction relative à la caméra
	    vec4 dir = vec4::fromValues(0,0,1,0);   // vecteur +z
	    vec4::transformMat4(dir, dir, local_vm);
	    //std::cout << "    Direction = " << vec4::str(dir) << std::endl;
	    alSource3f(source, AL_DIRECTION, dir[0], dir[1], dir[2]);
	}
}



vec3& Object::getPosition()
{
    return m_Position;
}

void Object::setPosition(vec3 pos)
{
    vec3::copy(m_Position, pos);
}



vec3& Object::getOrientation()
{
    return m_Orientation;
}

void Object::setOrientation(vec3 ori)
{
    vec3::copy(m_Orientation, ori);
}


/** destructeur */
Object::~Object()
{
    // libération du matériau
    delete m_Material;

    // libération des ressources openal
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
}
