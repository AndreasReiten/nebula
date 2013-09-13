#ifndef TOOLS_H
#define TOOLS_H

/* GL and CL*/
#ifdef _WIN32
    #define GLEW_STATIC
#endif

#include <QByteArray>
#include <QFile>
#include <QDateTime>
#include <QString>

#include <GL/glew.h>
#include <CL/opencl.h>
#include <CL/cl_gl.h>

#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstring>

#include "miniarray.h"
#include "matrix.h"

#pragma pack(2)
struct BMPHeader {
    unsigned short type;
    unsigned int file_size;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int raw_offset;
    unsigned int self_size;
    int pixel_width;
    int pixel_height;
    unsigned short color_planes;
    unsigned short pixel_bits;
    unsigned int comperssion;
    unsigned int image_size;
    int resolution_w;
    int resolution_h;
    unsigned int colors_used;
    unsigned int colors_important;
};
#pragma pack()

struct cl_device{
    cl_platform_id platform_id;
    cl_device_id device_id = NULL;

    char cl_device_name[64];
    char cl_device_version[64];
    char cl_driver_version[64];
    cl_uint gpu_clock_max;
    cl_uint gpu_max_mem_alloc_size;
    cl_uint gpu_global_mem_cache_line;
    cl_ulong gpu_global_mem;
    cl_ulong gpu_local_mem;
    cl_uint gpu_compute_units;
    cl_ulong gpu_work_group_size;
    cl_uint gpu_work_item_dim;
    cl_bool gpu_image_support;
    size_t gpu_work_item_sizes[3];
    cl_uint max_read_image_args;
    cl_uint max_write_image_args;
    cl_uint max_samplers;
    cl_ulong max_constant_buffer_size;
    char vendor[64];
    cl_device_type type;
    cl_uint vendor_id;
};

QByteArray open_resource(const char * path);
GLuint create_shader(const char* resource, GLenum type);

QString timeString(size_t ms);
const char * cl_error_cstring(cl_int error);
const char * gl_framebuffer_error_cstring(GLint error);
void glGetErrorMessage(const char * context);
void screenshot(int w, int h, const char* path);
void setVbo(GLuint * vbo, float * buf, size_t length);
void init_tsf(int color_style, int alpha_style, TsfMatrix<double> * transfer_function);
void writeToLogAndPrint(QString text, QString file, bool append);
#endif
