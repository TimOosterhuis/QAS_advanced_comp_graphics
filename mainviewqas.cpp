#include "mainviewqas.h"

MainViewQAS::MainViewQAS(QWidget *Parent) : QOpenGLWidget(Parent) {
  qDebug() << "✓✓ MainView constructor";

  //initialize global parameters
  modelLoaded = false;
  wireframeMode = true;

  rotAngle = 0.0;
  FoV = 60.0;

  ref_line_size = 1;
  show_ref_lines = false;
  show_ref_lines_old = show_ref_lines;
  ref_line_size_old = ref_line_size;

  pointSelected = false;

}

MainViewQAS::~MainViewQAS() {
  qDebug() << "✗✗ MainView destructor";

  glDeleteBuffers(1, &meshCoordsBO);
  glDeleteBuffers(1, &meshNormalsBO);
  glDeleteBuffers(1, &meshIndexBO);
  glDeleteVertexArrays(1, &meshVAO);

  debugLogger->stopLogging();

  delete mainShaderProg;
}

// ---

void MainViewQAS::createShaderPrograms() {
  qDebug() << ".. createShaderPrograms";

  mainShaderProg = new QOpenGLShaderProgram();
  mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertshader.glsl");
  mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragshader.glsl");

  mainShaderProg->link();

  qasShaderProg = new QOpenGLShaderProgram();
  qasShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, "/shaders/qas/vertshader.glsl");
  qasShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, "/shaders/qas/tessctrlshader.glsl");
  qasShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, "/shaders/qas/tessevalshader.glsl");
  qasShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, "/shaders/qas/fragshader.glsl");

  qasShaderProg->link();

  //qasShaderProg isn't bound anywhere yet

  //get uniform locations
  uniModelViewMatrix = glGetUniformLocation(qasShaderProg->programId(), "modelviewmatrix");
  uniProjectionMatrix = glGetUniformLocation(qasShaderProg->programId(), "projectionmatrix");
  uniNormalMatrix = glGetUniformLocation(qasShaderProg->programId(), "normalmatrix");
  uniShowRefLines = glGetUniformLocation(qasShaderProg->programId(), "show_isophotes");
  uniRefLineSize = glGetUniformLocation(qasShaderProg->programId(), "isophote_size");

  uniModelViewMatrixMain = glGetUniformLocation(mainShaderProg->programId(), "modelviewmatrix");
  uniProjectionMatrixMain = glGetUniformLocation(mainShaderProg->programId(), "projectionmatrix");
  uniNormalMatrixMain = glGetUniformLocation(mainShaderProg->programId(), "normalmatrix");
  uniShowRefLinesMain = glGetUniformLocation(mainShaderProg->programId(), "show_isophotes");
  uniRefLineSizeMain = glGetUniformLocation(mainShaderProg->programId(), "isophote_size");
}

void MainViewQAS::createBuffers() {

  qDebug() << ".. createBuffers";

  glGenVertexArrays(1, &meshVAO);
  glBindVertexArray(meshVAO);

  glGenBuffers(1, &meshCoordsBO);
  glBindBuffer(GL_ARRAY_BUFFER, meshCoordsBO);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &meshNormalsBO);
  glBindBuffer(GL_ARRAY_BUFFER, meshNormalsBO);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &meshIndexBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);

  glBindVertexArray(0);
}

void MainViewQAS::updateMeshBuffers(Mesh* currentMesh) {

  qDebug() << ".. updateBuffers";

  unsigned int k;
  unsigned short m;
  HalfEdge* currentEdge;

  vertexCoords.clear();
  vertexCoords.reserve(currentMesh->Vertices.size());

  for (k=0; k<currentMesh->Vertices.size(); k++) {
    vertexCoords.append(currentMesh->Vertices[k].coords);
  }

  vertexNormals.clear();
  vertexNormals.reserve(currentMesh->Vertices.size());

  for (k=0; k<currentMesh->Faces.size(); k++) {
    currentMesh->setFaceNormal(&currentMesh->Faces[k]);
  }

  for (k=0; k<currentMesh->Vertices.size(); k++) {
    vertexNormals.append( currentMesh->computeVertexNormal(&currentMesh->Vertices[k]) );
  }

  polyIndices.clear();
  polyIndices.reserve(currentMesh->HalfEdges.size() + currentMesh->Faces.size());

  for (k=0; k<currentMesh->Faces.size(); k++) {
    currentEdge = currentMesh->Faces[k].side;
    for (m=0; m<3; m++) {
      polyIndices.append(currentEdge->target->index);
      currentEdge = currentEdge->next;
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, meshCoordsBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*vertexCoords.size(), vertexCoords.data(), GL_DYNAMIC_DRAW);

  qDebug() << " → Updated meshCoordsBO";

  glBindBuffer(GL_ARRAY_BUFFER, meshNormalsBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*vertexNormals.size(), vertexNormals.data(), GL_DYNAMIC_DRAW);

  qDebug() << " → Updated meshNormalsBO";

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*polyIndices.size(), polyIndices.data(), GL_DYNAMIC_DRAW);

  qDebug() << " → Updated meshIndexBO";

  meshIBOSize = polyIndices.size();

  update();

}

