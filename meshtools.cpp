#include "meshtools.h"

void subdivideLoop(Mesh* inputMesh, Mesh* subdivMesh) {
  unsigned int numVerts, numHalfEdges, numFaces;
  unsigned int k, m, s, t;
  unsigned int vIndex, hIndex, fIndex;
  unsigned short n;
  HalfEdge* currentEdge;

  qDebug() << ":: Creating new Loop mesh";

  numVerts = inputMesh->Vertices.size();
  numHalfEdges = inputMesh->HalfEdges.size();
  numFaces = inputMesh->Faces.size();

  // Reserve memory
  subdivMesh->Vertices.reserve(numVerts + numHalfEdges/2);
  subdivMesh->HalfEdges.reserve(2*numHalfEdges + 6*numFaces);
  subdivMesh->Faces.reserve(4*numFaces);

  // Create vertex points
  for (k=0; k<numVerts; k++) {
    n = inputMesh->Vertices[k].val;
    // Coords (x,y,z), Out, Valence, Index
    HalfEdge* outEdge = inputMesh->Vertices[k].out;
    bool boundaryVert = false;
    if (!outEdge->twin->polygon) {
        boundaryVert = true;
    }
    HalfEdge* nextEdge = outEdge->twin;
    while (nextEdge != outEdge) {
        if (!nextEdge->twin->polygon) {
            boundaryVert = true;
            break;
        }
        if (nextEdge->target->coords == inputMesh->Vertices[k].coords) {
            nextEdge = nextEdge->next;
        } else {
            nextEdge = nextEdge->twin;
        }
    }
    if (boundaryVert) {
        subdivMesh->Vertices.append( Vertex( vertexPoint(3, inputMesh->Vertices[k].out),
                                         nullptr,
                                         n,
                                         k) );
    } else {
        subdivMesh->Vertices.append( Vertex( vertexPoint(2, inputMesh->Vertices[k].out),
                                         nullptr,
                                         n,
                                         k) );
    }
  }

  vIndex = numVerts;
  qDebug() << " * Created vertex points";

  // Create edge points
  for (k=0; k<numHalfEdges; k++) {
    currentEdge = &inputMesh->HalfEdges[k];
    bool boundaryEdge = false;
    if (!currentEdge->twin->polygon) {
        boundaryEdge = true;
    }
    if (k < currentEdge->twin->index) {
      // Coords (x,y,z), Out, Valence, Index
      if (boundaryEdge) {
          subdivMesh->Vertices.append( Vertex(edgePoint(3, currentEdge),
                                             nullptr,
                                             4,
                                             vIndex) );
      } else {
          subdivMesh->Vertices.append( Vertex(edgePoint(2, currentEdge),
                                             nullptr,
                                             6,
                                             vIndex) );
      }
      vIndex++;
    }
  }

  qDebug() << " * Created edge points";

  // Split halfedges
  splitHalfEdges(inputMesh, subdivMesh, numHalfEdges, numVerts, 0);

  qDebug() << " * Split halfedges";

  hIndex = 2*numHalfEdges;
  fIndex = 0;

  // Create faces and remaining halfedges
  for (k=0; k<numFaces; k++) {
    currentEdge = inputMesh->Faces[k].side;

    // Three outer faces

    for (m=0; m<3; m++) {

      s = currentEdge->prev->index;
      t = currentEdge->index;

      // Side, Val, Index
      subdivMesh->Faces.append(Face(nullptr,
                                    3,
                                    fIndex));

      subdivMesh->Faces[fIndex].side = &subdivMesh->HalfEdges[ 2*t ];

      // Target, Next, Prev, Twin, Poly, Index
      subdivMesh->HalfEdges.append(HalfEdge( subdivMesh->HalfEdges[2*s].target,
                                             &subdivMesh->HalfEdges[2*s+1],
                                             &subdivMesh->HalfEdges[ 2*t ],
                                             nullptr,
                                             &subdivMesh->Faces[fIndex],
                                             hIndex ));

      subdivMesh->HalfEdges.append(HalfEdge( subdivMesh->HalfEdges[2*t].target,
                                             nullptr,
                                             nullptr,
                                             &subdivMesh->HalfEdges[hIndex],
                                             nullptr,
                                             hIndex+1 ));

      subdivMesh->HalfEdges[hIndex].twin = &subdivMesh->HalfEdges[hIndex+1];

      subdivMesh->HalfEdges[2*s+1].next = &subdivMesh->HalfEdges[2*t];
      subdivMesh->HalfEdges[2*s+1].prev = &subdivMesh->HalfEdges[hIndex];
      subdivMesh->HalfEdges[2*s+1].polygon = &subdivMesh->Faces[fIndex];

      subdivMesh->HalfEdges[2*t].next = &subdivMesh->HalfEdges[hIndex];
      subdivMesh->HalfEdges[2*t].prev = &subdivMesh->HalfEdges[2*s+1];
      subdivMesh->HalfEdges[2*t].polygon = &subdivMesh->Faces[fIndex];

      // For edge points
      subdivMesh->HalfEdges[2*t].target->out = &subdivMesh->HalfEdges[hIndex];

      hIndex += 2;
      fIndex++;
      currentEdge = currentEdge->next;

    }

    // Inner face

    // Side, Val, Index
    subdivMesh->Faces.append(Face(nullptr,
                                  3,
                                  fIndex));

    subdivMesh->Faces[fIndex].side = &subdivMesh->HalfEdges[ hIndex-1 ];

    for (m=0; m<3; m++) {

      if (m==2) {
        subdivMesh->HalfEdges[hIndex - 1].next = &subdivMesh->HalfEdges[hIndex - 5];
      }
      else {
        subdivMesh->HalfEdges[hIndex - 5 + 2*m].next = &subdivMesh->HalfEdges[hIndex - 5 + 2*(m+1)];
      }

      if (m==0) {
        subdivMesh->HalfEdges[hIndex - 5].prev = &subdivMesh->HalfEdges[hIndex - 1];
      }
      else {
        subdivMesh->HalfEdges[hIndex - 5 + 2*m].prev = &subdivMesh->HalfEdges[hIndex - 5 + 2*(m-1)];
      }

      subdivMesh->HalfEdges[hIndex - 5 + 2*m].polygon = &subdivMesh->Faces[fIndex];

    }

    fIndex++;

  }

  qDebug() << " * Created faces";
  // For vertex points
  for (k=0; k<numVerts; k++) {
    subdivMesh->Vertices[k].out = &subdivMesh->HalfEdges[ 2*inputMesh->Vertices[k].out->index ];
  }

  //assign previous and next edges to all boundary halfedges
  for (k=0; k<subdivMesh->HalfEdges.length(); k++) {
      //if the current halfedge k does not have a pointer to a next halfedge
      if (!subdivMesh->HalfEdges[k].next) {
          //find the outgoing vertex from it's target that does not have a face and assign it to be the next of the current vertex
          HalfEdge* nextEdge = subdivMesh->HalfEdges[k].target->out;
          for (int m = 0; m < subdivMesh->HalfEdges[k].target->val; m++) {
              if(!nextEdge->polygon) {
                  subdivMesh->HalfEdges[k].next = nextEdge;
                  break;
              }
              nextEdge = nextEdge->prev->twin;
          }
       }
       //if the current halfedge k does not have a pointer to a previous halfedge
       if (!subdivMesh->HalfEdges[k].prev) {
          //find the incoming vertex from it's twin's target that does not have a face and assign it to be the previous of the current vertex
          HalfEdge* prevEdge = subdivMesh->HalfEdges[k].twin->target->out->twin;
          for (int m = 0; m < subdivMesh->HalfEdges[k].twin->target->val; m++) {
              if(!prevEdge->polygon) {
                  subdivMesh->HalfEdges[k].prev = prevEdge;
                  break;
              }
              prevEdge = prevEdge->next->twin;
          }
      }
  }


}


