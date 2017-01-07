#include "meshtools.h"

void subdivideLoop(Mesh* inputMesh, Mesh* subdivMesh)
{
    // Hope you don't mind I refractored this a bit, tried to make it more readable.
    // But everything I basically did was adding a .flag = FLAG_SUBDIV, so we can check if it was subdivided or an original vertex.
    
    unsigned int numVerts, numHalfEdges, numFaces;
    unsigned int vIndex, hIndex, fIndex;
    
    qDebug() << ":: Creating new Loop mesh";
    
    numVerts = inputMesh->Vertices.size();
    numHalfEdges = inputMesh->HalfEdges.size();
    numFaces = inputMesh->Faces.size();
    
    // Reserve memory
    subdivMesh->Vertices.reserve(numVerts + numHalfEdges/2);
    subdivMesh->HalfEdges.reserve(2*numHalfEdges + 6*numFaces);
    subdivMesh->Faces.reserve(4*numFaces);
    
    // Create vertex points
    for (int k=0; k<numVerts; k++)
    {
        int n = inputMesh->Vertices[k].val;
        
        // Coords (x,y,z), Out, Valence, Index
        HalfEdge* outEdge = inputMesh->Vertices[k].out;
        HalfEdge* nextEdge = outEdge->twin;
        bool boundaryVert = !outEdge->twin->polygon;
        
        while(nextEdge != outEdge)
        {
            if(!nextEdge->twin->polygon)
            {
                boundaryVert = true;
                break;
            }
            
            if(nextEdge->target->coords == inputMesh->Vertices[k].coords)
            {
                nextEdge = nextEdge->next;
            }
            else
            {
                nextEdge = nextEdge->twin;
            }
        }
        
        Vertex v = boundaryVert ? Vertex( vertexPoint(3, inputMesh->Vertices[k].out), nullptr, n, k) : Vertex( vertexPoint(2, inputMesh->Vertices[k].out), nullptr, n, k);
        
        subdivMesh->Vertices.append(v);
    }
    
    vIndex = numVerts;
    qDebug() << " * Created vertex points";
    
    // Create edge points
    for (int k=0; k<numHalfEdges; k++)
    {
        HalfEdge* currentEdge = &inputMesh->HalfEdges[k];
        bool boundaryEdge = !currentEdge->twin->polygon;
        
        if (k < currentEdge->twin->index)
        {
            // Coords (x,y,z), Out, Valence, Index
            Vertex v = boundaryEdge ? Vertex(edgePoint(3, currentEdge), nullptr, 4, vIndex++) : Vertex(edgePoint(2, currentEdge), nullptr, 6, vIndex++);
            
            v.flag = Vertex::FLAG_SUBDIV;
            
            subdivMesh->Vertices.append(v);
        }
    }
    
    qDebug() << " * Created edge points";
    
    // Split halfedges
    splitHalfEdges(inputMesh, subdivMesh, numHalfEdges, numVerts, 0);
    
    qDebug() << " * Split halfedges";
    
    hIndex = 2*numHalfEdges;
    fIndex = 0;
    
    // Create faces and remaining halfedges
    for (int k=0; k<numFaces; k++)
    {
        HalfEdge* currentEdge = inputMesh->Faces[k].side;
        
        // Three outer faces
        for (int m=0; m<3; m++)
        {
            int s = currentEdge->prev->index;
            int t = currentEdge->index;
            
            // Side, Val, Index
            subdivMesh->Faces.append(Face(nullptr, 3, fIndex));
            
            subdivMesh->Faces[fIndex].side = &subdivMesh->HalfEdges[ 2*t ];
            
            // We're not using 800x600 monitors anymore, are we?
            
            // Target, Next, Prev, Twin, Poly, Index
            subdivMesh->HalfEdges.append(HalfEdge( subdivMesh->HalfEdges[2*s].target, &subdivMesh->HalfEdges[2*s+1], &subdivMesh->HalfEdges[ 2*t ], nullptr, &subdivMesh->Faces[fIndex], hIndex ));
            
            subdivMesh->HalfEdges.append(HalfEdge( subdivMesh->HalfEdges[2*t].target, nullptr, nullptr, &subdivMesh->HalfEdges[hIndex], nullptr, hIndex+1 ));
            
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
            ++fIndex;
            
            // Go to next edge
            currentEdge = currentEdge->next;
        }
        
        // Inner face
        
        // Side, Val, Index
        subdivMesh->Faces.append(Face(nullptr, 3, fIndex));
        
        subdivMesh->Faces[fIndex].side = &subdivMesh->HalfEdges[ hIndex-1 ];
        
        for (int m=0; m<3; m++)
        {
            if (m==2)
            {
                subdivMesh->HalfEdges[hIndex - 1].next = &subdivMesh->HalfEdges[hIndex - 5];
            }
            else
            {
                subdivMesh->HalfEdges[hIndex - 5 + 2*m].next = &subdivMesh->HalfEdges[hIndex - 5 + 2*(m+1)];
            }
            
            if (m==0)
            {
                subdivMesh->HalfEdges[hIndex - 5].prev = &subdivMesh->HalfEdges[hIndex - 1];
            }
            else
            {
                subdivMesh->HalfEdges[hIndex - 5 + 2*m].prev = &subdivMesh->HalfEdges[hIndex - 5 + 2*(m-1)];
            }
            
            subdivMesh->HalfEdges[hIndex - 5 + 2*m].polygon = &subdivMesh->Faces[fIndex];
        }
        
        fIndex++;
    
    }
    
    qDebug() << " * Created faces";
    // For vertex points
    for (int k=0; k<numVerts; k++)
    {
        subdivMesh->Vertices[k].out = &subdivMesh->HalfEdges[ 2*inputMesh->Vertices[k].out->index ];
    }
    
    //assign previous and next edges to all boundary halfedges
    for (int k=0; k<subdivMesh->HalfEdges.length(); k++)
    {
        //if the current halfedge k does not have a pointer to a next halfedge
        if (!subdivMesh->HalfEdges[k].next)
        {
            //find the outgoing vertex from it's target that does not have a face and assign it to be the next of the current vertex
            HalfEdge* nextEdge = subdivMesh->HalfEdges[k].target->out;
            for (int m = 0; m < subdivMesh->HalfEdges[k].target->val; m++)
            {
                if(!nextEdge->polygon)
                {
                    subdivMesh->HalfEdges[k].next = nextEdge;
                    break;
                }
                nextEdge = nextEdge->prev->twin;
            }
        }
        
        //if the current halfedge k does not have a pointer to a previous halfedge
        if (!subdivMesh->HalfEdges[k].prev)
        {
            //find the incoming vertex from it's twin's target that does not have a face and assign it to be the previous of the current vertex
            HalfEdge* prevEdge = subdivMesh->HalfEdges[k].twin->target->out->twin;
            
            for (int m = 0; m < subdivMesh->HalfEdges[k].twin->target->val; m++)
            {
                if(!prevEdge->polygon)
                {
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

void toLimit(Mesh* inputMesh, Mesh* limitMesh) {
    //calculates limit points of a mesh, and assigns the resulting mesh to the limitmesh pointer
    unsigned int numVerts, numHalfEdges, numFaces;
    qDebug() << "*calculating limit vertex coordinates";

    numVerts = inputMesh->Vertices.size();
    numHalfEdges = inputMesh->HalfEdges.size();
    numFaces = inputMesh->Faces.size();
    //first the limitmesh is set up like a copy of the input mesh with respect to faces and halfedges
    limitMesh->Vertices.reserve(numVerts);
    limitMesh->Faces.reserve(numFaces);
    limitMesh->HalfEdges.reserve(numHalfEdges);
    for (int i = 0; i < numFaces; i++) {
        limitMesh->Faces.append(inputMesh->Faces[i]);
    }
    for (int i = 0; i < numHalfEdges; i++) {
        limitMesh->HalfEdges.append(inputMesh->HalfEdges[i]);
    }
    //then the vertices are added, and the correct positions are calculated
    unsigned int valency;
    //for every vertex
    for (int i = 0; i < numVerts; i++) {
        valency = inputMesh->Vertices[i].val;
        //append it to the limitmesh
        limitMesh->Vertices.append(inputMesh->Vertices[i]);
        bool boundary = false;
        //if it has an outgoing edge which does not have a polygon or which has a twin which does not have a polygon,
        //apply boundary rules
        HalfEdge* currentOutEdge = limitMesh->Vertices[i].out;
        for (int j = 0; j < valency; j++) {
            if (!currentOutEdge->polygon || !currentOutEdge->twin->polygon) {
                boundary = true;
                break;
            }
            currentOutEdge = currentOutEdge->twin->next;
        }
        QVector3D sum;
        if (boundary) {
            //find the previous and next boundary point and calculate new position according to the limit stencil for curves
            if(!currentOutEdge->polygon) {
                 sum = 4*limitMesh->Vertices[i].coords + currentOutEdge->target->coords + currentOutEdge->prev->twin->target->coords;
            } else {
                 sum = 4*limitMesh->Vertices[i].coords + currentOutEdge->target->coords + currentOutEdge->twin->next->target->coords;
            }
            limitMesh->Vertices[i].coords = sum/6;
        } else {
            //normal rules
            //contribution of the original vertex
            //sum = ((float)(valency - 3)/(float)(valency + 5)) * limitMesh->Vertices[i].coords; -> don't cast floats like this, try this on Suzanne model, in certain cases you'll get extreme values of >100000
            sum = (valency - 3.0f) / (valency + 5.0f) * limitMesh->Vertices[i].coords;
            //calculate the sum of edge and face points in the one ring neighborhood
            QVector3D sumPt;
            for (int j = 0; j < valency; j++) {
                QVector3D edgePt = (limitMesh->Vertices[i].coords + currentOutEdge->target->coords) / 2;
                QVector3D facePt = limitMesh->Vertices[i].coords;
                HalfEdge* polygonEdge = currentOutEdge;
                for (int k = 0; k < (currentOutEdge->polygon->val - 1); k++) {
                    facePt += polygonEdge->target->coords;
                    polygonEdge = polygonEdge->next;
                }
                sumPt += ((facePt/currentOutEdge->polygon->val) + edgePt);

                currentOutEdge = currentOutEdge->twin->next;
            }
            //add contribution of the one ring neighbourhood
            limitMesh->Vertices[i].coords = sum + (4.0f / (valency * (valency + 5.0f))) * sumPt;
        }
    }
}
