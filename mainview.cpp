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
    pointSelected = false;
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
    toLimit(&Meshes[1], &QAS_Meshes[0]);
    
    updateVertexArrayObjectQAS();
    
    
    // Finished loading model
    updateMatrices();
    modelLoaded = true;
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
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);
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
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO); -> why bind this?
    }
    glBindVertexArray(0);
}

void MainView::updateVertexArrayObjectQAS()
{
    Mesh* currentMesh = &QAS_Meshes[0];
    
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
        vertexNormalsQAS.append( currentMesh->computeVertexNormal(&currentMesh->Vertices[k]) );
    }
    
    // Update indices
    polyIndicesQAS.clear();
    polyIndicesQAS.reserve(currentMesh->HalfEdges.size() + currentMesh->Faces.size());
    
    // if useQAS, then we need to set control-net indices here in the *correct* order, and set GL_PATCH_VERTICES to 6 (and also in tessctrlshader set vertices = 6)
    // So idea: let's use the flag we set in Vertex.flag to check if we have an edge-point or not
    // And then we only need to get a NON-edge-point vertex, and get a triangle with other non-edge points, and catch the edge-points in between
    // Then put the indices of those vertices in the polyIndicesQAS.
    // We can either do v0 e0 v1 e1 v2 e2 or v0 v1 v2 e0 e1 e2, whatever is easier for using in the shader
    for (int k=0;k<currentMesh->Faces.size();++k)
    {
        HalfEdge* currentEdge = currentMesh->Faces[k].side;
        
        for(int m=0;m<3;++m)
        {
            polyIndicesQAS.append(currentEdge->target->index);
            currentEdge = currentEdge->next;
        }
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
    
    // if useQAS, then we need to set control-net indices here in the *correct* order, and set GL_PATCH_VERTICES to 6 (and also in tessctrlshader set vertices = 6)
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
    
    // TODO: Later we should load two buffers, one for Loop subdiv and one for QAS.
    // That way we can show in the Render-step both at the same time side-by-side.
    // There should be a slider to set the X-offset of both models in opposite directions,
    // so that the redendered models can be compared in overlay and next to each other.
    // The value of the slider should affect the modelview matrix uniform.
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
    
    update(); // this shouldn't be called here, but rather in the calling function
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
    
    glPatchParameteri(GL_PATCH_VERTICES, 3); // triangles: 3, quads: 4, ...
    // For reference: max tessellation level: GL_MAX_TESS_GEN_LEVEL, we need this for tessellation level slider
    
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
        // We can also draw both ontop of each other, and then set opacity with slider (QAS.opacity = 1 - Loop.opacity), so we can fade one into the other
        // Opacity then is a uniform we change on the fly
        
        // Draw QAS only if we want to
        if(viewMode == VIEW_MODE_LOOP_AND_QAS || viewMode == VIEW_MODE_QAS)
        {
            // qDebug() << "QAS render pass";
            
            qasShaderProg.bind();
            {
                //if(uniformUpdateRequired)
                {
                    // Update model/view/projection matrices
                    
                    glUniformMatrix4fv(uniformsQAS["modelviewmatrix"], 1, false, modelViewMatrixQAS.data());
                    glUniformMatrix4fv(uniformsQAS["projectionmatrix"], 1, false, projectionMatrixQAS.data());
                    glUniformMatrix3fv(uniformsQAS["normalmatrix"], 1, false, normalMatrixQAS.data());
                    
                    glUniform1f(uniformsQAS["tess_level_inner"], tessellationLevel);
                    glUniform1f(uniformsQAS["tess_level_outer"], tessellationLevel);
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
                    
                    // Draw the selected control point if it exists
                    if(pointSelected)
                    {
                        // Maybe it's better to specify a uniform that says if it is selected or not
                        // And then we can select any of the vectors, that is, just a uniform to override colors
                        // And then just use the same glDrawElements again here after changing that uniform
                    }
                }
                glBindVertexArray(0);
            }
            qasShaderProg.release();
        }
        
        if(viewMode == VIEW_MODE_LOOP_AND_QAS || viewMode == VIEW_MODE_LOOP)
        {
            // qDebug() << "Loop render pass";
            
            // Draw Loop Subdivision
            mainShaderProg->bind();
            {
                //if(uniformUpdateRequired) what is the performance gain here really, this can only give confusion, let's save optimizations for when we're done
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
                    
                    // Draw the selected control point if it exists
                    if(pointSelected)
                    {
                        glPointSize(12.0);
                        glDrawArrays(GL_POINTS, selectedPoint, 1);
                    }
                }
                glBindVertexArray(0);
            }
            mainShaderProg->release();
        }
        
        
        uniformUpdateRequired = false;
    }
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
        // rotAngle is actually rotAngleY, we can add rotAngleZ here too..
        // and then we should allocate x,y based on current rotation values to always rotate around axes MOST perpendicular to camera
        // but one rotational axis is good enough for now
        
        updateMatrices();
    }
}

//on a mouse click, try to determine the nearest vertex
void MainView::mousePressEvent(QMouseEvent* event)
{
    setFocus();
    
    // Enable tracking for use in mouseMoveEvent
    setMouseTracking(true);
    
    drag = 1;
    grabX = event->x();
    grabY = event->y();
    grabRotAngle = rotAngle;
    
    
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

void MainView::rayTrace(QVector4D ray) {
    float min_dist = 1;
    //calculate the ray origin (camera position) in world/object space
    QVector4D camera_pos = modelViewMatrix.inverted() * QVector4D(0.0,0.0,0.0,1.0);
    //cast a ray for every vertex
    for (int k = 0; k < vertexCoordsLoop.size(); k++) {
        //solve the equation between the ray and the plane obtained by the vertex coordinates and it's normalvector
        QVector4D V0 = QVector4D(vertexCoordsLoop[k], 1.0) - camera_pos;
        float dotV0Normal = V0.x() * vertexNormalsLoop[k].x() + V0.y() * vertexNormalsLoop[k].y() + V0.z() * vertexNormalsLoop[k].z();
        float dotRayNormal = ray.x() * vertexNormalsLoop[k].x() + ray.y() * vertexNormalsLoop[k].y() + ray.z() * vertexNormalsLoop[k].z();
        float t = dotV0Normal/dotRayNormal;

        QVector4D intersect = camera_pos + (t * ray);

        //calculate the distance between the ray plane intersection point vertex
        float dist = (QVector4D(vertexCoordsLoop[k], 1.0) - intersect).length();

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
    pointSelected = min_dist < 0.2;
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
