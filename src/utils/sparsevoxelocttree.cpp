#include "sparsevoxelocttree.h"
#include "ubmatrix.h"

SparseVoxelOcttree::SparseVoxelOcttree()
{
    this->filesize = 0;
    this->levels = 2;
    this->brick_inner_dimension = 7;
    this->brick_outer_dimension = 8;
    this->brick_pool_power = 7;
    this->extent.reserve(1,8);
    this->version_major = 0;
    this->version_minor = 3;
    this->minmax.reserve(1,2);
    UB.setIdentity(3);
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
    ss << "Data min:                "<< minmax[0] << std::endl;
    ss << "Data max:                "<< minmax[1] << std::endl;
    ss << "Metadata:                \n"<< metadata.toStdString() << std::endl;
    ss << "UB:" << UB << std::endl;
    ss << "Extent:" << extent << std::endl;
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

Matrix<double> * SparseVoxelOcttree::getExtent()
{
    return &extent;
}
Matrix<double> * SparseVoxelOcttree::getMinMax()
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

void SparseVoxelOcttree::setMetaData(QString text)
{
    metadata = text;    
}

QString SparseVoxelOcttree::getMetaData()
{
    return metadata;
}

void SparseVoxelOcttree::save(QString path)
{
    if (path != "")
    {
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        QDataStream out(&file);
        out << version_major;
        out << version_minor;
        out << brick_outer_dimension;
        out << brick_inner_dimension;
        out << brick_pool_power;
        out << levels;
        out << minmax.toQVector();
        out << extent.toQVector();
        out << pool.toQVector();
        out << index.toQVector();
        out << brick.toQVector();
        out << UB.toQVector();
        out << metadata;
    }

    this->print();
}

void SparseVoxelOcttree::setMax(float value)
{
    minmax[1] = value;
}

void SparseVoxelOcttree::setMin(float value)
{
    minmax[0] = value;
}

void SparseVoxelOcttree::setUB(UBMatrix<double> mat)
{
    UB = mat;
}

void SparseVoxelOcttree::open(QString path)
{
    // Disabled chunking and compression due to problems under Windows
    if ((path != ""))
    {
        QVector<double> qvec_extent;
        QVector<float> qvec_pool;
        QVector<unsigned int> qvec_index;
        QVector<unsigned int> qvec_brick;
        QVector<double> qvec_ub;
        QVector<double> qvec_minmax;

        QFile file(path);
        file.open(QIODevice::ReadOnly);
        QDataStream in(&file);
        
        in >> version_major;
        in >> version_minor;
        in >> brick_outer_dimension;
        in >> brick_inner_dimension;
        in >> brick_pool_power;
        in >> levels;
        in >> qvec_minmax;
        in >> qvec_extent;
        in >> qvec_pool;
        in >> qvec_index;
        in >> qvec_brick;
        in >> qvec_ub;
        in >> metadata;

        minmax.setDeep(1, qvec_minmax.size(), qvec_minmax.data());
        extent.setDeep(1, qvec_extent.size(), qvec_extent.data());
        pool.setDeep(1, qvec_pool.size(), qvec_pool.data());
        index.setDeep(1, qvec_index.size(), qvec_index.data());
        brick.setDeep(1, qvec_brick.size(), qvec_brick.data());
        UB.setDeep(3, 3, qvec_ub.data());
    }

    this->print();
}
unsigned int SparseVoxelOcttree::getLevels()
{
    return levels;
}


UBMatrix<double> SparseVoxelOcttree::getUB()
{
    return UB;
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
