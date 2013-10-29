#include "sparsevoxelocttree.h"
SparseVoxelOcttree::SparseVoxelOcttree()
{
    this->filesize = 0;
    this->levels = 2;
    this->brick_inner_dimension = 7;
    this->brick_outer_dimension = 8;
    this->brick_pool_power = 7;
    this->extent.reserve(8);
    this->version_major = 0;
    this->version_minor = 2;
    this->minmax.reserve(2);
};

SparseVoxelOcttree::~SparseVoxelOcttree()
{
}

void SparseVoxelOcttree::print()
{
    std::stringstream ss;
    ss << std::endl;
    ss << "____ SparseVoxelOcttree ____" << std::endl;
    ss << "Brick inner dimension:   "<< brick_inner_dimension << std::endl;
    ss << "Brick outer dimension:   "<< brick_outer_dimension << std::endl;
    ss << "Brick pool power:        "<< brick_pool_power << std::endl;
    ss << "Levels:                  "<< levels << std::endl;
    ss << "File version:            "<< version_major << "." << version_minor << std::endl;
    ss << "Index elements:          "<< index.size() << std::endl;
    ss << "Brick elements:          "<< brick.size() << std::endl;
    ss << "Pool size:               "<< pool.size() << std::endl;
    ss << "____________________________\n";

    qDebug() << ss.str().c_str();
}


void SparseVoxelOcttree::setExtent(float Q)
{
    this->extent[0] = -Q;
    this->extent[1] = Q;
    this->extent[2] = -Q;
    this->extent[3] = Q;
    this->extent[4] = -Q;
    this->extent[5] = Q;
    this->extent[6] = 1;
    this->extent[7] = 1;
}

MiniArray<double> * SparseVoxelOcttree::getExtent()
{
    return &extent;
}
MiniArray<double> * SparseVoxelOcttree::getDataHistogram()
{
    return &data_histogram;
}
MiniArray<double> * SparseVoxelOcttree::getDataHistogramLog()
{
    return &data_histogram_log;
}
MiniArray<double> * SparseVoxelOcttree::getMinMax()
{
    return &minmax;
}

void SparseVoxelOcttree::setLevels(int value)
{
    this->levels = value;
}

void SparseVoxelOcttree::set(unsigned int levels, unsigned int brick_inner_dimension, unsigned int brick_outer_dimension, unsigned int brick_pool_power)
{
    this->levels = levels;
    this->brick_inner_dimension = brick_inner_dimension;
    this->brick_outer_dimension = brick_outer_dimension;
    this->brick_pool_power = brick_pool_power;
}

void SparseVoxelOcttree::save(QString path)
{
    if (path != "")
    {
        quint64 bins = 1000;
        double min = pool.min();
        double max = pool.max();

        double * hist_log = pool.histogram(bins, min, max, 1, 1);
        double * hist_norm = pool.histogram(bins, min, max, 0, 1);

        hist_log[0] = 0.0;
        hist_norm[0] = 0.0;

        data_histogram.setDeep(bins, hist_norm);
        data_histogram_log.setDeep(bins, hist_log);

        minmax[0] = 1.0;
        minmax[1] = max;


        QFile file(path);
        file.open(QIODevice::WriteOnly);
        QDataStream out(&file);
        out << QString("While HDF is nice, it does not work easily for Windows (to my stressful experience). Therefore the (more naive) QDataStream class is used instead.");
        out << version_major;
        out << version_minor;
        out << brick_outer_dimension;
        out << brick_inner_dimension;
        out << brick_pool_power;
        out << levels;
        out << minmax[0];
        out << minmax[1];
        out << data_histogram.toQVector();
        out << data_histogram_log.toQVector();
        out << extent.toQVector();
        out << pool.toQVector();
        out << index.toQVector();
        out << brick.toQVector();
    }

    this->print();
}
void SparseVoxelOcttree::open(QString path)
{
    // Disabled chunking and compression due to problems under Windows
    if ((path != ""))
    {
        QString cool_story_bro;

        QVector<double> qvec_data_histogram;
        QVector<double> qvec_data_histogram_log;
        QVector<double> qvec_extent;
        QVector<float> qvec_pool;
        QVector<unsigned int> qvec_index;
        QVector<unsigned int> qvec_brick;

        QFile file(path);
        file.open(QIODevice::ReadOnly);
        QDataStream in(&file);
        in >> cool_story_bro;
        in >> version_major;
        in >> version_minor;
        in >> brick_outer_dimension;
        in >> brick_inner_dimension;
        in >> brick_pool_power;
        in >> levels;
        in >> minmax[0];
        in >> minmax[1];
        in >> qvec_data_histogram;
        in >> qvec_data_histogram_log;
        in >> qvec_extent;
        in >> qvec_pool;
        in >> qvec_index;
        in >> qvec_brick;

        data_histogram.setDeep(qvec_data_histogram.size(), qvec_data_histogram.data());
        data_histogram_log.setDeep(qvec_data_histogram_log.size(), qvec_data_histogram_log.data());
        extent.setDeep(qvec_extent.size(), qvec_extent.data());
        pool.setDeep(qvec_pool.size(), qvec_pool.data());
        index.setDeep(qvec_index.size(), qvec_index.data());
        brick.setDeep(qvec_brick.size(), qvec_brick.data());
    }

    this->print();
}
unsigned int SparseVoxelOcttree::getLevels()
{
    return levels;
}

unsigned int SparseVoxelOcttree::getBrickPoolPower()
{
    return brick_pool_power;
}
unsigned int SparseVoxelOcttree::getBrickInnerDimension()
{
    return brick_inner_dimension;
}
unsigned int SparseVoxelOcttree::getBrickOuterDimension()
{
    return brick_outer_dimension;
}

unsigned int SparseVoxelOcttree::getBrickNumber()
{
    return pool.size()/(brick_outer_dimension*brick_outer_dimension*brick_outer_dimension);
}

quint64 SparseVoxelOcttree::getBytes()
{
    return brick.bytes() + index.bytes() + pool.bytes();
}
