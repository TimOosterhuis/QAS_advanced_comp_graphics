#include "mainview.h"

// TODO: We can separate QAS and Loop suffixed functions into separate files mainview_qas.h/cpp and mainview_loop.h/cpp,
// and then use wrapper-functions here that call both. But this is for later refractoring, not really necessary.

MainView::MainView(QWidget *Parent) : QOpenGLWidget(Parent)
{
    qDebug() << "✓✓ MainView constructor";
    
    //initialize global parameters
    viewMode = VIEW_MODE_LOOP_AND_QAS;
    modelLoaded = false;
    wireframeMode = true;
    tessellationLevel = 1;
    
    rotAngle = 0.0;
    FoV = 70.0;
    
    ref_line_size = 1;
    show_ref_lines = false;
}

MainView::~MainView()
{
    qDebug() << "✗✗ MainView destructor";
    
    glDeleteBuffers(1, &vboQASCoords);
    glDeleteBuffers(1, &vboQASNormals);
    glDeleteBuffers(1, &vboQASIndices);
    glDeleteVertexArrays(1, &vaoQAS);
    
    glDeleteBuffers(1, &meshCoordsBO);
    glDeleteBuffers(1, &meshNormalsBO);
    glDeleteBuffers(1, &meshIndexBO);
    glDeleteVertexArrays(1, &meshVAO);
    
    debugLogger->stopLogging();
    
    delete mainShaderProg;
    
    Meshes.clear();
    Meshes.squeeze();
    
    QAS_Meshes.clear();
    QAS_Meshes.squeeze();
}

void MainView::loadModelFromOBJFile(OBJFile model)
{
    // Preload Loop-subdivision meshes
    Meshes.clear();
    Meshes.squeeze();
    Meshes.append(Mesh(&model));
    
    for(int k = 0;k < MAX_SUBDIV_STEPS;++k)
    {
        Meshes.append(Mesh());
        subdivideLoop(&Meshes[k], &Meshes[k + 1]);
    }
    
    updateVertexArrayObjectLoop();
    
    // Preload QAS meshes (actually just one because we can use the preloaded Loop-subdivision mesh)
    QAS_Meshes.clear();
    QAS_Meshes.squeeze();
    QAS_Meshes.append(Mesh());
    
    // Prepare QAS Mesh
    toLimit(&Meshes[1 + qasSubdivOffset], &QAS_Meshes[0]);
    
    updateVertexArrayObjectQAS();
    
    
    // Finished loading model
    updateMatrices();
    modelLoaded = true;
    update();
}

void MainView::updateQASMesh()
{
    // Additional feature:
    QAS_Meshes.clear();
    QAS_Meshes.squeeze();
    QAS_Meshes.append(Mesh());
    
    toLimit(&Meshes[1 + qasSubdivOffset], &QAS_Meshes[0]);
    
    updateVertexArrayObjectQAS();
    
    update();
}

// ---

void MainView::createShaderProgramsQAS()
{
    qasShaderProg.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/qas/vertshader.glsl");
    qasShaderProg.addShaderFromSourceFile(QOpenGLShader::TessellationControl, ":/shaders/qas/tessctrlshader.glsl");
    qasShaderProg.addShaderFromSourceFile(QOpenGLShader::TessellationEvaluation, ":/shaders/qas/tessevalshader.glsl");
    qasShaderProg.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/qas/fragshader.glsl");
    qasShaderProg.link();
}

void MainView::createShaderProgramsLoop()
{
    mainShaderProg = new QOpenGLShaderProgram();
    mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertshader.glsl");
    mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragshader.glsl");
    mainShaderProg->link();
}

void MainView::initUniformLocationsQAS()
{
    uniformsQAS["tess_level_inner"] = qasShaderProg.uniformLocation("tess_level_inner");
    uniformsQAS["tess_level_outer"] = qasShaderProg.uniformLocation("tess_level_outer");
    uniformsQAS["modelviewmatrix"] = qasShaderProg.uniformLocation("modelviewmatrix");
    uniformsQAS["projectionmatrix"] = qasShaderProg.uniformLocation("projectionmatrix");
    uniformsQAS["normalmatrix"] = qasShaderProg.uniformLocation("normalmatrix");
    uniformsQAS["show_reflines"] = qasShaderProg.uniformLocation("show_isophotes");
    uniformsQAS["refline_size"] = qasShaderProg.uniformLocation("isophote_size");
    uniformsQAS["adaptive_tessellation"] = qasShaderProg.uniformLocation("adaptive_tessellation");
}

