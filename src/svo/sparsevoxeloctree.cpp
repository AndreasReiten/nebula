#include "sparsevoxeloctree.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <QDataStream>
#include <QVector>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

SparseVoxelOctree::SparseVoxelOctree() :
//    p_version_major(0),
//    p_version_minor(1),
    p_levels(0),
    p_pool_dim_x(0),
    p_pool_dim_y(0),
    p_pool_dim_z(0),
    p_brick_dim(8)
{
    p_extent.resize(6);
}

SparseVoxelOctree::~SparseVoxelOctree()
{

}

QString SparseVoxelOctree::info(unsigned int n)
{
    QString str;
    str += "p_levels: "+QString::number(p_levels)+"\n";
    str += "p_pool_dim_x: "+QString::number(p_pool_dim_x)+"\n";
    str += "p_pool_dim_y: "+QString::number(p_pool_dim_y)+"\n";
    str += "p_pool_dim_z: "+QString::number(p_pool_dim_z)+"\n";
    str += "p_brick_dim: "+QString::number(p_brick_dim)+"\n";
    str += "UB size: "+QString::number(p_ub.size())+"\n";
    str += "Extent: "+QString::number(p_extent[0])+" "+QString::number(p_extent[1])+" "+QString::number(p_extent[2])+" "+QString::number(p_extent[3])+" "+QString::number(p_extent[4])+" "+QString::number(p_extent[5])+"\n";

    str += "p_index, p_brick ("+QString::number(p_index.size())+" items)\n";
    for (int i = 0; i < std::min((long int) n, (long int) p_index.size()); i++)
    {
        uint mask_brick_id_x = ((1 << 10) - 1) << 20;
        uint mask_brick_id_y = ((1 << 10) - 1) << 10;
        uint mask_brick_id_z = ((1 << 10) - 1) << 0;

        uint mask_leaf_flag   = ((1u << 1u) - 1u) << 31u;
        uint mask_data_flag   = ((1 << 1) - 1) << 30;
        uint mask_child_index = ((1 << 30) - 1) << 0;

        str += QString::number(i)+" -> Leaf: "+QString::number((p_index[i] & mask_leaf_flag) >> 31)+" Data: "+QString::number((p_index[i] & mask_data_flag) >> 30)+(((p_index[i] & mask_leaf_flag) >> 31) == 0 ? " [Child: "+QString::number((p_index[i] & mask_child_index))+"]" : "")+"\n";
        if (((p_index[i] & mask_data_flag) >> 30) == 1) str += " -- Brick x: "+QString::number((p_brick[i] & mask_brick_id_x) >> 20)+" y: "+QString::number((p_brick[i] & mask_brick_id_y) >> 10)+" z: "+QString::number((p_brick[i] & mask_brick_id_z))+"\n";
    }

    return str;
}

void SparseVoxelOctree::save(QString path)
{
    QFile file(path);

    if (file.open(QIODevice::WriteOnly))
    {
        QDataStream out(&file);

        out << (qint64) 0;
        out << (qint64) 1;
        out << p_index;
        out << p_brick;
        out << p_pool;
        out << p_extent;
        out << p_levels;
        out << p_pool_dim_x;
        out << p_pool_dim_y;
        out << p_pool_dim_z;
        out << p_brick_dim;
        out << p_ub;


        file.close();
    }
}
void SparseVoxelOctree::open(QString path)
{
    QFile file(path);

    if (file.open(QIODevice::ReadOnly))
    {
        QDataStream in(&file);

        quint64 version_major, version_minor;

        in >> version_minor;
        in >> version_major;

        in >> p_index;
        in >> p_brick;
        in >> p_pool;
        in >> p_extent;
        in >> p_levels;
        in >> p_pool_dim_x;
        in >> p_pool_dim_y;
        in >> p_pool_dim_z;
        in >> p_brick_dim;
        in >> p_ub;


        file.close();
    }
}

void SparseVoxelOctree::clear()
{
    p_index.clear();
    p_brick.clear();
    p_pool.clear();

    p_extent.clear();
    p_ub.clear();
}

