#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QThread>
#include "guiupdater.h"
#include "objfile.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {

  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  void loadOBJ();
  void updateStats();

private slots:
  void on_qasTessMinSlider_valueChanged(int value);
  void on_qasTessMaxSlider_valueChanged(int value);
  void on_zoomTessSlider_valueChanged(double value);
  void on_curvTessSlider_valueChanged(double value);
  void on_normTessSlider_valueChanged(double value);
  void on_normalCurvatureBox_toggled(bool checked);
  void on_RotateDial_valueChanged(int value);
  void on_RotateDial_QAS_valueChanged(int value);
  void on_SubdivSteps_valueChanged(int value);
  void on_set_ref_line_size_valueChanged(int value);
  void on_LoadOBJ_clicked();
  void on_toggle_wf_mode_toggled(bool checked);
  void on_toggle_ref_lines_toggled(bool checked);
  void on_viewModeBox_currentIndexChanged(int value);
  void on_tessellationLevelSlider_valueChanged(int value);
  void on_qasSubdivOffsetBox_valueChanged(int value);
  void on_adaptiveTessellationBox_toggled(bool checked);
  void on_backgroundBox_currentIndexChanged(int value);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
