#ifndef FILE_FORMATS_H
#define FILE_FORMATS_H

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <fstream>
#include <cmath>
#include <limits>


/* GL and CL*/
#ifdef _WIN32
    #define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <CL/opencl.h>
#include <CL/cl_gl.h>

/* QT */
#include <QMouseEvent>
#include <QGLWidget>
#include <QRegExp>
#include <QString>
#include <QDebug>
#include <QFileInfo>

#ifdef _WIN32
    #include <windows.h>
#endif
#ifdef __linux__
    #include <GL/glx.h>
#endif

#include "imagerender.h"
#include "miniarray.h"
#include "matrix.h"
#include "tools.h"


/* This is the file to edit/append if you would like to add your own file formats */

class PilatusFile
{
    /* Refer to PILATUS CBF header specs for details */
    public:
        ~PilatusFile();
        PilatusFile();
        PilatusFile(QString path, cl_context * context, cl_command_queue * queue);

        QString getPath() const;

        int set(QString path, cl_context * context, cl_command_queue * queue);
        int readData();
        void setOpenCLBuffers(cl_mem * alpha_img_clgl, cl_mem * beta_img_clgl, cl_mem * gamma_img_clgl, cl_mem * tsf_img_clgl);
        int filterData(size_t * n, float * outBuf, int threshold_reduce_low, int threshold_reduce_high, int threshold_project_low, int threshold_project_high, bool isProjectionActive = true);
        //~int project(size_t * n, float * outBuf, int threshold_project_low, int threshold_project_high);

        MiniArray<float> getTest();
        int getWidth() const;
        int getHeight() const;
        size_t getBytes() const;
        float getSearchRadiusLowSuggestion();
        float getSearchRadiusHighSuggestion();
        float getQSuggestion();
        float getMaxCount();
        void clearData();
        void setBackground(Matrix<float> * buffer, float flux, float exposure_time);
        float getFlux();
        float getExpTime();
        void setProjectionKernel(cl_kernel * kernel);

    private:
        void writeLog(QString str);

        cl_mem * alpha_img_clgl;
        cl_mem * beta_img_clgl;
        cl_mem * gamma_img_clgl;
        cl_mem * tsf_img_clgl;
        cl_command_queue * queue;
        cl_context * context;
        cl_kernel * project_kernel;
        cl_int err;

        size_t loc_ws[2];
        size_t glb_ws[2];

        MiniArray<float> data_buf;
        Matrix<float> * background;
        float background_flux;
        float backgroundExpTime;

        QString path;
        size_t fast_dimension, slow_dimension;
        float max_counts;
        int STATUS_OK;

        int threshold_reduce_low, threshold_reduce_high;
        int threshold_project_low, threshold_project_high;
        float srchrad_sugg_low, srchrad_sugg_high;

        void suggestSearchRadius();
        int readHeader();
        QString regExp(QString * regular_expression, QString * source, size_t offset, size_t i);

        /* Non-optional keywords */
        QString detector;
        float pixel_size_x, pixel_size_y;
        float silicon_sensor_thickness;
        float exposure_time;
        float exposure_period;
        float tau;
        int count_cutoff;
        int threshold_setting;
        QString gain_setting;
        int n_excluded_pixels;
        QString excluded_pixels;
        QString flat_field;
        QString time_file;
        QString image_path;

        /* Optional keywords */
        float wavelength;
        int energy_range_low, energy_range_high;
        float detector_distance;
        float detector_voffset;
        float beam_x, beam_y;
        float flux;
        float filter_transmission;
        float start_angle;
        float angle_increment;
        float detector_2theta;
        float polarization;
        float alpha;
        float kappa;
        float phi;
        float phi_increment;
        float chi;
        float chi_increment;
        float omega;
        float omega_increment;
        QString oscillation_axis;
        int n_oscillations;
        float start_position;
        float position_increment;
        float shutter_time;

        float beta;
};


#endif