void SparseVoxelOctree::setLevels(int value)
{
    p_levels = value;
}
void SparseVoxelOctree::setExtent(float value)
{
    p_extent[0] = -value;
    p_extent[1] = value;
    p_extent[2] = -value;
    p_extent[3] = value;
    p_extent[4] = -value;
    p_extent[5] = value;

}
void SparseVoxelOctree::setUB(UBMatrix<double> mat)
{
    p_ub = mat;
}

void SparseVoxelOctree::setPoolDim(unsigned int x, unsigned int y, unsigned int z)
{
    p_pool_dim_x = x;
    p_pool_dim_y = y;
    p_pool_dim_z = z;
}

QVector<double> SparseVoxelOctree::extent()
{
    return p_extent;
}

unsigned int SparseVoxelOctree::levels()
{
    return p_levels;
}
unsigned int SparseVoxelOctree::poolDimX()
{
    return p_pool_dim_x;
}
unsigned int SparseVoxelOctree::poolDimY()
{
    return p_pool_dim_y;
}
unsigned int SparseVoxelOctree::poolDimZ()
{
    return p_pool_dim_z;
}
unsigned int SparseVoxelOctree::side()
{
    return p_brick_dim;
}
UBMatrix<double> SparseVoxelOctree::UB()
{
    return p_ub;
}

QVector<unsigned int> & SparseVoxelOctree::index()
{
    return p_index;
}
QVector<unsigned int> & SparseVoxelOctree::brick()
{
    return p_brick;
}
QVector<float> & SparseVoxelOctree::pool()
{
    return p_pool;
}

SparseVoxelOctreeOld::SparseVoxelOctreeOld()
{
    this->p_filesize = 0;
    this->p_levels = 2;
    this->p_brick_inner_dimension = 7;
    this->p_brick_outer_dimension = 8;
    this->p_brick_pool_power = 7;
    this->p_extent.reserve(1, 8);
    this->p_version_major = 0;
    this->p_version_minor = 5;
    this->p_minmax.reserve(1, 2);
    p_ub.setIdentity(3);
};

SparseVoxelOctreeOld::~SparseVoxelOctreeOld()
{
}

void SparseVoxelOctreeOld::print()
{
    std::stringstream ss;
    ss << std::endl;
    ss << "____ SparseVoxelOctree ____" << std::endl;
    ss << "Brick inner dimension:   " << p_brick_inner_dimension << std::endl;
    ss << "Brick outer dimension:   " << p_brick_outer_dimension << std::endl;
    ss << "Brick pool power:        " << p_brick_pool_power << std::endl;
    ss << "Levels:                  " << p_levels << std::endl;
    ss << "File version:            " << p_version_major << "." << p_version_minor << std::endl;
    ss << "Index elements:          " << p_index.size() << std::endl;
    ss << "Brick elements:          " << p_brick.size() << std::endl;
    ss << "Pool size:               " << p_pool.size() << std::endl;
    ss << "Data min:                " << p_minmax[0] << std::endl;
    ss << "Data max:                " << p_minmax[1] << std::endl;

    ss << "View data min:           " << p_view_data_min << std::endl;
    ss << "View data max:           " << p_view_data_max << std::endl;
    ss << "View alpha:              " << p_view_alpha << std::endl;
    ss << "View brightness:         " << p_view_brightness << std::endl;
    ss << "View mode:               " << p_view_mode << std::endl;
    //    ss << "Metadata:                \n"<< metadata.toStdString() << std::endl;
    //    ss << "UB:" << UB << std::endl;
    //    ss << "Extent:" << extent << std::endl;
    ss << "____________________________\n";

    qDebug() << ss.str().c_str();

}


void SparseVoxelOctreeOld::setExtent(float Q)
{
    this->p_extent[0] = -Q;
    this->p_extent[1] = Q;
    this->p_extent[2] = -Q;
    this->p_extent[3] = Q;
    this->p_extent[4] = -Q;
    this->p_extent[5] = Q;
    this->p_extent[6] = 1;
    this->p_extent[7] = 1;
}

Matrix<double> SparseVoxelOctreeOld::extent()
{
    return p_extent;
}
Matrix<double> SparseVoxelOctreeOld::minMax()
{
    return p_minmax;
}

void SparseVoxelOctreeOld::setLevels(int value)
{
    this->p_levels = value;
}