QVector3D vertexPoint(unsigned short subdivType, HalfEdge* firstEdge) {
  unsigned short k, n;
  QVector3D sumStarPts, sumFacePts;
  QVector3D vertexPt;
  float stencilValue;
  HalfEdge* currentEdge;
  Vertex* currentVertex;

  currentVertex = firstEdge->twin->target;
  n = currentVertex->val;

  sumStarPts = QVector3D();
  sumFacePts = QVector3D();
  currentEdge = firstEdge;

  switch (subdivType) {
  case 2:
    // Loop
    for (k=0; k<n; k++) {
      sumStarPts += currentEdge->target->coords;
      currentEdge = currentEdge->prev->twin;
    }

    // Warren's rules
    if (n == 3) {
      stencilValue = 3.0/16.0;
    }
    else {
      stencilValue = 3.0/(8*n);
    }

    vertexPt = (1.0 - n*stencilValue) * currentVertex->coords + stencilValue * sumStarPts;
    break;
  case 3:
    //If the new vertex is a boundary vertex, it's coordinates should be the same as the coordinates of the corresponding
    //vertex in the current subdivision level
    HalfEdge* outEdge = currentVertex->out;
    for (unsigned int i = 0; i < currentVertex->val; i++) {
        if (!outEdge->polygon) {
            break;
        }
        outEdge = outEdge->twin->next;
    }
    vertexPt = (6 * currentVertex->coords + outEdge->target->coords + outEdge->prev->twin->target->coords) / 8;
    break;
  }

  return vertexPt;

}

