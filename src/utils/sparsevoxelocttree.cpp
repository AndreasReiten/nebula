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
    std::cout << "### SparseVoxelOcttree ###" << std::endl;
    std::cout << "brick_inner_dimension "<< brick_inner_dimension << std::endl;
    std::cout << "brick_outer_dimension "<< brick_outer_dimension << std::endl;
    std::cout << "brick_pool_power "<< brick_pool_power << std::endl;
    std::cout << "levels "<< levels << std::endl;
    std::cout << "version_major "<< version_major << std::endl;
    std::cout << "version_minor "<< version_minor << std::endl;
    std::cout << "filesize "<< filesize << std::endl;
    std::cout << "index.size() "<< index.size() << std::endl;
    std::cout << "brick.size() "<< brick.size() << std::endl;
    std::cout << "pool.size() "<< pool.size() << std::endl;
    std::cout << "data_histogram.size() "<< data_histogram.size() << std::endl;
    std::cout << "data_histogram_log.size() "<< data_histogram_log.size() << std::endl;
    minmax.print(2, "minmax");
    extent.print(2, "extent");
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
void SparseVoxelOcttree::save(QString path, int compression)
{
    if (index.size() == 0 ) return;

    if ((path != ""))
    {
        /* HDF5 File structure
        * File ->
        *   /bricks -> (Data)
        *       n_bricks (Attribute)
        *       n_nodes (Attribute)
        *       brick_outer_dimension (Attribute)
        *       brick_inner_dimension (Attribute)
        *       brick_pool_power (Attribute)
        *       levels (Attribute)
        *       extent (Attribute)
        *       And other metadata...
        *
        *   /oct_index -> (Data)
        *   /oct_brick -> (Data)
        */

        // HDF5
        hid_t file_id;
        hid_t dset_id, dspace_id, atrib_id, plist_id;
        herr_t status;
        hsize_t dims[1];
        size_t size_t_value;

        // Create file
        file_id = H5Fcreate(path.toStdString().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

        // Create property list needed for chunking and compression
        plist_id  = H5Pcreate (H5P_DATASET_CREATE);

        // Dataset must be chunked for compression
        dims[0] = (size_t)(brick_outer_dimension*brick_outer_dimension*brick_outer_dimension);
        status = H5Pset_chunk (plist_id, 1, dims);

        // Set ZLIB / DEFLATE Compressi.
        status = H5Pset_deflate (plist_id, compression);

        // Save traversal index part of octtree data
        dims[0] = index.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        dset_id = H5Dcreate(file_id, "/oct_index", H5T_STD_U32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, index.data());
        status = H5Dclose(dset_id);
        status = H5Sclose(dspace_id);


        // Save the brick index part of the octtree data
        dims[0] = brick.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        dset_id = H5Dcreate(file_id, "/oct_brick", H5T_STD_U32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, brick.data());
        status = H5Dclose(dset_id);
        status = H5Sclose(dspace_id);



        // Save the actual brick data and all other metadata
        dims[0] = pool.size();
        dspace_id = H5Screate_simple(1, dims, NULL);
        dset_id = H5Dcreate(file_id, "/bricks", H5T_IEEE_F32LE, dspace_id, H5P_DEFAULT, plist_id, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, pool.data());
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = pool.size()/(brick_outer_dimension*brick_outer_dimension*brick_outer_dimension);
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "n_bricks", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = index.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "n_nodes", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = brick_outer_dimension;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "brick_outer_dimension", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = brick_inner_dimension;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "brick_inner_dimension", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = levels;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "levels", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = brick_pool_power;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "brick_pool_power", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 8;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "extent", H5T_IEEE_F32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_FLOAT, extent.toFloat().data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = version_major;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "version_major", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = version_minor;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "version_minor", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        size_t bins = 1000;
        double min = 1.0;
        double max = pool.max();

        double * hist_log = pool.histogram(bins, min, max, 1, 1);
        double * hist_norm = pool.histogram(bins, min, max, 0, 1);

        hist_log[0] = 0.0;
        hist_norm[0] = 0.0;

        data_histogram.setDeep(bins, hist_norm);
        data_histogram_log.setDeep(bins, hist_log);

        minmax[0] = min;
        minmax[1] = max;

        dims[0] = data_histogram.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "data_histogram", H5T_IEEE_F64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_DOUBLE, data_histogram.data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        size_t_value = data_histogram.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "hist_norm_len", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = data_histogram_log.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "data_histogram_log", H5T_IEEE_F64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_DOUBLE, data_histogram_log.data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = 1;
        dspace_id = H5Screate_simple (1, dims, NULL);
        size_t_value = data_histogram_log.size();
        atrib_id = H5Acreate(dset_id, "hist_log_len", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);

        dims[0] = minmax.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "minmax", H5T_IEEE_F64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_DOUBLE, minmax.data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
        status = H5Dclose(dset_id);

        status = H5Pclose (plist_id);

        /* Close the file */
        status = H5Fget_filesize(file_id, &dims[0] );
        status = H5Fclose(file_id);

        filesize = dims[0];
    }

    this->print();
}
void SparseVoxelOcttree::open(QString path)
{
    if ((path != ""))
    {
        hid_t file_id;
        hid_t dset_id, atrib_id, plist_id;
        herr_t status;

        size_t   nelmts;
        unsigned flags, filter_info;
        H5Z_filter_t filter_type;

        /* Open file */
        file_id = H5Fopen(path.toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);


        // Get brick data
        dset_id = H5Dopen(file_id, "/bricks", H5P_DEFAULT);

        size_t n_bricks;
        atrib_id = H5Aopen(dset_id, "n_bricks", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &n_bricks );
        status = H5Aclose(atrib_id);

        size_t n_nodes;
        atrib_id = H5Aopen(dset_id, "n_nodes", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &n_nodes );
        status = H5Aclose(atrib_id);

        atrib_id = H5Aopen(dset_id, "brick_outer_dimension", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &brick_outer_dimension );
        status = H5Aclose(atrib_id);

        atrib_id = H5Aopen(dset_id, "brick_inner_dimension", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &brick_inner_dimension );
        status = H5Aclose(atrib_id);

        atrib_id = H5Aopen(dset_id, "brick_pool_power", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &brick_pool_power );
        status = H5Aclose(atrib_id);

        atrib_id = H5Aopen(dset_id, "levels", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &levels );
        status = H5Aclose(atrib_id);

        pool.reserve(n_bricks*brick_outer_dimension*brick_outer_dimension*brick_outer_dimension);

        std::cout << n_bricks << std::endl;
        plist_id = H5Dget_create_plist(dset_id);
        nelmts = 0;
        filter_type = H5Pget_filter(plist_id, 0, &flags, &nelmts, NULL, 0, NULL, &filter_info);
        status = H5Dread(dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, pool.data());

        atrib_id = H5Aopen(dset_id, "version_major", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &version_major );
        status = H5Aclose(atrib_id);

        atrib_id = H5Aopen(dset_id, "version_minor", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &version_minor );
        status = H5Aclose(atrib_id);

        size_t hist_norm_len;
        atrib_id = H5Aopen(dset_id, "hist_norm_len", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &hist_norm_len );
        status = H5Aclose(atrib_id);

        size_t hist_log_len;
        atrib_id = H5Aopen(dset_id, "hist_log_len", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &hist_log_len );
        status = H5Aclose(atrib_id);

        std::cout << "Line " << __LINE__  << hist_norm_len <<" " << hist_log_len<< std::endl;
        data_histogram.reserve(hist_norm_len);
        data_histogram_log.reserve(hist_log_len);

        atrib_id = H5Aopen(dset_id, "data_histogram", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_DOUBLE, data_histogram.data() );
        status = H5Aclose(atrib_id);

        atrib_id = H5Aopen(dset_id, "data_histogram_log", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_DOUBLE, data_histogram_log.data() );
        status = H5Aclose(atrib_id);

        atrib_id = H5Aopen(dset_id, "minmax", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_DOUBLE, minmax.data() );
        status = H5Aclose(atrib_id);


        MiniArray<float> tmp(8);
        atrib_id = H5Aopen(dset_id, "extent", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_FLOAT, tmp.data() );
        status = H5Aclose(atrib_id);

        for (int i = 0; i < 8; i++)
        {
            extent[i] = tmp[i];
        }
        std::cout << "Line "<< __LINE__ <<  " n_nodes " <<n_nodes << std::endl;
        index.reserve(n_nodes);
        brick.reserve(n_nodes);

        status = H5Dclose(dset_id);


        // Get octtree data
        dset_id = H5Dopen(file_id, "/oct_index", H5P_DEFAULT);

        status = H5Dread(dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, index.data());
        status = H5Dclose(dset_id);

        dset_id = H5Dopen(file_id, "/oct_brick", H5P_DEFAULT);
        status = H5Dread(dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, brick.data());
        status = H5Dclose(dset_id);

        //~for (int i = 0; i < n_nodes; i++)
        //~{
            //~unsigned int mask_brick_id_x = ((1 << 10) - 1) << 20;
            //~unsigned int mask_brick_id_y = ((1 << 10) - 1) << 10;
            //~unsigned int mask_brick_id_z = ((1 << 10) - 1) << 0;
//~
            //~unsigned int val = brick[i];
            //~std::cout << ((val & mask_brick_id_x) >> 20 ) <<" " <<  ((val & mask_brick_id_y) >> 10) <<" " << (val & mask_brick_id_z) << std::endl;
        //~}

        /* Close the file */
        status = H5Pclose (plist_id);
        status = H5Fclose(file_id);
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

size_t SparseVoxelOcttree::getBytes()
{
    return brick.bytes() + index.bytes() + pool.bytes();
}
