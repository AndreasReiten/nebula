#include "devicecl.h"

DeviceCL::DeviceCL()
{
}

DeviceCL::DeviceCL(cl_platform_id platform_id, cl_device_id device_id)
{
    this->platform_id = platform_id;
    this->device_id = device_id;

    getDeviceInfo ();
}

DeviceCL::~DeviceCL()
{
}

cl_platform_id DeviceCL::getPlatformId()
{
    return platform_id;
}

cl_device_id DeviceCL::getDeviceId()
{
    return device_id;
}

cl_device_type DeviceCL::getDeviceType()
{
    return type;
}

void DeviceCL::getDeviceInfo ()
{
    clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, sizeof(char)*128, &platform_name, NULL);
    clGetPlatformInfo(platform_id, CL_PLATFORM_VERSION, sizeof(char)*128, &platform_version, NULL);
    clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, sizeof(char)*128, &platform_vendor, NULL);

    clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(char)*64, &name, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_VERSION, sizeof(char)*64, &version, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_TYPE, sizeof(cl_device_type), &type, NULL);
    clGetDeviceInfo(device_id, CL_DRIVER_VERSION, sizeof(char)*64, &driver_version, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_VENDOR, sizeof(char)*64, &vendor, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_VENDOR_ID, sizeof(cl_uint), &vendor_id, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_EXTENSIONS, sizeof(char)*2048, &extensions, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &image_support, NULL);

    clGetDeviceInfo(device_id, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &max_clock_frequency, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &global_mem_size, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &local_mem_size, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &max_compute_units, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(cl_ulong), &max_work_group_size, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &max_work_item_dimensions, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t)*3, max_work_item_sizes, NULL);

    clGetDeviceInfo(device_id, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &max_mem_alloc_size, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cl_uint), &global_mem_mem_cacheline_size, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(cl_uint), &max_read_image_args, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(cl_uint), &max_write_image_args, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_SAMPLERS, sizeof(cl_uint), &max_samplers, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(cl_ulong), &max_constant_buffer_size, NULL);
}

std::string DeviceCL::getDeviceInfoString()
{
    std::stringstream ss;
    ss << std::endl;
    ss << "__________________ OpenCL Device Info ____________________" << std::endl;
    ss << "CL_DEVICE_NAME:                      " << name << std::endl;
    ss << "CL_DEVICE_VERSION:                   " << version << std::endl;
//    ss << "CL_DEVICE_TYPE:                      " << type << std::endl;
    ss << "CL_DRIVER_VERSION:                   " << driver_version << std::endl;
    ss << "CL_DEVICE_VENDOR:                    " << vendor << std::endl;
    ss << "CL_DEVICE_VENDOR_ID:                 " << vendor_id << std::endl;
    ss << std::endl;
    ss << "___ Platform ___" << std::endl;
    ss << "CL_PLATFORM_NAME:                      " << platform_name << std::endl;
    ss << "CL_PLATFORM_VERSION:                   " << platform_version << std::endl;
    ss << "CL_PLATFORM_VENDOR:                    " << platform_vendor << std::endl;
    ss << std::endl;

    ss << "___ Memory and Compute Units ___" << std::endl;
    ss << "CL_DEVICE_MAX_COMPUTE_UNITS:         " << max_compute_units << std::endl;
    ss << "CL_DEVICE_MAX_CLOCK_FREQUENCY:       " << max_clock_frequency << " MHz" << std::endl;
    ss << "CL_DEVICE_GLOBAL_MEM_SIZE:           " << (global_mem_size >> 20) << " MB" << std::endl;
    ss << "CL_DEVICE_MAX_MEM_ALLOC_SIZE:        " << (max_mem_alloc_size >> 20) << " MB" << std::endl;
    ss << "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: " << global_mem_mem_cacheline_size << " B" << std::endl;
    ss << "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:  " << (max_constant_buffer_size >> 10) << " KB" << std::endl;
    ss << "CL_DEVICE_LOCAL_MEM_SIZE:            " << (local_mem_size >> 10) << " KB" << std::endl;
    ss << std::endl;

    ss << "___ Work Items and Work Groups ___" << std::endl;
    ss << "CL_DEVICE_MAX_WORK_GROUP_SIZE:       " << max_work_group_size << std::endl;
    ss << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:  " << max_work_item_dimensions << std::endl;
    ss << "CL_DEVICE_MAX_WORK_ITEM_SIZES[0]:    " << max_work_item_sizes[0] << std::endl;
    ss << "CL_DEVICE_MAX_WORK_ITEM_SIZES[1]:    " << max_work_item_sizes[1] << std::endl;
    ss << "CL_DEVICE_MAX_WORK_ITEM_SIZES[2]:    " << max_work_item_sizes[2] << std::endl;
    ss << std::endl;

    ss << "___ Misc ___" << std::endl;
    ss << "CL_DEVICE_IMAGE_SUPPORT:             " << image_support << std::endl;
    ss << "CL_DEVICE_EXTENSIONS:                " << extensions << std::endl;
    ss << "CL_DEVICE_MAX_READ_IMAGE_ARGS:       " << max_read_image_args << std::endl;
    ss << "CL_DEVICE_MAX_WRITE_IMAGE_ARGS:      " << max_write_image_args << std::endl;
    ss << "CL_DEVICE_MAX_SAMPLERS:              " << max_samplers << std::endl;
    ss << "__________________________________________________________" << std::endl;

    return ss.str();
}