void SparseVoxelOctreeOld::set(unsigned int levels, unsigned int brick_inner_dimension, unsigned int brick_outer_dimension, unsigned int brick_pool_power)
{
    this->p_levels = levels;
    this->p_brick_inner_dimension = brick_inner_dimension;
    this->p_brick_outer_dimension = brick_outer_dimension;
    this->p_brick_pool_power = brick_pool_power;
}

void SparseVoxelOctreeOld::setMetaData(QString text)
{
    p_note = text;
}

QString SparseVoxelOctreeOld::metaData()
{
    return p_note;
}

void SparseVoxelOctreeOld::save(QString path)
{
    if (path != "")
    {
        QFile file(path);

        if (file.open(QIODevice::WriteOnly))
        {
            // v 0.3
            QDataStream out(&file);
            out << (quint64) 0;
            out << (quint64) 5;
            out << p_brick_outer_dimension;
            out << p_brick_inner_dimension;
            out << p_brick_pool_power;
            out << p_levels;
            out << p_minmax;
            out << p_extent;
            out << p_pool;
            out << p_index;
            out << p_brick;
            out << p_ub;
            out << p_note;

            // v 0.4
            // Creation settings
            out << creation_date;
            out << creation_noise_cutoff_low;
            out << creation_noise_cutoff_high;
            out << creation_post_cutoff_low;
            out << creation_post_cutoff_high;
            out << creation_correction_omega;
            out << creation_correction_kappa;
            out << creation_correction_phi;
            out << creation_file_paths;

            // View settings
            out << p_view_mode;
            out << p_view_tsf_style;
            out << p_view_tsf_texture;
            out << p_view_data_min;
            out << p_view_data_max;
            out << p_view_alpha;
            out << p_view_brightness;

            // v 0.5
            //            qDebug() << p_lines.size();

            out << p_lines;

            file.close();
        }
    }

    this->print();
}

void SparseVoxelOctreeOld::setMax(float value)
{
    p_minmax[1] = value;
}

void SparseVoxelOctreeOld::setMin(float value)
{
    p_minmax[0] = value;
}

void SparseVoxelOctreeOld::setUB(UBMatrix<double> mat)
{
    p_ub = mat;
}

void SparseVoxelOctreeOld::open(QString path)
{
    if ((path != ""))
    {
        QFile file(path);

        if (file.open(QIODevice::ReadOnly))
        {
            QDataStream in(&file);

            in >> p_version_major;
            in >> p_version_minor;
            in >> p_brick_outer_dimension;
            in >> p_brick_inner_dimension;
            in >> p_brick_pool_power;
            in >> p_levels;
            in >> p_minmax;
            in >> p_extent;
            in >> p_pool;
            in >> p_index;
            in >> p_brick;
            in >> p_ub;
            in >> p_note;

            // v 0.4
            if ((p_version_major >= 0) && (p_version_minor >= 4))
            {
                // Creation settings
                in >> creation_date;
                in >> creation_noise_cutoff_low;
                in >> creation_noise_cutoff_high;
                in >> creation_post_cutoff_low;
                in >> creation_post_cutoff_high;
                in >> creation_correction_omega;
                in >> creation_correction_kappa;
                in >> creation_correction_phi;
                in >> creation_file_paths;

                // View settings
                in >> p_view_mode;
                in >> p_view_tsf_style;
                in >> p_view_tsf_texture;
                in >> p_view_data_min;
                in >> p_view_data_max;
                in >> p_view_alpha;
                in >> p_view_brightness;
            }
            else
            {
                // Creation settings
                creation_date = QFileInfo(file).created();
                creation_noise_cutoff_low = -1;
                creation_noise_cutoff_high = -1;
                creation_post_cutoff_low = -1;
                creation_post_cutoff_high = -1;
                creation_correction_omega = -1;
                creation_correction_kappa = -1;
                creation_correction_phi = -1;

                // View settings
                p_view_mode = 0;
                p_view_tsf_style = 2;
                p_view_tsf_texture = 1;
                p_view_data_min = 0;
                p_view_data_max = p_minmax.at(1);
                p_view_alpha = 0.05;
                p_view_brightness = 2.0;
            }

            qDebug() << p_version_major << p_version_minor;

            // v 0.5
            if ((p_version_major >= 0) && (p_version_minor >= 5))
            {
                //                qDebug() << "Loading p_lines";
                in >> p_lines;
            }

            file.close();
        }
    }

    this->print();
}

