#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  qDebug() << "✓✓ MainWindow constructor";
  ui->setupUi(this);
  ui->toggle_wf_mode->setChecked(ui->MainDisplay->wireframeMode);
  ui->toggle_ref_lines->setChecked(ui->MainDisplay->show_ref_lines);
  ui->viewModeBox->setCurrentIndex(ui->MainDisplay->viewMode);
  ui->tessellationLevelSlider->setValue(ui->MainDisplay->tessellationLevel);
  ui->qasSubdivOffsetBox->setValue(ui->MainDisplay->qasSubdivOffset);
  ui->adaptiveTessellationBox->setChecked(ui->MainDisplay->adaptiveTessellation);
}

MainWindow::~MainWindow()
{
    qDebug() << "✗✗ MainWindow destructor";
    delete ui;
}

void MainWindow::on_adaptiveTessellationBox_toggled(bool checked)
{
    ui->MainDisplay->adaptiveTessellation = checked;
    ui->MainDisplay->update();
}

void MainWindow::loadOBJ()
{
    OBJFile newModel = OBJFile(QFileDialog::getOpenFileName(this, "Import OBJ File", "models/", tr("Obj Files (*.obj)")));
    ui->MainDisplay->loadModelFromOBJFile(newModel);
}

void MainWindow::on_qasSubdivOffsetBox_valueChanged(int value)
{
    ui->MainDisplay->qasSubdivOffset = value;
    ui->MainDisplay->updateQASMesh();
}

void MainWindow::on_tessellationLevelSlider_valueChanged(int value)
{
    ui->MainDisplay->tessellationLevel = value;
    ui->MainDisplay->update();
}

void MainWindow::on_viewModeBox_currentIndexChanged(int value)
{
    ui->MainDisplay->viewMode = value;
    ui->MainDisplay->updateMatrices();
    ui->MainDisplay->update(); // because updateMatrices should not call update()
}

void MainWindow::on_RotateDial_valueChanged(int value) {
    
    // You might wonder, where did this go. Don't worry, I'll implement mousemove handler to rotate the model using a mouse instead.
    
  //ui->MainDisplay->rotAngle = value;
  //ui->MainDisplay->updateMatrices();
}

void MainWindow::on_RotateDial_QAS_valueChanged(int value) {
  //ui->QASDisplay->rotAngle = value;
  //ui->QASDisplay->updateMatrices();
}

void MainWindow::on_set_ref_line_size_valueChanged(int value) {
  ui->MainDisplay->ref_line_size = value;
  ui->MainDisplay->uniformUpdateRequired = true;
  ui->MainDisplay->update();
}

void MainWindow::on_toggle_wf_mode_toggled(bool checked) {
  ui->MainDisplay->wireframeMode = checked;
  ui->MainDisplay->update();
}

void MainWindow::on_toggle_ref_lines_toggled(bool checked) {
  ui->MainDisplay->show_ref_lines = checked;
  ui->MainDisplay->uniformUpdateRequired = true;
  ui->MainDisplay->update();
}

void MainWindow::on_SubdivSteps_valueChanged(int value)
{
    ui->MainDisplay->currentSubdivSteps = ui->MainDisplay->MAX_SUBDIV_STEPS > value ? value : ui->MainDisplay->MAX_SUBDIV_STEPS;
    ui->SubdivSteps->setValue(ui->MainDisplay->currentSubdivSteps);
    ui->MainDisplay->updateVertexArrayObjectLoop();
    ui->MainDisplay->update();
}

void MainWindow::on_LoadOBJ_clicked() {
  loadOBJ();
  ui->LoadOBJ->setEnabled(false);
  ui->SubdivSteps->setEnabled(true);
}