void MainView::initUniformLocationsLoop()
{
    uniModelViewMatrix = glGetUniformLocation(mainShaderProg->programId(), "modelviewmatrix");
    uniProjectionMatrix = glGetUniformLocation(mainShaderProg->programId(), "projectionmatrix");
    uniNormalMatrix = glGetUniformLocation(mainShaderProg->programId(), "normalmatrix");
    uniShowRefLines = glGetUniformLocation(mainShaderProg->programId(), "show_isophotes");
    uniRefLineSize = glGetUniformLocation(mainShaderProg->programId(), "isophote_size");
}

void MainView::createBuffersQAS()
{
    glGenVertexArrays(1, &vaoQAS);
    glBindVertexArray(vaoQAS);
    {
        glGenBuffers(1, &vboQASCoords);
        glBindBuffer(GL_ARRAY_BUFFER, vboQASCoords);
        {
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        }
        
        glGenBuffers(1, &vboQASNormals);
        glBindBuffer(GL_ARRAY_BUFFER, vboQASNormals);
        {
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        }
        
        glGenBuffers(1, &vboQASIndices);
    }
    glBindVertexArray(0);
}

void MainView::createBuffersLoop()
{
    glGenVertexArrays(1, &meshVAO);
    glBindVertexArray(meshVAO);
    {
        glGenBuffers(1, &meshCoordsBO);
        glBindBuffer(GL_ARRAY_BUFFER, meshCoordsBO);
        {
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        }
        
        glGenBuffers(1, &meshNormalsBO);
        glBindBuffer(GL_ARRAY_BUFFER, meshNormalsBO);
        {
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        }
        
        glGenBuffers(1, &meshIndexBO);
    }
    glBindVertexArray(0);
}

