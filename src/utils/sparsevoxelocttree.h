#ifndef SPARSEVOXELOCTTREE_H
#define SPARSEVOXELOCTTREE_H

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <QDataStream>
#include <QVector>
#include <QString>

#include "miniarray.h"
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

        void save(QString path);
        void open(QString path);

        MiniArray<double> * getExtent();
        MiniArray<double> * getDataHistogram();
        MiniArray<double> * getDataHistogramLog();
        MiniArray<double> * getMinMax();

        unsigned int getLevels();
        unsigned int getBrickPoolPower();
        unsigned int getBrickInnerDimension();
        unsigned int getBrickOuterDimension();
        unsigned int getBrickNumber();
        void print();

        quint64 getBytes();

        MiniArray<unsigned int> index;
        MiniArray<unsigned int> brick;
        MiniArray<float> pool;

    private:
        MiniArray<double> data_histogram;
        MiniArray<double> data_histogram_log;
        MiniArray<double> minmax;
        MiniArray<double> extent;

        quint64 version_major;
        quint64 version_minor;

        quint64 filesize;
        quint64 levels;
        quint64 brick_pool_power;
        quint64 brick_inner_dimension;
        quint64 brick_outer_dimension;
};
#endif
