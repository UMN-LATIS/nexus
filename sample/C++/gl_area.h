#ifndef GL_AREA_H
#define GL_AREA_H

#include <GL/glew.h>
#include <iostream>
#include <QGLWidget>

#include "nexus.h"
#include "renderer.h"

class GLArea : public QGLWidget {
    Q_OBJECT

public:
    nx::Nexus *nexus;
    nx::Renderer renderer;

    GLArea(nx::Nexus *_nexus, QWidget *parent = 0);

public slots:
    void checkForUpdate();

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void closeEvent ( QCloseEvent * event );
    void wheelEvent(QWheelEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent* e);
    void keyReleaseEvent(QKeyEvent* e);
    void keyPressEvent(QKeyEvent* e);

    float phi;   //angle around Z axis
    float theta; //angle around Y axis
    float zoom;
    bool mouse_down;
    qint32 start_x, start_y;

};

#endif

