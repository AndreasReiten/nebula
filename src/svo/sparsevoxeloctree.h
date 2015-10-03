#ifndef SPARSEVOXELOCTTREE_H
#define SPARSEVOXELOCTTREE_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QList>

#include "../math/matrix.h"
#include "../math/ubmatrix.h"
#include "../misc/line.h"

class SparseVoxelOctreeOld
{
        /* This class represents Sparse Voxel Matrix. It is the datastructure that is used by the OpenCL raytracer */

    public:
        SparseVoxelOctreeOld();
        ~SparseVoxelOctreeOld();

        void set(unsigned int p_levels = 0, unsigned int p_brick_inner_dimension = 7, unsigned int p_brick_outer_dimension = 8, unsigned int p_brick_pool_power = 7);
        void setLevels(int value);
        void setExtent(float Q);
        void setMax(float value);
        void setMin(float value);
        void setUB(UBMatrix<double> mat);
        void setMetaData(QString text);

        QString metaData();
        void save(QString path);
        void open(QString path);
        void saveMetadata(QString path);
        void openMetadata(QString path);

        Matrix<double> extent();
        Matrix<double> minMax();

        unsigned int levels();
        unsigned int brickPoolPower();
        unsigned int brickInnerDimension();
        unsigned int brickOuterDimension();
        unsigned int brickNumber();
        UBMatrix<double> UB();
        void print();


        QList<Line> * lines();
        Matrix<unsigned int> * index();
        Matrix<unsigned int> * brick();
        Matrix<float> * pool();
        qreal viewMode();
        qreal viewTsfStyle();
        qreal viewTsfTexture();
        qreal viewDataMin();
        qreal viewDataMax();
        qreal viewAlpha();
        qreal viewBrightness();

        void setViewMode(qreal value);
        void setViewTsfStyle(qreal value);
        void setViewTsfTexture(qreal value);
        void setViewDataMin(qreal value);
        void setViewDataMax(qreal value);
        void setViewAlpha(qreal value);
        void setViewBrightness(qreal value);

        quint64 bytes();

    private:
        Matrix<unsigned int> p_index;
        Matrix<unsigned int> p_brick;
        Matrix<float> p_pool;

        qreal p_view_mode;
        qreal p_view_tsf_style;
        qreal p_view_tsf_texture;
        qreal p_view_data_min;
        qreal p_view_data_max;
        qreal p_view_alpha;
        qreal p_view_brightness;

        QDateTime creation_date;
        qreal creation_noise_cutoff_low;
        qreal creation_noise_cutoff_high;
        qreal creation_post_cutoff_low;
        qreal creation_post_cutoff_high;
        qreal creation_correction_omega;
        qreal creation_correction_kappa;
        qreal creation_correction_phi;
        QStringList creation_file_paths;

        Matrix<double> p_minmax;
        Matrix<double> p_extent;

        quint64 p_version_major;
        quint64 p_version_minor;

        quint64 p_filesize;
        quint64 p_levels;
        quint64 p_brick_pool_power;
        quint64 p_brick_inner_dimension;
        quint64 p_brick_outer_dimension;

        QString p_note;

        UBMatrix<double> p_ub;

        QList<Line> p_lines;
};

class SparseVoxelOctree
{
        /* This class represents Sparse Voxel Matrix. It is the datastructure that is used by the OpenCL raytracer */

    public:
        SparseVoxelOctree();
        ~SparseVoxelOctree();

        void save(QString path);
        void open(QString path);

        void setLevels(int value);
        void setExtent(float value);
        void setUB(UBMatrix<double> mat);
        void setPoolDim(unsigned int x, unsigned int y, unsigned int z);
        void clear();

        QVector<double> extent();

        unsigned int levels();
        unsigned int poolDimX();
        unsigned int poolDimY();
        unsigned int poolDimZ();
        unsigned int side();
        UBMatrix<double> UB();

        QVector<unsigned int> & index();
        QVector<unsigned int> & brick();
        QVector<float> & pool();

    private:
        QVector<unsigned int> p_index;
        QVector<unsigned int> p_brick;
        QVector<float> p_pool;

        QVector<double> p_extent;

        quint64 p_version_major, p_version_minor;

        quint64 p_levels;
        quint64 p_pool_dim_x;
        quint64 p_pool_dim_y;
        quint64 p_pool_dim_z;
        quint64 p_brick_dim;

        UBMatrix<double> p_ub;
};

#endif
