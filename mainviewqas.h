#ifndef MAINVIEWQAS_H
#define MAINVIEWQAS_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLDebugLogger>

#include <QOpenGLShaderProgram>

#include <QMouseEvent>
#include "mesh.h"

class MainViewQAS : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core {

  Q_OBJECT

public:
  MainViewQAS(QWidget *Parent = 0);
  ~MainViewQAS();

  bool modelLoaded;
  bool wireframeMode;

  float FoV;
  float dispRatio;
  float rotAngle;

  int show_ref_lines;
  int ref_line_size;

  bool uniformUpdateRequired;

  void updateMatrices();
  void updateUniforms();
  void updateMeshBuffers(Mesh* currentMesh);
  void updateMeshBuffers(Mesh* currentMesh, Mesh* edgePointMesh);


protected:
  void initializeGL();
  void resizeGL(int newWidth, int newHeight);
  void paintGL();

  void renderMesh();

  void mousePressEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);
  void keyPressEvent(QKeyEvent* event);

private:
  QOpenGLDebugLogger* debugLogger;

  QMatrix4x4 modelViewMatrix, projectionMatrix;
  QMatrix3x3 normalMatrix;

  // Uniforms
  GLint uniModelViewMatrix, uniProjectionMatrix, uniNormalMatrix;
  GLint uniShowRefLines, uniRefLineSize;

  GLint uniModelViewMatrixMain, uniProjectionMatrixMain, uniNormalMatrixMain;
  GLint uniShowRefLinesMain, uniRefLineSizeMain;

  // ---

  QOpenGLShaderProgram* mainShaderProg, qasShaderProg;

  GLuint meshVAO, meshCoordsBO, meshNormalsBO, meshIndexBO;
  unsigned int meshIBOSize;

  // ---
  bool pointSelected;
  int selectedPoint;
  void rayTrace(QVector4D ray);

  QVector<QVector3D> vertexCoords;
  QVector<QVector3D> vertexNormals;
  QVector<unsigned int> polyIndices;

  void createShaderPrograms();
  void createBuffers();
  bool shaderSettingsChanged();
  bool show_ref_lines_old;
  bool ref_line_size_old;

private slots:
  void onMessageLogged( QOpenGLDebugMessage Message );

};

#endif // MAINVIEWQAS_H
