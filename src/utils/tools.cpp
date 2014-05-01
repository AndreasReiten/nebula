#include "tools.h"

QString timeString(size_t ms)
{
    size_t hours = ms/(1000*60*60);
    size_t minutes = (ms - (1000*60*60)*hours)/(1000*60);
    size_t seconds = (ms - (1000*60*60)*hours - minutes*(1000*60))/(1000);
    size_t milliseconds = (ms - (1000*60*60)*hours - minutes*(1000*60) - seconds*1000);

    QString time("");
    if (hours > 0) time += QString::number(hours)+"h ";
    if (minutes > 0) time += QString::number(minutes)+"m ";
    if (seconds > 0) time += QString::number(seconds)+"s ";
    time += QString::number(milliseconds)+"ms";

    return time;
}

void writeToLogAndPrint(QString text, QString file, bool append)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = QString("["+dateTime.toString("hh:mm:ss")+"] ");

    std::ofstream myfile (file.toStdString().c_str(), std::ios::out | ((append == true) ? std::ios::app : std::ios::trunc));
    if (myfile.is_open())
    {
        myfile << dateTimeString.toStdString().c_str() << text.toStdString().c_str() << std::endl;
        std::cout << "[Log]"<< dateTimeString.toStdString().c_str() << text.toStdString().c_str() << std::endl;
    }
    else std::cout << "Unable to open log file" << std::endl;
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

QString openResource(const char * path)
{
    QFile file( path );
    file.open( QFile::ReadOnly );
    QString qsrc(file.readAll());
    file.close();

    return qsrc;
}

QByteArray openFile(const char * path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    std::string contents;

    if (in)
    {
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }
    else
    {
        qDebug(QString("Could not open file: " + QString(path)).toStdString().c_str());
    }

    QByteArray ba = QString(contents.c_str()).toUtf8();

    return ba;
}

