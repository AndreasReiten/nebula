#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

/*
 * This class can open a file from another thread and display its contents as a texture using QOpenGLPainter or raw GL calls
 * */

#include <QObject>
#include "openglwindow.h"


class ImagePreview : public OpenGLWorker
{
    Q_OBJECT
public:
    explicit ImagePreview(QObject *parent = 0);
    
signals:
//    setImage();
public slots:
    
};

#endif // IMAGEPREVIEW_H