void MainView::updateVertexArrayObjectQAS()
{
    Mesh* currentMesh = &QAS_Meshes[0];
    Mesh* originalMesh = &Meshes[0 + qasSubdivOffset];
    
    qDebug() << ".. updateBuffers";
    
    // Update vertex coords
    vertexCoordsQAS.clear();
    vertexCoordsQAS.reserve(currentMesh->Vertices.size());
    for (int k=0;k<currentMesh->Vertices.size();++k)
    {
        vertexCoordsQAS.append(currentMesh->Vertices[k].coords);
    }
    
    // Update vertex normals
    vertexNormalsQAS.clear();
    vertexNormalsQAS.reserve(currentMesh->Vertices.size());
    for (int k=0;k<currentMesh->Faces.size();++k)
    {
        currentMesh->setFaceNormal(&currentMesh->Faces[k]);
    }

    for (int k=0;k<currentMesh->Vertices.size();++k)
    {
        QVector3D vertexNormal;
        HalfEdge* currentEdge;
        int m;
        float faceAngle;

        vertexNormal = QVector3D();
        currentEdge = currentMesh->Vertices[k].out;

        for (m=0; m<currentMesh->Vertices[k].val; m++) {
          faceAngle = acos( fmax(-1.0, QVector3D::dotProduct(
                                   (currentEdge->target->coords - currentMesh->Vertices[k].coords).normalized(),
                                   (currentEdge->prev->twin->target->coords - currentMesh->Vertices[k].coords).normalized() ) ) );
          if (currentEdge->polygon) {
            //For some reason currentEdge->polygon->normal is undefined at this point,
            //so therefore the built in computeVertexNormal doesn't return anything and it's easiest calculate the vertex normal like this
            vertexNormal += faceAngle * currentMesh->Faces[currentEdge->polygon->index].normal;
          }
          currentEdge = currentEdge->twin->next;
        }
        vertexNormalsQAS.append( vertexNormal );
    }

    // Update indices
    polyIndicesQAS.clear();

    polyIndicesQAS.reserve(currentMesh->HalfEdges.size());
    
    //We need to set control-net vertex indices here in the *correct* order
    for (unsigned int k=0; k<originalMesh->Faces.size(); k++)
    {
        //The way the halfedges are split when subdividing can be used to find the right vertices,
         HalfEdge* currentEdge = originalMesh->Faces[k].side;

         int e12 = currentMesh->HalfEdges[currentEdge->index * 2].target->index;
         int p1 = currentMesh->HalfEdges[currentEdge->index * 2 + 1].target->index;
         
         currentEdge = currentEdge->next;
         int e01 = currentMesh->HalfEdges[currentEdge->index * 2].target->index;
         int p0 = currentMesh->HalfEdges[currentEdge->index * 2 + 1].target->index;
         
         currentEdge = currentEdge->next;
         int e02 = currentMesh->HalfEdges[currentEdge->index * 2].target->index;
         int p2 = currentMesh->HalfEdges[currentEdge->index * 2 + 1].target->index;
         
         polyIndicesQAS.append(p0);
         polyIndicesQAS.append(e01);
         polyIndicesQAS.append(p1);
         polyIndicesQAS.append(e12);
         polyIndicesQAS.append(p2);
         polyIndicesQAS.append(e02);
     }

    qDebug() << "Putting " << polyIndicesQAS.size() << " indices in buffer to draw...";
    
    glBindVertexArray(vaoQAS);
    {
        glBindBuffer(GL_ARRAY_BUFFER, vboQASCoords);
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D) * vertexCoordsQAS.size(), vertexCoordsQAS.data(), GL_DYNAMIC_DRAW);
        }
        qDebug() << " → Updated meshCoordsBO " << vertexCoordsQAS.size();
        
        glBindBuffer(GL_ARRAY_BUFFER, vboQASNormals);
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D) * vertexNormalsQAS.size(), vertexNormalsQAS.data(), GL_DYNAMIC_DRAW);
        }
        qDebug() << " → Updated meshNormalsBO " << vertexNormalsQAS.size();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboQASIndices);
        {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polyIndicesQAS.size(), polyIndicesQAS.data(), GL_DYNAMIC_DRAW);
        }
        qDebug() << " → Updated meshIndexBO " << polyIndicesQAS.size();
    }
    glBindVertexArray(0);
    
    vboQASIndicesCount = polyIndicesQAS.size();
}

void MainView::updateVertexArrayObjectLoop()
{
    Mesh* currentMesh = &Meshes[currentSubdivSteps];
    
    qDebug() << ".. updateBuffers";
    
    // Update vertex coords
    vertexCoordsLoop.clear();
    vertexCoordsLoop.reserve(currentMesh->Vertices.size());
    for (int k=0;k<currentMesh->Vertices.size();++k)
    {
        vertexCoordsLoop.append(currentMesh->Vertices[k].coords);
    }
    
    // Update vertex normals
    vertexNormalsLoop.clear();
    vertexNormalsLoop.reserve(currentMesh->Vertices.size());
    for (int k=0;k<currentMesh->Faces.size();++k)
    {
        currentMesh->setFaceNormal(&currentMesh->Faces[k]);
    }
    for (int k=0;k<currentMesh->Vertices.size();++k)
    {
        vertexNormalsLoop.append( currentMesh->computeVertexNormal(&currentMesh->Vertices[k]) );
    }
    // Update indices
    polyIndicesLoop.clear();
    polyIndicesLoop.reserve(currentMesh->HalfEdges.size() + currentMesh->Faces.size());
    
    // Update polyindices
    for (int k=0;k<currentMesh->Faces.size();++k)
    {
        HalfEdge* currentEdge = currentMesh->Faces[k].side;
        
        for(int m=0;m<3;++m)
        {
            polyIndicesLoop.append(currentEdge->target->index);
            currentEdge = currentEdge->next;
        }
    }
    
    qDebug() << "Putting " << polyIndicesLoop.size() << " indices in buffer to draw...";
    
    glBindVertexArray(meshVAO);
    {
        glBindBuffer(GL_ARRAY_BUFFER, meshCoordsBO);
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D) * vertexCoordsLoop.size(), vertexCoordsLoop.data(), GL_DYNAMIC_DRAW);
        }
        qDebug() << " → Updated meshCoordsBO " << vertexCoordsLoop.size();
        
        glBindBuffer(GL_ARRAY_BUFFER, meshNormalsBO);
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D) * vertexNormalsLoop.size(), vertexNormalsLoop.data(), GL_DYNAMIC_DRAW);
        }
        qDebug() << " → Updated meshNormalsBO " << vertexNormalsLoop.size();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);
        {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * polyIndicesLoop.size(), polyIndicesLoop.data(), GL_DYNAMIC_DRAW);
        }
        qDebug() << " → Updated meshIndexBO " << polyIndicesLoop.size();
    }
    glBindVertexArray(0);
    
    meshIBOSize = polyIndicesLoop.size(); // TODO: Refractor mesh* to vbo/vaoLoop... (like QAS)
}

