QT += opengl network
CONFIG += qtestlib

DEFINES += GL_COMPATIBILITY
#DEFINES += NEXUS_USE_QT
win32:DEFINES += NOMINMAX

SOURCES += main.cpp \
    gl_area.cpp \
    ../../common/traversal.cpp \
    ../../common/renderer.cpp \
    ../../common/prioritizer.cpp \
    ../../common/nexus.cpp \
    ../../common/controller.cpp \
    ../../common/cone.cpp \
    ../../common/frustum.cpp \
    ../../common/ram_cache.cpp

HEADERS += gl_area.h \
    ../../common/traversal.h \
    ../../common/signature.h \
    ../../common/renderer.h \
    ../../common/ram_cache.h \
    ../../common/prioritizer.h \
    ../../common/nexus.h \
    ../../common/metric.h \
    ../../common/controller.h \
    ../../common/cone.h \
    ../../common/gpu_cache.h \
    ../../../../../vcglib/wrap/gcache/token.h \
    ../../../../../vcglib/wrap/gcache/provider.h \
    ../../../../../vcglib/wrap/gcache/controller.h \
    ../../../../../vcglib/wrap/gcache/cache.h

INCLUDEPATH += ../../../../../vcglib \
    ../../common \
    ../../../../code/lib/glew/include

CONFIG(release, debug|release) {
  win32:LIBS += \
    ../../../../code/lib/glew/lib/glew32.lib \
    ../../nexus/lib/nexus.lib
}

CONFIG(debug, debug|release) {
  win32:LIBS += \
    ../../../../code/lib/glew/lib/glew32d.lib \
    ../../nexus/lib/nexusd.lib
}


#win32:LIBS += ../../../../code\lib\glew\lib\glew32s.lib ..\lib\nexus.lib

#unix:LIBS += -lGLEW -lGLU -L ../lib -lnexus
unix:LIBS += -lGLEW -lGLU -lcurl

DESTDIR = "../../bin"
