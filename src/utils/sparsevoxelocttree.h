#ifndef SPARSEVOXELOCTTREE_H
#define SPARSEVOXELOCTTREE_H

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <QDataStream>
#include <QVector>
#include <QString>
#include <QDebug>

//#include "miniarray.h"
#include "tools.h"

class SparseVoxelOcttree
{
    /* This class represents Sparse Voxel Matrix. It is the datastructure that is used by the OpenCL raytracer */

    public:
        SparseVoxelOcttree();
        ~SparseVoxelOcttree();

        void set(unsigned int levels = 0, unsigned int brick_inner_dimension = 7, unsigned int brick_outer_dimension = 8, unsigned int brick_pool_power = 7);
        void setLevels(int value);
        void setExtent(float Q);
        void setMax(float value);
        void setMin(float value);
        
        void save(QString path);
        void open(QString path);

        Matrix<double> * getExtent();
        Matrix<double> * getDataHistogram();
        Matrix<double> * getDataHistogramLog();
        Matrix<double> *getMinMax();

        unsigned int getLevels();
        unsigned int getBrickPoolPower();
        unsigned int getBrickInnerDimension();
        unsigned int getBrickOuterDimension();
        unsigned int getBrickNumber();
        void print();

        quint64 getBytes();

        Matrix<unsigned int> index;
        Matrix<unsigned int> brick;
        Matrix<float> pool;

    private:
        Matrix<double> data_histogram;
        Matrix<double> data_histogram_log;
        Matrix<double> minmax;
        Matrix<double> extent;

        quint64 version_major;
        quint64 version_minor;

        quint64 filesize;
        quint64 levels;
        quint64 brick_pool_power;
        quint64 brick_inner_dimension;
        quint64 brick_outer_dimension;
        
        float max, min;
};
#endif
