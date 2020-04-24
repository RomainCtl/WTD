#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>

#include <utils.h>

#include <CompassNeedle.h>
#include <Light.h>
#include <Texture2D.h>

using namespace mesh;

CompassNeedle::CompassNeedle(): Mesh("CompassNeedle") {
    // material
    m_Material = new MaterialRectangle("data/compass_needle_2.png");
    setMaterials(m_Material);

    // vertices
    points[0] = new Vertex(this, -1.0, 0.6, 0.0);
    points[0]->setTexCoords(0.0, 0.0);
    points[1] = new Vertex(this, -0.6, 0.6, 0.0);
    points[1]->setTexCoords(1.0, 0.0);
    points[2] = new Vertex(this, -0.6, 1.0, 0.0);
    points[2]->setTexCoords(1.0, 1.0);
    points[3] = new Vertex(this, -1.0, 1.0, 0.0);
    points[3]->setTexCoords(0.0, 1.0);

    rotation_center = vec2::fromValues(
        (points[0]->getCoords()[0] + points[1]->getCoords()[0]) / 2.0,
        (points[0]->getCoords()[1] + points[3]->getCoords()[1]) / 2.0
    );

    addQuad(points[0],points[1], points[2], points[3]);

    last_azimuth = 0.0;
}

void CompassNeedle::rotate(double azimuth) {
    azimuth = -Utils::radians(azimuth);

    if (azimuth != last_azimuth) {
        double tmp = azimuth;
        azimuth -= last_azimuth; // no cumulation
        last_azimuth = tmp;

        // rotation points
        for (Vertex* p : points) {
            double x = p->getCoords()[0];
            double y = p->getCoords()[1];
            p->setCoords( vec3::fromValues(
                ( (x - rotation_center[0]) * cos(azimuth) ) - ( (y - rotation_center[1]) * sin(azimuth) ) + rotation_center[0],
                ( (x - rotation_center[0]) * sin(azimuth) ) + ( (y - rotation_center[1]) * cos(azimuth) ) + rotation_center[1],
                p->getCoords()[2]
            ) );
        }

        addQuad(points[0],points[1], points[2], points[3]);
    }
}

CompassNeedle::~CompassNeedle() {
    delete m_Material;
}
