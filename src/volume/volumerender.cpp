﻿#include "volumerender.h"

/* Useful C++ libs */
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

#include <CL/opencl.h>

/* QT */
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QPolygonF>
#include <QScreen>
#include <QPainter>
#include <QOpenGLFramebufferObject>
#include <QCoreApplication>
#include <QOpenGLPaintDevice>


VolumeWorker::VolumeWorker() :
    p_surface_ab_res(128),
    p_surface_c_res(1024),
    p_line_ab_res(128),
    p_line_c_res(1024)
{
    initializeOpenCLFunctions();
}

VolumeWorker::~VolumeWorker()
{

}

void VolumeWorker::setOpenCLContext(OpenCLContextQueueProgram *context)
{
    context_cl = context;

    initializeOpenCLKernels();
}

void VolumeWorker::initializeOpenCLKernels()
{
    // Build programs from OpenCL kernel source
//    QStringList paths;


//    context_cl->createProgram(paths, &err);

//    if ( err != CL_SUCCESS)
//    {
//        qFatal(cl_error_cstring(err));
//    }

//    context_cl->buildProgram("-Werror -cl-std=CL1.2");

    // Kernel handles
    p_line_integral_kernel =  QOpenCLCreateKernel(context_cl->program(), "integrateLine", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    p_plane_integral_kernel =  QOpenCLCreateKernel(context_cl->program(), "integratePlane", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    p_weightpoint_kernel =  QOpenCLCreateKernel(context_cl->program(), "weightpointVolumetric", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }
}

void VolumeWorker::saveSurfaceAsText(QString path)
{
    if (path != "")
    {
        QFile file(path);

        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream stream(&file);
            stream << p_line;

            for (int i = 0; i < p_surface_data.m(); i++)
            {
                for (int j = 0; j < p_surface_data.n(); j++)
                {
                    stream << QString::number(p_surface_data[i * p_surface_data.n() + j]) << "\t";
                }

                stream << "\n";
            }

            file.close();
        }
    }
}

void VolumeWorker::saveLineAsText(QString path)
{
    if (path != "")
    {
        QFile file(path);

        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream stream(&file);
            stream << p_line;

            for (int i = 0; i < p_line_data_x.size(); i++)
            {
                stream << QString::number(p_line_data_x[i]) << "\t" << QString::number(p_line_data_y[i]) << "\n";
            }

            file.close();
        }
    }
}

Matrix<double> VolumeWorker::getLineIntegralDataX()
{
    return p_line_data_x.toDouble();
}

Matrix<double> VolumeWorker::getLineIntegralDataY()
{
    return p_line_data_y.toDouble();
}

Matrix<double> VolumeWorker::getPlaneIntegralData()
{
    return p_surface_data.toDouble();
}

double VolumeWorker::getLineIntegralXmin()
{
    return p_line_integral_xmin;
}

double VolumeWorker::getLineIntegralXmax()
{
    return p_line_integral_xmax;
}

double VolumeWorker::getLineIntegralYmin()
{
    return p_line_integral_ymin;
}

double VolumeWorker::getLineIntegralYmax()
{
    return p_line_integral_ymax;
}

void VolumeWorker::setKernel(cl_kernel kernel)
{
    p_raytrace_kernel = kernel;
}

void VolumeWorker::raytrace(Matrix<size_t> ray_glb_ws, Matrix<size_t> ray_loc_ws)
{
    // Launch rendering kernel
    Matrix<size_t> area_per_call(1, 2);
    area_per_call[0] = 128;
    area_per_call[1] = 128;

    Matrix<size_t> call_offset(1, 2);
    call_offset[0] = 0;
    call_offset[1] = 0;

    // Launch the kernel
    for (size_t glb_x = 0; glb_x < ray_glb_ws[0]; glb_x += area_per_call[0])
    {
        for (size_t glb_y = 0; glb_y < ray_glb_ws[1]; glb_y += area_per_call[1])
        {
            call_offset[0] = glb_x;
            call_offset[1] = glb_y;

            err = QOpenCLEnqueueNDRangeKernel(context_cl->queue(), p_raytrace_kernel, 2, call_offset.data(), area_per_call.data(), ray_loc_ws.data(), 0, NULL, NULL);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }
        }
    }

    err = QOpenCLFinish(context_cl->queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    emit rayTraceFinished();
}

void VolumeWorker::setCLObjects(cl_mem * pool,
                                cl_sampler * pool_sampler,
                                cl_mem * oct_index,
                                cl_mem * oct_brick,
                                cl_mem * data_extent,
                                cl_mem * data_view_extent,
                                cl_mem * misc_int)
{
    p_pool = pool;
    p_pool_sampler = pool_sampler;
    p_oct_index = oct_index;
    p_oct_brick = oct_brick;
    p_data_extent = data_extent;
    p_data_view_extent = data_view_extent;
    p_misc_int = misc_int;
}

void VolumeWorker::setSurfaceABRes(int value)
{
    p_surface_ab_res = value;
}
void VolumeWorker::setSurfaceCRes(int value)
{
    p_surface_c_res = value;
}
void VolumeWorker::setLineABRes(int value)
{
    p_line_ab_res = value;
}
void VolumeWorker::setLineCRes(int value)
{
    p_line_c_res = value;
}

