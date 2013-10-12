#ifndef DEVICECL_H
#define DEVICECL_H

#include <CL/opencl.h>
#include <iostream>
#include <sstream>
#include <string>

class DeviceCL
{
public:
    DeviceCL();
    DeviceCL(cl_platform_id platform_id, cl_device_id device_id);
    ~DeviceCL();

    std::string getDeviceInfoString();
    cl_platform_id getPlatformId();
    cl_device_id getDeviceId();
    cl_device_type getDeviceType();

private:
    void getDeviceInfo();

    cl_platform_id platform_id;
    cl_device_id device_id;

    char platform_name[128];
    char platform_version[128];
    char platform_vendor[128];

    char name[64];
    char version[64];
    cl_device_type type;
    char driver_version[64];
    char vendor[64];
    cl_uint vendor_id;
    char extensions[2048];
    cl_bool image_support;

    cl_uint max_clock_frequency;
    cl_ulong global_mem_size;
    cl_ulong local_mem_size;
    cl_uint max_compute_units;
    cl_ulong max_work_group_size;
    cl_uint max_work_item_dimensions;
    size_t max_work_item_sizes[3];

    cl_ulong max_mem_alloc_size;
    cl_uint global_mem_mem_cacheline_size;
    cl_uint max_read_image_args;
    cl_uint max_write_image_args;
    cl_uint max_samplers;
    cl_ulong max_constant_buffer_size;
};

#endif // DEVICECL_H