void MainView::updateMatrices()
{
    updateMatricesLoop();
    updateMatricesQAS();
    
    update();
}

void MainView::updateMatricesQAS()
{
    modelViewMatrixQAS.setToIdentity();
    modelViewMatrixQAS.translate(QVector3D(0.0, 0.0, -3.0));
    modelViewMatrixQAS.scale(QVector3D(1.0, 1.0, 1.0));
    modelViewMatrixQAS.rotate(rotAngle, QVector3D(0.0, 1.0, 0.0));
    
    projectionMatrixQAS.setToIdentity();
    
    if(viewMode == VIEW_MODE_LOOP_AND_QAS)
    {
        // Translate projectionMatrix to the right for a perspective independent side-by-side view
        projectionMatrixQAS.translate(QVector3D(0.5, 0.0, 0.0));
    }
    
    projectionMatrixQAS.perspective(FoV, dispRatio, 0.1, 1000.0);
    
    normalMatrixQAS = modelViewMatrixQAS.normalMatrix();
    
    uniformUpdateRequired = true;
}

void MainView::updateMatricesLoop()
{
    modelViewMatrix.setToIdentity();
    modelViewMatrix.translate(QVector3D(0.0, 0.0, -3.0));
    modelViewMatrix.scale(QVector3D(1.0, 1.0, 1.0));
    modelViewMatrix.rotate(rotAngle, QVector3D(0.0, 1.0, 0.0));
    
    projectionMatrix.setToIdentity();
    
    if(viewMode == VIEW_MODE_LOOP_AND_QAS)
    {
        // Translate projectionMatrix to the left for a perspective independent side-by-side view
        projectionMatrix.translate(QVector3D(-0.5, 0.0, 0.0));
    }
    
    projectionMatrix.perspective(FoV, dispRatio, 0.1, 1000.0);
    
    normalMatrix = modelViewMatrix.normalMatrix();
    
    uniformUpdateRequired = true;
}


// ---

