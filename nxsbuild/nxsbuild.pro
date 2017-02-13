#-------------------------------------------------
#
# Project created by QtCreator 2012-05-20T21:38:32
#
#-------------------------------------------------

QT       += core
QT       += gui 

QTPLUGIN     += qjpeg    # image formats

TARGET = nxsbuild
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11 -g

INCLUDEPATH += ../vcglib

DEFINES += _FILE_OFFSET_BITS=64 TEXTURE

SOURCES += main.cpp \
    meshstream.cpp \
    meshloader.cpp \
    plyloader.cpp \
    ../vcglib/wrap/system/qgetopt.cpp \
    ../vcglib/wrap/ply/plylib.cpp \
    partition.cpp \
    kdtree.cpp \
    trianglesoup.cpp \
    mesh.cpp \
    ../common/cone.cpp \
    tsploader.cpp \
    ../common/virtualarray.cpp \
    nexusbuilder.cpp \
    objloader.cpp \
    tmesh.cpp

win32-msvc: DEFINES += NOMINMAX

HEADERS += \
    meshstream.h \
    ../common/signature.h \
    meshloader.h \
    plyloader.h \
    ../vcglib/wrap/system/qgetopt.h \
    ../vcglib/wrap/ply/plylib.h \
    ../common/geometry.h \
    partition.h \
    kdtree.h \
    trianglesoup.h \
    mesh.h \
    ../common/cone.h \
    tsploader.h \
    ../common/virtualarray.h \
    nexusbuilder.h \
    objloader.h \
    tmesh.h

DESTDIR = "../bin"

OTHER_FILES += \
    textures_plan.txt
