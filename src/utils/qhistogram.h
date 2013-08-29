#ifndef QHISTOGRAM_H
#define QHISTOGRAM_H

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <cmath>
#include <cerrno>
#include <vector>
#include <limits>

#include <QDebug>
#include <QObject>
#include <QWidget>
#include <QPainter>
#include <QString>
#include <QStyleOption>
#include <QStyle>
#include <QVarLengthArray>
#include <QVector>
#include <QList>

#include "miniarray.h"
#include "file.h"

class QHistogram : public QWidget
{
    Q_OBJECT
    private:
        MiniArray<double> bins;
        QPolygon myPolygon;
        float lim_low_x;
        float lim_high_x;
        float lim_low_y;
        float lim_high_y;
        bool log_x;
        bool log_y;
        QString label;
        
    public:
        //~ QHistogram();
        
        void historize(QString label, MiniArray<float> * data, int n_bins, int stride, int offset, float lim_low_x, float lim_high_x, float lim_low_y, float lim_high_y, bool auto_lim_low_x,  bool auto_lim_high_x, bool auto_lim_low_y,  bool auto_lim_high_y, bool log_x, bool log_y);
        
        void historize(QString label, MiniArray<float> *  data_x, MiniArray<float> *  data_y, int n_bins, int stride_x, int stride_y, int offset_x, int offset_y, float lim_low_x, float lim_high_x, float lim_low_y, float lim_high_y, bool auto_lim_low_x,  bool auto_lim_high_x, bool auto_lim_low_y,  bool auto_lim_high_y, bool log_x, bool log_y);
        
        void historize(QString label, QList<file> data, int n_bins, float lim_low_x, float lim_high_x, float lim_low_y, float lim_high_y, bool auto_lim_low_x,  bool auto_lim_high_x, bool auto_lim_low_y,  bool auto_lim_high_y, bool log_x, bool log_y);
        
    protected:
        void paintEvent(QPaintEvent*);
};
#endif