void VolumeWorker::resolveWeightpoint()
{
    Matrix<size_t> loc_ws(1, 3, 8);
    Matrix<size_t> glb_ws(1, 3, 128);

    Matrix<float> result(1, 4 * (glb_ws[0] / loc_ws[0]) * (glb_ws[1] / loc_ws[1]) * (glb_ws[2] / loc_ws[2]));

    cl_mem result_cl = QOpenCLCreateBuffer(context_cl->context(),
                                           CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                           result.bytes(),
                                           result.data(), &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Kernel arguments
    err = QOpenCLSetKernelArg(p_weightpoint_kernel, 0, sizeof(cl_mem), (void *) p_pool);
    err |= QOpenCLSetKernelArg(p_weightpoint_kernel, 1, sizeof(cl_sampler), p_pool_sampler);
    err |= QOpenCLSetKernelArg(p_weightpoint_kernel, 2, sizeof(cl_mem), (void *) p_oct_index);
    err |= QOpenCLSetKernelArg(p_weightpoint_kernel, 3, sizeof(cl_mem), (void *) p_oct_brick);
    err |= QOpenCLSetKernelArg(p_weightpoint_kernel, 4, sizeof(cl_mem), (void *) p_data_extent);
    err |= QOpenCLSetKernelArg(p_weightpoint_kernel, 5, sizeof(cl_mem), (void *) p_data_view_extent);
    err |= QOpenCLSetKernelArg(p_weightpoint_kernel, 6, sizeof(cl_mem), (void *) p_misc_int);
    err |= QOpenCLSetKernelArg(p_weightpoint_kernel, 7, sizeof(cl_mem), (void *) &result_cl);
    err |= QOpenCLSetKernelArg(p_weightpoint_kernel, 8, 4 * sizeof(cl_float) * loc_ws[0] * loc_ws[1] * loc_ws[2], NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueNDRangeKernel(context_cl->queue(), p_weightpoint_kernel, 3, NULL, glb_ws.data(), loc_ws.data(), 0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLFinish(context_cl->queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueReadBuffer ( context_cl->queue(),
                                     result_cl,
                                     CL_TRUE,
                                     0,
                                     result.bytes(),
                                     result.data(),
                                     0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLReleaseMemObject(result_cl);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    double xi = result.sum(0, result.size() * 1 / 4);
    double yi = result.sum(result.size() * 1 / 4, result.size() * 2 / 4);
    double zi = result.sum(result.size() * 2 / 4, result.size() * 3 / 4);
    double i = result.sum(result.size() * 3 / 4, result.size() * 4 / 4);

    double x = xi / i;
    double y = yi / i;
    double z = zi / i;

    if (i <= 0)
    {
        x = 0;
        y = 0;
        z = 0;
    }

    emit weightpointResolved(x, y, z);
}

void VolumeWorker::resolveLineIntegral(Line line)
{
    //*

    p_line = line;

    // Kernel launch parameters
    Matrix<size_t> loc_ws(1, 2);
    loc_ws[0] = 1;
    loc_ws[1] = 512;

    // Samples in each direction
    Matrix<int> samples(3, 1);

    double sample_interdist_ab;
    double sample_interdist_c = p_line.length() / (double) p_line_c_res;

    if (p_line.prismSideA() >= p_line.prismSideB())
    {
        sample_interdist_ab = p_line.prismSideA() / (double) (p_line_ab_res - 1);

        samples[0] = p_line_ab_res;
        samples[1] = (p_line.prismSideB() / sample_interdist_ab) + 1;
        samples[2] = p_line_c_res;
    }
    else
    {
        sample_interdist_ab = p_line.prismSideB() / (double) (p_line_ab_res - 1);

        samples[0] = (p_line.prismSideA() / sample_interdist_ab) + 1;
        samples[1] = p_line_ab_res;
        samples[2] = p_line_c_res;
    }

    // Other kernel variables
    Matrix<size_t> glb_ws(1, 2);
    glb_ws[0] = samples[2];
    glb_ws[1] = loc_ws[1];

    Matrix<double> aVecSegment = vecNormalize(p_line.aVec()) * sample_interdist_ab;
    Matrix<double> bVecSegment = vecNormalize(p_line.bVec()) * sample_interdist_ab;
    Matrix<double> cVecSegment = vecNormalize(p_line.cVec()) * sample_interdist_c;

    p_line_data_y.set(1, samples[2]);

    cl_mem result_cl = QOpenCLCreateBuffer(context_cl->context(),
                                           CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                           p_line_data_y.bytes(),
                                           p_line_data_y.data(), &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Kernel arguments
    err = QOpenCLSetKernelArg(p_line_integral_kernel, 0, sizeof(cl_mem), (void *) p_pool);
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 1, sizeof(cl_sampler), p_pool_sampler);
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 2, sizeof(cl_mem), (void *) p_oct_index);
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 3, sizeof(cl_mem), (void *) p_oct_brick);
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 4, sizeof(cl_mem), (void *) p_data_extent);
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 5, sizeof(cl_mem), (void *) p_misc_int);
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 6, sizeof(cl_mem), (void *) &result_cl); // Have here
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 7, sizeof(cl_float3), p_line.basePos().toFloat().data());
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 8, sizeof(cl_float3), aVecSegment.toFloat().data());
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 9, sizeof(cl_float3), bVecSegment.toFloat().data());
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 10, sizeof(cl_float3), cVecSegment.toFloat().data());
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 11, sizeof(cl_int3), samples.data());
    err |= QOpenCLSetKernelArg(p_line_integral_kernel, 12, sizeof(cl_float) * loc_ws[1], NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    //    glb_ws.print(0, "glb_ws");
    //    loc_ws.print(0, "loc_ws");

    //    p_line.basePos().print(3, "basePos");
    //    aVecSegment.print(6, "aVecSegment");
    //    bVecSegment.print(6, "bVecSegment");
    //    cVecSegment.print(6, "cVecSegment");
    //    samples.print(3, "samples");



    err = QOpenCLEnqueueNDRangeKernel(context_cl->queue(), p_line_integral_kernel, 2, NULL, glb_ws.data(), loc_ws.data(), 0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLFinish(context_cl->queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueReadBuffer ( context_cl->queue(),
                                     result_cl,
                                     CL_TRUE,
                                     0,
                                     p_line_data_y.bytes(),
                                     p_line_data_y.data(),
                                     0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLReleaseMemObject(result_cl);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    p_line_integral_xmin = 0;
    p_line_integral_xmax = p_line.length();
    p_line_data_y.toDouble().minmax(&p_line_integral_ymin, &p_line_integral_ymax);


    p_line_data_x.resize(p_line_data_y.m(), p_line_data_y.n());

    for (int i = 0; i < p_line_data_x.size(); i++)
    {
        p_line_data_x[i] = p_line_integral_xmin +  i * (p_line_integral_xmax - p_line_integral_xmin) / (p_line_data_x.size() - 1);
    }

    emit lineIntegralResolved();

    //*/
}

void VolumeWorker::resolvePlaneIntegral(Line line)
{
    p_line = line;

    // Kernel launch parameters
    Matrix<size_t> loc_ws(3, 1);
    loc_ws[0] = 1;
    loc_ws[1] = 1;
    loc_ws[2] = 64;

    // Samples in each direction
    Matrix<int> samples(3, 1);

    double sample_interdist_ab;
    double sample_interdist_c = p_line.length() / (double) p_surface_c_res;

    if (p_line.prismSideA() >= p_line.prismSideB())
    {
        sample_interdist_ab = p_line.prismSideA() / (double) (p_surface_ab_res - 1);

        samples[0] = p_surface_ab_res;
        samples[1] = (p_line.prismSideB() / sample_interdist_ab) + 1;
        samples[2] = p_surface_c_res;
    }
    else
    {
        sample_interdist_ab = p_line.prismSideB() / (double) (p_surface_ab_res - 1);

        samples[0] = (p_line.prismSideA() / sample_interdist_ab) + 1;
        samples[1] = p_surface_ab_res;
        samples[2] = p_surface_c_res;
    }

    // Other kernel variables
    Matrix<size_t> glb_ws(1, 3);
    glb_ws[0] = samples[0];
    glb_ws[1] = samples[1];
    glb_ws[2] = loc_ws[2];

    Matrix<double> aVecSegment = vecNormalize(p_line.aVec()) * sample_interdist_ab;
    Matrix<double> bVecSegment = vecNormalize(p_line.bVec()) * sample_interdist_ab;
    Matrix<double> cVecSegment = vecNormalize(p_line.cVec()) * sample_interdist_c;

    p_surface_data.set(samples[0], samples[1]);

    cl_mem result_cl = QOpenCLCreateBuffer(context_cl->context(),
                                           CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                           p_surface_data.bytes(),
                                           p_surface_data.data(), &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Kernel arguments
    err = QOpenCLSetKernelArg(p_plane_integral_kernel, 0, sizeof(cl_mem), (void *) p_pool);
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 1, sizeof(cl_sampler), p_pool_sampler);
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 2, sizeof(cl_mem), (void *) p_oct_index);
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 3, sizeof(cl_mem), (void *) p_oct_brick);
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 4, sizeof(cl_mem), (void *) p_data_extent);
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 5, sizeof(cl_mem), (void *) p_misc_int);
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 6, sizeof(cl_mem), (void *) &result_cl);
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 7, sizeof(cl_float3), p_line.basePos().toFloat().data());
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 8, sizeof(cl_float3), aVecSegment.toFloat().data());
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 9, sizeof(cl_float3), bVecSegment.toFloat().data());
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 10, sizeof(cl_float3), cVecSegment.toFloat().data());
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 11, sizeof(cl_int3), samples.data());
    err |= QOpenCLSetKernelArg(p_plane_integral_kernel, 12, sizeof(cl_float) * loc_ws[2], NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueNDRangeKernel(context_cl->queue(), p_plane_integral_kernel, 3, NULL, glb_ws.data(), loc_ws.data(), 0, NULL, NULL);



    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLFinish(context_cl->queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueReadBuffer ( context_cl->queue(),
                                     result_cl,
                                     CL_TRUE,
                                     0,
                                     p_surface_data.bytes(),
                                     p_surface_data.data(),
                                     0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLReleaseMemObject(result_cl);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    emit planeIntegralResolved();
}


VolumeOpenGLWidget::VolumeOpenGLWidget(QObject * parent)
    : isCLInitialized(false),
      isGLInitialized(false),
      isRayTexInitialized(false),
      isTsfTexInitialized(false),
      isIntegrationTexInitialized(false),
      isDSActive(false),
      isOrthonormal(false),
      isLogarithmic(true),
      isModelActive(true),
      isUnitcellActive(true),
      isSvoInitialized(false),
      isScalebarActive(false),
      isSlicingActive(false),
      isIntegration2DActive(false),
      isIntegration3DActive(true),
      isShadowActive(false),
      isLogarithmic2D(false),
      isOrthoGridActive(false),
      isBackgroundBlack(false),
      isDataExtentReadOnly(true),
      isCenterlineActive(true),
      isRulerActive(false),
      isHklTextActive(true),
      isURotationActive(false),
      isLabFrameActive(false),
      isMiniCellActive(true),
      isCountIntegrationActive(false),
      n_marker_indices(0),
      quality_percentage(15),
      displayDistance(false),
      displayResolution(true),
      currentLineIndex(0)
{
    // Worker
    workerThread = new QThread;
    volumeWorker = new VolumeWorker;
    volumeWorker->moveToThread(workerThread);
    connect(workerThread, SIGNAL(finished()), volumeWorker, SLOT(deleteLater()));
    volumeWorker->setCLObjects(&cl_svo_pool, &cl_svo_pool_sampler, &cl_svo_index, &cl_svo_brick, &cl_data_extent, &cl_data_view_extent, &cl_misc_ints);
    connect(this, SIGNAL(lineChanged(Line)), volumeWorker, SLOT(resolveLineIntegral(Line)));
    connect(this, SIGNAL(lineChanged(Line)), volumeWorker, SLOT(resolvePlaneIntegral(Line)));
    connect(this, SIGNAL(dataViewExtentChanged()), volumeWorker, SLOT(resolveWeightpoint()));
    connect(volumeWorker, SIGNAL(weightpointResolved(double, double, double)), this, SLOT(setWeightpoint(double, double, double)));
    workerThread->start();

    weightpoint.set(3, 1, 0);
    p_translate_vecA.set(3, 1, 0);
    p_translate_vecB.set(3, 1, 0);

    // Clip planes
    clip_plane0.set(4, 1, 0);
    clip_plane1.set(4, 1, 0);
    clip_plane2.set(4, 1, 0);

    // Marker
    markers_selected_indices.set(100, 1, 0);

    // Matrices
    double extent[8] =
    {
        -pi, pi,
        -pi, pi,
        -pi, pi,
        1.0, 1.0
    };
    data_extent.setDeep(4, 2, extent);
    data_view_extent.setDeep(4, 2, extent);
    tsf_parameters_svo.set(1, 6, 0.0);
    tsf_parameters_model.set(1, 6, 0.0);
    misc_ints.set(1, 16, 0.0);
    model_misc_floats.set(1, 16, 0.0);

    // View matrices
    view_matrix.setIdentity(4);
    ctc_matrix.setIdentity(4);
    rotation.setIdentity(4);
    data_translation.setIdentity(4);
    data_scaling.setIdentity(4);
    bbox_scaling.setIdentity(4);
    bbox_scaling = bbox_scaling * 0.2;
    bbox_scaling[15] = 1.0;
    minicell_scaling.setIdentity(4);
    bbox_translation.setIdentity(4);
    //    normalization_scaling.setIdentity(4);
    scalebar_view_matrix.setIdentity(4);
    scalebar_rotation.setIdentity(4);
    //    projection_scaling.setIdentity(4);
    //    projection_scaling[0] = 0.7;
    //    projection_scaling[5] = 0.7;
    //    projection_scaling[10] = 0.7;
//    unitcell_view_matrix.setIdentity(4);

    double N = 0.1;
    double F = 10.0;
    double fov = 20.0;

    bbox_translation[11] = -N - (F - N) * 0.5;
    ctc_matrix.setN(N);
    ctc_matrix.setF(F);
    ctc_matrix.setFov(fov);
    ctc_matrix.setWindow(200, 200);
    ctc_matrix.setProjection(isOrthonormal);

    // hkl selection
    hklCurrent.set(1, 3, 0);

    // hkl text
    hkl_text.set(3 * 3 * 3, 6, 0);

    // Timer
    session_age.start();

    // Ray tex
    ray_tex_dim.reserve(1, 2);
    ray_glb_ws.reserve(1, 2);
    ray_loc_ws.reserve(1, 2);
    ray_loc_ws[0] = 16;
    ray_loc_ws[1] = 16;
    pixel_size.reserve(2, 1);

    // Center line
    centerline_coords.set(2, 3, 0.0);

    // Transfer texture
//    tsf_color_scheme = "Hot";
//    tsf_alpha_scheme = "Uniform";

    tsf_parameters_model[0] = 0.0; // texture min
    tsf_parameters_model[1] = 1.0; // texture max

    tsf_parameters_svo[0] = 0.0; // texture min
    tsf_parameters_svo[1] = 1.0; // texture max
    tsf_parameters_svo[4] = 0.5; // alpha
    tsf_parameters_svo[5] = 2.0; // brightness

    // Scalebars
    position_scalebar_ticks.reserve(100, 3);
    count_major_scalebar_ticks.reserve(100, 3);
    count_minor_scalebar_ticks.reserve(100, 3);
    n_count_scalebar_ticks = 0;
    n_count_minor_scalebar_ticks = 0;
    n_position_scalebar_ticks = 0;


    // Color
    white.set(1, 1, 1, 1.0); // Note: Changed alpha from 0.4
    black.set(0, 0, 0, 1.0); // Note: Changed alpha from 0.4
    yellow.set(1.00, 	0.50, 	0.00, 0.8);
    epic.set(0.64, 	0.21, 	0.93, 1.0);
    red.set(1, 0, 0, 1);
    green.set(0, 1, 0, 1);
    green_light.set(0.3, 1, 0.3, 0.9);
    blue.set(0, 0, 1, 1);
    blue_light.set(0.1, 0.1, 1.0, 0.9);
    magenta.set(1.0, 0.0, 0.7, 0.9);
    magenta_light.set(1.0, 0.1, 0.7, 0.9);
    clear_color = white;
    clear_color_inverse = black;
    centerline_color = yellow;
    marker_line_color = blue;

    // Fps
    fps_string_width_prev = 0;

    // Shadow
    shadow_vector.reserve(4, 1);
    shadow_vector[0] = -1.0;
    shadow_vector[1] = +1.0;
    shadow_vector[2] = -1.0;
    shadow_vector[3] = 0.0;

    // Ruler
    ruler.reserve(1, 4);

    // Roll
    accumulated_roll = 0;

    identity.setIdentity(4);
}

void VolumeOpenGLWidget::setViewExtentVbo()
{
    if (!isGLInitialized)
    {
        return;
    }

    Matrix<GLfloat> vertices(8, 3, 0);

    int i = 0;

    vertices[i++] = data_extent[0];
    vertices[i++] = data_extent[2];
    vertices[i++] = data_extent[4];

    vertices[i++] = data_extent[0];
    vertices[i++] = data_extent[2];
    vertices[i++] = data_extent[5];

    vertices[i++] = data_extent[0];
    vertices[i++] = data_extent[3];
    vertices[i++] = data_extent[4];

    vertices[i++] = data_extent[0];
    vertices[i++] = data_extent[3];
    vertices[i++] = data_extent[5];

    vertices[i++] = data_extent[1];
    vertices[i++] = data_extent[2];
    vertices[i++] = data_extent[4];

    vertices[i++] = data_extent[1];
    vertices[i++] = data_extent[2];
    vertices[i++] = data_extent[5];

    vertices[i++] = data_extent[1];
    vertices[i++] = data_extent[3];
    vertices[i++] = data_extent[4];

    vertices[i++] = data_extent[1];
    vertices[i++] = data_extent[3];
    vertices[i++] = data_extent[5];

    setVbo(view_extent_vbo, vertices.data(), vertices.size(), GL_DYNAMIC_DRAW);
}

VolumeOpenGLWidget::~VolumeOpenGLWidget()
{
    if (!(isCLInitialized && isGLInitialized))
    {
        return;
    }

    glDeleteBuffers(1, &lab_frame_vbo);
    glDeleteBuffers(1, &scalebar_vbo);
    glDeleteBuffers(1, &count_scalebar_vbo);
    glDeleteBuffers(1, &centerline_vbo);
    glDeleteBuffers(1, &point_vbo);
    glDeleteBuffers(1, &point_vbo);
    glDeleteBuffers(1, &unitcell_vbo);
    glDeleteBuffers(1, &marker_centers_vbo);
    glDeleteBuffers(1, &minicell_vbo);
    glDeleteBuffers(1, &view_extent_vbo);
    glDeleteBuffers(1, &line_translate_vbo);

    glDeleteTextures(1, &ray_tex_gl);
    glDeleteTextures(1, &tsf_tex_gl);
    glDeleteTextures(1, &tsf_tex_gl_thumb);

    workerThread->quit();
    workerThread->wait();
}

void VolumeOpenGLWidget::toggleHkl(bool value)
{
    isHklTextActive = value;
    update();
}

void VolumeOpenGLWidget::paintGL()
{
    // The problem is that the Z buffer prevents OpenGL from drawing pixels that are behind things that have already been drawn

    QOpenGLPaintDevice paint_device_gl(this->size());

    QPainter painter(&paint_device_gl);

    painter.setRenderHint(QPainter::Antialiasing);

    isDataExtentReadOnly = true;
    setDataExtent();
    setViewMatrices();

    beginRawGLCalls(&painter);
    glClearColor(clear_color[0], clear_color[1], clear_color[2], 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glLineWidth(1.0);
    const qreal retinaScale = this->devicePixelRatio();
    glViewport(0, 0, this->width() * retinaScale, this->height() * retinaScale);
    endRawGLCalls(&painter);

    if (isLabFrameActive)
    {
        drawViewExtent(&painter);
    }

    if (isScalebarActive)
    {
        drawPositionScalebars(&painter);
    }

    if (isLabFrameActive)
    {
        drawCenterLine(&painter);
    }

    if (isLabFrameActive)
    {
        drawLabFrame(&painter);
    }

    if (isSvoInitialized)
    {
        if (isScalebarActive)
        {
            drawLineIntegrationVolumeVisualAssist(&painter);
            drawWeightCenter(&painter);
            drawLineTranslationVec(&painter);
        }
    }

    isDataExtentReadOnly = false;

    // Draw raytracing texture
    drawRayTex(&painter);



    // Compute the projected pixel size in orthonormal configuration
    //    computePixelSize();

    if (isIntegration2DActive)
    {
        drawIntegral(&painter);
    }

    drawOverlay(&painter);

    if (isHklTextActive)
    {
        drawHklText(&painter);
    }

    if (n_marker_indices > 0)
    {
        drawMarkers(&painter);
    }

    if (isMiniCellActive)
    {
        drawHelpCell(&painter);    //
    }

    if (isUnitcellActive)
    {
        drawUnitCell(&painter);
    }

    drawCountScalebar(&painter); //

    if (isSvoInitialized && isCountIntegrationActive)
    {
        drawCountIntegral(&painter);
    }
}

void VolumeOpenGLWidget::resizeGL(int w, int h)
{
    ctc_matrix.setWindow(this->width(), this->height());

    setRayTexture(quality_percentage);
}

QPointF VolumeOpenGLWidget::posGLtoQt(QPointF coord)
{
    QPointF QtPoint;

    QtPoint.setX(0.5 * (float)this->width() * (coord.x() + 1.0) - 1.0);
    QtPoint.setY(0.5 * (float)this->height() * (1.0 - coord.y()) - 1.0);

    return QtPoint;
}

QPointF VolumeOpenGLWidget::posQttoGL(QPointF coord)
{
    QPointF GLPoint;
    GLPoint.setX((coord.x() + 1.0) / (float) (this->width()) * 2.0 - 1.0);
    GLPoint.setY((1.0 - (coord.y() + 1.0) / (float) this->height()) * 2.0 - 1.0);
    return GLPoint;
}

void VolumeOpenGLWidget::initializeGL()
{
    if (isCLInitialized && isGLInitialized)
    {
        return;
    }

    // Initialize OpenGL
    QOpenGLFunctions::initializeOpenGLFunctions();

    glGenBuffers(1, &lab_frame_vbo);
    glGenBuffers(1, &scalebar_vbo);
    glGenBuffers(1, &count_scalebar_vbo);
    glGenBuffers(1, &centerline_vbo);
    glGenBuffers(1, &point_vbo);
    glGenBuffers(1, &unitcell_vbo);
    glGenBuffers(1, &marker_centers_vbo);
    glGenBuffers(1, &minicell_vbo);
    glGenBuffers(1, &weightpoint_vbo);
    glGenBuffers(1, &view_extent_vbo);
    glGenBuffers(1, &line_translate_vbo);

    glGenTextures(1, &ray_tex_gl);
    glGenTextures(1, &tsf_tex_gl);
    glGenTextures(1, &tsf_tex_gl_thumb);

    // Shader for drawing textures in 2D
    std_2d_tex_program = new QOpenGLShaderProgram(this);
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_2d_tex.v.glsl");
    std_2d_tex_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_2d_tex.f.glsl");

    if (!std_2d_tex_program->link())
    {
        qFatal(std_2d_tex_program->log().toStdString().c_str());
    }

    if ((std_2d_tex_fragpos = std_2d_tex_program->attributeLocation("fragpos")) == -1)
    {
        qFatal("Invalid attribute");
    }

    if ((std_2d_tex_pos = std_2d_tex_program->attributeLocation("texpos")) == -1)
    {
        qFatal("Invalid attribute");
    }

    if ((std_2d_tex_texture = std_2d_tex_program->uniformLocation("texture")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_2d_tex_transform = std_2d_tex_program->uniformLocation("transform")) == -1)
    {
        qFatal("Invalid uniform");
    }

    // Shader for drawing lines and similar in 3D
    std_3d_col_program = new QOpenGLShaderProgram(this);
    std_3d_col_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_3d_col.v.glsl");
    std_3d_col_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_3d_col.f.glsl");

    if (!std_3d_col_program->link())
    {
        qFatal(std_3d_col_program->log().toStdString().c_str());
    }

    if ((std_3d_col_fragpos = std_3d_col_program->attributeLocation("fragpos")) == -1)
    {
        qFatal("Invalid attribute");
    }

    if ((std_3d_col_color = std_3d_col_program->uniformLocation("color")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_3d_col_model_transform = std_3d_col_program->uniformLocation("model_transform")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_3d_col_fragpos_transform = std_3d_col_program->uniformLocation("fragpos_transform")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_3d_col_projection_transform = std_3d_col_program->uniformLocation("projection_transform")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_3d_col_clip_plane0 = std_3d_col_program->uniformLocation("clip_plane0")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_3d_col_clip_plane1 = std_3d_col_program->uniformLocation("clip_plane1")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_3d_col_clip_plane2 = std_3d_col_program->uniformLocation("clip_plane2")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_3d_col_clip_plane3 = std_3d_col_program->uniformLocation("clip_plane3")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_3d_col_clip_plane4 = std_3d_col_program->uniformLocation("clip_plane4")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_3d_col_clip_plane5 = std_3d_col_program->uniformLocation("clip_plane5")) == -1)
    {
        qFatal("Invalid uniform");
    }

    // Shader for drawing lines and similar in 2D
    std_2d_col_program = new QOpenGLShaderProgram(this);
    std_2d_col_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "shaders/std_2d_col.v.glsl");
    std_2d_col_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "shaders/std_2d_col.f.glsl");

    if (!std_2d_col_program->link())
    {
        qFatal(std_2d_col_program->log().toStdString().c_str());
    }

    if ((std_2d_col_fragpos = std_2d_col_program->attributeLocation("fragpos")) == -1)
    {
        qFatal("Invalid attribute");
    }

    if ((std_2d_col_color = std_2d_col_program->uniformLocation("color")) == -1)
    {
        qFatal("Invalid uniform");
    }

    if ((std_2d_col_transform = std_2d_col_program->uniformLocation("transform")) == -1)
    {
        qFatal("Invalid uniform");
    }

    // Initialize OpenCL
    initializeCL();

    isGLInitialized = true;
    isCLInitialized = true;

    // Textures
    setRayTexture(20);

    tsf.setRgb("Hot");
    tsf.setAlpha("Uniform");
    tsf.setSpline(256);
    setTsfTexture(tsf);

    // Core set functions
    setDataExtent();
    setViewMatrices();
    setTsfParameters();
    setMiscArrays();

    // Pens
    initializePaintTools();

    // Initialize some VBOs
    setViewExtentVbo();

    // Initial drawings
    updateUnitCellVertices();
    updateUnitCellText();
}

float VolumeOpenGLWidget::sumGpuArray(cl_mem cl_data, unsigned int read_size, size_t work_group_size)
{
    /* Set initial kernel parameters (they will change for each iteration)*/
    Matrix<size_t> local_size(1, 1, work_group_size);
    Matrix<size_t> global_size(1, 1);
    unsigned int read_offset = 0;
    unsigned int write_offset;

    global_size[0] = read_size + (read_size % local_size[0] ? local_size[0] - (read_size % local_size[0]) : 0);
    write_offset = global_size[0];

    bool forth = true;
    float sum;

    /* Pass arguments to kernel */
    err = QOpenCLSetKernelArg(cl_parallel_reduce, 0, sizeof(cl_mem), (void *) &cl_data);
    err |= QOpenCLSetKernelArg(cl_parallel_reduce, 1, local_size[0] * sizeof(cl_float), NULL);
    err |= QOpenCLSetKernelArg(cl_parallel_reduce, 2, sizeof(cl_uint), &read_size);
    err |= QOpenCLSetKernelArg(cl_parallel_reduce, 3, sizeof(cl_uint), &read_offset);
    err |= QOpenCLSetKernelArg(cl_parallel_reduce, 4, sizeof(cl_uint), &write_offset);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    /* Launch kernel repeatedly until the summing is done */
    while (read_size > 1)
    {
        err = QOpenCLEnqueueNDRangeKernel(context_cl.queue(), cl_parallel_reduce, 1, 0, global_size.data(), local_size.data(), 0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        err = QOpenCLFinish(context_cl.queue());

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        /* Extract the sum */
        err = QOpenCLEnqueueReadBuffer ( context_cl.queue(),
                                         cl_data,
                                         CL_TRUE,
                                         forth ? global_size[0] * sizeof(cl_float) : 0,
                                         sizeof(cl_float),
                                         &sum,
                                         0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        /* Prepare the kernel parameters for the next iteration */
        forth = !forth;

        // Prepare to read memory in front of the separator and write to the memory behind it
        if (forth)
        {
            read_size = (global_size[0]) / local_size[0];

            if (read_size % local_size[0])
            {
                global_size[0] = read_size + local_size[0] - (read_size % local_size[0]);
            }
            else
            {
                global_size[0] = read_size;
            }

            read_offset = 0;
            write_offset = global_size[0];
        }
        // Prepare to read memory behind the separator and write to the memory in front of it
        else
        {
            read_offset = global_size[0];
            write_offset = 0;

            read_size = global_size[0] / local_size[0];

            if (read_size % local_size[0])
            {
                global_size[0] = read_size + local_size[0] - (read_size % local_size[0]);
            }
            else
            {
                global_size[0] = read_size;
            }
        }

        err = QOpenCLSetKernelArg(cl_parallel_reduce, 2, sizeof(cl_uint), &read_size);
        err |= QOpenCLSetKernelArg(cl_parallel_reduce, 3, sizeof(cl_uint), &read_offset);
        err |= QOpenCLSetKernelArg(cl_parallel_reduce, 4, sizeof(cl_uint), &write_offset);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

    }

    return sum;
}

void VolumeOpenGLWidget::setHCurrent(int value)
{
    hklCurrent[0] = value;
    setHkl(hklCurrent);

    update();
}

void VolumeOpenGLWidget::setKCurrent(int value)
{
    hklCurrent[1] = value;
    setHkl(hklCurrent);

    update();
}

void VolumeOpenGLWidget::setLCurrent(int value)
{
    hklCurrent[2] = value;
    setHkl(hklCurrent);

    update();
}

void VolumeOpenGLWidget::setHkl(Matrix<int> &hkl)
{
    Matrix<double> hkl_focus(1, 3, 0);

    hkl_focus[0] = hkl[0] * UB[0] + hkl[1] * UB[1] + hkl[2] * UB[2];
    hkl_focus[1] = hkl[0] * UB[3] + hkl[1] * UB[4] + hkl[2] * UB[5];
    hkl_focus[2] = hkl[0] * UB[6] + hkl[1] * UB[7] + hkl[2] * UB[8];

    data_translation[3] = -hkl_focus[0];
    data_translation[7] = -hkl_focus[1];
    data_translation[11] = -hkl_focus[2];

    data_view_extent =  (data_scaling * data_translation).inverse4x4() * data_extent;



    {
        std_3d_col_program->bind();

        clip_plane0 = data_view_extent.planeXY0();
        clip_plane1 = data_view_extent.planeXY1();
        clip_plane2 = data_view_extent.planeXZ0();
        clip_plane3 = data_view_extent.planeXZ1();
        clip_plane4 = data_view_extent.planeYZ0();
        clip_plane5 = data_view_extent.planeYZ1();

        glUniform4fv(std_3d_col_clip_plane0, 1, clip_plane0.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane1, 1, clip_plane1.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane2, 1, clip_plane2.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane3, 1, clip_plane3.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane4, 1, clip_plane4.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane5, 1, clip_plane5.toFloat().data());

        std_3d_col_program->release();
    }



    updateUnitCellText();
}

void VolumeOpenGLWidget::mousePressEvent(QMouseEvent * event)
{
    if (isRulerActive && (event->buttons() & Qt::LeftButton))
    {
        ruler[0] = event->x();
        ruler[1] = event->y();
        ruler[2] = event->x();
        ruler[3] = event->y();
    }

    accumulated_roll = 0;

    GLfloat depth;
    glReadPixels(event->x(), this->height() - event->y() - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    if (( depth < 1.0) && (markers.size() > 0) && (event->buttons() & Qt::LeftButton) && !(event->buttons() & Qt::RightButton) && !(event->buttons() & Qt::MidButton))
    {
        Matrix<double> xyz_clip(4, 1, 1.0);
        xyz_clip[0] = 2.0 * (double) event->x() / (double) this->width() - 1.0;
        xyz_clip[1] = 2.0 * (double) (this->height() - event->y()) / (double) this->height() - 1.0;
        xyz_clip[2] = 2.0 * depth - 1.0;

        Matrix<double> xyz = (ctc_matrix * view_matrix).inverse4x4() * xyz_clip;

        xyz[0] /= xyz[3];
        xyz[1] /= xyz[3];
        xyz[2] /= xyz[3];

        int closest = 0;
        double min_distance = 1e9;

        for (int i = 0; i < markers.size(); i++)
        {
            // Find the closest marker
            if (min_distance > markers[i].getDistance(xyz[0], xyz[1], xyz[2]))
            {
                min_distance = markers[i].getDistance(xyz[0], xyz[1], xyz[2]);
                closest = i;
            }
        }

        if ((event->modifiers() & Qt::ControlModifier))
        {
            markers.remove(closest);
            glDeleteBuffers(1, &marker_vbo[closest]);
            marker_vbo.remove(closest);
        }
        else
        {
            markers[closest].setTagged(!markers[closest].getTagged());
        }

        Matrix<float> marker_selected(10, 3);
        int n_markers_selected = 0;
        int iter = 0;
        n_marker_indices = 0;

        for (int i = 0; i < markers.size(); i++)
        {
            // Generate a vbo to draw lines between selected markers
            if (markers[i].getTagged())
            {
                marker_selected[n_markers_selected * 3 + 0] = markers[i].getCenter()[0];
                marker_selected[n_markers_selected * 3 + 1] = markers[i].getCenter()[1];
                marker_selected[n_markers_selected * 3 + 2] = markers[i].getCenter()[2];

                for (int i = 0; i < n_markers_selected; i++)
                {
                    markers_selected_indices[iter * 2 + 0] = n_markers_selected;
                    markers_selected_indices[iter * 2 + 1] = i;
                    iter++;
                    n_marker_indices += 2;
                }

                n_markers_selected++;
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, marker_centers_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*marker_selected.size(), marker_selected.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void VolumeOpenGLWidget::mouseReleaseEvent(QMouseEvent * event)
{

}



void VolumeOpenGLWidget::mouseMoveEvent(QMouseEvent * event)
{
    if ((event->buttons() & Qt::LeftButton) && isRulerActive)
    {
        ruler[2] = event->x();
        ruler[3] = event->y();
    }

    {
        float move_scaling = 0.5;

        if ((event->modifiers() & Qt::ControlModifier))
        {
            move_scaling = 0.05;
        }

        if ((event->buttons() & Qt::LeftButton) && !(event->buttons() & Qt::RightButton) && !isRulerActive)
        {
            /* Rotation happens multiplicatively around a rolling axis given
             * by the mouse move direction and magnitude.
             * Moving the mouse alters rotation.
             * */

            double eta = std::atan2((double)event->x() - last_mouse_pos_x, (double) event->y() - last_mouse_pos_y) - pi * 1.0;
            double roll = move_scaling * pi / ((float) this->height()) * std::sqrt((double)(event->x() - last_mouse_pos_x) * (event->x() - last_mouse_pos_x) + (event->y() - last_mouse_pos_y) * (event->y() - last_mouse_pos_y));

            RotationMatrix<double> roll_rotation;
            roll_rotation.setArbRotation(-0.5 * pi, eta, roll);

            if ((event->modifiers() & Qt::ShiftModifier) && isURotationActive && isUnitcellActive)
            {
                U = rotation.inverse4x4() * roll_rotation * rotation * U;
                UB.setUMatrix(U.to3x3());
                updateUnitCellText();
            }
            else if ((event->modifiers() & Qt::ShiftModifier))
            {
                scalebar_rotation = rotation.inverse4x4() * roll_rotation * rotation * scalebar_rotation;
            }
            else
            {
                rotation = roll_rotation * rotation;
            }

            update();
        }
        else if ((event->buttons() & Qt::LeftButton) && (event->buttons() & Qt::RightButton) && !(event->buttons() & Qt::MidButton) && !isRulerActive)
        {
            /* Rotation happens multiplicatively around a rolling axis given
             * by the mouse move direction and magnitude.
             * Moving the mouse alters rotation.
             * */

            RotationMatrix<double> roll_rotation;
            double roll = move_scaling * pi / ((float) this->height()) * (event->y() - last_mouse_pos_y);

            accumulated_roll += roll;

            roll_rotation.setArbRotation(0, 0, roll);

            if ((event->modifiers() & Qt::ShiftModifier) && isURotationActive && isUnitcellActive)
            {
                U = rotation.inverse4x4() * roll_rotation *  rotation * U;
                UB.setUMatrix(U.to3x3());
                updateUnitCellText();
            }
            else if ((event->modifiers() & Qt::ShiftModifier))
            {
                scalebar_rotation = rotation.inverse4x4() * roll_rotation *  rotation * scalebar_rotation;
            }
            else
            {
                rotation = roll_rotation * rotation;
            }

            update();
        }
        else if ((event->buttons() & Qt::MidButton) && !(event->buttons() & Qt::LeftButton) && !(event->buttons() & Qt::RightButton))
        {
            /* X/Y translation happens multiplicatively. Here it is
             * important to retain the bounding box accordingly  */
            float dx = move_scaling * 2.0 * (data_view_extent[1] - data_view_extent[0]) / ((float) this->height()) * (event->x() - last_mouse_pos_x);
            float dy = move_scaling * -2.0 * (data_view_extent[3] - data_view_extent[2]) / ((float) this->height()) * (event->y() - last_mouse_pos_y);

            Matrix<double> data_translation_prev;
            data_translation_prev.setIdentity(4);
            data_translation_prev = data_translation;

            data_translation.setIdentity(4);
            data_translation[3] = dx;
            data_translation[7] = dy;

            data_translation = ( rotation.inverse4x4() * data_translation * rotation) * data_translation_prev;

            this->data_view_extent =  (data_scaling * data_translation).inverse4x4() * data_extent;

            {
                std_3d_col_program->bind();

                clip_plane0 = data_view_extent.planeXY0();
                clip_plane1 = data_view_extent.planeXY1();
                clip_plane2 = data_view_extent.planeXZ0();
                clip_plane3 = data_view_extent.planeXZ1();
                clip_plane4 = data_view_extent.planeYZ0();
                clip_plane5 = data_view_extent.planeYZ1();

                glUniform4fv(std_3d_col_clip_plane0, 1, clip_plane0.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane1, 1, clip_plane1.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane2, 1, clip_plane2.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane3, 1, clip_plane3.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane4, 1, clip_plane4.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane5, 1, clip_plane5.toFloat().data());

                std_3d_col_program->release();
            }

            updateUnitCellText();

            update();
        }
        else if (!(event->buttons() & Qt::LeftButton) && (event->buttons() & Qt::RightButton) && !(event->buttons() & Qt::MidButton))
        {
            /* Z translation happens multiplicatively */
            float dz = move_scaling * 2.0 * (data_view_extent[5] - data_view_extent[4]) / ((float) this->height()) * (event->y() - last_mouse_pos_y);

            Matrix<double> data_translation_prev;
            data_translation_prev.setIdentity(4);
            data_translation_prev = data_translation;

            data_translation.setIdentity(4);
            data_translation[11] = dz;

            data_translation = ( rotation.inverse4x4() * data_translation * rotation) * data_translation_prev;

            this->data_view_extent =  (data_scaling * data_translation).inverse4x4() * data_extent;

            {
                std_3d_col_program->bind();

                clip_plane0 = data_view_extent.planeXY0();
                clip_plane1 = data_view_extent.planeXY1();
                clip_plane2 = data_view_extent.planeXZ0();
                clip_plane3 = data_view_extent.planeXZ1();
                clip_plane4 = data_view_extent.planeYZ0();
                clip_plane5 = data_view_extent.planeYZ1();

                glUniform4fv(std_3d_col_clip_plane0, 1, clip_plane0.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane1, 1, clip_plane1.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane2, 1, clip_plane2.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane3, 1, clip_plane3.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane4, 1, clip_plane4.toFloat().data());
                glUniform4fv(std_3d_col_clip_plane5, 1, clip_plane5.toFloat().data());

                std_3d_col_program->release();
            }

            updateUnitCellText();

            update();
        }

    }

    last_mouse_pos_x = event->x();
    last_mouse_pos_y = event->y();


}

void VolumeOpenGLWidget::setUB_a(double value)
{
    UB.setA(value);

    if ((isCLInitialized && isGLInitialized))
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }

    update();
}

void VolumeOpenGLWidget::setUB_b(double value)
{
    UB.setB(value);

    if ((isCLInitialized && isGLInitialized))
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }

    update();
}
void VolumeOpenGLWidget::setUB_c(double value)
{
    UB.setC(value);

    if ((isCLInitialized && isGLInitialized))
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }

    update();
}

void VolumeOpenGLWidget::setUB_alpha(double value)
{
    UB.setAlpha(value * pi / 180.0);

    if ((isCLInitialized && isGLInitialized))
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }

    update();
}
void VolumeOpenGLWidget::setUB_beta(double value)
{
    UB.setBeta(value * pi / 180.0);

    if ((isCLInitialized && isGLInitialized))
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }

    update();
}
void VolumeOpenGLWidget::setUB_gamma(double value)
{
    UB.setGamma(value * pi / 180.0);

    if ((isCLInitialized && isGLInitialized))
    {
        updateUnitCellVertices();
        updateUnitCellText();
    }

    update();
}


void VolumeOpenGLWidget::setUBMatrix(UBMatrix<double> &mat)
{
    this->UB = mat;
    this->U.setIdentity(4);
    U.setFrom3x3(UB.getUMatrix());

    update();
}

VolumeWorker * VolumeOpenGLWidget::worker()
{
    return volumeWorker;
}

UBMatrix<double> &VolumeOpenGLWidget::getUBMatrix()
{
    return UB;
}

void VolumeOpenGLWidget::updateUnitCellText()
{
    /* Generates a unit cell grid based on an UB matrix. The grid consists of one set of reciprocal basis vectors per hkl */

    // Start by setting limits of the unitcell that we will visualize
    Matrix<int> hkl_limits(1, 6);
    hkl_limits[0] = -10;
    hkl_limits[1] = 10;
    hkl_limits[2] = -10;
    hkl_limits[3] = 10;
    hkl_limits[4] = -10;
    hkl_limits[5] = 10;


    size_t i = 0;
    hkl_text_counter = 0;

    Matrix<double> B = UB; // Why B and not UB?

    for (int h = hkl_limits[0]; h < hkl_limits[1]; h++)
    {
        for (int k = hkl_limits[2]; k < hkl_limits[3]; k++)
        {
            for (int l = hkl_limits[4]; l < hkl_limits[5]; l++)
            {
                // Assign a coordinate to use for font rendering since we want to see the indices of the peaks. Only render those that lie within the view extent and only when this list is less than some number of items to avoid clutter.
                double x = h * B[0] + k * B[1] + l * B[2];
                double y = h * B[3] + k * B[4] + l * B[5];
                double z = h * B[6] + k * B[7] + l * B[8];

                if (i + 5 < hkl_text.size())
                {
                    if (((x > data_view_extent[0]) && (x < data_view_extent[1])) &&
                            ((y > data_view_extent[2]) && (y < data_view_extent[3])) &&
                            ((z > data_view_extent[4]) && (z < data_view_extent[5])))
                    {
                        hkl_text[i + 0] = h;
                        hkl_text[i + 1] = k;
                        hkl_text[i + 2] = l;
                        hkl_text[i + 3] = x;
                        hkl_text[i + 4] = y;
                        hkl_text[i + 5] = z;

                        i += 6;

                        hkl_text_counter++;
                    }
                }
                else
                {
                    hkl_text_counter = 0;
                }
            }
        }
    }
}

void VolumeOpenGLWidget::updateUnitCellVertices()
{
    /* Generates a unit cell grid based on an UB matrix. The grid consists of one set of reciprocal basis vectors per hkl */

    // Start by setting limits of the unitcell that we will visualize
    Matrix<int> hkl_limits(1, 6);
    hkl_limits[0] = -10;
    hkl_limits[1] = 10;
    hkl_limits[2] = -10;
    hkl_limits[3] = 10;
    hkl_limits[4] = -10;
    hkl_limits[5] = 10;

    int n_basis = ((hkl_limits[1] - hkl_limits[0] + 1) * (hkl_limits[3] - hkl_limits[2] + 1) * (hkl_limits[5] - hkl_limits[4] + 1));

    // Generate the positions ([number of bases] X [number of vertices per basis] X [number of float values per vertice]);
    Matrix<float> vertices(n_basis * 6, 3);

    size_t m = 0;
    hkl_text_counter = 0;

    Matrix<double> B = UB.getBMatrix(); // Why B and not UB?

    for (int h = hkl_limits[0]; h < hkl_limits[1]; h++)
    {
        for (int k = hkl_limits[2]; k < hkl_limits[3]; k++)
        {
            for (int l = hkl_limits[4]; l < hkl_limits[5]; l++)
            {
                // Assign 6 vertices, each with 4 float values;
                double x = h * B[0] + k * B[1] + l * B[2];
                double y = h * B[3] + k * B[4] + l * B[5];
                double z = h * B[6] + k * B[7] + l * B[8];

                vertices[m + 0] = x;
                vertices[m + 1] = y;
                vertices[m + 2] = z;

                vertices[m + 3] = (1 + h) * B[0] + k * B[1] + l * B[2];
                vertices[m + 4] = (1 + h) * B[3] + k * B[4] + l * B[5];
                vertices[m + 5] = (1 + h) * B[6] + k * B[7] + l * B[8];

                vertices[m + 6] = x;
                vertices[m + 7] = y;
                vertices[m + 8] = z;

                vertices[m + 9] = h * B[0] + (1 + k) * B[1] + l * B[2];
                vertices[m + 10] = h * B[3] + (1 + k) * B[4] + l * B[5];
                vertices[m + 11] = h * B[6] + (1 + k) * B[7] + l * B[8];

                vertices[m + 12] = x;
                vertices[m + 13] = y;
                vertices[m + 14] = z;

                vertices[m + 15] = h * B[0] + k * B[1] + (1 + l) * B[2];
                vertices[m + 16] = h * B[3] + k * B[4] + (1 + l) * B[5];
                vertices[m + 17] = h * B[6] + k * B[7] + (1 + l) * B[8];

                m += 6 * 3;
            }
        }
    }

    setVbo(unitcell_vbo, vertices.data(), vertices.size(), GL_STATIC_DRAW);
    unitcell_nodes = n_basis * 6;
}

void VolumeOpenGLWidget::setVbo(GLuint vbo, float * buf, size_t length, GLenum usage)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*length, buf, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VolumeOpenGLWidget::drawUnitCell(QPainter * painter)
{
    beginRawGLCalls(painter);

    glLineWidth(0.7);
    std_3d_col_program->bind();
    glEnableVertexAttribArray(std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, unitcell_vbo);
    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, view_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_fragpos_transform, 1, GL_FALSE, U.colmajor().toFloat().data());

    float alpha = pow((std::max(std::max(UB.cStar(), UB.bStar()), UB.cStar()) / (data_view_extent[1] - data_view_extent[0])) * 5.0, 2);

    if (alpha > 0.3)
    {
        alpha = 0.3;
    }

    Matrix<float> color = clear_color_inverse;

    color[3] = alpha;

    glUniform4fv(std_3d_col_color, 1, color.data());

    {
        clip_plane0 = data_view_extent.planeXY0();
        clip_plane1 = data_view_extent.planeXY1();
        clip_plane2 = data_view_extent.planeXZ0();
        clip_plane3 = data_view_extent.planeXZ1();
        clip_plane4 = data_view_extent.planeYZ0();
        clip_plane5 = data_view_extent.planeYZ1();

        glUniform4fv(std_3d_col_clip_plane0, 1, clip_plane0.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane1, 1, clip_plane1.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane2, 1, clip_plane2.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane3, 1, clip_plane3.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane4, 1, clip_plane4.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane5, 1, clip_plane5.toFloat().data());
    }

    glDrawArrays(GL_LINES,  0, unitcell_nodes);

    glDisableVertexAttribArray(std_3d_col_fragpos);

    glUniformMatrix4fv(std_3d_col_fragpos_transform, 1, GL_FALSE, identity.colmajor().toFloat().data());

    std_3d_col_program->release();


    glLineWidth(1.0);

    endRawGLCalls(painter);
}

void VolumeOpenGLWidget::drawHelpCell(QPainter * painter)
{
    // Generate the vertices for the minicell
    Matrix<double> B = UB.getBMatrix();

    // Offset by half a diagonal to center the cell
    Matrix<float> hd(3, 1);
    hd[0] = (B[0] + B[1] + B[2]) * 0.5;
    hd[1] = (B[3] + B[4] + B[5]) * 0.5;
    hd[2] = (B[6] + B[7] + B[8]) * 0.5;

    Matrix<float> vertices(8, 3, 0);

    vertices[3 * 0 + 0] = 0 - hd[0];
    vertices[3 * 0 + 1] = 0 - hd[1];
    vertices[3 * 0 + 2] = 0 - hd[2];

    vertices[3 * 1 + 0] = B[0] - hd[0];
    vertices[3 * 1 + 1] = B[3] - hd[1];
    vertices[3 * 1 + 2] = B[6] - hd[2];

    vertices[3 * 2 + 0] = B[1] - hd[0];
    vertices[3 * 2 + 1] = B[4] - hd[1];
    vertices[3 * 2 + 2] = B[7] - hd[2];

    vertices[3 * 3 + 0] = B[2] - hd[0];
    vertices[3 * 3 + 1] = B[5] - hd[1];
    vertices[3 * 3 + 2] = B[8] - hd[2];

    vertices[3 * 4 + 0] = B[0] + B[1] - hd[0];
    vertices[3 * 4 + 1] = B[3] + B[4] - hd[1];
    vertices[3 * 4 + 2] = B[6] + B[7] - hd[2];

    vertices[3 * 5 + 0] = B[1] + B[2] - hd[0];
    vertices[3 * 5 + 1] = B[4] + B[5] - hd[1];
    vertices[3 * 5 + 2] = B[7] + B[8] - hd[2];

    vertices[3 * 6 + 0] = B[0] + B[2] - hd[0];
    vertices[3 * 6 + 1] = B[3] + B[5] - hd[1];
    vertices[3 * 6 + 2] = B[6] + B[8] - hd[2];

    vertices[3 * 7 + 0] = B[0] + B[1] + B[2] - hd[0];
    vertices[3 * 7 + 1] = B[3] + B[4] + B[5] - hd[1];
    vertices[3 * 7 + 2] = B[6] + B[7] + B[8] - hd[2];


    // Scaling. The cell has four diagonals. We find the longest one  and scale the cell to it
    Matrix<float> diagonal_one(1, 3);
    diagonal_one[0] = vertices[3 * 0 + 0] - vertices[3 * 7 + 0];
    diagonal_one[1] = vertices[3 * 0 + 1] - vertices[3 * 7 + 1];
    diagonal_one[2] = vertices[3 * 0 + 1] - vertices[3 * 7 + 2];

    Matrix<float> diagonal_two(1, 3);
    diagonal_two[0] = vertices[3 * 1 + 0] - vertices[3 * 5 + 0];
    diagonal_two[1] = vertices[3 * 1 + 1] - vertices[3 * 5 + 1];
    diagonal_two[2] = vertices[3 * 1 + 2] - vertices[3 * 5 + 2];

    Matrix<float> diagonal_three(1, 3);
    diagonal_three[0] = vertices[3 * 2 + 0] - vertices[3 * 6 + 0];
    diagonal_three[1] = vertices[3 * 2 + 1] - vertices[3 * 6 + 1];
    diagonal_three[2] = vertices[3 * 2 + 2] - vertices[3 * 6 + 2];

    Matrix<float> diagonal_four(1, 3);
    diagonal_four[0] = vertices[3 * 3 + 0] - vertices[3 * 4 + 0];
    diagonal_four[1] = vertices[3 * 3 + 1] - vertices[3 * 4 + 1];
    diagonal_four[2] = vertices[3 * 3 + 2] - vertices[3 * 4 + 2];

    double scale_factor = 1.5 / std::max(vecLength(diagonal_one), std::max(vecLength(diagonal_two), std::max(vecLength(diagonal_three), vecLength(diagonal_four))));

    minicell_scaling[0] = scale_factor;
    minicell_scaling[5] = scale_factor;
    minicell_scaling[10] = scale_factor;

    // Minicell backdrop (Defines overall position)
    double rect_side = std::min(this->width(), this->height()) * 0.3;
    QRectF minicell_rect(this->width() - rect_side - 5, 5, rect_side, rect_side);

    painter->setPen(*normal_pen);
    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(minicell_rect, 5, 5, Qt::AbsoluteSize);

    beginRawGLCalls(painter);
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);
    glDisable(GL_CLIP_PLANE4);
    glDisable(GL_CLIP_PLANE5);

    setVbo(minicell_vbo, vertices.data(), vertices.size(), GL_DYNAMIC_DRAW);


    // Generate indices for glDrawElements
    Matrix<GLuint> a_indices(1, 2, 0);
    a_indices[1] = 1;
    Matrix<GLuint> b_indices(1, 2, 0);
    b_indices[1] = 2;
    Matrix<GLuint> c_indices(1, 2, 0);
    c_indices[1] = 3;
    Matrix<GLuint> wire;
    unsigned int wire_buf[] = {1, 4, 2, 4, 1, 6, 4, 7, 2, 5, 3, 6, 3, 5, 5, 7, 6, 7};
    wire.setDeep(1, 18, wire_buf);

    // Draw it
    const qreal retinaScale = this->devicePixelRatio();
    glViewport(minicell_rect.left(),
               (this->height() - minicell_rect.bottom())*retinaScale,
               minicell_rect.width()*retinaScale,
               minicell_rect.height()*retinaScale);
    glLineWidth(3.0);

    std_3d_col_program->bind();

    glEnableVertexAttribArray(std_3d_col_fragpos);

    ctc_matrix.setWindow(200, 200);
    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    ctc_matrix.setWindow(this->width(), this->height());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, minicell_view_matrix.colmajor().toFloat().data());

    glBindBuffer(GL_ARRAY_BUFFER, minicell_vbo);
    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniform4fv(std_3d_col_color, 1, red.data());
    glDrawElements(GL_LINES,  2, GL_UNSIGNED_INT, a_indices.data());
    glUniform4fv(std_3d_col_color, 1, green.data());
    glDrawElements(GL_LINES,  2, GL_UNSIGNED_INT, b_indices.data());
    glUniform4fv(std_3d_col_color, 1, blue.data());
    glDrawElements(GL_LINES,  2, GL_UNSIGNED_INT, c_indices.data());

    ColorMatrix<GLfloat> wire_color = clear_color_inverse;
    wire_color[3] = 0.7;
    glUniform4fv(std_3d_col_color, 1, wire_color.data());
    glLineWidth(0.7);
    glDrawElements(GL_LINES,  18, GL_UNSIGNED_INT, wire.data());

    glDisableVertexAttribArray(std_3d_col_fragpos);

    glLineWidth(1.0);

    std_3d_col_program->release();

    glViewport(0, 0, this->width()*retinaScale, this->height()*retinaScale);

    endRawGLCalls(painter);

    // Minicell text
    Matrix<float> x_2d(1, 2, 0), y_2d(1, 2, 0), z_2d(1, 2, 0);
    getPosition2D(x_2d.data(), vertices.data() + 3, &minicell_view_matrix);
    getPosition2D(y_2d.data(), vertices.data() + 6, &minicell_view_matrix);
    getPosition2D(z_2d.data(), vertices.data() + 9, &minicell_view_matrix);

    painter->setFont(*font_mono_13bi);

    painter->drawText(QPointF((x_2d[0] + 1.0) * 0.5 * minicell_rect.width() + minicell_rect.left(), (1.0 - ( x_2d[1] + 1.0) * 0.5) *minicell_rect.height() + minicell_rect.top()), QString("a*"));
    painter->drawText(QPointF((y_2d[0] + 1.0) * 0.5 * minicell_rect.width() + minicell_rect.left(), (1.0 - ( y_2d[1] + 1.0) * 0.5) *minicell_rect.height() + minicell_rect.top()), QString("b*"));
    painter->drawText(QPointF((z_2d[0] + 1.0) * 0.5 * minicell_rect.width() + minicell_rect.left(), (1.0 - ( z_2d[1] + 1.0) * 0.5) *minicell_rect.height() + minicell_rect.top()), QString("c*"));

    painter->setFont(*font_mono_10b);
}