QVector3D edgePoint(unsigned short subdivType, HalfEdge* firstEdge) {
  QVector3D EdgePt;
  HalfEdge* currentEdge;

  EdgePt = QVector3D();
  currentEdge = firstEdge;

  switch (subdivType) {
  case 2:
    // Loop
    EdgePt = QVector3D();
    currentEdge = firstEdge;
    EdgePt  = 6.0 * currentEdge->target->coords;
    EdgePt += 2.0 * currentEdge->next->target->coords;
    EdgePt += 6.0 * currentEdge->twin->target->coords;
    EdgePt += 2.0 * currentEdge->twin->next->target->coords;
    EdgePt /= 16.0;
    break;
  case 3:
      EdgePt = QVector3D();
      currentEdge = firstEdge;
      // if the new edge vertex is a boundary vertex, it should be rendered in the middle of the corresponding boundary edge in the
      //current subdivision level
      EdgePt = (currentEdge->target->coords + currentEdge->twin->target->coords) / 2;
      break;
  }

  return EdgePt;

}

void splitHalfEdges(Mesh* inputMesh, Mesh* subdivMesh, unsigned int numHalfEdges, unsigned int numVertPts, unsigned int numFacePts) {
  unsigned int k, m;
  unsigned int vIndex;
  HalfEdge* currentEdge;

  vIndex = numFacePts + numVertPts;

  for (k=0; k<numHalfEdges; k++) {
    currentEdge = &inputMesh->HalfEdges[k];
    m = currentEdge->twin->index;

    // Target, Next, Prev, Twin, Poly, Index
    subdivMesh->HalfEdges.append(HalfEdge(nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          2*k));

    subdivMesh->HalfEdges.append(HalfEdge(nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          2*k+1));

    if (k < m) {
      subdivMesh->HalfEdges[2*k].target = &subdivMesh->Vertices[ vIndex ];
      subdivMesh->HalfEdges[2*k+1].target = &subdivMesh->Vertices[ numFacePts + currentEdge->target->index ];
      vIndex++;
    }
    else {
      subdivMesh->HalfEdges[2*k].target = subdivMesh->HalfEdges[2*m].target;
      subdivMesh->HalfEdges[2*k+1].target = &subdivMesh->Vertices[ numFacePts + currentEdge->target->index ];

      // Assign Twins
      subdivMesh->HalfEdges[2*k].twin = &subdivMesh->HalfEdges[2*m+1];
      subdivMesh->HalfEdges[2*k+1].twin = &subdivMesh->HalfEdges[2*m];
      subdivMesh->HalfEdges[2*m].twin = &subdivMesh->HalfEdges[2*k+1];
      subdivMesh->HalfEdges[2*m+1].twin = &subdivMesh->HalfEdges[2*k];
    }
  }

  // Note that Next, Prev and Poly are not yet assigned at this point.

}