#ifndef SPARSEVOXELOCTTREE_H
#define SPARSEVOXELOCTTREE_H

#ifdef _win32
#include "HDF/hdf5.h"
#endif
#ifdef __linux__
#include "hdf5.h"
#endif

#include <QString>

#include "miniarray.h"

class SparseVoxelOcttree
{
    /* This class represents Sparse Voxel Matrix. It is the datastructure that is used by the OpenCL raytracer */

    public:
        SparseVoxelOcttree();
        ~SparseVoxelOcttree();

        void set(unsigned int levels = 0, unsigned int brick_inner_dimension = 7, unsigned int brick_outer_dimension = 8, unsigned int brick_pool_power = 7);
        void setLevels(int value);
        void setExtent(float Q);

        void save(QString path, int compression = 0);
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

        size_t getBytes();

        MiniArray<unsigned int> index;
        MiniArray<unsigned int> brick;
        MiniArray<float> pool;

    private:
        MiniArray<double> data_histogram;
        MiniArray<double> data_histogram_log;
        MiniArray<double> minmax;
        MiniArray<double> extent;

        size_t version_major;
        size_t version_minor;

        size_t filesize;
        size_t levels;
        size_t brick_pool_power;
        size_t brick_inner_dimension;
        size_t brick_outer_dimension;
};
#endif
