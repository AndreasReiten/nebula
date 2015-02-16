#ifndef SPARSEVOXELOCTTREE_H
#define SPARSEVOXELOCTTREE_H

#include <QString>
#include <QStringList>
#include <QDateTime>

#include "../../math/qxmathlib.h"

class SparseVoxelOctree
{
    /* This class represents Sparse Voxel Matrix. It is the datastructure that is used by the OpenCL raytracer */

    public:
        SparseVoxelOctree();
        ~SparseVoxelOctree();

        void set(unsigned int levels = 0, unsigned int brick_inner_dimension = 7, unsigned int brick_outer_dimension = 8, unsigned int brick_pool_power = 7);
        void setLevels(int value);
        void setExtent(float Q);
        void setMax(float value);
        void setMin(float value);
        void setUB(UBMatrix<double> mat);
        void setMetaData(QString text);
        QString getMetaData();
        
        void save(QString path);
        void open(QString path);

        Matrix<double> * getExtent();
        Matrix<double> *getMinMax();

        unsigned int getLevels();
        unsigned int getBrickPoolPower();
        unsigned int getBrickInnerDimension();
        unsigned int getBrickOuterDimension();
        unsigned int getBrickNumber();
        UBMatrix<double> getUB();
        void print();

        quint64 getBytes();
        
        // Get/set functions should be made for the following
        Matrix<unsigned int> index;
        Matrix<unsigned int> brick;
        Matrix<float> pool;
        
        QDateTime creation_date;
        qreal creation_noise_cutoff_low;
        qreal creation_noise_cutoff_high;
        qreal creation_post_cutoff_low;
        qreal creation_post_cutoff_high;
        qreal creation_correction_omega;
        qreal creation_correction_kappa;
        qreal creation_correction_phi;
        QStringList creation_file_paths;
        
        qreal view_mode;
        qreal view_tsf_style;
        qreal view_tsf_texture;
        qreal view_data_min;
        qreal view_data_max;
        qreal view_alpha;
        qreal view_brightness;
        
        
    private:
        Matrix<double> minmax;
        Matrix<double> extent;

        quint64 version_major;
        quint64 version_minor;

        quint64 filesize;
        quint64 levels;
        quint64 brick_pool_power;
        quint64 brick_inner_dimension;
        quint64 brick_outer_dimension;
        
        QString metadata;
        
        UBMatrix<double> UB;
};
#endif
