#include "guiupdater.h"

GUIUpdater::GUIUpdater(QMainWindow* window)
{
    this->window = window;
}

GUIUpdater::~GUIUpdater()
{
    
}

void GUIUpdater::process()
{
    qDebug() << "Updating GUI...";
    
    while(true)//((MainWindow*) window)->isVisible())
    {
        ((MainWindow*) window)->updateStats();
        
        QThread::msleep(INTERVAL_MS);
    }
    
    qDebug() << "Window not visible anymore, stopping with GUI updates.";
    
    emit finished();
}
