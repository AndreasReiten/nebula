#ifndef SPARSEVOXELOCTTREE_H
#define SPARSEVOXELOCTTREE_H

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
        void save(QString path, QString note = QString("My dataset"), int compression = 4);
        void open(QString path);

        unsigned int getLevels();
        unsigned int getBrickPoolPower();
        unsigned int getBrickInnerDimension();
        unsigned int getBrickOuterDimension();
        unsigned int getBrickNumber();
        size_t getBytes();

        MiniArray<unsigned int> index;
        MiniArray<unsigned int> brick;
        MiniArray<float> pool;

    private:
        unsigned int levels;
        unsigned int brick_pool_power;
        unsigned int brick_inner_dimension;
        unsigned int brick_outer_dimension;
};
#endif
