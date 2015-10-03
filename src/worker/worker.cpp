#include "worker.h"

/* C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <limits>

//#include <QtGlobal>
#include <QCoreApplication> // Remove?

#include <CL/opencl.h>

/* QT */
#include <QTimer>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDateTime>


static const size_t BRICK_POOL_SOFT_MAX_BYTES = 0.7e9; // Effectively limited by the max allocation size for global memory if the pool resides on the GPU during pool construction. 3D image can be used with OpenCL 1.2, allowing you to use the entire VRAM.
static const size_t MAX_POINTS_PER_CLUSTER = 10000000;
static const size_t MAX_NODES_PER_CLUSTER = 20000;


// ASCII from http://patorjk.com/software/taag/#p=display&c=c&f=Trek&t=Base%20Class
/***
 *        dBBBBb dBBBBBb  .dBBBBP   dBBBP     dBBBP  dBP dBBBBBb  .dBBBBP.dBBBBP
 *           dBP      BB  BP                                  BB  BP     BP
 *       dBBBK'   dBP BB  `BBBBb  dBBP      dBP    dBP    dBP BB  `BBBBb `BBBBb
 *      dB' db   dBP  BB     dBP dBP       dBP    dBP    dBP  BB     dBP    dBP
 *     dBBBBP'  dBBBBBBBdBBBBP' dBBBBP    dBBBBP dBBBBP dBBBBBBBdBBBBP'dBBBBP'
 *
 */
BaseWorker::BaseWorker()
    : isCLInitialized(false)
{
    initializeOpenCLFunctions();
}

BaseWorker::~BaseWorker()
{

}

void BaseWorker::setSVOFile(SparseVoxelOctreeOld * svo)
{

    this->svo = svo;
}

void BaseWorker::setOffsetOmega(double value)
{
    offset_omega = value * pi / 180.0;
}
void BaseWorker::setOffsetKappa(double value)
{
    offset_kappa = value * pi / 180.0;
}
void BaseWorker::setOffsetPhi(double value)
{
    offset_phi = value * pi / 180.0;
}

void BaseWorker::setActiveAngle(int value)
{
    active_angle = value;
}

void BaseWorker::killProcess()
{

    kill_flag = true;
}

void BaseWorker::setQSpaceInfo(float suggested_search_radius_low, float suggested_search_radius_high, float suggested_q)
{

    this->suggested_search_radius_low = suggested_search_radius_low;
    this->suggested_search_radius_high = suggested_search_radius_high;
    this->suggested_q = suggested_q;
}


void BaseWorker::setReducedPixels(Matrix<float> * reduced_pixels)
{

    this->reduced_pixels = reduced_pixels;
}

void BaseWorker::setSet(SeriesSet set)
{
    this->set = set;
}


/***
 *      dBP dP  dBBBBP`Bb  .BP    dBBBP  dBP    dBP dBBBBBP    dBBBP
 *             dBP.BP     .BP
 *     dB .BP dBP.BP    dBBK    dBBP   dBP    dBP     dBP    dBBP
 *     BB.BP dBP.BP    dB'     dBP    dBP    dBP     dBP    dBP
 *     BBBP dBBBBP    dB' dBP dBBBBP dBBBBP dBP     dBBBBP dBBBBP
 *
 */

VoxelizeWorker::VoxelizeWorker()
{
    isCLInitialized = false;
    initializeOpenCLFunctions();
}

VoxelizeWorker::~VoxelizeWorker()
{
    if (isCLInitialized)
    {
        err = QOpenCLReleaseKernel(voxelize_kernel);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }
}

unsigned int VoxelizeWorker::getOctIndex(unsigned int msdFlag, unsigned int dataFlag, unsigned int child)
{
    return (msdFlag << 31) | (dataFlag << 30) | child;
}

unsigned int VoxelizeWorker::getOctBrick(unsigned int poolX, unsigned int poolY, unsigned int poolZ)
{
    return (poolX << 20) | (poolY << 10) | (poolZ << 0);
}

