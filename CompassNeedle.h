#ifndef COMPASS_NEEDLE_H
#define COMPASS_NEEDLE_H

#include <Mesh.h>
#include <Light.h>
#include <MaterialRectangle.h>
#include <gl-matrix.h>

class CompassNeedle: public Mesh {
    private:
        MaterialRectangle* m_Material;
        Vertex *points[4];

        vec2 rotation_center;
        double last_azimuth;

    public:
        /**
         * constructor
        */
        CompassNeedle();

        /**
         * Needle rotation according to azimuth angle
         * @param azimuth
        */
        void rotate(double azimuth);

        /**
         * Destructor
        */
        ~CompassNeedle();
};

#endif