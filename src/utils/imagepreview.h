#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

/*
 * This class can open a file from another thread and display its contents as a texture using QOpenGLPainter or raw GL calls
 * */

#include <QObject>
#include "openglwindow.h"


//class ImagePreview : public OpenGLWorker
//{
//    Q_OBJECT
//public:
//    explicit ImagePreview(QObject *parent = 0);
//    int setImageFromPath(QString path);
        
//signals:
    
//public slots:
//    void setMode(int value);
    
//    void metaMouseMoveEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
//    void metaMousePressEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
//    void metaMouseReleaseEvent(int x, int y, int left_button, int mid_button, int right_button, int ctrl_button, int shift_button);
//    void wheelEvent(QWheelEvent* ev);
//    void resizeEvent(QResizeEvent * ev);
    
//private:
    
//};

//class ImagePreviewWindow : public OpenGLWindow
//{
//    Q_OBJECT

//public:
//    ImagePreviewWindow();
//    ~ImagePreviewWindow();

//    void setSharedWindow(SharedContextWindow * window);
//    VolumeRenderWorker *getWorker();

//    void initializeWorker();

//public slots:
//    void renderNow();

//private:
//    bool isInitialized;

//    SharedContextWindow * shared_window;
//    VolumeRenderWorker * gl_worker;
//};

#endif // IMAGEPREVIEW_H
