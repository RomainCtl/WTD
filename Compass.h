#ifndef COMPASS_H
#define COMPASS_H

#include <Mesh.h>
#include <Light.h>
#include <MaterialRectangle.h>
#include <gl-matrix.h>

class Compass: public Mesh {
    private:
        MaterialRectangle* m_Material;

    public:
        /**
         * constructor
        */
        Compass();

        /**
         * Destructor
        */
        ~Compass();
};

#endif