void VoxelizeWorker::initializeCLKernel()
{
    //    context_cl = new OpenCLContext;
    context_cl.initDevices();
    context_cl.initNormalContext();
    context_cl.initCommandQueue();

    QStringList paths;
    paths << "kernels/voxelize.cl";

    context_cl.createProgram(paths, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    context_cl.buildProgram("-Werror -cl-std=CL1.2");


    // Kernel handles
    voxelize_kernel = QOpenCLCreateKernel(context_cl.program(), "voxelize", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    fill_kernel = QOpenCLCreateKernel(context_cl.program(), "fill", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    isCLInitialized = true;
}

void VoxelizeWorker::process()
{
    // Note: The term node and brick are used a lot. A brick is a 3D voxel grid of data points, and each node has one brick and some extra metadata.

    QCoreApplication::processEvents();

    kill_flag = false;

    if (reduced_pixels->size() <= 0)
    {
        QString str("\n[" + QString(this->metaObject()->className()) + "] Warning: No data available!");
        emit message(str);
        kill_flag = true;
    }



    if (!kill_flag)
    {
        QElapsedTimer totaltime;
        totaltime.start();

        // Emit to appropriate slots
        emit changedFormatGenericProgress(QString(" Creating Interpolation Data Structure: %p%"));
        emit changedRangeGenericProcess(0, 100);
        emit progressTaskActive(true);

        emit message("\n[" + QString(this->metaObject()->className()) + "] Generating Sparse Voxel Octree " + QString::number(svo->levels()) + " levels deep.");
        emit message("\n[" + QString(this->metaObject()->className()) + "] The source data is " + QString::number(reduced_pixels->bytes() / 1000000.0, 'g', 3) + " MB");

        // The number of data points in a single brick
        size_t n_points_brick = svo->brickOuterDimension() * svo->brickOuterDimension() * svo->brickOuterDimension();

        // Save the extent of the volume
        svo->setExtent(suggested_q);

        // Save initial metadata (just text)
        svo->setMetaData("You can write notes about the dataset here.");

        // Prepare the brick pool in which we will store bricks
        Matrix<int> pool_dimension(1, 4, 0);
        pool_dimension[0] = (1 << svo->brickPoolPower()) * svo->brickOuterDimension();
        pool_dimension[1] = (1 << svo->brickPoolPower()) * svo->brickOuterDimension();
        pool_dimension[2] = (BRICK_POOL_SOFT_MAX_BYTES / (sizeof(cl_float) * svo->brickOuterDimension() * svo->brickOuterDimension() * svo->brickOuterDimension())) / ((1 << svo->brickPoolPower()) * (1 << svo->brickPoolPower()));

        if (pool_dimension[2] < 1)
        {
            pool_dimension[2] = 1;
        }

        pool_dimension[2] *= svo->brickOuterDimension();

        size_t BRICK_POOL_HARD_MAX_BYTES = pool_dimension[0] * pool_dimension[1] * pool_dimension[2] * sizeof(float);

        emit changedFormatMemoryUsage(QString("Mem usage: %p% (%v of %m MB)"));
        emit changedRangeMemoryUsage(0, BRICK_POOL_HARD_MAX_BYTES / 1e6);
        emit changedMemoryUsage(0);

        size_t n_max_bricks = BRICK_POOL_HARD_MAX_BYTES / (n_points_brick * sizeof(float));

        // Prepare the relevant OpenCL buffers
        cl_mem pool_cl = QOpenCLCreateBuffer(context_cl.context(),
                                             CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                             BRICK_POOL_HARD_MAX_BYTES,
                                             NULL,
                                             &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        cl_mem pool_cluster_cl = QOpenCLCreateBuffer(context_cl.context(),
                                 CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                 MAX_NODES_PER_CLUSTER * svo->brickOuterDimension() * svo->brickOuterDimension() * svo->brickOuterDimension() * sizeof(float),
                                 NULL,
                                 &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        cl_mem brick_extent_cl = QOpenCLCreateBuffer(context_cl.context(),
                                 CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                 MAX_NODES_PER_CLUSTER * 6 * sizeof(float),
                                 NULL,
                                 &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        cl_mem point_data_cl = QOpenCLCreateBuffer(context_cl.context(),
                               CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                               MAX_POINTS_PER_CLUSTER * sizeof(cl_float4),
                               NULL,
                               &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        cl_mem point_data_offset_cl = QOpenCLCreateBuffer(context_cl.context(),
                                      CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                      MAX_NODES_PER_CLUSTER * sizeof(cl_int),
                                      NULL,
                                      &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        cl_mem point_data_count_cl = QOpenCLCreateBuffer(context_cl.context(),
                                     CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                     MAX_NODES_PER_CLUSTER * sizeof(cl_int),
                                     NULL,
                                     &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        Matrix<float> min_check(1, MAX_NODES_PER_CLUSTER, 0);
        cl_mem min_check_cl = QOpenCLCreateBuffer(context_cl.context(),
                              CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                              MAX_NODES_PER_CLUSTER * sizeof(cl_float),
                              min_check.data(),
                              &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        Matrix<float> sum_check(1, MAX_NODES_PER_CLUSTER, 0);
        cl_mem sum_check_cl = QOpenCLCreateBuffer(context_cl.context(),
                              CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                              MAX_NODES_PER_CLUSTER * sizeof(cl_float),
                              sum_check.data(),
                              &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        Matrix<float> variance_check(1, MAX_NODES_PER_CLUSTER, 0);
        cl_mem variance_check_cl = QOpenCLCreateBuffer(context_cl.context(),
                                   CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                   MAX_NODES_PER_CLUSTER * sizeof(cl_float),
                                   variance_check.data(),
                                   &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        // Place all data points in an octree data structure from which to construct the bricks in the brick pool
        SearchNode root();//NULL, svo->extent().data());

//        for (size_t i = 0; i < reduced_pixels->size() / 4; i++)
//        {
//            if (kill_flag)
//            {
//                emit message("\n[" + QString(this->metaObject()->className()) + "] Warning: Process killed at iteration " + QString::number(i + 1) + " of " + QString::number(reduced_pixels->size() / 4));
//                break;
//            }

//            root.insert(reduced_pixels->data() + i * 4);
//            emit changedGenericProgress((i + 1) * 100 / (reduced_pixels->size() / 4));
//        }

        if (!kill_flag)
        {
            /* Create an octree from brick data. The nodes are maintained in a linear array rather than a tree.
             * This is mainly due to (current) lack of proper support for recursion on GPUs */
            Matrix<BrickNode> gpuHelpOctree(1, n_max_bricks * 16);
            gpuHelpOctree[0].setParent(0);

            // An array to store number of nodes per level
            Matrix<unsigned int> nodes;
            nodes.set(1, 16, (unsigned int) 0); // Note: make bigger
            nodes[0] = 1;

            unsigned int nodes_prev_lvls = 0, non_empty_node_counter = 0;

            QElapsedTimer timer;

            // Keep track of the maximum sum returned by a node and use it later to estimate max value in data set
            float max_brick_sum = 0.0;

            // Containers
            Matrix<double> brick_extent(1, 6 * MAX_NODES_PER_CLUSTER); // The extent of each brick in a kernel invocation
            Matrix<int> point_data_offset(1, MAX_NODES_PER_CLUSTER); // The data offset for each brick in a kernel invocation
            Matrix<int> point_data_count(1, MAX_NODES_PER_CLUSTER); // The data size for each brick in a kernel invocation
            Matrix<float> point_data(MAX_POINTS_PER_CLUSTER, 4); // Temporaily holds the data for a single brick



            // Cycle through the levels
            for (size_t lvl = 0; lvl < svo->levels(); lvl++)
            {
                emit message("\n[" + QString(this->metaObject()->className()) + "] Constructing Level " + QString::number(lvl + 1) + " (dim: " + QString::number(svo->brickInnerDimension() * (1 <<  lvl)) + ")");
                emit changedFormatGenericProgress("Constructing Level " + QString::number(lvl + 1) + " (dim: " + QString::number(svo->brickInnerDimension() * (1 <<  lvl)) + "): %p%");

                timer.start();

                // Find the correct range search radius, which will be smaller for each level until it approaches the distance between samples in the data set
                float search_radius = sqrt(3.0f) * 0.5f * ((svo->extent().at(1) - svo->extent().at(0)) / (svo->brickInnerDimension() * (1 << lvl)));

                if (search_radius < suggested_search_radius_high)
                {
                    search_radius = suggested_search_radius_high;
                }

                double tmp = (svo->extent().at(1) - svo->extent().at(0)) / (1 << lvl);

                size_t n_nodes_treated = 0;

                // For each cluster of nodes
                while (n_nodes_treated <= nodes[lvl])
                {
                    if ((non_empty_node_counter + 1) >= n_max_bricks)
                    {
                        QString str("\n[" + QString(this->metaObject()->className()) + "] Warning: Process killed due to memory overflow. The dataset has grown too large! (" + QString::number(non_empty_node_counter * n_points_brick * sizeof(cl_float) / 1e6, 'g', 3) + " MB)");
                        emit message(str);
                        kill_flag = true;
                    }

                    if (kill_flag)
                    {
                        break;
                    }

                    // First pass: find relevant data for each brick in the node cluster
                    unsigned int currentId;
                    size_t n_points_harvested = 0; // The number of xyzi data points gathered
                    size_t n_nodes_treated_in_cluster = 0; // The number of nodes treated in this iteration of the enclosing while loop

                    while (n_points_harvested < MAX_POINTS_PER_CLUSTER)
                    {
                        // The id of the octnode in the octnode array
                        currentId = nodes_prev_lvls + n_nodes_treated + n_nodes_treated_in_cluster;

                        // Set the level
                        gpuHelpOctree[currentId].setLevel(lvl);

                        // Set the brick id
                        gpuHelpOctree[currentId].calcBrickId((n_nodes_treated + n_nodes_treated_in_cluster) % 8 , &gpuHelpOctree[gpuHelpOctree[currentId].getParent()]);

                        // Based on brick id calculate the brick extent
                        unsigned int * brick_id = gpuHelpOctree[currentId].getBrickId();

                        brick_extent[n_nodes_treated_in_cluster * 6 + 0] = svo->extent().at(0) + tmp * brick_id[0];
                        brick_extent[n_nodes_treated_in_cluster * 6 + 1] = svo->extent().at(0) + tmp * (brick_id[0] + 1);
                        brick_extent[n_nodes_treated_in_cluster * 6 + 2] = svo->extent().at(2) + tmp * brick_id[1];
                        brick_extent[n_nodes_treated_in_cluster * 6 + 3] = svo->extent().at(2) + tmp * (brick_id[1] + 1);
                        brick_extent[n_nodes_treated_in_cluster * 6 + 4] = svo->extent().at(4) + tmp * brick_id[2];
                        brick_extent[n_nodes_treated_in_cluster * 6 + 5] = svo->extent().at(4) + tmp * (brick_id[2] + 1);

                        // Offset of points accumulated thus far
                        point_data_offset[n_nodes_treated_in_cluster] = n_points_harvested;
                        size_t premature_termination = 0;

                        // Get point data needed for this brick
//                        if ( root.getData(
//                                    MAX_POINTS_PER_CLUSTER,
//                                    brick_extent.data() + n_nodes_treated_in_cluster * 6,
//                                    point_data.data(),
//                                    &n_points_harvested,
//                                    search_radius))
//                        {
//                            premature_termination = n_nodes_treated_in_cluster;

//                            qDebug() << lvl + 1 << premature_termination << "of" << MAX_NODES_PER_CLUSTER << "pts:" << n_points_harvested;
//                        }

                        // Number of points for this node
                        point_data_count[n_nodes_treated_in_cluster] = n_points_harvested - point_data_offset[n_nodes_treated_in_cluster];

                        // Upload this point data to an OpenCL buffer
                        if (point_data_count[n_nodes_treated_in_cluster] > 0)
                        {
                            err = QOpenCLEnqueueWriteBuffer(context_cl.queue(),
                                                            point_data_cl ,
                                                            CL_TRUE,
                                                            point_data_offset[n_nodes_treated_in_cluster] * sizeof(cl_float4),
                                                            point_data_count[n_nodes_treated_in_cluster] * sizeof(cl_float4),
                                                            point_data.data() + point_data_offset[n_nodes_treated_in_cluster] * 4, // ?? Redundant storage? At least move outisde of loop?
                                                            0, NULL, NULL);

                            if ( err != CL_SUCCESS)
                            {
                                qFatal(cl_error_cstring(err));
                            }
                        }

                        n_nodes_treated_in_cluster++;


                        // Break off loop when all nodes are processed
                        if ((n_nodes_treated + n_nodes_treated_in_cluster >= nodes[lvl]) || (premature_termination) || (n_nodes_treated_in_cluster >= MAX_NODES_PER_CLUSTER) )
                        {
                            break;
                        }
                    }

                    // The extent of each brick
                    err = QOpenCLEnqueueWriteBuffer(
                              context_cl.queue(),
                              brick_extent_cl ,
                              CL_TRUE,
                              0,
                              brick_extent.toFloat().bytes(),
                              brick_extent.toFloat().data(),
                              0, NULL, NULL);

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }

                    // The data offset for each brick
                    err = QOpenCLEnqueueWriteBuffer(context_cl.queue(),
                                                    point_data_offset_cl ,
                                                    CL_TRUE,
                                                    0,
                                                    point_data_offset.bytes(),
                                                    point_data_offset.data(),
                                                    0, NULL, NULL);

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }

                    // The data size for each brick
                    err = QOpenCLEnqueueWriteBuffer(context_cl.queue(),
                                                    point_data_count_cl ,
                                                    CL_TRUE,
                                                    0,
                                                    point_data_count.bytes(),
                                                    point_data_count.data(),
                                                    0, NULL, NULL);

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }


                    // Second pass: calculate the data for each node in the cluster (OpenCL)
                    // Set kernel arguments
                    err = QOpenCLSetKernelArg( voxelize_kernel, 0, sizeof(cl_mem), (void *) &point_data_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 1, sizeof(cl_mem), (void *) &point_data_offset_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 2, sizeof(cl_mem), (void *) &point_data_count_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 3, sizeof(cl_mem), (void *) &brick_extent_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 4, sizeof(cl_mem), (void *) &pool_cluster_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 5, sizeof(cl_mem), (void *) &min_check_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 6, sizeof(cl_mem), (void *) &sum_check_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 7, sizeof(cl_mem), (void *) &variance_check_cl);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 8, svo->brickOuterDimension() * svo->brickOuterDimension() * svo->brickOuterDimension() * sizeof(cl_float), NULL);
                    int tmp = svo->brickOuterDimension(); // why a separate variable?
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 9, sizeof(cl_int), &tmp);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 10, sizeof(cl_float), &search_radius);
                    err |= QOpenCLSetKernelArg( voxelize_kernel, 11, sizeof(cl_float), &suggested_search_radius_high);

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }

                    err = QOpenCLFinish(context_cl.queue());

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }

                    // Interpolate data for each brick
                    for (size_t j = 0; j < n_nodes_treated_in_cluster; j++)
                    {
                        // Launch kernel
                        size_t glb_offset[3] = {0, 0, 8 * j};
                        size_t loc_ws[3] = {8, 8, 8};
                        size_t glb_ws[3] = {8, 8, 8};
                        err = QOpenCLEnqueueNDRangeKernel(
                                  context_cl.queue(),
                                  voxelize_kernel,
                                  3,
                                  glb_offset,
                                  glb_ws,
                                  loc_ws,
                                  0, NULL, NULL);

                        if ( err != CL_SUCCESS)
                        {
                            qDebug() << j << "of" << n_nodes_treated_in_cluster << tmp << search_radius;
                            qFatal(cl_error_cstring(err));
                        }

                        //                        if (i + j + 1 >= nodes[lvl]) break;
                    }

                    err = QOpenCLFinish(context_cl.queue());

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }

                    // The minimum value data point in each brick
                    err = QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                                     min_check_cl,
                                                     CL_TRUE,
                                                     0,
                                                     MAX_NODES_PER_CLUSTER * sizeof(float),
                                                     min_check.data(),
                                                     0, NULL, NULL);

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }

                    // The sum of data points in each brick
                    err = QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                                     sum_check_cl,
                                                     CL_TRUE,
                                                     0,
                                                     MAX_NODES_PER_CLUSTER * sizeof(float),
                                                     sum_check.data(),
                                                     0, NULL, NULL);

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }

                    // The variance of data points in each brick
                    err = QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                                     variance_check_cl,
                                                     CL_TRUE,
                                                     0,
                                                     MAX_NODES_PER_CLUSTER * sizeof(float),
                                                     variance_check.data(),
                                                     0, NULL, NULL);

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }


                    err = QOpenCLFinish(context_cl.queue());

                    if ( err != CL_SUCCESS)
                    {
                        qFatal(cl_error_cstring(err));
                    }


                    // Third pass: transfer non-empty nodes to svo data structure (OpenCL)
                    for (size_t j = 0; j < n_nodes_treated_in_cluster; j++)
                    {
                        // The id of the octnode in the octnode array
                        currentId = nodes_prev_lvls + n_nodes_treated + j;

                        // If a node has no relevant data
                        if ((sum_check[j] <= 0.0))
                        {
                            gpuHelpOctree[currentId].setDataFlag(0);
                            gpuHelpOctree[currentId].setMsdFlag(1);
                            gpuHelpOctree[currentId].setChild(0);
                        }
                        // Else a node has data and possibly qualifies for children
                        else
                        {
                            if ((non_empty_node_counter + 1)*n_points_brick * sizeof(float) >= BRICK_POOL_HARD_MAX_BYTES)
                            {
                                emit popup(QString("Warning - Data Overflow"), QString("The dataset you are trying to create grew too large, and exceeded the limit of " + QString::number(BRICK_POOL_HARD_MAX_BYTES / 1e6) + " MB. The issue can be remedied by applying more stringent reconstruction parameters or by reducing the octree level."));
                                kill_flag = true;
                                break;
                            }

                            gpuHelpOctree[currentId].setDataFlag(1);
                            gpuHelpOctree[currentId].setMsdFlag(0);

                            // Set maximum subdivision if the max level is reached or if the variance of the brick data is small compared to the average.
                            float average = sum_check[j] / (float)n_points_brick;
                            float std_dev = sqrt(variance_check[j]);

                            if ((lvl >= svo->levels() - 1) || // Max level
                                    ((std_dev <= 0.5 * average) && (min_check[j] > 0) && (svo->levels() - lvl < 3 )) || // Voxel data is self-similar and all voxels are non-zero
                                    ((std_dev <= 0.2 * average) && (svo->levels() - lvl < 3 ))) // Voxel data is self-similar
                            {
                                //                                qDebug() << "Terminated brick early at lvl" << lvl << "Average:" << sum_check[j]/(float)n_points_brick << "Variance:" << variance_check[j];
                                gpuHelpOctree[currentId].setMsdFlag(1);
                            }

                            // Set the pool id of the brick corresponding to the node
                            gpuHelpOctree[currentId].calcPoolId(svo->brickPoolPower(), non_empty_node_counter);

                            // Find the max sum of a brick
                            if (sum_check[j] > max_brick_sum)
                            {
                                max_brick_sum = sum_check[j];
                            }


                            // Transfer brick data to pool
                            err = QOpenCLSetKernelArg( fill_kernel, 0, sizeof(cl_mem), (void *) &pool_cluster_cl);
                            err |= QOpenCLSetKernelArg( fill_kernel, 1, sizeof(cl_mem), (void *) &pool_cl);
                            err |= QOpenCLSetKernelArg( fill_kernel, 2, sizeof(cl_int4), pool_dimension.data());
                            int tmp = svo->brickOuterDimension(); // ?
                            err |= QOpenCLSetKernelArg( fill_kernel, 3, sizeof(cl_uint), &tmp);
                            err |= QOpenCLSetKernelArg( fill_kernel, 4, sizeof(cl_uint), &non_empty_node_counter);

                            if ( err != CL_SUCCESS)
                            {
                                qFatal(cl_error_cstring(err));
                            }

                            // Write data to the brick pool
                            size_t glb_offset[3] = {0, 0, 8 * j};
                            size_t loc_ws[3] = {8, 8, 8};
                            size_t glb_ws[3] = {8, 8, 8};
                            err = QOpenCLEnqueueNDRangeKernel(
                                      context_cl.queue(),
                                      fill_kernel,
                                      3,
                                      glb_offset,
                                      glb_ws,
                                      loc_ws,
                                      0, NULL, NULL);

                            if ( err != CL_SUCCESS)
                            {
                                qFatal(cl_error_cstring(err));
                            }

                            err = QOpenCLFinish(context_cl.queue());

                            if ( err != CL_SUCCESS)
                            {
                                qFatal(cl_error_cstring(err));
                            }

                            non_empty_node_counter++;

                            // Account for children
                            if (!gpuHelpOctree[currentId].getMsdFlag())
                            {
                                unsigned int childId = nodes_prev_lvls + nodes[lvl] + nodes[lvl + 1];
                                gpuHelpOctree[currentId].setChild(childId); // Index points to first child only

                                // For each child
                                for (size_t k = 0; k < 8; k++)
                                {
                                    gpuHelpOctree[childId + k].setParent(currentId);
                                    nodes[lvl + 1]++;
                                }
                            }
                        }

                        //                        if (i + j + 1 >= nodes[lvl]) break;
                    }

                    n_nodes_treated += n_nodes_treated_in_cluster;

                    emit changedMemoryUsage(non_empty_node_counter * n_points_brick * sizeof(float) / 1e6);

                    emit changedGenericProgress((n_nodes_treated + 1) * 100 / nodes[lvl]);
                }

                if (kill_flag)
                {

                    QString str("\n[" + QString(this->metaObject()->className()) + "] Warning: Process killed at iteration " + QString::number(lvl + 1) + " of " + QString::number(svo->levels()) + "!");

                    emit message(str);
                    break;
                }

                nodes_prev_lvls += nodes[lvl];

                size_t t = timer.restart();
                emit message(" ...done (" + QString::number(t) + " ms, " + QString::number(nodes[lvl]) + " nodes)");
            }

            if (!kill_flag)
            {
                // Use the node structure to populate the GPU arrays
                emit changedFormatGenericProgress(QString(" Transforming: %p%"));
                svo->index()->reserve(1, nodes_prev_lvls);
                svo->brick()->reserve(1, nodes_prev_lvls);

                for (size_t i = 0; i < nodes_prev_lvls; i++)
                {
                    (*svo->index())[i] = getOctIndex(gpuHelpOctree[i].getMsdFlag(), gpuHelpOctree[i].getDataFlag(), gpuHelpOctree[i].getChild());
                    (*svo->brick())[i] = getOctBrick(gpuHelpOctree[i].getPoolId()[0], gpuHelpOctree[i].getPoolId()[1], gpuHelpOctree[i].getPoolId()[2]);

                    emit changedGenericProgress((i + 1) * 100 / nodes_prev_lvls);
                }

                // Round up to the lowest number of bricks that is multiple of the brick pool dimensions. Use this value to reserve data for the data pool
                unsigned int non_empty_node_counter_rounded_up = non_empty_node_counter + ((pool_dimension[0] * pool_dimension[1] / (svo->brickOuterDimension() * svo->brickOuterDimension())) - (non_empty_node_counter % (pool_dimension[0] * pool_dimension[1] / (svo->brickOuterDimension() * svo->brickOuterDimension()))));

                // Read results
                svo->pool()->reserve(1, non_empty_node_counter_rounded_up * n_points_brick);

                svo->setMin(0.0f);
                svo->setMax(max_brick_sum / (float)(n_points_brick));

                err = QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                                 pool_cl,
                                                 CL_TRUE,
                                                 0,
                                                 non_empty_node_counter_rounded_up * n_points_brick * sizeof(float),
                                                 svo->pool()->data(),
                                                 0, NULL, NULL);

                if ( err != CL_SUCCESS)
                {
                    qFatal(cl_error_cstring(err));
                }
            }

            if (!kill_flag)
            {
                emit message("\n[" + QString(this->metaObject()->className()) + "] Done (" + QString::number(totaltime.elapsed()) + " ms).\nThe dataset consists of " + QString::number(nodes_prev_lvls) + " bricks and is approx " + QString::number((svo->bytes()) / 1e6, 'g', 3) + " MB\nThe dataset can now be saved");

            }
        }

        emit progressTaskActive(false);

        err = QOpenCLReleaseMemObject(point_data_cl);
        err |= QOpenCLReleaseMemObject(point_data_offset_cl);
        err |= QOpenCLReleaseMemObject(point_data_count_cl);
        err |= QOpenCLReleaseMemObject(brick_extent_cl);
        err |= QOpenCLReleaseMemObject(pool_cluster_cl);
        err |= QOpenCLReleaseMemObject(pool_cl);
        err |= QOpenCLReleaseMemObject(min_check_cl);
        err |= QOpenCLReleaseMemObject(sum_check_cl);
        err |= QOpenCLReleaseMemObject(variance_check_cl);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

    emit finished();
}

