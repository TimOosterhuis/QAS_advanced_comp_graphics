#ifndef GUIUPDATER_H
#define GUIUPDATER_H

#include <QOpenGLDebugLogger>
#include "mainwindow.h"

class GUIUpdater : public QObject {
    Q_OBJECT
    
public:
    GUIUpdater(QMainWindow* w);
    ~GUIUpdater();
    
public slots:
    void process();
    
signals:
    void finished();
    void error(QString err);
    
private:
    // variables
    const static int INTERVAL_MS = 200;
    QMainWindow* window;
};

#endif // GUIUPDATER_H