void VolumeOpenGLWidget::getPosition2D(float * pos_2d, float * pos_3d, Matrix<double> * transform)
{
    Matrix<float> pos_3d_matrix;
    Matrix<float> pos_2d_matrix(4, 1);

    pos_3d_matrix.setDeep(4, 1, pos_3d);
    pos_3d_matrix[3] = 1.0;

    pos_2d_matrix = transform->toFloat() * pos_3d_matrix;

    pos_2d[0] = pos_2d_matrix[0] / pos_2d_matrix[3];
    pos_2d[1] = pos_2d_matrix[1] / pos_2d_matrix[3];
}

void VolumeOpenGLWidget::getPosition2D(double * pos_2d, double * pos_3d, Matrix<double> * transform)
{
    Matrix<double> pos_3d_matrix;
    Matrix<double> pos_2d_matrix(4, 1);

    pos_3d_matrix.setDeep(4, 1, pos_3d);
    pos_3d_matrix[3] = 1.0;

    pos_2d_matrix = *transform * pos_3d_matrix;

    pos_2d[0] = pos_2d_matrix[0] / pos_2d_matrix[3];
    pos_2d[1] = pos_2d_matrix[1] / pos_2d_matrix[3];
}

void VolumeOpenGLWidget::drawMarkers(QPainter * painter)
{
    beginRawGLCalls(painter);

    glLineWidth(3.0);

    std_3d_col_program->bind();

    glEnableVertexAttribArray(std_3d_col_fragpos);

    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, view_matrix.colmajor().toFloat().data());

    for (int i = 0; i < markers.size(); i++)
    {
        glUniform4fv(std_3d_col_color, 1, markers[i].getColor());

        glBindBuffer(GL_ARRAY_BUFFER, marker_vbo[i]);
        glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //        glEnable(GL_DEPTH_TEST);
        glDrawArrays(GL_LINES,  0, 6);
        //        glDisable(GL_DEPTH_TEST);
    }

    glLineWidth(1.5);

    glUniform4fv(std_3d_col_color, 1, marker_line_color.data());

    glBindBuffer(GL_ARRAY_BUFFER, marker_centers_vbo);
    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawElements(GL_LINES,  n_marker_indices, GL_UNSIGNED_INT, markers_selected_indices.data());

    glDisableVertexAttribArray(std_3d_col_fragpos);

    std_3d_col_program->release();

    glLineWidth(1.0);

    endRawGLCalls(painter);

    qDebug() << n_marker_indices;
}


