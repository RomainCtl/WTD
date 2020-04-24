#include <iostream>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>

#include <utils.h>

#include <Compass.h>
#include <Light.h>
#include <Texture2D.h>

using namespace mesh;

Compass::Compass(): Mesh("Compass") {
    // material
    m_Material = new MaterialRectangle("data/compass_dial.png");
    setMaterials(m_Material);

    // vertices
    Vertex *p1 = new Vertex(this, -1.0, 0.6, 0.0);
    p1->setTexCoords(0.0, 0.0);
    Vertex *p2 = new Vertex(this, -0.6, 0.6, 0.0);
    p2->setTexCoords(1.0, 0.0);
    Vertex *p3 = new Vertex(this, -0.6, 1.0, 0.0);
    p3->setTexCoords(1.0, 1.0);
    Vertex *p4 = new Vertex(this, -1.0, 1.0, 0.0);
    p4->setTexCoords(0.0, 1.0);

    addQuad(p1, p2, p3, p4);
}

Compass::~Compass() {
    delete m_Material;
}
