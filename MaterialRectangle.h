#ifndef MATERIALRECTANGLE_H
#define MATERIALRECTANGLE_H

#include <Material.h>
#include <Texture2D.h>

class MaterialRectangle: public Material {
    private:
        // texture
        GLint m_TextureLoc;
        Texture2D* m_Texture;

    public:
        /**
         * constructor
         * @param filename
        */
        MaterialRectangle(std::string filename);

        virtual void select(Mesh* mesh, const mat4& matP, const mat4& matVM);

        virtual void deselect();

        virtual ~MaterialRectangle();
};

#endif