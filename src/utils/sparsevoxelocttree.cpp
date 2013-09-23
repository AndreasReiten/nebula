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
    ss << "### SparseVoxelOcttree ###\n";
    ss << "brick_inner_dimension "<< brick_inner_dimension << "\n";
    ss << "brick_outer_dimension "<< brick_outer_dimension << "\n";
    ss << "brick_pool_power "<< brick_pool_power << "\n";
    ss << "levels "<< levels << "\n";
    ss << "version_major "<< version_major << "\n";
    ss << "version_minor "<< version_minor << "\n";
    ss << "filesize "<< filesize << "\n";
    ss << "index.size() "<< index.size() << "\n";
    ss << "brick.size() "<< brick.size() << "\n";
    ss << "pool.size() "<< pool.size() << "\n";
    ss << "data_histogram.size() "<< data_histogram.size() << "\n";
    ss << "data_histogram_log.size() "<< data_histogram_log.size() << "\n";

    writeToLogAndPrint(QString(ss.str().c_str()), "riv.log", 1);

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
//~ //~
        // Create file
        file_id = H5Fcreate(path.toStdString().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
//~ //~
        // Create property list needed for chunking and compression
        plist_id  = H5Pcreate (H5P_DATASET_CREATE);
//~ //~

        // Disabled chunking and compression due to problems under Windows
        // Dataset must be chunked for compression
        dims[0] = (size_t)(brick_outer_dimension*brick_outer_dimension*brick_outer_dimension);
        status = H5Pset_chunk (plist_id, 1, dims);
//~ //~
        // Set compression
        status = H5Pset_deflate (plist_id, compression);
//~ //~
        // Save traversal index part of octtree data
        dims[0] = index.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        dset_id = H5Dcreate(file_id, "oct_index", H5T_STD_U32LE, dspace_id, H5P_DEFAULT, plist_id, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, index.data());
        status = H5Dclose(dset_id);
        status = H5Sclose(dspace_id);
//~ //~
//~ //~
        // Save the brick index part of the octtree data
        dims[0] = brick.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        dset_id = H5Dcreate(file_id, "oct_brick", H5T_STD_U32LE, dspace_id, H5P_DEFAULT, plist_id, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, brick.data());
        status = H5Dclose(dset_id);
        status = H5Sclose(dspace_id);
//~ //~
//~ //~
//~ //~
        // Save the actual brick pool data and all other metadata
        dims[0] = pool.size();
        dspace_id = H5Screate_simple(1, dims, NULL);
        dset_id = H5Dcreate(file_id, "bricks", H5T_IEEE_F32LE, dspace_id, H5P_DEFAULT, plist_id, H5P_DEFAULT);
        status = H5Dwrite (dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, pool.data());
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = 1;
        size_t_value = pool.size()/(brick_outer_dimension*brick_outer_dimension*brick_outer_dimension);
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "n_bricks", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = 1;
        size_t_value = index.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "n_nodes", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = 1;
        size_t_value = brick_outer_dimension;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "brick_outer_dimension", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = 1;
        size_t_value = brick_inner_dimension;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "brick_inner_dimension", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = 1;
        size_t_value = levels;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "levels", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
writeToLogAndPrint("Saving: brick_pool_power "+QString::number(brick_pool_power), "riv.log", 1);
        dims[0] = 1;
        size_t_value = brick_pool_power;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "brick_pool_power", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
    writeToLogAndPrint("Saving: brick_pool_power "+QString::number(size_t_value), "riv.log", 1);
//~ //~
        dims[0] = 8;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "extent", H5T_IEEE_F32LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_FLOAT, extent.toFloat().data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = 1;
        size_t_value = version_major;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "version_major", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = 1;
        size_t_value = version_minor;
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "version_minor", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        size_t bins = 1000;
        double min = 1.0;
        double max = pool.maxValue();
//~ //~
        double * hist_log = pool.histogram(bins, min, max, 1, 1);
        double * hist_norm = pool.histogram(bins, min, max, 0, 1);
//~ //~
        hist_log[0] = 0.0;
        hist_norm[0] = 0.0;
//~ //~
        data_histogram.setDeep(bins, hist_norm);
        data_histogram_log.setDeep(bins, hist_log);
//~ //~
        minmax[0] = min;
        minmax[1] = max;
//~ //~
        dims[0] = data_histogram.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "data_histogram", H5T_IEEE_F64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_DOUBLE, data_histogram.data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = 1;
        size_t_value = data_histogram.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "hist_norm_len", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = data_histogram_log.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "data_histogram_log", H5T_IEEE_F64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_DOUBLE, data_histogram_log.data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~
        dims[0] = 1;
        dspace_id = H5Screate_simple (1, dims, NULL);
        size_t_value = data_histogram_log.size();
        atrib_id = H5Acreate(dset_id, "hist_log_len", H5T_STD_U64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_ULLONG, &size_t_value);
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
//~ //~

writeToLogAndPrint("hist_norm_len hist_log_len "+QString::number(data_histogram_log.size())+" "+QString::number(data_histogram.size()), "riv.log", 1);
        dims[0] = minmax.size();
        dspace_id = H5Screate_simple (1, dims, NULL);
        atrib_id = H5Acreate(dset_id, "minmax", H5T_IEEE_F64LE, dspace_id, H5P_DEFAULT, H5P_DEFAULT);
        status = H5Awrite(atrib_id, H5T_NATIVE_DOUBLE, minmax.data());
        status = H5Aclose(atrib_id);
        status = H5Sclose(dspace_id);
        status = H5Dclose(dset_id);
//~ //~
        status = H5Pclose (plist_id);
//~ //~
        /* Close the file */
        status = H5Fget_filesize(file_id, &dims[0] );
        status = H5Fclose(file_id);
//~ //~
        filesize = dims[0];
    }

    this->print();
}
void SparseVoxelOcttree::open(QString path)
{
    // Disabled chunking and compression due to problems under Windows
    if ((path != ""))
    {
        writeToLogAndPrint("a", "riv.log", 1);
        hid_t file_id;
        hid_t dset_id, atrib_id, plist_id;
        herr_t status;

writeToLogAndPrint("a", "riv.log", 1);
        size_t   nelmts;
        unsigned flags, filter_info;
        H5Z_filter_t filter_type;
//~ //~
writeToLogAndPrint("a", "riv.log", 1);
        /* Open file */
        file_id = H5Fopen(path.toStdString().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
//~ //~
//~ //~
writeToLogAndPrint("a", "riv.log", 1);
        // Get brick data
        dset_id = H5Dopen(file_id, "bricks", H5P_DEFAULT);
//~ //~
        size_t n_bricks;
        atrib_id = H5Aopen(dset_id, "n_bricks", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &n_bricks );
        status = H5Aclose(atrib_id);
//~ //~

writeToLogAndPrint("n_bricks "+QString::number(n_bricks), "riv.log", 1);
        size_t n_nodes;
        atrib_id = H5Aopen(dset_id, "n_nodes", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &n_nodes );
        status = H5Aclose(atrib_id);
//~ //~
        atrib_id = H5Aopen(dset_id, "brick_outer_dimension", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &brick_outer_dimension );
        status = H5Aclose(atrib_id);
//~ //~
        atrib_id = H5Aopen(dset_id, "brick_inner_dimension", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &brick_inner_dimension );
        status = H5Aclose(atrib_id);
writeToLogAndPrint("ad", "riv.log", 1);
        size_t size_t_value;
        atrib_id = H5Aopen(dset_id, "brick_pool_power", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &size_t_value );
        status = H5Aclose(atrib_id);
        writeToLogAndPrint("ad", "riv.log", 1);
//~ //~
brick_pool_power = size_t_value;
writeToLogAndPrint("Loading: brick_pool_power "+QString::number(brick_pool_power), "riv.log", 1);
        atrib_id = H5Aopen(dset_id, "levels", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &levels );
        status = H5Aclose(atrib_id);
writeToLogAndPrint("gf", "riv.log", 1);
        pool.reserve(n_bricks*brick_outer_dimension*brick_outer_dimension*brick_outer_dimension);
writeToLogAndPrint("gfasddas", "riv.log", 1);
        plist_id = H5Dget_create_plist(dset_id);
        writeToLogAndPrint("1gfasddas", "riv.log", 1);
        nelmts = 0;
        writeToLogAndPrint("2gfasddas", "riv.log", 1);
        filter_type = H5Pget_filter(plist_id, 0, &flags, &nelmts, NULL, 0, NULL, &filter_info);
        writeToLogAndPrint("3gfasddas", "riv.log", 1);
        status = H5Dread(dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, pool.data());
writeToLogAndPrint("sdsaadssadgf", "riv.log", 1);
        atrib_id = H5Aopen(dset_id, "version_major", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &version_major );
        status = H5Aclose(atrib_id);
writeToLogAndPrint("aaaaaaaaaagf", "riv.log", 1);
        atrib_id = H5Aopen(dset_id, "version_minor", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &version_minor );
        status = H5Aclose(atrib_id);
writeToLogAndPrint("as", "riv.log", 1);
        size_t hist_norm_len;
        atrib_id = H5Aopen(dset_id, "hist_norm_len", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &hist_norm_len );
        status = H5Aclose(atrib_id);
writeToLogAndPrint("ass", "riv.log", 1);
        size_t hist_log_len;
        atrib_id = H5Aopen(dset_id, "hist_log_len", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_ULONG, &hist_log_len );
        status = H5Aclose(atrib_id);
writeToLogAndPrint("hist_norm_len hist_log_len "+QString::number(hist_norm_len)+" "+QString::number(hist_log_len), "riv.log", 1);
        //~ std::cout << "Line " << __LINE__  << hist_norm_len <<" " << hist_log_len<< std::endl;
        data_histogram.reserve(hist_norm_len);
        data_histogram_log.reserve(hist_log_len);
writeToLogAndPrint("aas", "riv.log", 1);
        atrib_id = H5Aopen(dset_id, "data_histogram", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_DOUBLE, data_histogram.data() );
        status = H5Aclose(atrib_id);
//~ //~
        atrib_id = H5Aopen(dset_id, "data_histogram_log", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_DOUBLE, data_histogram_log.data() );
        status = H5Aclose(atrib_id);
//~ //~
        atrib_id = H5Aopen(dset_id, "minmax", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_DOUBLE, minmax.data() );
        status = H5Aclose(atrib_id);
//~ //~
//~ //~
        MiniArray<float> tmp(8);
        atrib_id = H5Aopen(dset_id, "extent", H5P_DEFAULT);
        status = H5Aread(atrib_id, H5T_NATIVE_FLOAT, tmp.data() );
        status = H5Aclose(atrib_id);
writeToLogAndPrint("asdada", "riv.log", 1);
        for (int i = 0; i < 8; i++)
        {
            extent[i] = tmp[i];
        }
        //~ std::cout << "Line "<< __LINE__ <<  " n_nodes " <<n_nodes << std::endl;
        index.reserve(n_nodes);
        brick.reserve(n_nodes);
//~ //~
        status = H5Dclose(dset_id);
//~ //~
//~ //~
        // Get octtree data
        dset_id = H5Dopen(file_id, "oct_index", H5P_DEFAULT);
//~ //~
        status = H5Dread(dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, index.data());
        status = H5Dclose(dset_id);
//~ //~
        dset_id = H5Dopen(file_id, "oct_brick", H5P_DEFAULT);
        status = H5Dread(dset_id, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, brick.data());
        status = H5Dclose(dset_id);
//~
        //~ //~for (int i = 0; i < n_nodes; i++)
        //~ //~{
            //~ //~unsigned int mask_brick_id_x = ((1 << 10) - 1) << 20;
            //~ //~unsigned int mask_brick_id_y = ((1 << 10) - 1) << 10;
            //~ //~unsigned int mask_brick_id_z = ((1 << 10) - 1) << 0;
//~ //~
            //~ //~unsigned int val = brick[i];
            //~ //~std::cout << ((val & mask_brick_id_x) >> 20 ) <<" " <<  ((val & mask_brick_id_y) >> 10) <<" " << (val & mask_brick_id_z) << std::endl;
        //~ //~}
//~
        //~ /* Close the file */
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
