#-------------------------------------------------
#
# Project created by QtCreator 2012-05-29T18:43:31
#
#-------------------------------------------------

QT       += core
#QT       -= gui

TARGET = nxsedit
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

DEFINES += _FILE_OFFSET_BITS=64 TEXTURE #PARALLELOGRAM

INCLUDEPATH += ../vcglib 

SOURCES += main.cpp \
    ../common/virtualarray.cpp \
    ../common/traversal.cpp \
    ../common/cone.cpp \
    ../vcglib/wrap/system/qgetopt.cpp \
    ../nxsbuild/tsploader.cpp \
    ../nxsbuild/trianglesoup.cpp \
    ../nxsbuild/plyloader.cpp \
    ../nxsbuild/partition.cpp \
    ../nxsbuild/meshstream.cpp \
    ../nxsbuild/meshloader.cpp \
    ../nxsbuild/kdtree.cpp \
    ../nxsbuild/mesh.cpp \
    ../vcglib/wrap/ply/plylib.cpp \
    extractor.cpp \
    ../nxsbuild/nexusbuilder.cpp \
    ../common/nexusdata.cpp \
    ../nxsbuild/objloader.cpp \
    ../nxszip/bitstream.cpp \
    ../nxszip/tunstall.cpp \
    ../nxszip/meshcoder.cpp \
    ../nxszip/meshcoder2.cpp \
    ../nxszip/meshdecoder2.cpp \
    ../nxsbuild/tmesh.cpp

HEADERS += \
    ../common/virtualarray.h \
    ../common/traversal.h \
    ../common/signature.h \
    ../vcglib/wrap/system/qgetopt.h \
    ../nxsbuild/tsploader.h \
    ../nxsbuild/trianglesoup.h \
    ../nxsbuild/plyloader.h \
    ../nxsbuild/partition.h \
    ../nxsbuild/meshstream.h \
    ../nxsbuild/meshloader.h \
    ../nxsbuild/kdtree.h \
    ../nxsbuild/mesh.h \
    extractor.h \
    ../nxsbuild/nexusbuilder.h \
    ../common/nexusdata.h \
    ../nxsbuild/objloader.h \
    ../nxszip/zpoint.h \
    ../nxszip/model.h \
    ../nxszip/range.h \
    ../nxszip/connectivity_coder.h \
    ../nxszip/fpu_precision.h \
    ../nxszip/bytestream.h \
    ../nxszip/math_class.h \
    ../nxszip/bitstream.h \
    ../nxszip/tunstall.h \
    ../nxszip/cstream.h \
    ../nxszip/connectivity.h \
    ../nxszip/meshcoder.h \
    ../nxszip/meshcoder2.h \
    ../nxszip/meshdecoder2.h \
    ../nxsbuild/tmesh.h

DESTDIR = "../bin"