void VolumeOpenGLWidget::drawCountIntegral(QPainter * painter)
{
    float sum = sumViewBox();

    QString sum_string("Integrated intensity: " + QString::number(sum, 'e', 3) + " 1/Å^3");
    QRect sum_string_rect = normal_fontmetric->boundingRect(sum_string);
    sum_string_rect += QMargins(5, 5, 5, 5);
    sum_string_rect.moveTopLeft(QPoint(5, 205));

    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(sum_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(sum_string_rect, Qt::AlignCenter, sum_string);
}

void VolumeOpenGLWidget::drawHklText(QPainter * painter)
{
    painter->setFont(*font_mono_9i);

    Matrix<double> transform = (ctc_matrix * view_matrix);

    for (size_t i = 0; i < hkl_text_counter; i++)
    {
        Matrix<double> pos2d(1, 2);

        getPosition2D(pos2d.data(), hkl_text.data() + i * 6 + 3, &transform);

        QString text = "(" + QString::number((int) hkl_text[i * 6 + 0]) + "," + QString::number((int) hkl_text[i * 6 + 1]) + "," + QString::number((int) hkl_text[i * 6 + 2]) + ")";

        painter->drawText(QPointF((pos2d[0] + 1.0) * 0.5 * this->width(), (1.0 - ( pos2d[1] + 1.0) * 0.5) *this->height()), text);
    }

    painter->setPen(*normal_pen);
    painter->setFont(*font_mono_10b);
}

void VolumeOpenGLWidget::setCenterLine()
{
    centerline_coords[3] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0]) * 0.5;
    centerline_coords[4] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2]) * 0.5;
    centerline_coords[5] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4]) * 0.5;

    // Center line
    setVbo(centerline_vbo, centerline_coords.data(), 6, GL_STATIC_DRAW);
}

void VolumeOpenGLWidget::addLine()
{
    if (!isSvoInitialized)
    {
        return;
    }

    Matrix<double> a(3, 1);
    a[0] = data_view_extent[0];
    a[1] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2]) * 0.5;
    a[2] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4]) * 0.5;

    Matrix<double> b(3, 1);
    b[0] = data_view_extent[1];
    b[1] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2]) * 0.5;
    b[2] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4]) * 0.5;

    lines->append(Line(a, b));
    glGenBuffers(1, lines->last().vbo());

    glBindBuffer(GL_ARRAY_BUFFER, *lines->last().vbo());
    glBufferData(GL_ARRAY_BUFFER, lines->last().vertices().bytes(), lines->last().vertices().data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    emit linesChanged();

    //    emit lineChanged(lines->last());

    update();
}

void VolumeOpenGLWidget::removeMarkedLine()
{
    if (!isSvoInitialized)
    {
        return;
    }

    for (int i = 0; i < lines->size(); i++)
    {
        if (lines->at(i).tagged() == true)
        {
            glDeleteBuffers(1, (*lines)[i].vbo());
            lines->removeAt(i);
            i--;
        }
    }

    emit linesChanged();

    update();
}

void VolumeOpenGLWidget::copyMarkedLine()
{
    if (!isSvoInitialized)
    {
        return;
    }

    for (int i = 0; i < lines->size(); i++)
    {
        if (lines->at(i).tagged() == true)
        {
            lines->append(lines->at(i));

            lines->last().setTagged(false);

            glGenBuffers(1, lines->last().vbo());

            glBindBuffer(GL_ARRAY_BUFFER, *lines->last().vbo());
            glBufferData(GL_ARRAY_BUFFER, lines->last().vertices().bytes(), lines->last().vertices().data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    emit linesChanged();

    update();
}


void VolumeOpenGLWidget::translateMarkedLine()
{
    if (!isSvoInitialized)
    {
        return;
    }

    for (int i = 0; i < lines->size(); i++)
    {
        if (lines->at(i).tagged() == true)
        {
            (*lines)[i].setPositionA(lines->at(i).positionA() + (p_translate_vecB - p_translate_vecA));
            (*lines)[i].setPositionB(lines->at(i).positionB() + (p_translate_vecB - p_translate_vecA));

            glBindBuffer(GL_ARRAY_BUFFER, *(*lines)[i].vbo());
            glBufferData(GL_ARRAY_BUFFER, lines->at(i).vertices().bytes(), lines->at(i).vertices().data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    emit linesChanged();

    update();
}

void VolumeOpenGLWidget::refreshLine(int value)
{
    glBindBuffer(GL_ARRAY_BUFFER, *(*lines)[value].vbo());
    glBufferData(GL_ARRAY_BUFFER, lines->at(value).vertices().bytes(), lines->at(value).vertices().data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    emit lineChanged(lines->at(value));
}

void VolumeOpenGLWidget::snapLinePosA()
{
    if (!isSvoInitialized)
    {
        return;
    }

    if (currentLineIndex < lines->size())
    {
        (*lines)[currentLineIndex].setPositionA(weightpoint);
        refreshLine(currentLineIndex);
        emit linesChanged();
        update();
    }
}

void VolumeOpenGLWidget::snapLinePosB()
{
    if (!isSvoInitialized)
    {
        return;
    }

    if (currentLineIndex < lines->size())
    {
        (*lines)[currentLineIndex].setPositionB(weightpoint);
        refreshLine(currentLineIndex);
        emit linesChanged();
        update();
    }
}

void VolumeOpenGLWidget::setWeightpoint(double x, double y, double z)
{
    weightpoint[0] = x;
    weightpoint[1] = y;
    weightpoint[2] = z;
}

void VolumeOpenGLWidget::zoomToLineModelIndex(QModelIndex index)
{
    zoomToLineIndex(index.row());
}

void VolumeOpenGLWidget::zoomToLineIndex(int value)
{
    Matrix<double> a = lines->at(value).effectivePosA();
    Matrix<double> b = lines->at(value).effectivePosB();

    Matrix<double> box(3, 2);
    box[0] = std::min(a[0], b[0]);
    box[1] = std::max(a[0], b[0]);

    box[2] = std::min(a[1], b[1]);
    box[3] = std::max(a[1], b[1]);

    box[4] = std::min(a[2], b[2]);
    box[5] = std::max(a[2], b[2]);

    double side_c = vecLength(b - a);
    double side_a = lines->at(value).prismSideA();
    double side_b = lines->at(value).prismSideB();

    double max_side = std::max(std::max(side_a, side_b), side_c);


    translateToBox(box);
    zoomToValue(0.66 * (data_extent[1] - data_extent[0]) / max_side);
    update();
}

void VolumeOpenGLWidget::zoomToValue(double value)
{
    // Set the zoom ( a high value gives a close zoom )
    data_scaling[0] = value;
    data_scaling[5] = value;
    data_scaling[10] = value;

    // Set the view extent
    data_view_extent = (data_scaling * data_translation).inverse4x4() * data_extent;

    {
        std_3d_col_program->bind();

        clip_plane0 = data_view_extent.planeXY0();
        clip_plane1 = data_view_extent.planeXY1();
        clip_plane2 = data_view_extent.planeXZ0();
        clip_plane3 = data_view_extent.planeXZ1();
        clip_plane4 = data_view_extent.planeYZ0();
        clip_plane5 = data_view_extent.planeYZ1();

        glUniform4fv(std_3d_col_clip_plane0, 1, clip_plane0.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane1, 1, clip_plane1.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane2, 1, clip_plane2.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane3, 1, clip_plane3.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane4, 1, clip_plane4.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane5, 1, clip_plane5.toFloat().data());

        std_3d_col_program->release();
    }
}

void VolumeOpenGLWidget::translateToBox(Matrix<double> box)
{
    // Box [x0,x1,y0,y1,z0,z1]
    // Set the translation
    data_translation[3] = -(box[0] + 0.5 * (box[1] - box[0]));
    data_translation[7] = -(box[2] + 0.5 * (box[3] - box[2]));
    data_translation[11] = -(box[4] + 0.5 * (box[5] - box[4]));

    // Set the view extent
    data_view_extent = (data_scaling * data_translation).inverse4x4() * data_extent;

    {
        std_3d_col_program->bind();

        clip_plane0 = data_view_extent.planeXY0();
        clip_plane1 = data_view_extent.planeXY1();
        clip_plane2 = data_view_extent.planeXZ0();
        clip_plane3 = data_view_extent.planeXZ1();
        clip_plane4 = data_view_extent.planeYZ0();
        clip_plane5 = data_view_extent.planeYZ1();

        glUniform4fv(std_3d_col_clip_plane0, 1, clip_plane0.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane1, 1, clip_plane1.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane2, 1, clip_plane2.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane3, 1, clip_plane3.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane4, 1, clip_plane4.toFloat().data());
        glUniform4fv(std_3d_col_clip_plane5, 1, clip_plane5.toFloat().data());

        std_3d_col_program->release();
    }
}

void VolumeOpenGLWidget::updateProxy(QModelIndex /*index*/)
{
    update();
}

void VolumeOpenGLWidget::refreshLineIntegral(QModelIndex index)
{
    currentLineIndex = index.row();
    emit refreshLine(index.row());
}

void VolumeOpenGLWidget::releaseLines()
{
    if (!isSvoInitialized)
    {
        return;
    }

    for (int i = 0; i < lines->size(); i++)
    {
        glDeleteBuffers(1, (*lines)[i].vbo());
    }
}

void VolumeOpenGLWidget::genLines()
{
    for (int i = 0; i < lines->size(); i++)
    {
        glGenBuffers(1, (*lines)[i].vbo());

        glBindBuffer(GL_ARRAY_BUFFER, *(*lines)[i].vbo());
        glBufferData(GL_ARRAY_BUFFER, (*lines)[i].vertices().bytes(), (*lines)[i].vertices().data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (lines->size() > 0)
    {
        emit lineChanged(lines->last());
    }
}

void VolumeOpenGLWidget::drawLineIntegrationVolumeVisualAssist(QPainter * painter)
{
    beginRawGLCalls(painter);

    std_3d_col_program->bind();

    glEnableVertexAttribArray(std_3d_col_fragpos);

    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, view_matrix.colmajor().toFloat().data());

    for (int i = 0; i < lines->size(); i++)
    {
        double line_scaling;

        if (lines->at(i).tagged())
        {
            line_scaling = 2.0;
        }
        else
        {
            line_scaling = 1.0;
        }

        glUniform4fv(std_3d_col_color, 1, clear_color_inverse.data());

        glBindBuffer(GL_ARRAY_BUFFER, *(*lines)[i].vbo());
        glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        GLuint indices_a[] = {2, 5, 3, 8, 6, 9, 4, 7};
        glLineWidth(2.0 * line_scaling);
        glDrawElements(GL_LINES,  8, GL_UNSIGNED_INT, indices_a);

        GLuint indices_b[] = {4, 6, 7, 9, 5, 8, 2, 3,  5, 7, 8, 9, 3, 6, 2, 4};

        if (isBackgroundBlack)
        {
            glUniform4fv(std_3d_col_color, 1, yellow.data());
        }
        else
        {
            glUniform4fv(std_3d_col_color, 1, epic.data());
        }

        glDrawElements(GL_LINES,  16, GL_UNSIGNED_INT, indices_b);

        GLuint indices_c[] = {0, 1};

        glDrawElements(GL_LINES,  2, GL_UNSIGNED_INT, indices_c);
    }

    glDisableVertexAttribArray(std_3d_col_fragpos);

    std_3d_col_program->release();

    endRawGLCalls(painter);
}



void VolumeOpenGLWidget::drawCenterLine(QPainter * painter)
{
    beginRawGLCalls(painter);

    setCenterLine();

    std_3d_col_program->bind();
    glEnableVertexAttribArray(std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, centerline_vbo);
    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, view_matrix.colmajor().toFloat().data());


    glUniform4fv(std_3d_col_color, 1, clear_color_inverse.data());

    glDrawArrays(GL_LINES,  0, 2);

    glDisableVertexAttribArray(std_3d_col_fragpos);

    std_3d_col_program->release();

    endRawGLCalls(painter);
}

//void VolumeOpenGLWidget::drawSenseOfRotation(double zeta, double eta, double rpm)
//{
//    Matrix<float> point_coords(1, 3 * 100, 0.0);
//    point_coords[0 * 3 + 2] = -5;
//    point_coords[1 * 3 + 2] = 5;

//    for (int i = 2; i < 100; i++)
//    {
//        point_coords[i * 3 + 2] = -5.0 + (10.0 / 97.0) * (i - 2);
//        point_coords[i * 3 + 0] = 0.5 * sin(2.0 * point_coords[i * 3 + 2]);
//        point_coords[i * 3 + 1] = 0.5 * cos(2.0 * point_coords[i * 3 + 2]);
//    }

//    setVbo(point_vbo, point_coords.data(), 3 * 100, GL_DYNAMIC_DRAW);


//    std_3d_col_program->bind();
//    glEnableVertexAttribArray(std_3d_col_fragpos);

//    glBindBuffer(GL_ARRAY_BUFFER, point_vbo);
//    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
//    glBindBuffer(GL_ARRAY_BUFFER, 0);


//    double gamma = fmod(session_age.elapsed() * (rpm / 60000) * 2 * pi, 2 * pi);

//    // Vertices on the axis
//    RotationMatrix<double> point_on_axis, RyPlus, RxPlus;
//    RyPlus.setYRotation(zeta);
//    RxPlus.setXRotation(eta);
//    point_on_axis = bbox_translation * bbox_scaling * rotation * RyPlus * RxPlus;

//    // Vertices rotating around the axis
//    RotationMatrix<double> point_around_axis, axis_rotation;
//    axis_rotation.setArbRotation(zeta, eta, gamma);
//    point_around_axis = bbox_translation * bbox_scaling * rotation * axis_rotation * RyPlus * RxPlus;


//    glUniform4fv(std_3d_col_color, 1, clear_color_inverse.data());

//    glPointSize(5);


//    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
//    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, point_around_axis.colmajor().toFloat().data());
//    glDrawArrays(GL_POINTS,  2, 98);


//    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
//    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, point_on_axis.colmajor().toFloat().data());
//    glDrawArrays(GL_LINE_STRIP,  0, 2);

//    glDisableVertexAttribArray(std_3d_col_fragpos);

//    std_3d_col_program->release();
//}

void VolumeOpenGLWidget::wheelEvent(QWheelEvent * ev)
{
    //    if (!isDataExtentReadOnly)
    {
        float move_scaling = 1.0;

        if (ev->modifiers() & Qt::ShiftModifier)
        {
            move_scaling = 5.0;
        }
        else if (ev->modifiers() & Qt::ControlModifier)
        {
            move_scaling = 0.2;
        }

        double delta = 0;

        if ((ev->delta() > -1000) && (ev->delta() < 1000))
        {
            delta = move_scaling * ((double)ev->delta()) * 0.0008;
        }

        if (!(ev->buttons() & Qt::LeftButton) && !(ev->buttons() & Qt::RightButton))
        {
            if ((data_scaling[0] > 0.0001) || (delta > 0))
            {
                data_scaling[0] += data_scaling[0] * delta;
                data_scaling[5] += data_scaling[5] * delta;
                data_scaling[10] += data_scaling[10] * delta;

                data_view_extent =  (data_scaling * data_translation).inverse4x4() * data_extent;

                {
                    std_3d_col_program->bind();

                    clip_plane0 = data_view_extent.planeXY0();
                    clip_plane1 = data_view_extent.planeXY1();
                    clip_plane2 = data_view_extent.planeXZ0();
                    clip_plane3 = data_view_extent.planeXZ1();
                    clip_plane4 = data_view_extent.planeYZ0();
                    clip_plane5 = data_view_extent.planeYZ1();

                    glUniform4fv(std_3d_col_clip_plane0, 1, clip_plane0.toFloat().data());
                    glUniform4fv(std_3d_col_clip_plane1, 1, clip_plane1.toFloat().data());
                    glUniform4fv(std_3d_col_clip_plane2, 1, clip_plane2.toFloat().data());
                    glUniform4fv(std_3d_col_clip_plane3, 1, clip_plane3.toFloat().data());
                    glUniform4fv(std_3d_col_clip_plane4, 1, clip_plane4.toFloat().data());
                    glUniform4fv(std_3d_col_clip_plane5, 1, clip_plane5.toFloat().data());

                    std_3d_col_program->release();
                }

                updateUnitCellText();
            }

        }
        else
        {
            if ((bbox_scaling[0] > 0.0001) || (delta > 0))
            {
                bbox_scaling[0] += bbox_scaling[0] * delta;
                bbox_scaling[5] += bbox_scaling[5] * delta;
                bbox_scaling[10] += bbox_scaling[10] * delta;
            }
        }

        update();
    }
}

//void VolumeOpenGLWidget::translateLine()
//{
//    emit lineTranslateVecChanged(p_translate_vecB - p_translate_vecA);
//}

void VolumeOpenGLWidget::setTranslateLineA()
{
    p_translate_vecA[0] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0]) * 0.5;
    p_translate_vecA[1] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2]) * 0.5;
    p_translate_vecA[2] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4]) * 0.5;

    update();
}

void VolumeOpenGLWidget::setTranslateLineB()
{
    p_translate_vecB[0] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0]) * 0.5;
    p_translate_vecB[1] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2]) * 0.5;
    p_translate_vecB[2] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4]) * 0.5;

    update();
}

void VolumeOpenGLWidget::initializePaintTools()
{
    // Fonts
    font_mono_10b = new QFont("Monospace", 10);
    font_mono_10b->setBold(false);
    font_mono_9i = new QFont("Monospace", 9);
    font_mono_9i->setItalic(true);
    font_mono_9i->setBold(false);
    font_mono_13bi = new QFont("Monospace", 13);
    font_mono_13bi->setBold(true);
    font_mono_13bi->setItalic(true);

    // Font metrics
    normal_fontmetric = new QFontMetrics(*font_mono_10b, this);
    minicell_fontmetric = new QFontMetrics(*font_mono_13bi, this);

    // Pens
    normal_pen = new QPen;
    anything_pen = new QPen;

    // Brushes
    fill_brush = new QBrush;
    fill_brush->setStyle(Qt::SolidPattern);
    fill_brush->setColor(QColor(255, 255, 255, 155));

    normal_brush = new QBrush;
    normal_brush->setStyle(Qt::NoBrush);
}

