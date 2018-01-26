# if (mac OS X)
DESTDIR = ../viewer
# ENDIF
QT       += core gui opengl
CONFIG += DEBUG

TARGET = myViewer
TEMPLATE = app

debug{
QMAKE_CXXFLAGS += -g -O0
QMAKE_LFLAGS += -g -O0
}

macx {
  QMAKE_CXXFLAGS += -Wno-unknown-pragmas
} else {
  QMAKE_LFLAGS += -Wno-unknown-pragmas -fopenmp
}

SOURCES +=  \
            src/main.cpp \
            src/openglwindow.cpp \
            src/glshaderwindow.cpp \
            src/joint.cpp

HEADERS  += \
            src/openglwindow.h \
            src/glshaderwindow.h \
            src/perlinNoise.h \
            src/joint.h

# trimesh library for loading objects.
# Reference/source: http://gfx.cs.princeton.edu/proj/trimesh2/
INCLUDEPATH += ../trimesh2/include/

LIBS += -L../trimesh2/lib -ltrimesh
