#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
  qDebug() << "✓✓ MainWindow constructor";
  ui->setupUi(this);
  ui->toggle_wf_mode->setChecked(ui->MainDisplay->wireframeMode);
  ui->toggle_ref_lines->setChecked(ui->MainDisplay->show_ref_lines);
  ui->viewModeBox->setCurrentIndex(ui->MainDisplay->viewMode);
  ui->tessellationLevelSlider->setValue(ui->MainDisplay->tessellationLevel);
  ui->qasSubdivOffsetBox->setValue(ui->MainDisplay->qasSubdivOffset);
  ui->adaptiveTessellationBox->setChecked(ui->MainDisplay->adaptiveTessellation);
  ui->normalCurvatureBox->setChecked(ui->MainDisplay->curvatureMode == 1);
  ui->zoomTessSlider->setValue(ui->MainDisplay->zoomTess);
  ui->curvTessSlider->setValue(ui->MainDisplay->curvTess);
  ui->normTessSlider->setValue(ui->MainDisplay->normTess);
  
  // Start thread
  QThread* thread = new QThread;
  GUIUpdater* updater = new GUIUpdater(this);
  updater->moveToThread(thread);
  connect(updater, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
  connect(thread, SIGNAL(started()), updater, SLOT(process()));
  connect(updater, SIGNAL(finished()), thread, SLOT(quit()));
  connect(updater, SIGNAL(finished()), updater, SLOT(deleteLater()));
  connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
  thread->start();
}

MainWindow::~MainWindow()
{
    qDebug() << "✗✗ MainWindow destructor";
    delete ui;
}

void MainWindow::updateStats()
{
    ui->qasPolygonCountLabel->setText(QString::number(ui->MainDisplay->qasPolygonCountValue));
    ui->loopPolygonCountLabel->setText(QString::number(ui->MainDisplay->loopPolygonCountValue));
}

void MainWindow::on_qasTessMinSlider_valueChanged(int value)
{
    ui->MainDisplay->tessMinValue = value;
    ui->MainDisplay->update();
}

void MainWindow::on_qasTessMaxSlider_valueChanged(int value)
{
    ui->MainDisplay->tessMaxValue = value;
    ui->MainDisplay->update();
}

void MainWindow::on_zoomTessSlider_valueChanged(double value)
{
    ui->MainDisplay->zoomTess = value;
    ui->MainDisplay->update();
}

void MainWindow::on_curvTessSlider_valueChanged(double value)
{
    ui->MainDisplay->curvTess = value;
    ui->MainDisplay->update();
}

void MainWindow::on_normTessSlider_valueChanged(double value)
{
    ui->MainDisplay->normTess = value;
    ui->MainDisplay->update();
}

void MainWindow::on_normalCurvatureBox_toggled(bool checked)
{
    ui->MainDisplay->curvatureMode = checked ? 1 : 0;
    ui->MainDisplay->update();
}

void MainWindow::on_backgroundBox_currentIndexChanged(int value)
{
    qDebug() << "Updating background color...";
    if(value == 0)
    {
        ui->MainDisplay->backgroundColor[0] = ui->MainDisplay->BACKGROUND_TRANSPARENT[0];
        ui->MainDisplay->backgroundColor[1] = ui->MainDisplay->BACKGROUND_TRANSPARENT[1];
        ui->MainDisplay->backgroundColor[2] = ui->MainDisplay->BACKGROUND_TRANSPARENT[2];
        ui->MainDisplay->backgroundColor[3] = ui->MainDisplay->BACKGROUND_TRANSPARENT[3];
    }
    else if(value == 1)
    {
        ui->MainDisplay->backgroundColor[0] = ui->MainDisplay->BACKGROUND_BLACK[0];
        ui->MainDisplay->backgroundColor[1] = ui->MainDisplay->BACKGROUND_BLACK[1];
        ui->MainDisplay->backgroundColor[2] = ui->MainDisplay->BACKGROUND_BLACK[2];
        ui->MainDisplay->backgroundColor[3] = ui->MainDisplay->BACKGROUND_BLACK[3];
    }
    else if(value == 2)
    {
        ui->MainDisplay->backgroundColor[0] = ui->MainDisplay->BACKGROUND_DARK_GRAY[0];
        ui->MainDisplay->backgroundColor[1] = ui->MainDisplay->BACKGROUND_DARK_GRAY[1];
        ui->MainDisplay->backgroundColor[2] = ui->MainDisplay->BACKGROUND_DARK_GRAY[2];
        ui->MainDisplay->backgroundColor[3] = ui->MainDisplay->BACKGROUND_DARK_GRAY[3];
        //std::copy(ui->MainDisplay->BACKGROUND_DARK_GRAY, ui->MainDisplay->BACKGROUND_DARK_GRAY+4, ui->MainDisplay->backgroundColor);
    }
    else if(value == 3)
    {
        ui->MainDisplay->backgroundColor[0] = ui->MainDisplay->BACKGROUND_WHITE[0];
        ui->MainDisplay->backgroundColor[1] = ui->MainDisplay->BACKGROUND_WHITE[1];
        ui->MainDisplay->backgroundColor[2] = ui->MainDisplay->BACKGROUND_WHITE[2];
        ui->MainDisplay->backgroundColor[3] = ui->MainDisplay->BACKGROUND_WHITE[3];
    }
    ui->MainDisplay->update();
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
