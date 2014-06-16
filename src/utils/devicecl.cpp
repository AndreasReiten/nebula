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
    
    GLOBAL_VRAM_ALLOC_MAX = max_mem_alloc_size;
    
    qDebug() << GLOBAL_VRAM_ALLOC_MAX;
}

std::string DeviceCL::getDeviceInfoString()
{
    std::stringstream ss;
    ss << std::endl;
    ss << "__________________ OpenCL Device Info ____________________" << std::endl;
    ss << "CL_DEVICE_NAME:                      " << name << std::endl;
    ss << "CL_DEVICE_VERSION:                   " << version << std::endl;
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
    ss << "CL_DEVICE_GLOBAL_MEM_SIZE:           " << (global_mem_size / 1e6) << " MB" << std::endl;
    ss << "CL_DEVICE_MAX_MEM_ALLOC_SIZE:        " << (max_mem_alloc_size / 1e6) << " MB" << std::endl;
    ss << "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: " << global_mem_mem_cacheline_size << " B" << std::endl;
    ss << "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:  " << (max_constant_buffer_size / 1e3) << " KB" << std::endl;
    ss << "CL_DEVICE_LOCAL_MEM_SIZE:            " << (local_mem_size / 1e3) << " KB" << std::endl;
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

const char * cl_error_cstring(cl_int error)
{
    switch (error) {
        case CL_SUCCESS:                            return "CL_SUCCESS";
        case CL_DEVICE_NOT_FOUND:                   return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE:               return "CL_DEVICE_NOT_AVAILABLE";
        case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "CL_COMPILER_NOT_AVAILABLE";
        case CL_OUT_OF_RESOURCES:                   return "CL_OUT_OF_RESOURCES";
        case CL_OUT_OF_HOST_MEMORY:                 return "CL_OUT_OF_HOST_MEMORY";
        case CL_PROFILING_INFO_NOT_AVAILABLE:       return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case CL_MEM_COPY_OVERLAP:                   return "CL_MEM_COPY_OVERLAP";
        case CL_IMAGE_FORMAT_MISMATCH:              return "CL_IMAGE_FORMAT_MISMATCH";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE:              return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case CL_MAP_FAILURE:                        return "CL_MAP_FAILURE";
        case CL_INVALID_VALUE:                      return "CL_INVALID_VALUE";
        case CL_INVALID_DEVICE_TYPE:                return "CL_INVALID_DEVICE_TYPE";
        case CL_INVALID_PLATFORM:                   return "CL_INVALID_PLATFORM";
        case CL_INVALID_DEVICE:                     return "CL_INVALID_DEVICE";
        case CL_INVALID_CONTEXT:                    return "CL_INVALID_CONTEXT";
        case CL_INVALID_QUEUE_PROPERTIES:           return "CL_INVALID_QUEUE_PROPERTIES";
        case CL_INVALID_COMMAND_QUEUE:              return "CL_INVALID_COMMAND_QUEUE";
        case CL_INVALID_HOST_PTR:                   return "CL_INVALID_HOST_PTR";
        case CL_INVALID_MEM_OBJECT:                 return "CL_INVALID_MEM_OBJECT";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case CL_INVALID_IMAGE_SIZE:                 return "CL_INVALID_IMAGE_SIZE";
        case CL_INVALID_SAMPLER:                    return "CL_INVALID_SAMPLER";
        case CL_INVALID_BINARY:                     return "CL_INVALID_BINARY";
        case CL_INVALID_BUILD_OPTIONS:              return "CL_INVALID_BUILD_OPTIONS";
        case CL_INVALID_PROGRAM:                    return "CL_INVALID_PROGRAM";
        case CL_INVALID_PROGRAM_EXECUTABLE:         return "CL_INVALID_PROGRAM_EXECUTABLE";
        case CL_INVALID_KERNEL_NAME:                return "CL_INVALID_KERNEL_NAME";
        case CL_INVALID_KERNEL_DEFINITION:          return "CL_INVALID_KERNEL_DEFINITION";
        case CL_INVALID_KERNEL:                     return "CL_INVALID_KERNEL";
        case CL_INVALID_ARG_INDEX:                  return "CL_INVALID_ARG_INDEX";
        case CL_INVALID_ARG_VALUE:                  return "CL_INVALID_ARG_VALUE";
        case CL_INVALID_ARG_SIZE:                   return "CL_INVALID_ARG_SIZE";
        case CL_INVALID_KERNEL_ARGS:                return "CL_INVALID_KERNEL_ARGS";
        case CL_INVALID_WORK_DIMENSION:             return "CL_INVALID_WORK_DIMENSION:";
        case CL_INVALID_WORK_GROUP_SIZE:            return "CL_INVALID_WORK_GROUP_SIZE";
        case CL_INVALID_WORK_ITEM_SIZE:             return "CL_INVALID_WORK_ITEM_SIZE";
        case CL_INVALID_GLOBAL_OFFSET:              return "CL_INVALID_GLOBAL_OFFSET";
        case CL_INVALID_EVENT_WAIT_LIST:            return "CL_INVALID_EVENT_WAIT_LIST";
        case CL_INVALID_EVENT:                      return "CL_INVALID_EVENT";
        case CL_INVALID_OPERATION:                  return "CL_INVALID_OPERATION";
        case CL_INVALID_GL_OBJECT:                  return "CL_INVALID_GL_OBJECT";
        case CL_INVALID_BUFFER_SIZE:                return "CL_INVALID_BUFFER_SIZE";
        case CL_INVALID_MIP_LEVEL:                  return "CL_INVALID_MIP_LEVEL";
        default: return "Unknown";
    }
}
