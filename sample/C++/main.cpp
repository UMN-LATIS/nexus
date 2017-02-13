
#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QTest>
#include "gl_area.h"

#include "nexus.h"
#include "controller.h"

#include <iostream>

using namespace std;

/* Important things to do to make it work:

glewInit()


*/

int main(int argc, char *argv[]) {
    if(argc != 2) {
        cerr << "Usage: \n";
        cerr << argv[0] << " file.nxs\n";
        return -1;
    }
    QApplication app(argc, argv);

    nx::Controller controller;
    controller.setRam(500*(1<<20));
    controller.setGpu(250*(1<<20));
    controller.start();

    nx::Nexus nexus(&controller);
    if(!nexus.open(argv[1])) {
        cerr << "Could not find or load nexus: " << argv[1] << endl;
        return -1;
    }

    GLArea *area = new GLArea(&nexus);
    area->renderer.setMode(nx::Renderer::PATCHES, true);
    area->show();

    try {
        app.exec();
    } catch(QString error) {
        cout << "Fatal error: " << qPrintable(error) << endl;
    }

    return 0;
}



