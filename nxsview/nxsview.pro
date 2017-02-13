#-------------------------------------------------
#
# Project created by QtCreator 2012-05-25T23:10:26
#
#-------------------------------------------------

QT       += core gui opengl widgets

TARGET = nxsview
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

DEFINES += GL_COMPATIBILITY
unix:DEFINES += USE_CURL
win32:DEFINES += NOMINMAX

SOURCES += main.cpp\
    ../common/controller.cpp \
    ../common/nexus.cpp \
    ../common/cone.cpp \
    gl_nxsview.cpp \
    scene.cpp \
    ../common/traversal.cpp \
    ../common/renderer.cpp \
    ../common/ram_cache.cpp \
    ../common/frustum.cpp \
    ../vcglib/wrap/gui/trackmode.cpp \
    ../vcglib/wrap/gui/trackball.cpp \
    ../vcglib/wrap/system/qgetopt.cpp \
    ../common/nexusdata.cpp \
    ../nxszip/bitstream.cpp \
    ../nxszip/tunstall.cpp \
    ../nxszip/meshcoder.cpp \
    ../nxszip/meshdecoder2.cpp

HEADERS  += ../common/signature.h \
    ../common/nexus.h \
    ../common/cone.h \
    gl_nxsview.h \
    scene.h \
    ../common/traversal.h \
    ../common/token.h \
    ../common/renderer.h \
    ../common/ram_cache.h \
    ../common/metric.h \
    ../common/gpu_cache.h \
    ../common/globalgl.h \
    ../common/frustum.h \
    ../common/dag.h \
    ../common/controller.h \
    ../common/nexusdata.h \
    ../vcglib/wrap/gcache/token.h \
    ../vcglib/wrap/gcache/provider.h \
    ../vcglib/wrap/gcache/door.h \
    ../vcglib/wrap/gcache/dheap.h \
    ../vcglib/wrap/gcache/controller.h \
    ../vcglib/wrap/gcache/cache.h \
    ../nxszip/bitstream.h \
    ../nxszip/tunstall.h \
    ../nxszip/meshcoder.h \
    ../nxszip/cstream.h \
    ../nxszip/connectivity.h \
    ../nxszip/zpoint.h \
    ../nxszip/meshdecoder2.h

FORMS    += \
    nxsview.ui

INCLUDEPATH += ../vcglib ../common/

win32:INCLUDEPATH += ../../../../code/lib/libcurl/include \
                     ../../../../code/lib/glew/include

win32:LIBS += ../../../../code/lib/libcurl/libcurl.lib \
              ../../../../code/lib/glew/lib/glew32.lib

unix:LIBS += -lGLEW -lGLU -lcurl

DESTDIR = "../bin"
