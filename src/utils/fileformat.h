#ifndef FILE_FORMATS_H
#define FILE_FORMATS_H

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <fstream>
#include <cmath>
#include <limits>

#include <QtGlobal>
#include <QDebug>

/* GL and CL*/
#include <CL/opencl.h>

/* QT */
#include <QRegExp>
#include <QString>
#include <QFileInfo>


#include "imagerender.h"
#include "miniarray.h"
#include "matrix.h"
#include "tools.h"
#include "sharedcontext.h"
#include "contextcl.h"



/* This is the file to edit/append if you would like to add your own file formats */

class PilatusFile
{
    /* Refer to PILATUS CBF header specs for details */
    public:
        ~PilatusFile();
        PilatusFile();
        PilatusFile(QString path, OpenCLContext * context);

        QString getPath() const;

        int set(QString path, OpenCLContext * context);
        int readData();
        void setOpenCLBuffers(cl_mem * cl_img_alpha, cl_mem * cl_img_beta, cl_mem * cl_img_gamma, cl_mem * cl_tsf_tex);
        int filterData(size_t * n, float * outBuf, float threshold_reduce_low, float threshold_reduce_high, float threshold_project_low, float threshold_project_high, bool isProjectionActive = true);
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
        void setBackground(float flux, float exposure_time);
        float getFlux();
        float getExpTime();
        void setProjectionKernel(cl_kernel * kernel);
        void print();
        QString getHeaderText();
        
    private:
        OpenCLContext * context_cl;
        cl_mem * cl_img_alpha;
        cl_mem * cl_img_beta;
        cl_mem * cl_img_gamma;
        cl_mem * cl_tsf_tex;
//        cl_command_queue * queue;
//        cl_context * context;
        cl_kernel * project_kernel;
        cl_int err;

        size_t loc_ws[2];
        size_t glb_ws[2];

        MiniArray<float> data_buf;
//        Matrix<float> * background;
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
