#include <iostream>
#include <QWheelEvent>
#include <QColorDialog>
#include <QFileDialog>
#include <QTest>
#include <QTime>
#include <QTimer>
#include "gl_area.h"
#include "controller.h"

using namespace std;
using namespace vcg;
using namespace nx;

GLArea::GLArea(Nexus *_nexus, QWidget *parent): QGLWidget(parent), nexus(_nexus),
    phi(0), theta(0), zoom(1), mouse_down(0) {

    setMouseTracking(true);

    QTimer *timer = new QTimer(this);
    timer->start(50);

    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
}

void GLArea::checkForUpdate() {
    if(nexus->controller->hasCacheChanged())
        update();
}

void GLArea::initializeGL() {
    glewInit();

    glClearColor(1, 1, 1, 1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    glDisable(GL_BLEND);
    glEnable(GL_NORMALIZE);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);


    glEnable(GL_LIGHTING);

    glLoadIdentity();
}

void GLArea::resizeGL(int w, int h) {
    glViewport(0, 0, (GLint)w, (GLint)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float r = w/(float)h;
    gluPerspective(60, r, 1, 10);

    glMatrixMode(GL_MODELVIEW);
}

void GLArea::paintGL() {

    // near and far plane
    float nearPlane = 0.01;
    float farPlane = 10;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat fAspect = (GLfloat)width()/ (GLfloat)height();
    float fov = 30;
    gluPerspective(fov, fAspect, nearPlane, farPlane);

    //compute distance so that unitary sphere is inside view.
    float ratio = 1.0f;
    float objDist = ratio / tanf(vcg::math::ToRad(fov * .5f));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, objDist,0, 0, 0, 0, 1, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.7f, 0.7f, 0.7f);

    renderer.startFrame();

    glPushMatrix();

    //center camera on sphere
    vcg::Sphere3f &sphere = nexus->header.sphere;
    float radius = sphere.Radius();
    vcg::Point3f center = sphere.Center();


    //Trackball transformations
    glRotatef(phi, 1, 0, 0);
    glRotatef(theta, 0, 1, 0);
    glScalef(zoom, zoom, zoom);

    //Translate model inside unitary sphere
    glScalef(1/radius, 1/radius, 1/radius);
    glTranslatef(-center[0], -center[1], -center[2]);

    //render the model
    renderer.render(nexus);

    glPopMatrix();

    //if the view is changed we need to update priorities in the cache.
    renderer.endFrame();
}

void GLArea::wheelEvent(QWheelEvent *e) {
    zoom *= exp(0.001*e->delta());
    updateGL();
}

void GLArea::mousePressEvent(QMouseEvent *e) {
    start_x = e->x();
    start_y = e->y();
    mouse_down = true;
}



void GLArea::mouseMoveEvent(QMouseEvent *e) {
    if(!mouse_down) return;
    qint32 dx = e->x() - start_x;
    qint32 dy = e->y() - start_y;
    start_x = e->x();
    start_y = e->y();

    theta += 0.5f * dx;
    phi += 0.5f * dy;

    if(phi > 85) phi = 85;
    if(phi < -85) phi = -85;
    //view is changed remember to update priorities in the cache after next rendering
    //we cannot do it here because we need the traversal algorithm to computer the new priorities
    update();
}


void GLArea::mouseReleaseEvent(QMouseEvent */*e*/) {
    mouse_down = false;
    //e->ignore();
}

void GLArea::mouseDoubleClickEvent ( QMouseEvent */*e*/ ) {
    //double click ignored
}

void GLArea::keyReleaseEvent ( QKeyEvent * e ) {
    e->ignore();

    if(e->modifiers() & (Qt::ControlModifier | Qt::ControlModifier | Qt::AltModifier))
        return;

    switch(e->key()) {
    case Qt::Key_W: break;
    }
}

void GLArea::keyPressEvent ( QKeyEvent * e ) {
    e->ignore();
}

void GLArea::closeEvent ( QCloseEvent * /*event*/ ) {
    //QTest::qSleep(10);
    //NexusController::instance().finish();
}


