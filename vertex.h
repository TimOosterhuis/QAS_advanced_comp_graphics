#ifndef VERTEX
#define VERTEX

#include <QVector3D>
#include <QDebug>

// Forward declaration
class HalfEdge;

class Vertex {
public:
    static const unsigned short FLAG_NONE = 0;
    static const unsigned short FLAG_SUBDIV = 1;
    
  QVector3D coords;
  HalfEdge* out;
  unsigned short val;
  unsigned int index;
  unsigned short sharpness;
  unsigned short flag = FLAG_NONE;

  // Inline constructors
  Vertex() {
    // qDebug() << "Default Vertex Constructor";
    coords = QVector3D();
    out = 0; //nullptr
    val = 0;
    index = 0;
    sharpness = 0;
  }

  Vertex(QVector3D vcoords, HalfEdge* vout, unsigned short vval, unsigned int vindex, float vsharpness = 0) {
    //qDebug() << "QVector3D Vertex Constructor";
    coords = vcoords;
    out = vout;
    val = vval;
    index = vindex;
    sharpness = vsharpness;
  }
};

#endif // VERTEX