void VolumeOpenGLWidget::initializeCL()
{
    initializeOpenCLFunctions();

    context_cl.initDevices();
    context_cl.initSharedContext();
    context_cl.initCommandQueue();

    // Build program from OpenCL kernel source
    QStringList paths;
    paths << "kernels/models.cl";
    paths << "kernels/volume_render_shared.cl";
    paths << "kernels/volume_render_svo.cl";
    paths << "kernels/volume_render_model.cl";
    paths << "kernels/integrate_image.cl";
    paths << "kernels/volume_sampler.cl";
    paths << "kernels/parallel_reduction.cl";
    paths << "kernels/integrate_line.cl";
    paths << "kernels/integrate_plane.cl";
    paths << "kernels/weightpoint.cl";

    context_cl.createProgram(paths, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    context_cl.buildProgram("-Werror -cl-std=CL1.2");


    // Kernel handles
    cl_svo_raytrace = QOpenCLCreateKernel(context_cl.program(), "svoRayTrace", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_model_raytrace = QOpenCLCreateKernel(context_cl.program(), "modelRayTrace", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_integrate_image = QOpenCLCreateKernel(context_cl.program(), "integrateImage", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_box_sampler = QOpenCLCreateKernel(context_cl.program(), "sampleScatteringVolume", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_parallel_reduce = QOpenCLCreateKernel(context_cl.program(), "parallelReduction", &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Buffers
    cl_view_matrix_inverse = QOpenCLCreateBuffer(context_cl.context(),
                             CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                             (ctc_matrix * view_matrix).inverse4x4().toFloat().bytes(),
                             NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_scalebar_rotation = QOpenCLCreateBuffer(context_cl.context(),
                           CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                           (rotation * scalebar_rotation).toFloat().bytes(),
                           NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    cl_data_extent = QOpenCLCreateBuffer(context_cl.context(),
                                         CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                         data_extent.toFloat().bytes(),
                                         NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    cl_data_view_extent = QOpenCLCreateBuffer(context_cl.context(),
                          CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                          data_view_extent.toFloat().bytes(),
                          NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    cl_tsf_parameters_svo = QOpenCLCreateBuffer(context_cl.context(),
                            CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                            tsf_parameters_svo.toFloat().bytes(),
                            NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_tsf_parameters_model = QOpenCLCreateBuffer(context_cl.context(),
                              CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                              tsf_parameters_model.toFloat().bytes(),
                              NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_misc_ints = QOpenCLCreateBuffer(context_cl.context(),
                                       CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                       misc_ints.toFloat().bytes(),
                                       NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_model_misc_floats = QOpenCLCreateBuffer(context_cl.context(),
                           CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                           model_misc_floats.toFloat().bytes(),
                           NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Enough for a 6000 x 6000 pixel texture
    cl_glb_work = QOpenCLCreateBuffer(context_cl.context(),
                                      CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                      (6000 * 6000) / (ray_loc_ws[0] * ray_loc_ws[1]) * sizeof(cl_float),
                                      NULL, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    volumeWorker->setOpenCLContext(&context_cl);
//    cl_oct_index_const = QOpenCLCreateBuffer(context_cl.context(),
//                          CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
//                          (1 + 8 + 8*8)* sizeof(cl_uint),
//                          NULL, &err);

//    if ( err != CL_SUCCESS)
//    {
//        qFatal(cl_error_cstring(err));
//    }

//    cl_oct_brick_const = QOpenCLCreateBuffer(context_cl.context(),
//                          CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
//                          (1 + 8 + 8*8)* sizeof(cl_uint),
//                          NULL, &err);

//    if ( err != CL_SUCCESS)
//    {
//        qFatal(cl_error_cstring(err));
//    }
}

void VolumeOpenGLWidget::setViewMatrices()
{
    view_matrix =           bbox_translation * bbox_scaling * data_scaling * rotation * data_translation;
//    unitcell_view_matrix =  bbox_translation * bbox_scaling * data_scaling * rotation * data_translation * U;
    scalebar_view_matrix =  bbox_translation * bbox_scaling * data_scaling * rotation * scalebar_rotation;
    minicell_view_matrix =  bbox_translation * minicell_scaling * rotation * U;

    err = QOpenCLEnqueueWriteBuffer (context_cl.queue(),
                                     cl_view_matrix_inverse,
                                     CL_TRUE,
                                     0,
                                     view_matrix.bytes() / 2,
                                     (ctc_matrix * view_matrix).inverse4x4().toFloat().data(),
                                     0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueWriteBuffer (context_cl.queue(),
                                     cl_scalebar_rotation,
                                     CL_TRUE,
                                     0,
                                     scalebar_rotation.bytes() / 2,
                                     scalebar_rotation.toFloat().data(),
                                     0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLSetKernelArg(cl_model_raytrace, 3, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    err |= QOpenCLSetKernelArg(cl_model_raytrace, 9, sizeof(cl_mem), (void *) &cl_scalebar_rotation);

    err |= QOpenCLSetKernelArg(cl_svo_raytrace, 7, sizeof(cl_mem), (void *) &cl_view_matrix_inverse);
    err |= QOpenCLSetKernelArg(cl_svo_raytrace, 12, sizeof(cl_mem), (void *) &cl_scalebar_rotation);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }
}

void VolumeOpenGLWidget::setDataExtent()
{
    err = QOpenCLEnqueueWriteBuffer (context_cl.queue(),
                                     cl_data_extent,
                                     CL_TRUE,
                                     0,
                                     data_extent.bytes() / 2,
                                     data_extent.toFloat().data(),
                                     0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueWriteBuffer (context_cl.queue(),
                                     cl_data_view_extent,
                                     CL_TRUE,
                                     0,
                                     data_view_extent.bytes() / 2,
                                     data_view_extent.toFloat().data(),
                                     0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLSetKernelArg(cl_model_raytrace, 4, sizeof(cl_mem),  &cl_data_extent);
    err |= QOpenCLSetKernelArg(cl_model_raytrace, 5, sizeof(cl_mem), &cl_data_view_extent);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    err = QOpenCLSetKernelArg(cl_svo_raytrace, 8, sizeof(cl_mem),  &cl_data_extent);
    err |= QOpenCLSetKernelArg(cl_svo_raytrace, 9, sizeof(cl_mem), &cl_data_view_extent);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    if (isSvoInitialized)
    {
        emit dataViewExtentChanged();
    }
}

void VolumeOpenGLWidget::setTsfParameters()
{
    if (!(isCLInitialized && isGLInitialized))
    {
        return;
    }

    err = QOpenCLEnqueueWriteBuffer (context_cl.queue(),
                                     cl_tsf_parameters_model,
                                     CL_TRUE,
                                     0,
                                     tsf_parameters_model.bytes() / 2,
                                     tsf_parameters_model.toFloat().data(),
                                     0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueWriteBuffer (context_cl.queue(),
                                     cl_tsf_parameters_svo,
                                     CL_TRUE,
                                     0,
                                     tsf_parameters_svo.bytes() / 2,
                                     tsf_parameters_svo.toFloat().data(),
                                     0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLSetKernelArg(cl_model_raytrace, 6, sizeof(cl_mem), &cl_tsf_parameters_model);
    err |= QOpenCLSetKernelArg(cl_svo_raytrace, 10, sizeof(cl_mem), &cl_tsf_parameters_svo);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    update();
}

void VolumeOpenGLWidget::setMiscArrays()
{
    //    model_misc_floats.print(2,"misc floats");
    if (!(isCLInitialized && isGLInitialized))
    {
        return;
    }

    misc_ints[2] = isLogarithmic;
    misc_ints[3] = isDSActive;
    misc_ints[4] = isSlicingActive;
    misc_ints[5] = isIntegration2DActive;
    misc_ints[6] = isShadowActive;
    misc_ints[7] = isIntegration3DActive;

    err = QOpenCLEnqueueWriteBuffer (context_cl.queue(),
                                     cl_misc_ints,
                                     CL_TRUE,
                                     0,
                                     misc_ints.bytes(),
                                     misc_ints.data(),
                                     0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueWriteBuffer (context_cl.queue(),
                                     cl_model_misc_floats,
                                     CL_TRUE,
                                     0,
                                     model_misc_floats.bytes() / 2,
                                     model_misc_floats.toFloat().data(),
                                     0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLSetKernelArg(cl_model_raytrace, 7, sizeof(cl_mem), &cl_model_misc_floats);
    err |= QOpenCLSetKernelArg(cl_model_raytrace, 8, sizeof(cl_mem), &cl_misc_ints);
    err |= QOpenCLSetKernelArg(cl_svo_raytrace, 11, sizeof(cl_mem), &cl_misc_ints);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    update();
}

void VolumeOpenGLWidget::setRayTexture(int percentage)
{
    if (!isCLInitialized || !isGLInitialized)
    {
        return;
    }

    if (1)
    {
        // Set a texture for the volume rendering kernel
        Matrix<int> ray_tex_dim_new(1, 2);

        ray_tex_dim_new[0] = (int)(sqrt((double) percentage * 0.01) * (float)this->width());
        ray_tex_dim_new[1] = (int)(sqrt((double) percentage * 0.01) * (float)this->height());

        // Clamp
        if (ray_tex_dim_new[0] < 16)
        {
            ray_tex_dim_new[0] = 16;
        }

        if (ray_tex_dim_new[1] < 16)
        {
            ray_tex_dim_new[1] = 16;
        }

        if (ray_tex_dim_new[0] > this->width())
        {
            ray_tex_dim_new[0] = this->width();
        }

        if (ray_tex_dim_new[1] > this->height())
        {
            ray_tex_dim_new[1] = this->height();
        }

        // Calculate the actual quality factor multiplier
        ray_tex_dim = ray_tex_dim_new;

        // Global work size
        if (ray_tex_dim[0] % ray_loc_ws[0])
        {
            ray_glb_ws[0] = ray_loc_ws[0] * (1 + (ray_tex_dim[0] / ray_loc_ws[0]));
        }
        else
        {
            ray_glb_ws[0] = ray_tex_dim[0];
        }

        if (ray_tex_dim[1] % ray_loc_ws[1])
        {
            ray_glb_ws[1] = ray_loc_ws[1] * (1 + (ray_tex_dim[1] / ray_loc_ws[1]));
        }
        else
        {
            ray_glb_ws[1] = ray_tex_dim[1];
        }

        if (isRayTexInitialized)
        {
            err = QOpenCLReleaseMemObject(ray_tex_cl);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }
        }

        // Update GL texture
        glBindTexture(GL_TEXTURE_2D, ray_tex_gl);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA32F,
            ray_tex_dim[0],
            ray_tex_dim[1],
            0,
            GL_RGBA,
            GL_FLOAT,
            NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Convert to CL texture
        ray_tex_cl = QOpenCLCreateFromGLTexture2D(context_cl.context(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, ray_tex_gl, &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        //        volumeWorker->setRayTexture(ray_tex_cl);

        isRayTexInitialized = true;

        // Integration texture
        if (isIntegrationTexInitialized)
        {
            err = QOpenCLReleaseMemObject(integration_tex_alpha_cl);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }

            err = QOpenCLReleaseMemObject(integration_tex_beta_cl);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }
        }

        cl_image_format integration_format;
        integration_format.image_channel_order = CL_INTENSITY;
        integration_format.image_channel_data_type = CL_FLOAT;

        integration_tex_alpha_cl = QOpenCLCreateImage2D ( context_cl.context(),
                                   CL_MEM_READ_WRITE  | CL_MEM_ALLOC_HOST_PTR,
                                   &integration_format,
                                   ray_tex_dim[0],
                                   ray_tex_dim[1],
                                   0,
                                   NULL,
                                   &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        integration_tex_beta_cl = QOpenCLCreateImage2D ( context_cl.context(),
                                  CL_MEM_READ_WRITE  | CL_MEM_ALLOC_HOST_PTR,
                                  &integration_format,
                                  ray_tex_dim[0],
                                  ray_tex_dim[1],
                                  0,
                                  NULL,
                                  &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        // Sampler for integration texture
        integration_sampler_cl = QOpenCLCreateSampler(context_cl.context(), false, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        isIntegrationTexInitialized = true;

        // Pass textures to CL kernels
        err = QOpenCLSetKernelArg(cl_integrate_image, 0, sizeof(cl_mem), (void *) &integration_tex_alpha_cl);
        err |= QOpenCLSetKernelArg(cl_integrate_image, 1, sizeof(cl_mem), (void *) &integration_tex_beta_cl);
        err |= QOpenCLSetKernelArg(cl_integrate_image, 3, sizeof(cl_sampler), &integration_sampler_cl);

        err |= QOpenCLSetKernelArg(cl_svo_raytrace, 0, sizeof(cl_mem), (void *) &ray_tex_cl);
        err |= QOpenCLSetKernelArg(cl_svo_raytrace, 13, sizeof(cl_mem), (void *) &integration_tex_alpha_cl);

        err |= QOpenCLSetKernelArg(cl_model_raytrace, 0, sizeof(cl_mem), (void *) &ray_tex_cl);
        err |= QOpenCLSetKernelArg(cl_model_raytrace, 10, sizeof(cl_mem), (void *) &integration_tex_alpha_cl);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }
}

void VolumeOpenGLWidget::setTsfTexture(TransferFunction &tsf)
{
    if (!(isCLInitialized && isGLInitialized))
    {
        return;
    }


    if (isTsfTexInitialized)
    {
        err = QOpenCLReleaseSampler(tsf_tex_sampler);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }

        err = QOpenCLReleaseMemObject(tsf_tex_cl);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

//    tsf.setColorScheme("Hot", "Opaque");
//    tsf.setSpline(256);

    // Buffer for tsf_tex_gl
    glBindTexture(GL_TEXTURE_2D, tsf_tex_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA32F,
        tsf.getSplined()->n(),
        1,
        0,
        GL_RGBA,
        GL_FLOAT,
        tsf.getSplined()->colmajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Buffer for tsf_tex_gl_thumb
    glBindTexture(GL_TEXTURE_2D, tsf_tex_gl_thumb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        tsf.getThumb()->n(),
        1,
        0,
        GL_RGB,
        GL_FLOAT,
        tsf.getThumb()->colmajor().toFloat().data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Buffer for tsf_tex_cl
    cl_image_format tsf_format;
    tsf_format.image_channel_order = CL_RGBA;
    tsf_format.image_channel_data_type = CL_FLOAT;

    tsf_tex_cl = QOpenCLCreateImage2D ( context_cl.context(),
                                        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                        &tsf_format,
                                        tsf.getSplined()->n(),
                                        1,
                                        0,
                                        tsf.getSplined()->colmajor().toFloat().data(),
                                        &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // The sampler for tsf_tex_cl
    tsf_tex_sampler = QOpenCLCreateSampler(context_cl.context(), true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    isTsfTexInitialized = true;

    // Set corresponding kernel arguments
    err = QOpenCLSetKernelArg(cl_svo_raytrace, 1, sizeof(cl_mem), (void *) &tsf_tex_cl);
    err |= QOpenCLSetKernelArg(cl_svo_raytrace, 6, sizeof(cl_sampler), &tsf_tex_sampler);
    err |= QOpenCLSetKernelArg(cl_model_raytrace, 1, sizeof(cl_mem), (void *) &tsf_tex_cl);
    err |= QOpenCLSetKernelArg(cl_model_raytrace, 2, sizeof(cl_sampler), &tsf_tex_sampler);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

}

float VolumeOpenGLWidget::sumViewBox()
{
    /* Sample the viewing box and put the values in an array. pr = parallel reduction */
    int box_samples_per_side = 256; // Power of two. This dictates the integration resolution
    int pr_read_size = box_samples_per_side * box_samples_per_side * box_samples_per_side;
    int pr_local_size = 64; // Power of two. Has constraints depending on GPU
    int pr_global_size = pr_read_size + (pr_read_size % pr_local_size ? pr_local_size - (pr_read_size % pr_local_size) : 0);
    int pr_padded_size = pr_global_size + pr_global_size / pr_local_size;

    /* Prepare array */
    cl_mem  cl_data_array = QOpenCLCreateBuffer(context_cl.context(),
                            CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                            pr_padded_size * sizeof(cl_float),
                            NULL,
                            &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    /* Prepare and launch the kernel */
    Matrix<size_t> box_global_size(1, 3, box_samples_per_side);
    Matrix<size_t> box_local_size(1, 3, 4);

    unsigned int n_tree_levels = misc_ints[0];
    unsigned int brick_dim = misc_ints[1];

    err = QOpenCLSetKernelArg(cl_box_sampler, 0, sizeof(cl_mem), (void *) &cl_svo_pool);
    err |= QOpenCLSetKernelArg(cl_box_sampler, 1, sizeof(cl_mem), (void *) &cl_svo_index);
    err |= QOpenCLSetKernelArg(cl_box_sampler, 2, sizeof(cl_mem), (void *) &cl_svo_brick);
    err |= QOpenCLSetKernelArg(cl_box_sampler, 3, sizeof(cl_mem), (void *) &cl_data_extent);
    err |= QOpenCLSetKernelArg(cl_box_sampler, 4, sizeof(cl_mem), (void *) &cl_data_view_extent);
    err |= QOpenCLSetKernelArg(cl_box_sampler, 5, sizeof(cl_sampler), &cl_svo_pool_sampler);
    err |= QOpenCLSetKernelArg(cl_box_sampler, 6, sizeof(cl_uint), &n_tree_levels);
    err |= QOpenCLSetKernelArg(cl_box_sampler, 7, sizeof(cl_uint), &brick_dim);
    err |= QOpenCLSetKernelArg(cl_box_sampler, 8, sizeof(cl_mem), (void *) &cl_data_array);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLEnqueueNDRangeKernel(context_cl.queue(), cl_box_sampler, 3, 0, box_global_size.data(), box_local_size.data(), 0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    float sum = sumGpuArray(cl_data_array, pr_read_size, pr_local_size);

    /* Clean up */
    err = QOpenCLReleaseMemObject(cl_data_array);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    return sum;
}

void VolumeOpenGLWidget::takeScreenShot(QString path)
{
    // Set resolution back to former value
    setRayTexture(100);
    displayResolution = false;

    QOpenGLFramebufferObjectFormat format;
    format.setSamples(64);
    format.setInternalTextureFormat(GL_RGBA32F);

    QOpenGLFramebufferObject buffy(this->size(), format);

    buffy.bind();

    // Render into buffer using max quality
    paintGL();
    glFinish();

    buffy.release();

    // Save buffer as image
    buffy.toImage().save(path);

    // Set resolution back to former value
    setRayTexture(quality_percentage);
    displayResolution = true;
}


void VolumeOpenGLWidget::drawIntegral(QPainter * painter)
{
    // Sum the rows and columns of the integrated texture (which resides as a pure OpenCL image buffer)

    // __ROWS__

    int direction = 0;
    int block_size = 128;

    // Set kernel arguments
    err = QOpenCLSetKernelArg(cl_integrate_image, 2, block_size * sizeof(cl_float), NULL);
    err |= QOpenCLSetKernelArg(cl_integrate_image, 4, sizeof(cl_int), &direction);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Launch kernel
    Matrix<size_t> local_size(1, 2);
    local_size[0] = block_size;
    local_size[1] = 1;

    Matrix<size_t> global_size(1, 2);
    global_size[0] = (block_size - (ray_tex_dim[0] % block_size)) + ray_tex_dim[0];
    global_size[1] = ray_tex_dim[1];

    Matrix<size_t> work_size(1, 2);
    work_size[0] = global_size[0];
    work_size[1] = 1;

    Matrix<size_t> global_offset(1, 2);
    global_offset[0] = 0;
    global_offset[1] = 0;

    for (size_t row = 0; row < global_size[1]; row += local_size[1])
    {
        global_offset[1] = row;

        err = QOpenCLEnqueueNDRangeKernel(context_cl.queue(), cl_integrate_image, 2, global_offset.data(), work_size.data(), local_size.data(), 0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

    err = QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    // Read the output data
    Matrix<float> output(ray_tex_dim[1], global_size[0] / block_size);

    Matrix<size_t> origin(1, 3);
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    Matrix<size_t> region(1, 3);
    region[0] = global_size[0] / block_size;
    region[1] = ray_tex_dim[1];
    region[2] = 1;


    err = QOpenCLEnqueueReadImage ( 	context_cl.queue(),
                                        integration_tex_beta_cl,
                                        CL_TRUE,
                                        origin.data(),
                                        region.data(),
                                        0,
                                        0,
                                        output.data(),
                                        0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Sum up the partially reduced array
    Matrix<float> row_sum(ray_tex_dim[1], 1, 0.0f);
    float max = 0;
    float min = 1e9;
    double sum = 0;

    for (size_t i = 0; i < output.m(); i++)
    {
        for (size_t j = 0; j < output.n(); j++)
        {
            row_sum[i] += output[i * output.n() + j];
        }

        if (row_sum[i] > max)
        {
            max = row_sum[i];
        }

        if ((row_sum[i] < min) && (row_sum[i] > 0))
        {
            min = row_sum[i];
        }

        sum += row_sum[i];
    }

    if (sum > 0)
    {
        for (size_t i = 0; i < row_sum.m(); i++)
        {
            row_sum[i] *= 100000.0 / sum;
        }

        max *=  100000.0 / sum;
        min *=  100000.0 / sum;

        if (isLogarithmic2D)
        {
            for (size_t i = 0; i < row_sum.m(); i++)
            {
                if (row_sum[i] <= 0)
                {
                    row_sum[i] = min;
                }

                row_sum[i] = log10(row_sum[i]);
            }

            max =  log10(max);
            min =  log10(min);
        }

        QPolygonF row_polygon;
        row_polygon << QPointF(0, 0);

        for (size_t i = 0; i < row_sum.m(); i++)
        {
            QPointF point_top, point_bottom;

            float value = ((row_sum[row_sum.m() - i - 1] - min) / (max - min)) * this->width() / 10.0;

            point_top.setX(value);
            point_top.setY(((float) i / (float) row_sum.m())*this->height());

            point_bottom.setX(value);
            point_bottom.setY((((float) i + 1) / (float) row_sum.m())*this->height());
            row_polygon << point_top << point_bottom;
        }

        row_polygon << QPointF(0, this->height());



        painter->setRenderHint(QPainter::Antialiasing);

        //        QBrush brush(clear_color_inverse.toQColor(),Qt::SolidPattern);

        QLinearGradient lgrad(QPointF(0, 0), QPointF(this->width() / 10.0, 0));
        lgrad.setColorAt(0.0, Qt::transparent);
        lgrad.setColorAt(1.0, clear_color_inverse.toQColor());

        QBrush histogram_brush_lg(lgrad);

        painter->setBrush(histogram_brush_lg);
        painter->setPen(*normal_pen);

        painter->drawPolygon(row_polygon);
    }


    // __COLUMNS__
    direction = 1;
    block_size = 128;

    // Set kernel arguments
    err = QOpenCLSetKernelArg(cl_integrate_image, 2, block_size * sizeof(cl_float), NULL);
    err |= QOpenCLSetKernelArg(cl_integrate_image, 4, sizeof(cl_int), &direction);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Launch kernel
    local_size[0] = 1;
    local_size[1] = block_size;

    global_size[0] = ray_tex_dim[0];
    global_size[1] = (block_size - (ray_tex_dim[1] % block_size)) + ray_tex_dim[1];

    work_size[0] = 1;
    work_size[1] = global_size[1];

    global_offset[0] = 0;
    global_offset[1] = 0;

    for (size_t column = 0; column < global_size[0]; column += local_size[0])
    {
        global_offset[0] = column;

        err = QOpenCLEnqueueNDRangeKernel(context_cl.queue(), cl_integrate_image, 2, global_offset.data(), work_size.data(), local_size.data(), 0, NULL, NULL);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

    err = QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }


    // Read the output data
    Matrix<float> output2(global_size[1] / block_size, ray_tex_dim[0]);

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;

    region[0] = ray_tex_dim[0];
    region[1] = global_size[1] / block_size;
    region[2] = 1;

    err = QOpenCLEnqueueReadImage ( 	context_cl.queue(),
                                        integration_tex_beta_cl,
                                        CL_TRUE,
                                        origin.data(),
                                        region.data(),
                                        0,
                                        0,
                                        output2.data(),
                                        0, NULL, NULL);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Sum up the partially reduced array
    Matrix<float> column_sum(1, ray_tex_dim[0], 0.0f);
    max = 0;
    min = 1e9;
    sum = 0;

    for (size_t i = 0; i < output2.n(); i++)
    {
        for (size_t j = 0; j < output2.m(); j++)
        {
            column_sum[i] += output2[j * output2.n() + i];
        }

        if (column_sum[i] > max)
        {
            max = column_sum[i];
        }

        if ((column_sum[i] < min) && (column_sum[i] > 0))
        {
            min = column_sum[i];
        }

        sum += column_sum[i];
    }

    if (sum > 0)
    {
        for (size_t i = 0; i < column_sum.n(); i++)
        {
            column_sum[i] *= 100000.0 / sum;
        }

        max *=  100000.0 / sum;
        min *=  100000.0 / sum;

        if (isLogarithmic2D)
        {
            for (size_t i = 0; i < column_sum.n(); i++)
            {
                if (column_sum[i] <= 0)
                {
                    column_sum[i] = min;
                }

                column_sum[i] = log10(column_sum[i]);
            }

            max =  log10(max);
            min =  log10(min);
        }

        QPolygonF column_polygon;
        column_polygon << QPointF(0, this->height());

        for (size_t i = 0; i < column_sum.n(); i++)
        {
            QPointF point_top, point_bottom;

            float value = this->height() - (column_sum[i] - min) / (max - min) * this->height() * 0.1;

            point_top.setX(((float) i / (float) column_sum.n())*this->width());
            point_top.setY(value);

            point_bottom.setX((((float) i + 1) / (float) column_sum.n())*this->width());
            point_bottom.setY(value);
            column_polygon << point_top << point_bottom;
        }

        column_polygon << QPointF(this->width(), this->height());



        painter->setRenderHint(QPainter::Antialiasing);

        QLinearGradient lgrad(QPointF(0, this->height() * 0.9), QPointF(0, this->height()));
        lgrad.setColorAt(0.0, clear_color_inverse.toQColor());
        lgrad.setColorAt(1.0, Qt::transparent);

        QBrush histogram_brush_lg(lgrad);

        painter->setBrush(histogram_brush_lg);
        painter->setPen(*normal_pen);

        painter->drawPolygon(column_polygon);
    }

}


void VolumeOpenGLWidget::beginRawGLCalls(QPainter * painter)
{
    painter->beginNativePainting();
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    glEnable(GL_CLIP_PLANE2);
    glEnable(GL_CLIP_PLANE3);
    glEnable(GL_CLIP_PLANE4);
    glEnable(GL_CLIP_PLANE5);
}

void VolumeOpenGLWidget::endRawGLCalls(QPainter * painter)
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);
    glDisable(GL_CLIP_PLANE4);
    glDisable(GL_CLIP_PLANE5);
    painter->endNativePainting();
}

void VolumeOpenGLWidget::setMiniCell(bool value)
{
    isMiniCellActive = value;
    update();
}

void VolumeOpenGLWidget::setOrthoGrid()
{
    isOrthoGridActive = !isOrthoGridActive;
    update();
}

void VolumeOpenGLWidget::drawRuler(QPainter * painter)
{
    // Draw ruler and alignment crosses
    QVector<QLine> lines;

    lines << QLine(ruler[0], ruler[1], ruler[2], ruler[3]);
    lines << QLine(ruler[0], ruler[1] + 8000, ruler[0], ruler[1] - 8000);
    lines << QLine(ruler[0] + 8000, ruler[1], ruler[0] - 8000, ruler[1]);
    lines << QLine(ruler[2], ruler[3] + 8000, ruler[2], ruler[3] - 8000);
    lines << QLine(ruler[2] + 8000, ruler[3], ruler[2] - 8000, ruler[3]);

    QVector<qreal> dashes;
    dashes << 2 << 2;

    anything_pen->setWidthF(1.0);
    anything_pen->setStyle(Qt::CustomDashLine);
    anything_pen->setDashPattern(dashes);
    anything_pen->setColor(QColor(
                               255.0 * clear_color_inverse[0],
                               255.0 * clear_color_inverse[1],
                               255.0 * clear_color_inverse[2],
                               255));
    painter->setPen(*anything_pen);

    painter->drawLines(lines.data(), 5);

    // Draw text with info
    ruler.print(2, "Ruler");
    double length = sqrt((ruler[2] - ruler[0]) * (ruler[2] - ruler[0]) * pixel_size[0] * pixel_size[0] + (ruler[3] - ruler[1]) * (ruler[3] - ruler[1]) * pixel_size[1] * pixel_size[1]);

    QString centerline_string(QString::number(length, 'g', 5) + " 1/Å (" + QString::number(1.0 / length, 'g', 5) + "Å)");
    QRect centerline_string_rect = normal_fontmetric->boundingRect(centerline_string);
    centerline_string_rect += QMargins(5, 5, 5, 5);
    centerline_string_rect.moveBottomLeft(QPoint(ruler[0] + 5, ruler[1] - 5));

    painter->setPen(*normal_pen);
    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(centerline_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(centerline_string_rect, Qt::AlignCenter, centerline_string);
}

void VolumeOpenGLWidget::drawGrid(QPainter * painter)
{
    // Draw grid lines, the center of the screen is (0,0)
    double screen_width = pixel_size[0] * this->width();
    double screen_height = pixel_size[1] * this->height();

    // Find appropriate tick intervals
    int levels = 2;
    int qualified_levels = 0;
    double level_min_ticks = 2.0;
    double min_pix_interdist = 20.0;
    double exponent = 5.0;

    Matrix<int> exponent_by_level(1, levels);
    Matrix<int> ticks_by_level(1, levels);

    while (qualified_levels < levels)
    {

        if (((screen_width / pow(10.0, (double) exponent)) > level_min_ticks))
        {
            if ((this->width() / (screen_width / pow(10.0, (double) exponent)) < min_pix_interdist))
            {
                break;
            }

            exponent_by_level[qualified_levels] = exponent;
            ticks_by_level[qualified_levels] = (screen_width / pow(10.0, (double) exponent)) + 1;
            qualified_levels++;
        }

        exponent--;
    }

    if (qualified_levels > 0)
    {
        // Draw lines according to tick intervals
        Matrix<QPointF> vertical_lines(1, (int)((screen_width / pow(10.0, (double) exponent_by_level[qualified_levels - 1])) + 5) * 2);
        Matrix<QPointF> horizontal_lines(1, (int)((screen_width / pow(10.0, (double) exponent_by_level[qualified_levels - 1])) + 5) * 2);

        for (int i = qualified_levels - 1; i >= 0; i--)
        {
            vertical_lines[2] = QPointF(this->width() * 0.5, this->height());
            vertical_lines[3] = QPointF(this->width() * 0.5, 0);

            for (int j = 1; j < ticks_by_level[i] / 2 + 1; j++)
            {
                vertical_lines[j * 4] = QPointF((j * pow(10.0, exponent_by_level[i])) / (screen_width * 0.5) * this->width() * 0.5 + this->width() * 0.5, this->height());
                vertical_lines[j * 4 + 1] = QPointF((j * pow(10.0, exponent_by_level[i])) / (screen_width * 0.5) * this->width() * 0.5 + this->width() * 0.5, 0);

                vertical_lines[j * 4 + 2] = QPointF((-j * pow(10.0, exponent_by_level[i])) / (screen_width * 0.5) * this->width() * 0.5 + this->width() * 0.5, this->height());
                vertical_lines[j * 4 + 3] = QPointF((-j * pow(10.0, exponent_by_level[i])) / (screen_width * 0.5) * this->width() * 0.5 + this->width() * 0.5, 0);
            }

            horizontal_lines[2] = QPointF(this->width(), this->height() * 0.5);
            horizontal_lines[3] = QPointF(0, this->height() * 0.5);

            for (int j = 1; j < ticks_by_level[i] / 2 + 1; j++)
            {
                horizontal_lines[j * 4] = QPointF(this->width(), (j * pow(10.0, exponent_by_level[i])) / (screen_height * 0.5) * this->height() * 0.5 + this->height() * 0.5);
                horizontal_lines[j * 4 + 1] = QPointF(0, (j * pow(10.0, exponent_by_level[i])) / (screen_height * 0.5) * this->height() * 0.5 + this->height() * 0.5);

                horizontal_lines[j * 4 + 2] = QPointF(this->width(), (-j * pow(10.0, exponent_by_level[i])) / (screen_height * 0.5) * this->height() * 0.5 + this->height() * 0.5);
                horizontal_lines[j * 4 + 3] = QPointF(0, (-j * pow(10.0, exponent_by_level[i])) / (screen_height * 0.5) * this->height() * 0.5 + this->height() * 0.5);
            }

            if (i == 0)
            {
                anything_pen->setWidthF(1.0);
                anything_pen->setStyle(Qt::SolidLine);
                anything_pen->setColor(QColor(50, 50, 255, 255));
                painter->setPen(*anything_pen);
            }
            else
            {
                QVector<qreal> dashes;
                dashes << 3 << 3;

                anything_pen->setWidthF(0.3);
                anything_pen->setStyle(Qt::CustomDashLine);
                anything_pen->setDashPattern(dashes);
                anything_pen->setColor(QColor(255.0 * clear_color_inverse[0],
                                              255.0 * clear_color_inverse[1],
                                              255.0 * clear_color_inverse[2],
                                              255));
                painter->setPen(*anything_pen);
            }

            painter->drawLines(vertical_lines.data() + 2, (ticks_by_level[i] / 2) * 2 + 1);
            painter->drawLines(horizontal_lines.data() + 2, (ticks_by_level[i] / 2) * 2 + 1);
        }

        normal_pen->setWidthF(1.0);
    }
}

void VolumeOpenGLWidget::alignSlicetoAStar()
{
    Matrix<double> vec(4, 1, 1);
    vec[0] = UB[0];
    vec[1] = UB[3];
    vec[2] = UB[6];

    RotationMatrix<double> zeta_rotation;
    zeta_rotation.setYRotation(zeta(vec));

    RotationMatrix<double> eta_rotation;
    eta_rotation.setXRotation(eta(vec));

    scalebar_rotation = zeta_rotation * eta_rotation;

    setViewMatrices();

    update();
}

void VolumeOpenGLWidget::alignSlicetoBStar()
{
    Matrix<double> vec(4, 1, 1);
    vec[0] = UB[1];
    vec[1] = UB[4];
    vec[2] = UB[7];

    RotationMatrix<double> zeta_rotation;
    zeta_rotation.setYRotation(zeta(vec));

    RotationMatrix<double> eta_rotation;
    eta_rotation.setXRotation(eta(vec));

    scalebar_rotation = zeta_rotation * eta_rotation;

    setViewMatrices();

    update();
}

void VolumeOpenGLWidget::alignSlicetoCStar()
{
    Matrix<double> vec(4, 1, 1);
    vec[0] = UB[2];
    vec[1] = UB[5];
    vec[2] = UB[8];

    RotationMatrix<double> zeta_rotation;
    zeta_rotation.setYRotation(zeta(vec));

    RotationMatrix<double> eta_rotation;
    eta_rotation.setXRotation(eta(vec));

    scalebar_rotation = zeta_rotation * eta_rotation;

    setViewMatrices();

    update();
}


void VolumeOpenGLWidget::alignAStartoZ()
{
    Matrix<double> vec(4, 1, 1);
    vec[0] = UB[0];
    vec[1] = UB[3];
    vec[2] = UB[6];

    RotationMatrix<double> zeta_rotation;
    zeta_rotation.setYRotation(-zeta(vec));

    RotationMatrix<double> eta_rotation;
    eta_rotation.setXRotation(-eta(vec));

    rotation = eta_rotation * zeta_rotation;

    setViewMatrices();

    update();

    //    view_matrix.inverse4x4(1);
}
void VolumeOpenGLWidget::alignBStartoZ()
{
    Matrix<double> vec(4, 1, 1);
    vec[0] = UB[1];
    vec[1] = UB[4];
    vec[2] = UB[7];

    RotationMatrix<double> zeta_rotation;
    zeta_rotation.setYRotation(-zeta(vec));

    RotationMatrix<double> eta_rotation;
    eta_rotation.setXRotation(-eta(vec));

    rotation = eta_rotation * zeta_rotation;

    setViewMatrices();

    update();
}
void VolumeOpenGLWidget::alignCStartoZ()
{
    Matrix<double> vec(4, 1, 1);
    vec[0] = UB[2];
    vec[1] = UB[5];
    vec[2] = UB[8];

    RotationMatrix<double> zeta_rotation;
    zeta_rotation.setYRotation(-zeta(vec));

    RotationMatrix<double> eta_rotation;
    eta_rotation.setXRotation(-eta(vec));

    rotation = eta_rotation * zeta_rotation;

    setViewMatrices();

    update();
}


void VolumeOpenGLWidget::alignLabXtoSliceX()
{
    RotationMatrix<double> x_aligned;
    x_aligned.setYRotation(pi * 0.5);

    rotation =  x_aligned * scalebar_rotation.inverse4x4();

    update();
}

void VolumeOpenGLWidget::alignLabYtoSliceY()
{
    RotationMatrix<double> y_aligned;
    y_aligned.setXRotation(-pi * 0.5);

    rotation =  y_aligned * scalebar_rotation.inverse4x4();

    update();
}
void VolumeOpenGLWidget::alignLabZtoSliceZ()
{
    RotationMatrix<double> z_aligned;
    z_aligned.setYRotation(0.0);

    rotation =  z_aligned * scalebar_rotation.inverse4x4();

    update();
}

void VolumeOpenGLWidget::alignSliceToLab()
{
    scalebar_rotation.setIdentity(4);

    update();
}

void VolumeOpenGLWidget::rotateLeft()
{
    RotationMatrix<double> rot;
    rot.setYRotation(pi * 0.25);

    rotation = rot * rotation;

    update();
}
void VolumeOpenGLWidget::rotateRight()
{
    RotationMatrix<double> rot;
    rot.setYRotation(-pi * 0.25);

    rotation = rot * rotation;

    update();
}
void VolumeOpenGLWidget::rotateUp()
{
    RotationMatrix<double> rot;
    rot.setXRotation(pi * 0.25);

    rotation = rot * rotation;

    update();
}
void VolumeOpenGLWidget::rotateDown()
{
    RotationMatrix<double> rot;
    rot.setXRotation(-pi * 0.25);

    rotation = rot * rotation;

    update();
}

void VolumeOpenGLWidget::rollCW()
{
    RotationMatrix<double> rot;
    rot.setZRotation(pi * 0.25);

    rotation = rot * rotation;

    update();
}
void VolumeOpenGLWidget::rollCCW()
{
    RotationMatrix<double> rot;
    rot.setZRotation(-pi * 0.25);

    rotation = rot * rotation;

    update();
}

void VolumeOpenGLWidget::computePixelSize()
{
    setViewMatrices();

    // Calculate the current pixel size()
    Matrix<double> ndc00(4, 1);
    ndc00[0] = 0.0;
    ndc00[1] = 0.0;
    ndc00[2] = -1.0;
    ndc00[3] = 1.0;

    // The height of a single pixel
    Matrix<double> ndc01(4, 1);
    ndc01[0] = 0.0;
    ndc01[1] = 2.0 / (double)this->height();
    ndc01[2] = -1.0;
    ndc01[3] = 1.0;

    // The width of a single pixel
    Matrix<double> ndc10(4, 1);
    ndc10[0] = 2.0 / (double)this->width();
    ndc10[1] = 0.0;
    ndc10[2] = -1.0;
    ndc10[3] = 1.0;

    Matrix<double> xyz_00(4, 1);
    Matrix<double> xyz_01(4, 1);
    Matrix<double> xyz_10(4, 1);

    xyz_00 = (ctc_matrix * view_matrix).inverse4x4() * ndc00;
    xyz_01 = (ctc_matrix * view_matrix).inverse4x4() * ndc01;
    xyz_10 = (ctc_matrix * view_matrix).inverse4x4() * ndc10;

    Matrix<double> w_vec = xyz_00 - xyz_10;
    Matrix<double> h_vec = xyz_00 - xyz_01;

    pixel_size[0] = std::sqrt(w_vec[0] * w_vec[0] + w_vec[1] * w_vec[1] + w_vec[2] * w_vec[2]);
    pixel_size[1] = std::sqrt(h_vec[0] * h_vec[0] + h_vec[1] * h_vec[1] + h_vec[2] * h_vec[2]);
}

void VolumeOpenGLWidget::drawOverlay(QPainter * painter)
{
    painter->setPen(*normal_pen);
    painter->setFont(*font_mono_10b);

    // Position scalebar tick labels
    if (isScalebarActive)
    {
        for (size_t i = 0; i < n_position_scalebar_ticks; i++)
        {
            painter->drawText(QPointF(position_scalebar_ticks[i * 3 + 0], position_scalebar_ticks[i * 3 + 1]), QString::number(position_scalebar_ticks[i * 3 + 2]));
        }
    }

    // Distance from (000), length of center line
    if (displayDistance)
    {
        double distance = std::sqrt(centerline_coords[3] * centerline_coords[3] + centerline_coords[4] * centerline_coords[4] + centerline_coords[5] * centerline_coords[5]);

        QString centerline_string("Distance from (000): " + QString::number(distance, 'g', 5) + " 1/Å");
        QRect centerline_string_rect = normal_fontmetric->boundingRect(centerline_string);
        centerline_string_rect += QMargins(5, 5, 5, 5);
        centerline_string_rect.moveBottomRight(QPoint(this->width() - 5, this->height() - 5));

        painter->setBrush(*fill_brush);
        painter->drawRoundedRect(centerline_string_rect, 5, 5, Qt::AbsoluteSize);
        painter->drawText(centerline_string_rect, Qt::AlignCenter, centerline_string);
    }

    // Fps
    //    if (displayFps)
    //    {
    //        QString fps_string("Fps: "+QString::number(getFps(), 'f', 0));
    //        fps_string_rect = normal_fontmetric->boundingRect(fps_string);
    //        fps_string_rect.setWidth(std::max(fps_string_width_prev, fps_string_rect.width()));
    //        fps_string_width_prev = fps_string_rect.width();
    //        fps_string_rect += QMargins(5,5,5,5);
    //        fps_string_rect.moveTopRight(QPoint(this->width()-5,5));

    //        painter->setBrush(*fill_brush);
    //        painter->drawRoundedRect(fps_string_rect, 5, 5, Qt::AbsoluteSize);
    //        painter->drawText(fps_string_rect, Qt::AlignCenter, fps_string);
    //    }

    // Texture resolution
    if (displayResolution)
    {
        QString resolution_string("Texture resolution: " + QString::number(100.0 * (ray_tex_dim[0]*ray_tex_dim[1]) / (this->width()*this->height()), 'f', 1) + "%");
        QRect resolution_string_rect = normal_fontmetric->boundingRect(resolution_string);
        resolution_string_rect += QMargins(5, 5, 5, 5);
        resolution_string_rect.moveBottomLeft(QPoint(5, this->height() - 5));

        painter->drawRoundedRect(resolution_string_rect, 5, 5, Qt::AbsoluteSize);
        painter->drawText(resolution_string_rect, Qt::AlignCenter, resolution_string);
    }

    // Draw accumulated roll for a mouse move event
    /*QString roll_string("Roll: "+QString::number(accumulated_roll*180/pi, 'g', 5)+" deg");
    QRect roll_string_rect = normal_fontmetric->boundingRect(roll_string);
    roll_string_rect += QMargins(5,5,5,5);
    roll_string_rect.moveBottomLeft(QPoint(5,this->height() -10 -resolution_string_rect.height()));

    painter->setBrush(*fill_brush);
    painter->drawRoundedRect(roll_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(roll_string_rect, Qt::AlignCenter, roll_string);*/

    // Scalebar multiplier
    /*QString multiplier_string("x"+QString::number(scalebar_multiplier)+" 1/Å");
    multiplier_string_rect = normal_fontmetric->boundingRect(multiplier_string);
    multiplier_string_rect += QMargins(5,5,5,5);
    multiplier_string_rect.moveTopRight(QPoint(this->width()-5, fps_string_rect.bottom() + 5));

    painter->drawRoundedRect(multiplier_string_rect, 5, 5, Qt::AbsoluteSize);
    painter->drawText(multiplier_string_rect, Qt::AlignCenter, multiplier_string);*/
}

void VolumeOpenGLWidget::snapLineCenter()
{
    if (currentLineIndex < lines->size())
    {
        (*lines)[currentLineIndex].setCenter(weightpoint);
        refreshLine(currentLineIndex);
        emit linesChanged();
        update();
    }
}

void VolumeOpenGLWidget::setLineCenter()
{
    if (!isSvoInitialized)
    {
        return;
    }

    Matrix<double> center(3, 1);
    center[0] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0]) * 0.5;
    center[1] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2]) * 0.5;
    center[2] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4]) * 0.5;

    if (currentLineIndex < lines->size())
    {
        (*lines)[currentLineIndex].setCenter(center);
        refreshLine(currentLineIndex);
        emit linesChanged();
        update();
    }
}

void VolumeOpenGLWidget::alignLineWithA()
{
    if (!isSvoInitialized)
    {
        return;
    }

    Matrix<double> vec(3, 1);
    vec[0] = UB[0];
    vec[1] = UB[3];
    vec[2] = UB[6];

    if (currentLineIndex < lines->size())
    {
        (*lines)[currentLineIndex].alignWithVec(vec);
        refreshLine(currentLineIndex);
        emit linesChanged();
        update();
    }
}

void VolumeOpenGLWidget::alignLineWithB()
{
    if (!isSvoInitialized)
    {
        return;
    }

    Matrix<double> vec(3, 1);
    vec[0] = UB[0+1];
    vec[1] = UB[3+1];
    vec[2] = UB[6+1];

    if (currentLineIndex < lines->size())
    {
        (*lines)[currentLineIndex].alignWithVec(vec);
        refreshLine(currentLineIndex);
        emit linesChanged();
        update();
    }
}

void VolumeOpenGLWidget::alignLineWithC()
{
    if (!isSvoInitialized)
    {
        return;
    }

    Matrix<double> vec(3, 1);
    vec[0] = UB[0+2];
    vec[1] = UB[3+2];
    vec[2] = UB[6+2];

    if (currentLineIndex < lines->size())
    {
        (*lines)[currentLineIndex].alignWithVec(vec);
        refreshLine(currentLineIndex);
        emit linesChanged();
        update();
    }
}

void VolumeOpenGLWidget::setLinePosA()
{
    if (!isSvoInitialized)
    {
        return;
    }

    Matrix<double> middle(3, 1);
    middle[0] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0]) * 0.5;
    middle[1] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2]) * 0.5;
    middle[2] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4]) * 0.5;

    if (currentLineIndex < lines->size())
    {
        (*lines)[currentLineIndex].setPositionA(middle);
        refreshLine(currentLineIndex);
        emit linesChanged();
        update();
    }
}

void VolumeOpenGLWidget::setLinePosB()
{
    if (!isSvoInitialized)
    {
        return;
    }

    Matrix<double> middle(3, 1);
    middle[0] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0]) * 0.5;
    middle[1] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2]) * 0.5;
    middle[2] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4]) * 0.5;

    if (currentLineIndex < lines->size())
    {
        (*lines)[currentLineIndex].setPositionB(middle);
        refreshLine(currentLineIndex);
        emit linesChanged();
        update();
    }
}

void VolumeOpenGLWidget::drawWeightCenter(QPainter * painter)
{
    beginRawGLCalls(painter);

    Matrix<float> weightpoint_vertices(6, 3);
    weightpoint_vertices[0] = data_view_extent[0];
    weightpoint_vertices[1] = weightpoint[1];
    weightpoint_vertices[2] = weightpoint[2];

    weightpoint_vertices[3] = data_view_extent[1];
    weightpoint_vertices[4] = weightpoint[1];
    weightpoint_vertices[5] = weightpoint[2];


    weightpoint_vertices[6] = weightpoint[0];
    weightpoint_vertices[7] = data_view_extent[2];
    weightpoint_vertices[8] = weightpoint[2];

    weightpoint_vertices[9] = weightpoint[0];
    weightpoint_vertices[10] = data_view_extent[3];
    weightpoint_vertices[11] = weightpoint[2];


    weightpoint_vertices[12] = weightpoint[0];
    weightpoint_vertices[13] = weightpoint[1];
    weightpoint_vertices[14] = data_view_extent[4];

    weightpoint_vertices[15] = weightpoint[0];
    weightpoint_vertices[16] = weightpoint[1];
    weightpoint_vertices[17] = data_view_extent[5];


    setVbo(weightpoint_vbo, weightpoint_vertices.data(), weightpoint_vertices.size(), GL_DYNAMIC_DRAW);

    std_3d_col_program->bind();
    glEnableVertexAttribArray(std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, weightpoint_vbo);
    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniform4fv(std_3d_col_color, 1, red.data());

    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, view_matrix.colmajor().toFloat().data());

    glLineWidth(2.0);

    glDrawArrays(GL_LINES,  0, 6);

    glDisableVertexAttribArray(std_3d_col_fragpos);

    std_3d_col_program->release();

    endRawGLCalls(painter);
}

void VolumeOpenGLWidget::drawPositionScalebars(QPainter * painter)
{
    beginRawGLCalls(painter);
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);
    glDisable(GL_CLIP_PLANE4);
    glDisable(GL_CLIP_PLANE5);

    scalebar_coord_count = setScaleBars();

    std_3d_col_program->bind();
    glEnableVertexAttribArray(std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, scalebar_vbo);
    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //    if (isBackgroundBlack)
    //    {
    //        glUniform4fv(std_3d_col_color, 1, magenta_light.data());
    //    }
    //    else
    //    {
    //        glUniform4fv(std_3d_col_color, 1, blue_light.data());
    //    }

    glUniform4fv(std_3d_col_color, 1, clear_color_inverse.data());

    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, scalebar_view_matrix.colmajor().toFloat().data());

    glDrawArrays(GL_LINES,  0, scalebar_coord_count);

    glDisableVertexAttribArray(std_3d_col_fragpos);

    std_3d_col_program->release();

    endRawGLCalls(painter);
}

void VolumeOpenGLWidget::drawLineTranslationVec(QPainter * painter)
{
    beginRawGLCalls(painter);

    Matrix<float> vertices(3, 2);

    vertices[0] = p_translate_vecA[0];
    vertices[1] = p_translate_vecA[1];
    vertices[2] = p_translate_vecA[2];

    vertices[3] = p_translate_vecB[0];
    vertices[4] = p_translate_vecB[1];
    vertices[5] = p_translate_vecB[2];

    setVbo(line_translate_vbo, vertices.data(), vertices.size(), GL_DYNAMIC_DRAW);

    // Draw the lab reference frame
    std_3d_col_program->bind();
    glEnableVertexAttribArray(std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, line_translate_vbo);
    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniform4fv(std_3d_col_color, 1, clear_color_inverse.data());

    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, (view_matrix).colmajor().toFloat().data());

    GLuint indices[] = {0, 1};
    glLineWidth(2.0);
    glDrawElements(GL_LINES,  2, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(std_3d_col_fragpos);

    std_3d_col_program->release();

    endRawGLCalls(painter);
}

void VolumeOpenGLWidget::drawViewExtent(QPainter * painter)
{
    beginRawGLCalls(painter);
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);
    glDisable(GL_CLIP_PLANE4);
    glDisable(GL_CLIP_PLANE5);

    // Draw the lab reference frame
    std_3d_col_program->bind();
    glEnableVertexAttribArray(std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, view_extent_vbo);
    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUniform4fv(std_3d_col_color, 1, clear_color_inverse.data());

    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, (bbox_translation * bbox_scaling * rotation).colmajor().toFloat().data());

    GLuint indices[] = {0, 1, 0, 2, 2, 3, 1, 3,  5, 7, 4, 5, 4, 6, 6, 7,  3, 7, 2, 6 , 1, 5, 0, 4};
    glLineWidth(0.5);
    glDrawElements(GL_LINES,  24, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(std_3d_col_fragpos);

    std_3d_col_program->release();

    endRawGLCalls(painter);
}

void VolumeOpenGLWidget::drawLabFrame(QPainter * painter)
{
    Matrix<double> model_transform = bbox_translation * bbox_scaling * rotation;

    beginRawGLCalls(painter);
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);
    glDisable(GL_CLIP_PLANE4);
    glDisable(GL_CLIP_PLANE5);

    // Generate the vertices
    Matrix<GLfloat> vertices(4, 3, 0);

    vertices[1 * 3 + 0] = (data_extent[1] - data_extent[0]) * 0.5;
    vertices[2 * 3 + 1] = (data_extent[1] - data_extent[0]) * 0.5;
    vertices[3 * 3 + 2] = (data_extent[1] - data_extent[0]) * 0.5;

    setVbo(lab_frame_vbo, vertices.data(), vertices.size(), GL_STATIC_DRAW);

    // Draw the lab reference frame
    std_3d_col_program->bind();
    glEnableVertexAttribArray(std_3d_col_fragpos);

    glBindBuffer(GL_ARRAY_BUFFER, lab_frame_vbo);
    glVertexAttribPointer(std_3d_col_fragpos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (isBackgroundBlack)
    {
        glUniform4fv(std_3d_col_color, 1, magenta_light.data());
    }
    else
    {
        glUniform4fv(std_3d_col_color, 1, blue_light.data());
    }

    glUniformMatrix4fv(std_3d_col_projection_transform, 1, GL_FALSE, ctc_matrix.colmajor().toFloat().data());
    glUniformMatrix4fv(std_3d_col_model_transform, 1, GL_FALSE, model_transform.colmajor().toFloat().data());

    GLuint indices[] = {0, 1, 0, 2, 0, 3};
    glDrawElements(GL_LINES,  6, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(std_3d_col_fragpos);

    std_3d_col_program->release();

    endRawGLCalls(painter);

    // Draw text to indicate lab reference frame directions
    painter->setPen(*normal_pen);
    painter->setFont(*font_mono_10b);
    painter->setBrush(*fill_brush);

    Matrix<double> transform = (ctc_matrix * model_transform);

    Matrix<float> x_2d(1, 2, 0), y_2d(1, 2, 0), z_2d(1, 2, 0);
    getPosition2D(x_2d.data(), vertices.data() + 3, &transform);
    getPosition2D(y_2d.data(), vertices.data() + 6, &transform);
    getPosition2D(z_2d.data(), vertices.data() + 9, &transform);

    painter->drawText(QPointF((x_2d[0] + 1.0) * 0.5 * this->width(), (1.0 - ( x_2d[1] + 1.0) * 0.5) *this->height()), QString("X (towards source)"));
    painter->drawText(QPointF((y_2d[0] + 1.0) * 0.5 * this->width(), (1.0 - ( y_2d[1] + 1.0) * 0.5) *this->height()), QString("Y (up)"));
    painter->drawText(QPointF((z_2d[0] + 1.0) * 0.5 * this->width(), (1.0 - ( z_2d[1] + 1.0) * 0.5) *this->height()), QString("Z"));
}

void VolumeOpenGLWidget::drawCountScalebar(QPainter * painter)
{
    /*
     * Based on the current display values (min and max), draw ticks on the counts scalebar. There are major and minor ticks. This function shold be redone to better treat the case of logarithmic values
     * */

//    double data_min, data_max;
//    double tick_interdist_min = 10; // pixels
//    double exponent;

    // Draw transfer function bounding box (Defines position)
    double height = 0.7 * this->height();
    double width = height * 0.1;
    QRectF tsf_rect(5, (this->height() - height) * 0.5, width, height);

//    QBrush brush(QColor(0, 0, 0, 0));
//    QPen pen(QColor(clear_color_inverse.toQColor()));

//    // Painter
//    QFont f("Monospace", 9);
//    QFontMetricsF fm(f);
//    painter->setFont(f);
//    painter->setPen(pen);
//    painter->setBrush(brush);
//    painter->drawRect(tsf_rect);

//    // Rectangle in GL coords
    Matrix<GLfloat> gl_tsf_rect;
    gl_tsf_rect = glRect(tsf_rect);

//    // Find appropriate tick positions
//    if (isModelActive)
//    {
//        data_min = tsf_parameters_model[2];
//        data_max = tsf_parameters_model[3];
//    }
//    else
//    {
//        data_min = tsf_parameters_svo[2];
//        data_max = tsf_parameters_svo[3];
//    }

//    if (isLogarithmic)
//    {
//        if (data_min <= 0)
//        {
//            data_min = 1.0e-3;
//        }

//        if (data_max <= 0)
//        {
//            data_min = 1.0e-3;
//        }

//        data_min = log10(data_min);
//        data_max = log10(data_max);
//    }

//    if (data_min < data_max)
//    {
//        double start, current;
//        size_t iter = 0, num_ticks = 0;
//        tickzerize(data_min, data_max, (double) tsf_rect.height(), tick_interdist_min, &exponent, &start, &num_ticks);
//        current = start;
//        n_count_scalebar_ticks = 0, n_count_minor_scalebar_ticks = 0;;

//        Matrix<double> ticks(num_ticks + 1, 4);

//        while ((current < data_max) && (iter < ticks.size() / 4))
//        {
//            ticks[iter * 4 + 0] = -1.0 + ((tsf_rect.right() - tsf_rect.width() * 0.15) / (double) this->width()) * 2.0;
//            ticks[iter * 4 + 1] = -1.0 + ((this->height() - tsf_rect.bottom() + (current - data_min) / (data_max - data_min) * tsf_rect.height()) / (double) this->height()) * 2.0;
//            ticks[iter * 4 + 2] = -1.0 + ((tsf_rect.right() + tsf_rect.width() * 0.1) / (double) this->width()) * 2.0;
//            ticks[iter * 4 + 3] = -1.0 + ((this->height() - tsf_rect.bottom() + (current - data_min) / (data_max - data_min) * tsf_rect.height()) / (double) this->height()) * 2.0;

//            if (((int)round(current * pow(10.0, -exponent)) % 10) == 0)
//            {
//                ticks[iter * 4 + 0] = -1.0 + ((tsf_rect.right() - tsf_rect.width() * 0.35) / (double) this->width()) * 2.0;
//                ticks[iter * 4 + 2] = -1.0 + ((tsf_rect.right() + tsf_rect.width() * 0.25) / (double) this->width()) * 2.0;

//                if (n_count_scalebar_ticks < count_major_scalebar_ticks.size())
//                {
//                    count_major_scalebar_ticks[n_count_scalebar_ticks * 3 + 0] = tsf_rect.right() + tsf_rect.width() * 0.40;
//                    count_major_scalebar_ticks[n_count_scalebar_ticks * 3 + 1] = tsf_rect.bottom() - (current - data_min) / (data_max - data_min) * tsf_rect.height() + fm.xHeight() * 0.5;

//                    if (isLogarithmic)
//                    {
//                        count_major_scalebar_ticks[n_count_scalebar_ticks * 3 + 2] = pow(10, current);
//                    }
//                    else
//                    {
//                        count_major_scalebar_ticks[n_count_scalebar_ticks * 3 + 2] = current;
//                    }

//                    n_count_scalebar_ticks++;
//                }
//            }

//            if (n_count_minor_scalebar_ticks < count_minor_scalebar_ticks.size())
//            {
//                count_minor_scalebar_ticks[n_count_minor_scalebar_ticks * 3 + 0] = tsf_rect.right() + tsf_rect.width() * 0.20;
//                count_minor_scalebar_ticks[n_count_minor_scalebar_ticks * 3 + 1] = tsf_rect.bottom() - (current - data_min) / (data_max - data_min) * tsf_rect.height() + fm.xHeight() * 0.5;

//                if (isLogarithmic)
//                {
//                    count_minor_scalebar_ticks[n_count_minor_scalebar_ticks * 3 + 2] = pow(10, current);
//                }
//                else
//                {
//                    count_minor_scalebar_ticks[n_count_minor_scalebar_ticks * 3 + 2] = current;
//                }

//                n_count_minor_scalebar_ticks++;
//            }

//            current += pow(10.0, exponent);
//            iter++;
//        }

        beginRawGLCalls(painter);
        glDisable(GL_DEPTH_TEST);

//        setVbo(count_scalebar_vbo, ticks.toFloat().data(), iter * 4, GL_STATIC_DRAW);

        // Draw transfer function texture
        std_2d_tex_program->bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tsf_tex_gl_thumb);
        std_2d_tex_program->setUniformValue(std_2d_tex_texture, 0);

        GLfloat texpos[] =
        {
            0.0, 1.0,
            0.0, 0.0,
            1.0, 0.0,
            1.0, 1.0
        };

        GLuint indices[] = {0, 1, 3, 1, 2, 3};

        glUniformMatrix4fv(std_2d_tex_transform, 1, GL_FALSE, identity.data());

        glVertexAttribPointer(std_2d_tex_fragpos, 2, GL_FLOAT, GL_FALSE, 0, gl_tsf_rect.data());
        glVertexAttribPointer(std_2d_tex_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

        glEnableVertexAttribArray(std_2d_tex_fragpos);
        glEnableVertexAttribArray(std_2d_tex_pos);

        glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

        glDisableVertexAttribArray(std_2d_tex_pos);
        glDisableVertexAttribArray(std_2d_tex_fragpos);
        glBindTexture(GL_TEXTURE_2D, 0);

        std_2d_tex_program->release();


        // Draw the ticks
//        std_2d_col_program->bind();
//        glEnableVertexAttribArray(std_2d_col_fragpos);

//        glBindBuffer(GL_ARRAY_BUFFER, count_scalebar_vbo);
//        glVertexAttribPointer(std_2d_col_fragpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
//        glBindBuffer(GL_ARRAY_BUFFER, 0);

//        glUniformMatrix4fv(std_2d_col_transform, 1, GL_FALSE, identity.data());

//        glUniform4fv(std_2d_col_color, 1, clear_color_inverse.data());

//        glLineWidth(1.0);

//        glDrawArrays(GL_LINES,  0, iter * 2);

//        glDisableVertexAttribArray(std_2d_col_fragpos);

//        std_2d_col_program->release();

        endRawGLCalls(painter);
//    }

    // Count scalebar tick labels
//    if (n_count_scalebar_ticks >= 2)
//    {
//        for (size_t i = 0; i < n_count_scalebar_ticks; i++)
//        {
//            double value = count_major_scalebar_ticks[i * 3 + 2];
//            painter->drawText(QPointF(count_major_scalebar_ticks[i * 3 + 0], count_major_scalebar_ticks[i * 3 + 1]), QString::number(value, 'g', 4));
//        }
//    }
//    else
//    {
//        for (size_t i = 0; i < n_count_minor_scalebar_ticks; i++)
//        {
//            double value = count_minor_scalebar_ticks[i * 3 + 2];
//            painter->drawText(QPointF(count_minor_scalebar_ticks[i * 3 + 0], count_minor_scalebar_ticks[i * 3 + 1]), QString::number(value, 'g', 4));
//        }
//    }

}

Matrix<GLfloat> VolumeOpenGLWidget::glRect(QRectF &qt_rect)
{
    Matrix<GLfloat> gl_rect(1, 8);

    qreal x, y, w, h;
    qreal xf, yf, wf, hf;
    qt_rect = qt_rect.normalized();
    qt_rect.getRect(&x, &y, &w, &h);

    xf = (x / (qreal) this->width()) * 2.0 - 1.0;
    yf = 1.0 - (y + h) / (qreal) this->height() * 2.0;
    wf = (w / (qreal) this->width()) * 2.0;
    hf = (h / (qreal) this->height()) * 2.0;

    gl_rect[0] = (GLfloat) xf;
    gl_rect[1] = (GLfloat) yf;
    gl_rect[2] = (GLfloat) xf + wf;
    gl_rect[3] = (GLfloat) yf;
    gl_rect[4] = (GLfloat) xf + wf;
    gl_rect[5] = (GLfloat) yf + hf;
    gl_rect[6] = (GLfloat) xf;
    gl_rect[7] = (GLfloat) yf + hf;

    return gl_rect;
}

void VolumeOpenGLWidget::addMarker()
{
    if (markers.size() < 10)
    {
        Matrix<double> xyz(1, 3);
        xyz[0] = data_view_extent[0] + (data_view_extent[1] - data_view_extent[0]) * 0.5;
        xyz[1] = data_view_extent[2] + (data_view_extent[3] - data_view_extent[2]) * 0.5;
        xyz[2] = data_view_extent[4] + (data_view_extent[5] - data_view_extent[4]) * 0.5;

        markers.append(Marker(xyz[0], xyz[1], xyz[2]));

        Matrix<float> vertices = markers.last().getVerts();


        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        marker_vbo.append(vbo);
    }
}

void VolumeOpenGLWidget::tickzerize(double min, double max, double size, double min_interdist, double * qualified_exponent, double * start, size_t * num_ticks)
{
    double delta = max - min;
    double exponent = -10;

    while (exponent < 10)
    {
        if ((size / (delta / pow(10.0, exponent))) >= min_interdist)
        {
            *qualified_exponent = exponent;
            *num_ticks = delta / pow(10.0, exponent);
            break;
        }

        exponent++;
    }

    *start = ((int) ceil(min * pow(10.0, -*qualified_exponent))) * pow(10.0, *qualified_exponent);;
}

void VolumeOpenGLWidget::drawRayTex(QPainter * painter)
{
    beginRawGLCalls(painter);

    // Volume rendering
    setShadowVector();

    if (isModelActive)
    {
        raytrace(cl_model_raytrace);
    }
    else if (isSvoInitialized)
    {
        raytrace(cl_svo_raytrace);
    }

    // Draw texture given one of the above is true
    if (isModelActive || isSvoInitialized)
    {
        std_2d_tex_program->bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ray_tex_gl);
        std_2d_tex_program->setUniformValue(std_2d_tex_texture, 0);

        GLfloat fragpos[] =
        {
            -1.0, -1.0,
            1.0, -1.0,
            1.0, 1.0,
            -1.0, 1.0
        };

        GLfloat texpos[] =
        {
            0.0, -1.0,
            1.0, -1.0,
            1.0, -2.0,
            0.0, -2.0
        };

        GLuint indices[] = {0, 1, 3, 1, 2, 3};

        glUniformMatrix4fv(std_2d_col_transform, 1, GL_FALSE, identity.data());

        glVertexAttribPointer(std_2d_tex_fragpos, 2, GL_FLOAT, GL_FALSE, 0, fragpos);
        glVertexAttribPointer(std_2d_tex_pos, 2, GL_FLOAT, GL_FALSE, 0, texpos);

        glEnableVertexAttribArray(std_2d_tex_fragpos);
        glEnableVertexAttribArray(std_2d_tex_pos);

        glDisable(GL_DEPTH_TEST);
        glDrawElements(GL_TRIANGLES,  6,  GL_UNSIGNED_INT,  indices);

        glDisableVertexAttribArray(std_2d_tex_pos);
        glDisableVertexAttribArray(std_2d_tex_fragpos);
        glBindTexture(GL_TEXTURE_2D, 0);

        std_2d_tex_program->release();
    }

    endRawGLCalls(painter);
}

void VolumeOpenGLWidget::raytrace(cl_kernel kernel)
{
    // Aquire shared CL/GL objects
    glFinish();
    err = QOpenCLEnqueueAcquireGLObjects(context_cl.queue(), 1, &ray_tex_cl, 0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Launch rendering kernel
    Matrix<size_t> area_per_call(1, 2);
    area_per_call[0] = 128;
    area_per_call[1] = 128;

    Matrix<size_t> call_offset(1, 2);
    call_offset[0] = 0;
    call_offset[1] = 0;

    // Launch the kernel and take the time
    ray_kernel_timer.start();

    for (size_t glb_x = 0; glb_x < ray_glb_ws[0]; glb_x += area_per_call[0])
    {
        for (size_t glb_y = 0; glb_y < ray_glb_ws[1]; glb_y += area_per_call[1])
        {
            call_offset[0] = glb_x;
            call_offset[1] = glb_y;

            err = QOpenCLEnqueueNDRangeKernel(context_cl.queue(), kernel, 2, call_offset.data(), area_per_call.data(), ray_loc_ws.data(), 0, NULL, NULL);

            if ( err != CL_SUCCESS)
            {
                qFatal(cl_error_cstring(err));
            }
        }
    }

    err = QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Release shared CL/GL objects
    err = QOpenCLEnqueueReleaseGLObjects(context_cl.queue(), 1, &ray_tex_cl, 0, 0, 0);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    err = QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }
}

void VolumeOpenGLWidget::setSvo(SparseVoxelOctree * svo)
{
    // Load the contents into a CL texture
    if (isSvoInitialized)
    {
        err = QOpenCLReleaseMemObject(cl_svo_brick);

        err |= QOpenCLReleaseMemObject(cl_svo_index);

        err |= QOpenCLReleaseMemObject(cl_svo_pool);

        if ( err != CL_SUCCESS)
        {
            qFatal(cl_error_cstring(err));
        }
    }

    err =   QOpenCLFinish(context_cl.queue());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    isSvoInitialized = false;

    releaseLines();
    lines = svo->lines();

    data_extent.setDeep(4, 2, svo->extent().data());
    setViewExtentVbo();
    data_view_extent.setDeep(4, 2, svo->extent().data());

    misc_ints[0] = (int) svo->levels();;
    misc_ints[1] = (int) svo->brickOuterDimension();
    tsf_parameters_svo[2] = svo->minMax().at(0);
    tsf_parameters_svo[3] = svo->minMax().at(1);

    setDataExtent();
    resetViewMatrix();
    setMiscArrays();
    setTsfParameters();
    isModelActive = false;

    size_t n_bricks = svo->pool()->size() / (svo->brickOuterDimension() * svo->brickOuterDimension() * svo->brickOuterDimension());

    Matrix<size_t> pool_dim(1, 3);
    pool_dim[0] = (1 << svo->brickPoolPower()) * svo->brickOuterDimension();
    pool_dim[1] = (1 << svo->brickPoolPower()) * svo->brickOuterDimension();
    pool_dim[2] = ((n_bricks) / ((1 << svo->brickPoolPower()) * (1 << svo->brickPoolPower())) + 1) * svo->brickOuterDimension();





    cl_svo_index = QOpenCLCreateBuffer(context_cl.context(),
                                       CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       svo->index()->size() * sizeof(cl_uint),
                                       svo->index()->data(),
                                       &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_svo_brick = QOpenCLCreateBuffer(context_cl.context(),
                                       CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       svo->brick()->size() * sizeof(cl_uint),
                                       svo->brick()->data(),
                                       &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_image_format cl_pool_format;
    cl_pool_format.image_channel_order = CL_INTENSITY;
    cl_pool_format.image_channel_data_type = CL_FLOAT;

    Matrix<float> tmp(1, pool_dim[0]*pool_dim[1]*pool_dim[2], 0);

    // This will be obsolete when the voxels are stored in a better way
    for (size_t i = 0; i < svo->pool()->size(); i++)
    {
        tmp[i] = (*svo->pool())[i];
    }

    cl_svo_pool = QOpenCLCreateImage3D ( context_cl.context(),
                                         CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                         &cl_pool_format,
                                         pool_dim[0],
                                         pool_dim[1],
                                         pool_dim[2],
                                         0,
                                         0,
                                         tmp.data(),
                                         &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    cl_svo_pool_sampler = QOpenCLCreateSampler(context_cl.context(), CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR, &err);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    // Send stuff to the kernel
    err = QOpenCLSetKernelArg(cl_svo_raytrace, 2, sizeof(cl_mem), &cl_svo_pool);
    err |= QOpenCLSetKernelArg(cl_svo_raytrace, 3, sizeof(cl_mem), &cl_svo_index);
    err |= QOpenCLSetKernelArg(cl_svo_raytrace, 4, sizeof(cl_mem), &cl_svo_brick);
    err |= QOpenCLSetKernelArg(cl_svo_raytrace, 5, sizeof(cl_sampler), &cl_svo_pool_sampler);

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }

    genLines();

    isSvoInitialized = true;

    update();
}

void VolumeOpenGLWidget::setSvoMetadata(SparseVoxelOctree * svo)
{
    if (!isSvoInitialized) return;

    releaseLines();
    lines = svo->lines();
    genLines();

    update();
}

void VolumeOpenGLWidget::resetViewMatrix()
{
    data_scaling.setIdentity(4);
    scalebar_rotation.setIdentity (4);
    data_translation.setIdentity(4);
}

size_t VolumeOpenGLWidget::setScaleBars()
{
    // Draw the scalebars. The coordinates of the ticks are independent of the position in the volume, so it is a relative scalebar.
    double length = data_view_extent[1] - data_view_extent[0];

    //    double tick_interdistance_min = 0.005 * length; // % of length

    //    int tick_levels = 0;
    //    int tick_levels_max = 2;

    size_t coord_counter = 0;

    Matrix<GLfloat> scalebar_coords(6, 3);

    //    n_position_scalebar_ticks = 0;

    //    // Calculate positions of ticks
    //    for (int i = 5; i >= -5; i--)
    //    {
    //        double tick_interdistance = std::pow((double) 10.0, (double) - i);

    //        if (( tick_interdistance >= tick_interdistance_min) && (tick_levels < tick_levels_max))
    //        {
    //            int tick_number = ((length * 0.5) / tick_interdistance);

    //            double x_start = data_view_extent[0] + length * 0.5;
    //            double y_start = data_view_extent[2] + length * 0.5;
    //            double z_start = data_view_extent[4] + length * 0.5;
    //            double tick_width = tick_interdistance * 0.2;

    //            // Each tick consists of 4 points to form a cross
    //            for (int j = -tick_number; j <= tick_number; j++)
    //            {
    //                if (j != 0)
    //                {
    //                    // X-tick cross
    //                    scalebar_coords[(coord_counter + 0) * 3 + 0] = j * tick_interdistance;
    //                    scalebar_coords[(coord_counter + 0) * 3 + 1] = tick_width * 0.5;
    //                    scalebar_coords[(coord_counter + 0) * 3 + 2] = 0;

    //                    scalebar_coords[(coord_counter + 1) * 3 + 0] = j * tick_interdistance;
    //                    scalebar_coords[(coord_counter + 1) * 3 + 1] = - tick_width * 0.5;
    //                    scalebar_coords[(coord_counter + 1) * 3 + 2] = 0;

    //                    scalebar_coords[(coord_counter + 2) * 3 + 0] = j * tick_interdistance;
    //                    scalebar_coords[(coord_counter + 2) * 3 + 1] = 0;
    //                    scalebar_coords[(coord_counter + 2) * 3 + 2] = tick_width * 0.5;

    //                    scalebar_coords[(coord_counter + 3) * 3 + 0] = j * tick_interdistance;
    //                    scalebar_coords[(coord_counter + 3) * 3 + 1] = 0;
    //                    scalebar_coords[(coord_counter + 3) * 3 + 2] = - tick_width * 0.5;

    //                    // Y-tick cross
    //                    scalebar_coords[(coord_counter + 4) * 3 + 0] = tick_width * 0.5;
    //                    scalebar_coords[(coord_counter + 4) * 3 + 1] = j * tick_interdistance;
    //                    scalebar_coords[(coord_counter + 4) * 3 + 2] = 0;

    //                    scalebar_coords[(coord_counter + 5) * 3 + 0] = - tick_width * 0.5;
    //                    scalebar_coords[(coord_counter + 5) * 3 + 1] = j * tick_interdistance;
    //                    scalebar_coords[(coord_counter + 5) * 3 + 2] = 0;

    //                    scalebar_coords[(coord_counter + 6) * 3 + 0] = 0;
    //                    scalebar_coords[(coord_counter + 6) * 3 + 1] = j * tick_interdistance;
    //                    scalebar_coords[(coord_counter + 6) * 3 + 2] = tick_width * 0.5;

    //                    scalebar_coords[(coord_counter + 7) * 3 + 0] = 0;
    //                    scalebar_coords[(coord_counter + 7) * 3 + 1] = j * tick_interdistance;
    //                    scalebar_coords[(coord_counter + 7) * 3 + 2] = - tick_width * 0.5;

    //                    // Z-tick cross
    //                    scalebar_coords[(coord_counter + 8) * 3 + 0] = tick_width * 0.5;
    //                    scalebar_coords[(coord_counter + 8) * 3 + 1] = 0;
    //                    scalebar_coords[(coord_counter + 8) * 3 + 2] = j * tick_interdistance;

    //                    scalebar_coords[(coord_counter + 9) * 3 + 0] = - tick_width * 0.5;
    //                    scalebar_coords[(coord_counter + 9) * 3 + 1] = 0;
    //                    scalebar_coords[(coord_counter + 9) * 3 + 2] = j * tick_interdistance;

    //                    scalebar_coords[(coord_counter + 10) * 3 + 0] = 0;
    //                    scalebar_coords[(coord_counter + 10) * 3 + 1] = tick_width * 0.5;
    //                    scalebar_coords[(coord_counter + 10) * 3 + 2] = j * tick_interdistance;

    //                    scalebar_coords[(coord_counter + 11) * 3 + 0] = 0;
    //                    scalebar_coords[(coord_counter + 11) * 3 + 1] = - tick_width * 0.5;
    //                    scalebar_coords[(coord_counter + 11) * 3 + 2] = j * tick_interdistance;


    //                    // Get positions for tick text
    //                    if (tick_levels == tick_levels_max - 1)
    //                    {
    //                        scalebar_multiplier = tick_interdistance * 10.0;

    //                        if ((size_t) n_position_scalebar_ticks + 3 < position_scalebar_ticks.m())
    //                        {
    //                            getPosition2D(position_scalebar_ticks.data() + 3 * n_position_scalebar_ticks, scalebar_coords.data() + (coord_counter + 0) * 3, &scalebar_view_matrix);
    //                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] = (position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] + 1.0) * 0.5 * this->width();
    //                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] = (1.0 - (position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] + 1.0) * 0.5) * this->height();
    //                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 2] = j * 0.1 * scalebar_multiplier;
    //                            n_position_scalebar_ticks++;

    //                            getPosition2D(position_scalebar_ticks.data() + 3 * n_position_scalebar_ticks, scalebar_coords.data() + (coord_counter + 4) * 3, &scalebar_view_matrix);
    //                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] = (position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] + 1.0) * 0.5 * this->width();
    //                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] = (1.0 - (position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] + 1.0) * 0.5) * this->height();
    //                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 2] = j * 0.1 * scalebar_multiplier;
    //                            n_position_scalebar_ticks++;

    //                            getPosition2D(position_scalebar_ticks.data() + 3 * n_position_scalebar_ticks, scalebar_coords.data() + (coord_counter + 8) * 3, &scalebar_view_matrix);
    //                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] = (position_scalebar_ticks[3 * n_position_scalebar_ticks + 0] + 1.0) * 0.5 * this->width();
    //                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] = (1.0 - (position_scalebar_ticks[3 * n_position_scalebar_ticks + 1] + 1.0) * 0.5) * this->height();
    //                            position_scalebar_ticks[3 * n_position_scalebar_ticks + 2] = j * 0.1 * scalebar_multiplier;
    //                            n_position_scalebar_ticks++;
    //                        }
    //                    }

    //                    coord_counter += 12;
    //                }
    //            }

    //            tick_levels++;
    //        }
    //    }

    // Base cross
    // X
    scalebar_coords[(coord_counter + 0) * 3 + 0] = -length * 0.5;
    scalebar_coords[(coord_counter + 0) * 3 + 1] = 0;
    scalebar_coords[(coord_counter + 0) * 3 + 2] = 0;

    scalebar_coords[(coord_counter + 1) * 3 + 0] = length * 0.5;
    scalebar_coords[(coord_counter + 1) * 3 + 1] = 0;
    scalebar_coords[(coord_counter + 1) * 3 + 2] = 0;

    // Y
    scalebar_coords[(coord_counter + 2) * 3 + 0] = 0;
    scalebar_coords[(coord_counter + 2) * 3 + 1] = -length * 0.5;
    scalebar_coords[(coord_counter + 2) * 3 + 2] = 0;

    scalebar_coords[(coord_counter + 3) * 3 + 0] = 0;
    scalebar_coords[(coord_counter + 3) * 3 + 1] = length * 0.5;
    scalebar_coords[(coord_counter + 3) * 3 + 2] = 0;

    // Z
    scalebar_coords[(coord_counter + 4) * 3 + 0] = 0;
    scalebar_coords[(coord_counter + 4) * 3 + 1] = 0;
    scalebar_coords[(coord_counter + 4) * 3 + 2] = -length * 0.5;

    scalebar_coords[(coord_counter + 5) * 3 + 0] = 0;
    scalebar_coords[(coord_counter + 5) * 3 + 1] = 0;
    scalebar_coords[(coord_counter + 5) * 3 + 2] = length * 0.5;

    coord_counter += 6;

    setVbo(scalebar_vbo, scalebar_coords.data(), coord_counter * 3, GL_DYNAMIC_DRAW);

    return coord_counter;
}

void VolumeOpenGLWidget::setQuality(int value)
{
    quality_percentage = value;
}

void VolumeOpenGLWidget::refreshTexture()
{
    setRayTexture(quality_percentage);
}

void VolumeOpenGLWidget::setCountIntegration()
{
    isCountIntegrationActive = !isCountIntegrationActive;
}

void VolumeOpenGLWidget::setProjection()
{
    isOrthonormal = !isOrthonormal;
    ctc_matrix.setProjection(isOrthonormal);

    //    float f;

    //    if (isOrthonormal)
    //    {
    //        f = 0.9;
    //    }
    //    else
    //    {
    //        f = 0.7;
    //    }

    //    projection_scaling[0] = f;
    //    projection_scaling[5] = f;
    //    projection_scaling[10] = f;
}

void VolumeOpenGLWidget::setLabFrame()
{
    isLabFrameActive = !isLabFrameActive;
}

void VolumeOpenGLWidget::setBackground()
{
    isBackgroundBlack = !isBackgroundBlack;

    Matrix<GLfloat> tmp;
    tmp = clear_color;

    // Swap color
    clear_color = clear_color_inverse;
    clear_color_inverse = tmp;

    QColor normal_color = clear_color_inverse.toQColor();
    normal_color.setAlphaF(1.0);
    QColor fill_color = clear_color.toQColor();
    fill_color.setAlphaF(0.7);

    normal_pen->setColor(normal_color);
    anything_pen->setColor(normal_color);
    fill_brush->setColor(fill_color);

    update();
}

void VolumeOpenGLWidget::setURotation(bool value)
{
    isURotationActive = value;
}

void VolumeOpenGLWidget::setLog(bool value)
{
    isLogarithmic = value;
    setMiscArrays();
}
void VolumeOpenGLWidget::setLogarithmic2D()
{
    isLogarithmic2D = !isLogarithmic2D;
    update();
}
void VolumeOpenGLWidget::setDataStructure()
{
    isDSActive = !isDSActive;
    setMiscArrays();
}
void VolumeOpenGLWidget::setSlicing()
{
    isSlicingActive = !isSlicingActive;
    setMiscArrays();
}
void VolumeOpenGLWidget::setShadow()
{
    isShadowActive = !isShadowActive;

    setMiscArrays();
}

void VolumeOpenGLWidget::setShadowVector()
{
    Matrix<float> shadow_kernel_arg;

    shadow_kernel_arg = shadow_vector;

    shadow_kernel_arg = rotation.inverse4x4().toFloat() * shadow_kernel_arg;

    err = QOpenCLSetKernelArg(cl_model_raytrace, 11, sizeof(cl_float4),  shadow_kernel_arg.data());

    if ( err != CL_SUCCESS)
    {
        qFatal(cl_error_cstring(err));
    }
}

void VolumeOpenGLWidget::setIntegration2D()
{
    isIntegration2DActive = !isIntegration2DActive;

    setMiscArrays();
}
void VolumeOpenGLWidget::setIntegration3D()
{
    isIntegration3DActive = !isIntegration3DActive;

    setMiscArrays();
}
void VolumeOpenGLWidget::setViewMode(int value)
{
    // This could be done more neatly by maintaining a simple index to indicate the mode. Sins of the past.

    if (value == 0)
    {
        isIntegration3DActive = true;
        isSlicingActive = false;
    }
    else if (value == 1)
    {
        isIntegration3DActive = false;
        isSlicingActive = false;
    }
    else if (value == 2)
    {
        isSlicingActive = true;
    }

    setMiscArrays();
}

void VolumeOpenGLWidget::setTsfColor(QString style)
{
//    tsf_color_scheme = value;

    tsf.setRgb(style);
    tsf.setSpline(256);

    if (isInitialized)
    {
        setTsfTexture(tsf);
    }

    update();
}
void VolumeOpenGLWidget::setTsfAlpha(QString style)
{
//    tsf_alpha_scheme = value;

    tsf.setAlpha(style);
    tsf.setSpline(256);

    if (isInitialized)
    {
        setTsfTexture(tsf);
    }

    update();
}
void VolumeOpenGLWidget::setDataMin(double value)
{
    if (isModelActive)
    {
        tsf_parameters_model[2] = value;
    }
    else
    {
        tsf_parameters_svo[2] = value;
    }

    setTsfParameters();
}
void VolumeOpenGLWidget::setDataMax(double value)
{
    if (isModelActive)
    {
        tsf_parameters_model[3] = value;
    }
    else
    {
        tsf_parameters_svo[3] = value;
    }

    setTsfParameters();
}
void VolumeOpenGLWidget::setAlpha(double value)
{
    if (isModelActive)
    {
        tsf_parameters_model[4] = value;
    }
    else
    {
        tsf_parameters_svo[4] = value;
    }

    setTsfParameters();
}
void VolumeOpenGLWidget::setBrightness(double value)
{
    if (isModelActive)
    {
        tsf_parameters_model[5] = value;
    }
    else
    {
        tsf_parameters_svo[5] = value;
    }

    setTsfParameters();
}

void VolumeOpenGLWidget::setUnitcell(bool value)
{
    isUnitcellActive = value;
    update();
}


void VolumeOpenGLWidget::setModel()
{
    isModelActive = !isModelActive;
    update();
}
void VolumeOpenGLWidget::setModelParam0(double value)
{
    model_misc_floats[0] = value;
    setMiscArrays();
}
void VolumeOpenGLWidget::setModelParam1(double value)
{
    model_misc_floats[1] = value;
    setMiscArrays();
}
void VolumeOpenGLWidget::setModelParam2(double value)
{
    model_misc_floats[2] = value;
    setMiscArrays();
}
void VolumeOpenGLWidget::setModelParam3(double value)
{
    model_misc_floats[3] = value;
    setMiscArrays();
}
void VolumeOpenGLWidget::setModelParam4(double value)
{
    model_misc_floats[4] = value;
    setMiscArrays();
}
void VolumeOpenGLWidget::setModelParam5(double value)
{
    model_misc_floats[5] = value;
    setMiscArrays();
}

void VolumeOpenGLWidget::setScalebar()
{
    isScalebarActive = !isScalebarActive;
}
void VolumeOpenGLWidget::toggleRuler()
{
    isRulerActive = !isRulerActive;
}

