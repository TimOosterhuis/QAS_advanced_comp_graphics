#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  qDebug() << "✓✓ MainWindow constructor";
  ui->setupUi(this);
  ui->toggle_wf_mode->setChecked(true);
  ui->toggle_ref_lines->setChecked(false);
}

MainWindow::~MainWindow() {
  qDebug() << "✗✗ MainWindow destructor";
  delete ui;

  Meshes.clear();
  Meshes.squeeze();
}

void MainWindow::loadOBJ() {
  OBJFile newModel = OBJFile(QFileDialog::getOpenFileName(this, "Import OBJ File", "models/", tr("Obj Files (*.obj)")));
  Meshes.clear();
  Meshes.squeeze();
  Meshes.append( Mesh(&newModel) );

  ui->MainDisplay->updateMeshBuffers( &Meshes[0] );
  ui->MainDisplay->modelLoaded = true;

  ui->MainDisplay->update();
}

void MainWindow::on_RotateDial_valueChanged(int value) {
  ui->MainDisplay->rotAngle = value;
  ui->MainDisplay->updateMatrices();
}

void MainWindow::on_set_ref_line_size_valueChanged(int value) {
  ui->MainDisplay->ref_line_size = value;
  ui->MainDisplay->update();
}

void MainWindow::on_toggle_wf_mode_toggled(bool checked) {
  ui->MainDisplay->wireframeMode = checked;
  ui->MainDisplay->update();
}

void MainWindow::on_toggle_ref_lines_toggled(bool checked) {
  ui->MainDisplay->show_ref_lines = checked;
  ui->MainDisplay->update();
}

//void MainWindow::on_use_qas_toggled(bool checked) {
//    Mesh subdividedMesh, limitMesh;
//    if (checked) {
//        subdivideLoop(&Meshes[0], &subdividedMesh);
//        toLimit(&subdividedMesh, &limitMesh);
//        ui->MainDisplay->updateMeshBuffers( &limitMesh);
//    } else {
//        ui->MainDisplay->updateMeshBuffers( &Meshes[ui->SubdivSteps->value()]);
//    }
//    ui->MainDisplay->update();
//}

void MainWindow::on_SubdivSteps_valueChanged(int value) {
  unsigned short k;

  for (k=Meshes.size(); k<value+1; k++) {
    Meshes.append(Mesh());
    subdivideLoop(&Meshes[k-1], &Meshes[k]);
  }

  ui->MainDisplay->updateMeshBuffers( &Meshes[value] );
}

void MainWindow::on_LoadOBJ_clicked() {
  loadOBJ();
  ui->LoadOBJ->setEnabled(false);
  ui->SubdivSteps->setEnabled(true);
}