void MainViewQAS::updateMeshBuffersQAS(Mesh* currentMesh, Mesh* edgePointMesh) {

  //When using QAS a control hexagon (with normals) is sent to the shaders for each triangle in the original mesh
  //The edgepoints of this control hexagon can be found in the edgepoint mesh (which has been
  //subdivided and has had limitpoint calculated), hence the two mesh pointers as parameters

  //nr of vertices and normals come from edgepointMesh, nr of faces comes from currentMesh

  qDebug() << ".. updateBuffers";

  unsigned int k;
  unsigned short m;
  HalfEdge* currentEdge;
  unsigned int subdivEdgeIdx;

  vertexCoords.clear();
  vertexCoords.reserve(edgePointMesh->Vertices.size());

  for (k=0; k<edgePointMesh->Vertices.size(); k++) {
    vertexCoords.append(edgePointMesh->Vertices[k].coords);
  }

  vertexNormals.clear();
  vertexNormals.reserve(edgePointMesh->Vertices.size());

  for (k=0; k<edgePointMesh->Faces.size(); k++) {
    edgePointMesh->setFaceNormal(&edgePointMesh->Faces[k]);
  }

  for (k=0; k<edgePointMesh->Vertices.size(); k++) {
    vertexNormals.append( edgePointMesh->computeVertexNormal(&edgePointMesh->Vertices[k]) );
  }

  polyIndices.clear();
  polyIndices.reserve(edgePointMesh->HalfEdges.size() + currentMesh->Faces.size());

  for (k=0; k<currentMesh->Faces.size(); k++) {
    currentEdge = currentMesh->Faces[k].side;
    //the way the halfedges are split when subdividing preserves the order,
    //for an halfedge at position k in the original mesh the corresponding halfedges in the subdivided mesh can be found
    //at positions 2*k and 2*k + 1
    for (m=0; m<3; m++) {
      subdivEdgeIdx = 2 * currentEdge->index;
      polyIndices.append(edgePointMesh->HalfEdges[subdivEdgeIdx]->target->index);

      subdivEdgeIdx = 2 * currentEdge->index + 1;
      polyIndices.append(edgePointMesh->HalfEdges[subdivEdgeIdx]->target->index);

      currentEdge = currentEdge->next;
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, meshCoordsBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*vertexCoords.size(), vertexCoords.data(), GL_DYNAMIC_DRAW);

  qDebug() << " → Updated meshCoordsBO";

  glBindBuffer(GL_ARRAY_BUFFER, meshNormalsBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*vertexNormals.size(), vertexNormals.data(), GL_DYNAMIC_DRAW);

  qDebug() << " → Updated meshNormalsBO";

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*polyIndices.size(), polyIndices.data(), GL_DYNAMIC_DRAW);

  qDebug() << " → Updated meshIndexBO";

  meshIBOSize = polyIndices.size();

  update();

}

void MainViewQAS::updateMatrices() {

  modelViewMatrix.setToIdentity();
  modelViewMatrix.translate(QVector3D(0.0, 0.0, -3.0));
  modelViewMatrix.scale(QVector3D(1.0, 1.0, 1.0));
  modelViewMatrix.rotate(rotAngle, QVector3D(0.0, 1.0, 0.0));

  projectionMatrix.setToIdentity();
  projectionMatrix.perspective(FoV, dispRatio, 0.2, 4.0);

  normalMatrix = modelViewMatrix.normalMatrix();

  uniformUpdateRequired = true;
  update();

}

void MainViewQAS::updateUniforms() {

  // mainShaderProg should be bound at this point!


  glUniformMatrix4fv(uniModelViewMatrix, 1, false, modelViewMatrix.data());
  glUniformMatrix4fv(uniProjectionMatrix, 1, false, projectionMatrix.data());
  glUniformMatrix3fv(uniNormalMatrix, 1, false, normalMatrix.data());
  //update isophote uniforms
  glUniform1i(uniShowRefLines, show_ref_lines);
  glUniform1i(uniRefLineSize, ref_line_size);
  //set old values of isophote uniforms (used for checking whether a uniform update is required)
  show_ref_lines_old = show_ref_lines;
  ref_line_size_old = ref_line_size;

}

// ---

void MainViewQAS::initializeGL() {

  initializeOpenGLFunctions();
  qDebug() << ":: OpenGL initialized";

  debugLogger = new QOpenGLDebugLogger();
  connect( debugLogger, SIGNAL( messageLogged( QOpenGLDebugMessage ) ), this, SLOT( onMessageLogged( QOpenGLDebugMessage ) ), Qt::DirectConnection );

  if ( debugLogger->initialize() ) {
    qDebug() << ":: Logging initialized";
    debugLogger->startLogging( QOpenGLDebugLogger::SynchronousLogging );
    debugLogger->enableMessages();
  }

  QString glVersion;
  glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  qDebug() << ":: Using OpenGL" << qPrintable(glVersion);

  // Enable depth buffer
  glEnable(GL_DEPTH_TEST);
  // Default is GL_LESS
  glDepthFunc(GL_LEQUAL);


  createShaderPrograms();
  createBuffers();


  updateMatrices();
}