void MainView::initializeGL()
{
    initializeOpenGLFunctions();
    qDebug() << ":: OpenGL initialized";
    
    debugLogger = new QOpenGLDebugLogger();
    connect( debugLogger, SIGNAL( messageLogged( QOpenGLDebugMessage ) ), this, SLOT( onMessageLogged( QOpenGLDebugMessage ) ), Qt::DirectConnection );
    
    if(debugLogger->initialize())
    {
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
    
    //set gl_patch_vertices to match control hexagon
    glPatchParameteri(GL_PATCH_VERTICES, 6);
    
    glEnable(GL_PRIMITIVE_RESTART);
    unsigned int maxInt = ((unsigned int) -1);
    glPrimitiveRestartIndex(maxInt);
    
    qDebug() << ".. initializing shader programs";
    createShaderProgramsLoop();
    createShaderProgramsQAS();
    
    qDebug() << ".. initializing shader uniform-locations";
    initUniformLocationsLoop();
    initUniformLocationsQAS();
    
    qDebug() << ".. creating buffers";
    createBuffersLoop();
    createBuffersQAS();
    
    updateMatrices();
}

void MainView::resizeGL(int newWidth, int newHeight)
{
    qDebug() << ".. resizeGL";
    
    dispRatio = (float) newWidth / newHeight;
    
    updateMatrices();
}

void MainView::paintGL()
{
    // Clean framebuffer
    glClearColor(239/255.0f, 235/255.0f, 231/255.0f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    
    if(modelLoaded)
    {
        
        // Draw QAS only if we want to
        if(viewMode == VIEW_MODE_LOOP_AND_QAS || viewMode == VIEW_MODE_QAS)
        {
            
            qasShaderProg.bind();
            {
                {
                    // Update model/view/projection matrices
                    
                    glUniformMatrix4fv(uniformsQAS["modelviewmatrix"], 1, false, modelViewMatrixQAS.data());
                    glUniformMatrix4fv(uniformsQAS["projectionmatrix"], 1, false, projectionMatrixQAS.data());
                    glUniformMatrix3fv(uniformsQAS["normalmatrix"], 1, false, normalMatrixQAS.data());
                    
                    glUniform1f(uniformsQAS["tess_level_inner"], tessellationLevel);
                    glUniform1f(uniformsQAS["tess_level_outer"], tessellationLevel);

                    glUniform1i(uniformsQAS["show_reflines"], show_ref_lines);
                    glUniform1i(uniformsQAS["refline_size"], ref_line_size);
                    
                    glUniform1i(uniformsQAS["adaptive_tessellation"], adaptiveTessellation ? 1 : 0);
                }
                
                // Render mesh
                glBindVertexArray(vaoQAS);
                {

                    // Apply wireframe/fill-mode
                    glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
                    
                    // Draw mesh buffer
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboQASIndices);
                    {
                        glDrawElements(GL_PATCHES, vboQASIndicesCount, GL_UNSIGNED_INT, 0);
                    }
                }
                glBindVertexArray(0);
            }
            qasShaderProg.release();

        }
        
        if(viewMode == VIEW_MODE_LOOP_AND_QAS || viewMode == VIEW_MODE_LOOP)
        {

            // Draw Loop Subdivision
            mainShaderProg->bind();
            {
                {
                    // Update model/view/projection matrices
                    glUniformMatrix4fv(uniModelViewMatrix, 1, false, modelViewMatrix.data());
                    glUniformMatrix4fv(uniProjectionMatrix, 1, false, projectionMatrix.data());
                    glUniformMatrix3fv(uniNormalMatrix, 1, false, normalMatrix.data());
                    
                    // Update isophote uniforms
                    glUniform1i(uniShowRefLines, show_ref_lines);
                    glUniform1i(uniRefLineSize, ref_line_size);
                }
                
                // Render mesh
                glBindVertexArray(meshVAO);
                {
                    // Apply wireframe/fill-mode
                    glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
                    
                    // Draw mesh buffer
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);
                    {
                        glDrawElements(GL_TRIANGLES, meshIBOSize, GL_UNSIGNED_INT, 0);
                    }
                }
                glBindVertexArray(0);
            }
            mainShaderProg->release();
        }

        uniformUpdateRequired = false;
    }
}

void MainView::mousePressEvent(QMouseEvent* event)
{
    setFocus();

    // Enable tracking for use in mouseMoveEvent
    setMouseTracking(true);

    drag = 1;
    grabX = event->x();
    grabY = event->y();
    grabRotAngle = rotAngle;

}

void MainView::mouseReleaseEvent(QMouseEvent *event)
{
    // Disable tracking
    setMouseTracking(false);
    drag = 0;
}

void MainView::mouseMoveEvent(QMouseEvent *event)
{
    // Move the model around its Y-axis
    if(drag)
    {
        int dx = grabX - event->x();
        int dy = grabY - event->y();
        
        rotAngle = grabRotAngle + dx * 0.5f; // 2px is 1deg
        
        updateMatrices();
    }
}

void MainView::wheelEvent(QWheelEvent* event) {
  FoV -= event->delta() / 60.0;
  updateMatrices();
}

void MainView::keyPressEvent(QKeyEvent* event) {
  switch(event->key()) {
  case 'Z':
    wireframeMode = !wireframeMode;
    update();
    break;
  }
}

// ---

void MainView::onMessageLogged( QOpenGLDebugMessage Message ) {
  qDebug() << " → Log:" << Message;
}