void SparseVoxelOctreeOld::openMetadata(QString path)
{
    if ((path != ""))
    {
        QFile file(path);

        if (file.open(QIODevice::ReadOnly))
        {
            QDataStream in(&file);

            in >> p_version_major;
            in >> p_version_minor;
            in >> p_ub;

            // View settings
            in >> p_view_mode;
            in >> p_view_tsf_style;
            in >> p_view_tsf_texture;
            in >> p_view_data_min;
            in >> p_view_data_max;
            in >> p_view_alpha;
            in >> p_view_brightness;

            in >> p_lines;

            file.close();
        }
    }
}

void SparseVoxelOctreeOld::saveMetadata(QString path)
{
    if (path != "")
    {
        QFile file(path);

        if (file.open(QIODevice::WriteOnly))
        {
            // v 0.3
            QDataStream out(&file);
            out << (quint64) 0;
            out << (quint64) 5;
            out << p_ub;

            // View settings
            out << p_view_mode;
            out << p_view_tsf_style;
            out << p_view_tsf_texture;
            out << p_view_data_min;
            out << p_view_data_max;
            out << p_view_alpha;
            out << p_view_brightness;

            out << p_lines;

            file.close();
        }
    }
}

unsigned int SparseVoxelOctreeOld::levels()
{
    return p_levels;
}


UBMatrix<double> SparseVoxelOctreeOld::UB()
{
    return p_ub;
}

unsigned int SparseVoxelOctreeOld::brickPoolPower()
{
    return p_brick_pool_power;
}
unsigned int SparseVoxelOctreeOld::brickInnerDimension()
{
    return p_brick_inner_dimension;
}
unsigned int SparseVoxelOctreeOld::brickOuterDimension()
{
    return p_brick_outer_dimension;
}

unsigned int SparseVoxelOctreeOld::brickNumber()
{
    return p_pool.size() / (p_brick_outer_dimension * p_brick_outer_dimension * p_brick_outer_dimension);
}

quint64 SparseVoxelOctreeOld::bytes()
{
    return p_brick.bytes() + p_index.bytes() + p_pool.bytes();
}

QList<Line> * SparseVoxelOctreeOld::lines()
{
    return &p_lines;
}
Matrix<unsigned int> * SparseVoxelOctreeOld::index()
{
    return &p_index;
}
Matrix<unsigned int> * SparseVoxelOctreeOld::brick()
{
    return &p_brick;
}
Matrix<float> * SparseVoxelOctreeOld::pool()
{
    return &p_pool;
}
qreal SparseVoxelOctreeOld::viewMode()
{
    return p_view_mode;
}
qreal SparseVoxelOctreeOld::viewTsfStyle()
{
    return p_view_tsf_style;
}
qreal SparseVoxelOctreeOld::viewTsfTexture()
{
    return p_view_tsf_texture;
}
qreal SparseVoxelOctreeOld::viewDataMin()
{
    return p_view_data_min;
}
qreal SparseVoxelOctreeOld::viewDataMax()
{
    return p_view_data_max;
}
qreal SparseVoxelOctreeOld::viewAlpha()
{
    return p_view_alpha;
}
qreal SparseVoxelOctreeOld::viewBrightness()
{
    return p_view_brightness;
}
void SparseVoxelOctreeOld::setViewMode(qreal value)
{
    p_view_mode = value;
}
void SparseVoxelOctreeOld::setViewTsfStyle(qreal value)
{
    p_view_tsf_style = value;
}
void SparseVoxelOctreeOld::setViewTsfTexture(qreal value)
{
    p_view_tsf_texture = value;
}
void SparseVoxelOctreeOld::setViewDataMin(qreal value)
{
    p_view_data_min = value;
}
void SparseVoxelOctreeOld::setViewDataMax(qreal value)
{
    p_view_data_max = value;
}
void SparseVoxelOctreeOld::setViewAlpha(qreal value)
{
    p_view_alpha = value;
}
void SparseVoxelOctreeOld::setViewBrightness(qreal value)
{
    p_view_brightness = value;
}
