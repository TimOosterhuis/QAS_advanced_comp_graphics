#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLDebugLogger>

#include <QOpenGLShaderProgram>

#include <QMouseEvent>
#include "objfile.h"
#include "mesh.h"
#include "meshtools.h"
#include <unordered_map>
#include <cmath>

class MainView : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core {

  Q_OBJECT

public:
  MainView(QWidget *Parent = 0);
  ~MainView();

  static const int VIEW_MODE_LOOP = 0;
  static const int VIEW_MODE_QAS = 1;
  static const int VIEW_MODE_LOOP_AND_QAS = 2;
  
  float BACKGROUND_TRANSPARENT[4] = {239/255.0f, 235/255.0f, 231/255.0f, 1.0f};
  float BACKGROUND_BLACK[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float BACKGROUND_DARK_GRAY[4] = {64/255.0f, 64/255.0f, 64/255.0f, 1.0f};
  float BACKGROUND_WHITE[4] = {255/255.0f, 255/255.0f, 255/255.0f, 1.0f};
  
  const int MAX_SUBDIV_STEPS = 4; // or 5 maybe
  
  QVector<Mesh> Meshes;
  QVector<Mesh> QAS_Meshes;
  
  GLuint qasPolygonCountValue = 0;
  GLuint loopPolygonCountValue = 0;
  
  float backgroundColor[4];
  
  int curvatureMode = 0;
  
  float zoomTess = 1.0;
  float curvTess = 1.0;
  float normTess = 1.0;
  
  int qasSubdivOffset = 0;
  bool adaptiveTessellation = false;
  
  int tessMinValue = 1;
  int tessMaxValue = 64;
  int tessellationLevel = 1;
  int currentSubdivSteps = 0;
  int viewMode = 0;
  
  bool modelLoaded;
  bool wireframeMode;

  float FoV;
  float dispRatio;
  float rotAngle;
  
  int drag;
  int grabX;
  int grabY;
  float grabRotAngle;

  int show_ref_lines;
  int ref_line_size;

  bool uniformUpdateRequired;

  void updateQASMesh();
  void loadModelFromOBJFile(OBJFile model);
  void updateMatrices();
  void updateMatricesLoop();
  void updateMatricesQAS();
  //void updateMeshBuffers();//Mesh* currentMesh);
  void updateVertexArrayObjectQAS();
  void updateVertexArrayObjectLoop();


protected:
  void initializeGL();
  void resizeGL(int newWidth, int newHeight);
  void paintGL();

  void renderMesh();

  void mouseReleaseEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);
  void keyPressEvent(QKeyEvent* event);

private:
  QOpenGLDebugLogger* debugLogger;

  QMatrix4x4 modelViewMatrixQAS;
  QMatrix4x4 projectionMatrixQAS;
  QMatrix3x3 normalMatrixQAS;
  
  QMatrix4x4 modelViewMatrix;
  QMatrix4x4 projectionMatrix;
  QMatrix3x3 normalMatrix;

  // QAS variables
  QOpenGLShaderProgram qasShaderProg;
  GLuint vaoQAS;
  GLuint vboQASCoords;
  GLuint vboQASNormals;
  GLuint vboQASIndices;
  int vboQASIndicesCount;
  std::unordered_map<std::string, GLint> uniformsQAS;
  
  // QAS query
  GLuint pgQuery;
  
  // Uniforms
  GLint uniModelViewMatrix, uniProjectionMatrix, uniNormalMatrix;
  GLint uniShowRefLines, uniRefLineSize;

  // ---

  QOpenGLShaderProgram* mainShaderProg;

  GLuint meshVAO, meshCoordsBO, meshNormalsBO, meshIndexBO;
  unsigned int meshIBOSize;

  // ---
  bool pointSelected;
  int selectedPoint;
  void rayTrace(QVector4D ray);

  QVector<QVector3D> vertexCoordsLoop;
  QVector<QVector3D> vertexNormalsLoop;
  QVector<unsigned int> polyIndicesLoop;
  
  QVector<QVector3D> vertexCoordsQAS;
  QVector<QVector3D> vertexNormalsQAS;
  QVector<unsigned int> polyIndicesQAS;

  void createShaderProgramsLoop();
  void createShaderProgramsQAS();
  void initUniformLocationsLoop();
  void initUniformLocationsQAS();
  void createBuffersLoop();
  void createBuffersQAS();
  bool shaderSettingsChanged();

private slots:
  void onMessageLogged( QOpenGLDebugMessage Message );

};

#endif // MAINVIEW_H