bool MainViewQAS::shaderSettingsChanged() {
    //return true if the value of a global parameter which requires a uniform update in the shader is changed
    return (show_ref_lines != show_ref_lines_old || ref_line_size != ref_line_size_old);
}

void MainViewQAS::resizeGL(int newWidth, int newHeight) {

  qDebug() << ".. resizeGL";

  dispRatio = (float)newWidth/newHeight;
  updateMatrices();

}

void MainViewQAS::paintGL() {
  if (modelLoaded) {
    //check whether uniform update is necessary as far as isophotes are concerned
    if (shaderSettingsChanged()) {
        uniformUpdateRequired = true;
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mainShaderProg->bind();
    //update uniforms if necessary
    if (uniformUpdateRequired) {
      updateUniforms();
      uniformUpdateRequired = false;
    }
    //render mesh
    renderMesh();

    //draw the selected control point if it exists
    glPointSize(12.0);

    if (pointSelected) {
      glBindVertexArray(meshVAO);
      glDrawArrays(GL_POINTS, selectedPoint, 1);
      glBindVertexArray(0);
    }

    mainShaderProg->release();

  }
}

// ---

void MainViewQAS::renderMesh() {

  glBindVertexArray(meshVAO);

  if (wireframeMode) {
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  }
  else {
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
  }

  glDrawElements(GL_TRIANGLES, meshIBOSize, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);

}

//on a mouse click, try to determine the nearest vertex
void MainViewQAS::mousePressEvent(QMouseEvent* event) {
  setFocus();

  float xRatio, yRatio, xScene, yScene;
  //x and y coordinates in window space
  xRatio = (float)event->x() / width();
  yRatio = (float)event->y() / height();

  // By default, the drawing canvas is the square [-1,1]^2:
  xScene = (1-xRatio)*-1 + xRatio*1;
  // Note that the origin of the canvas is in the top left corner (not the lower left).
  yScene = yRatio*-1 + (1-yRatio)*1;
  //ray direction in clipping space
  QVector4D ray_clip = QVector4D(xScene, yScene, -1.0, 1.0);
  //multiply by the inverted projection matrix to get the x and y direction in camera space
  QVector4D ray_eye = projectionMatrix.inverted() * ray_clip;
  //manually set the z direction and w coordinate to create a ray direction vector camera space
  ray_eye.setZ(-1.0);
  ray_eye.setW(0.0);
  //multiply by the inverted modelview matrix and normalize to get the unit ray direction vector
  //in world/object space
  QVector4D ray_world = modelViewMatrix.inverted() * ray_eye;
  ray_world = ray_world.normalized();

  //ray trace and rerender if a ray intersects a mesh vertex
  rayTrace(ray_world);
  update();

}

void MainViewQAS::rayTrace(QVector4D ray) {
    float min_dist = 1;
    //calculate the ray origin (camera position) in world/object space
    QVector4D camera_pos = modelViewMatrix.inverted() * QVector4D(0.0,0.0,0.0,1.0);
    //cast a ray for every vertex
    for (int k = 0; k < vertexCoords.size(); k++) {
        //solve the equation between the ray and the plane obtained by the vertex coordinates and it's normalvector
        QVector4D V0 = QVector4D(vertexCoords[k], 1.0) - camera_pos;
        float dotV0Normal = V0.x() * vertexNormals[k].x() + V0.y() * vertexNormals[k].y() + V0.z() * vertexNormals[k].z();
        float dotRayNormal = ray.x() * vertexNormals[k].x() + ray.y() * vertexNormals[k].y() + ray.z() * vertexNormals[k].z();
        float t = dotV0Normal/dotRayNormal;

        QVector4D intersect = camera_pos + (t * ray);

        //calculate the distance between the ray plane intersection point vertex
        float dist = (QVector4D(vertexCoords[k], 1.0) - intersect).length();

        //future TO DO: somehow incorporate the angle between the ray and the vertex plane in this distance measurement
        //(as of now the selection area (in screen space) is slightly larger for some vertexes than others, because the same distance
        //away from the vertex in screen space will result in a relatively larger increase of the distance of the ray plane intersection point
        //from the vertex point the more parallel the vertex plane is to the ray)

        //the index of the point with the closest ray intersection through it's plane is saved globally for later rendering
        if (dist < min_dist) {
            selectedPoint = k;
            min_dist = dist;
        }
    }
    //If the mininum intersection distance is 'close' enough, consider the vertex selected.
    if (min_dist < 0.2) {
        pointSelected = true;
    } else {
        pointSelected = false;
    }
}

void MainViewQAS::wheelEvent(QWheelEvent* event) {
  FoV -= event->delta() / 60.0;
  updateMatrices();
}

void MainViewQAS::keyPressEvent(QKeyEvent* event) {
  switch(event->key()) {
  case 'Z':
    wireframeMode = !wireframeMode;
    update();
    break;
  }
}

// ---

void MainViewQAS::onMessageLogged( QOpenGLDebugMessage Message ) {
  qDebug() << " → Log:" << Message;
}
