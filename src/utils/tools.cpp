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

const char * gl_framebuffer_error_cstring(GLint error)
{
    switch (error) {
        case GL_FRAMEBUFFER_COMPLETE:                       return "Success!";
        case GL_FRAMEBUFFER_UNDEFINED:                      return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:         return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:         return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_UNSUPPORTED:                    return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:         return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:       return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
        default:                                            return "Unknown";
    }
}

void glGetErrorMessage(const char * context)
{
    GLint error = glGetError();
    while (error != GL_NO_ERROR)
    {
        switch (error)
        {
            case GL_INVALID_FRAMEBUFFER_OPERATION:      std::cout << "GL Error in " << context <<  ": GL_INVALID_FRAMEBUFFER_OPERATION";
            case GL_OUT_OF_MEMORY:                      std::cout << "GL Error in " << context <<  ": GL_OUT_OF_MEMORY";
            case GL_STACK_UNDERFLOW:                    std::cout << "GL Error in " << context <<  ": GL_STACK_UNDERFLOW";
            case GL_STACK_OVERFLOW:                     std::cout << "GL Error in " << context <<  ": GL_STACK_OVERFLOW";
            case GL_INVALID_ENUM:                       std::cout << "GL Error in " << context <<  ": GL_INVALID_ENUM";
            case GL_INVALID_VALUE:                      std::cout << "GL Error in " << context <<  ": GL_INVALID_VALUE";
            case GL_INVALID_OPERATION:                  std::cout << "GL Error in " << context <<  ": GL_INVALID_OPERATION";
            case GL_NO_ERROR:                           std::cout << "GL Error in " << context <<  ": GL_NO_ERROR";
            default:                                    std::cout << "GL Error in " << context <<  ": Unknown";
        }
        error = glGetError();
    }
}

void screenshot(int w, int h, const char* path)
{
    // HEADER
    BMPHeader bmp_header;

    bmp_header.type = 19778;
    bmp_header.file_size = w*h*4+54;
    bmp_header.reserved1 = 0;
    bmp_header.reserved2 = 0;
    bmp_header.raw_offset = 54;

    bmp_header.self_size = 40;
    bmp_header.pixel_width = w;
    bmp_header.pixel_height = h;
    bmp_header.color_planes = 1;
    bmp_header.pixel_bits = 32;
    bmp_header.comperssion = 0;
    bmp_header.image_size = w*h*4;
    bmp_header.resolution_w = 2835;
    bmp_header.resolution_h = 2835;
    bmp_header.colors_used = 0;
    bmp_header.colors_important = 0;

    // DATA
    unsigned char* buf = new unsigned char[w*h*4];
    glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, buf);

    // FILE:  OPEN, WRITE, CLOSE
    std::ofstream file (path, std::ios::out | std::ios::trunc | std::ios::binary);
    file.write(reinterpret_cast<char *>(&bmp_header), 54);
    file.write(reinterpret_cast<char *>(buf), w*h*4);
    file.close();

    // CLEAN UP
    delete[] buf;
}

void setVbo(GLuint * vbo, float * buf, size_t length)
{
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*length, buf, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

QByteArray open_resource(const char * path)
{
//    std::cout << "OPENING RESOURCE: " << path << std::endl;

    QFile file( path );
    file.open( QFile::ReadOnly );
    QString qsrc(file.readAll());
    file.close();

//    std::cout << qsrc.toStdString().c_str() << std::endl;
    QByteArray ba = qsrc.toUtf8();

    return ba;
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

    QByteArray ba = QString(contents.c_str()).toUtf8();

    return ba;
}

GLuint create_shader(const char* resource, GLenum type)
{

    QByteArray qsrc = open_resource(resource);
    const char * source = qsrc.data();

    if (source == NULL)
    {
        std::cout <<  "Error opening: " << resource << std::endl;
        return 0;
    }
    GLuint shader = glCreateShader(type);
    const GLchar* sources[] = {"#version 330\n", source };
    glShaderSource(shader, 2, sources, NULL);


    glCompileShader(shader);
    GLint compile_ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok == GL_FALSE)
    {
        std::cout << "Error building GL shader: " << resource << std::endl;

        GLint log_length = 0;

        if (glIsShader(shader))
        {
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            char* log = new char[log_length];

            glGetShaderInfoLog(shader, log_length, NULL, log);
            std::cout << log << std::endl;

            delete[] log;
            glDeleteShader(shader);
        }
        else
        {
            std::cout << "Could not create GL shader: Supplied argument is not a shader!" << std::endl;
            return 0;
        }
        return 0;
    }
    return shader;
}

void init_tsf(int color_style, int alpha_style, TsfMatrix<double> * transfer_function)
{
    /* Some hand crafted transfer functions. Only the RGB part is used. The alpha (A) is computed later */

    double buf_hot[32] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.7f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.5f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.5f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        };

    double buf_galaxy[32] = {
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.5f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.5f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        };

    double buf_hsv[32] = {
        1.f, 0.f, 0.f, 0.f,
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 1.f, 1.f,
        0.f, 0.f, 1.f, 1.f,
        0.f, 1.f, 1.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        1.f, 1.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,
        };

    double buf_binary[32] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.143f, 0.143f, 0.143f, 1.0f,
        0.286f, 0.286f, 0.286f, 1.0f,
        0.429f, 0.429f, 0.429f, 1.0f,
        0.571f, 0.571f, 0.571f, 1.0f,
        0.714f, 0.714f, 0.714f, 1.0f,
        0.857f, 0.857f, 0.857f, 1.0f,
        1.f, 1.0f, 1.0f, 1.0f,
        };

    double buf_yranib[32] = {
        1.f, 1.0f, 1.0f, 0.0f,
        0.857f, 0.857f, 0.857f, 1.0f,
        0.714f, 0.714f, 0.714f, 1.0f,
        0.571f, 0.571f, 0.571f, 1.0f,
        0.429f, 0.429f, 0.429f, 1.0f,
        0.286f, 0.286f, 0.286f, 1.0f,
        0.143f, 0.143f, 0.143f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        };

    double buf_winter[32] = {
        0.0f, 1.f, 0.4f, 0.f,
        0.0f, 1.f, 0.6f, 1.f,
        0.0f, 0.8f, 0.8f, 1.f,
        0.0f, 0.6f, 1.f, 1.f,
        0.0f, 0.4f, 1.f, 1.f,
        0.0f, 0.2f, 1.f, 1.f,
        0.0f, 0.1f, 1.f, 1.f,
        0.0f, 0.0f, 1.f, 1.f,
        };

    double buf_ice[32] = {
        1.f, 1.f, 1.f, 0.f,
        1.f, 1.0f, 1.f, 1.f,
        0.f, 0.9f, 1.f, 1.f,
        0.f, 0.75f, 1.f, 1.f,
        0.f, 0.65f, 1.f, 1.f,
        0.f, 0.525f, 1.f, 1.f,
        0.f, 0.4f, 1.f, 1.f,
        0.f, 0.3f, 1.f, 1.f,
        };

    double buf_rainbow[32] = {
        1.f, 0.f, 0.f, 0.f,
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.5f, 0.f, 1.f,
        1.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 0.f, 1.f, 1.f,
        0.3f, 0.f, 0.6f, 1.f,
        0.7f, 0.f, 1.0f, 1.f,
        };

    double buf_white_contrast[32] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.7f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.2f, 1.0f,
        1.0f, 0.0f, 0.7f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.0f, 1.0f, 1.0f,
        };

    double buf_white[32] = {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        };

    double buf_black[32] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        };

    Matrix<double> tmp;
    switch (color_style)
    {
        case 0:
            tmp.setDeep(8, 4, buf_hot);
            break;
        case 1:
            tmp.setDeep(8, 4, buf_winter);
            break;
        case 2:
            tmp.setDeep(8, 4, buf_ice);
            break;
        case 3:
            tmp.setDeep(8, 4, buf_rainbow);
            break;
        case 4:
            tmp.setDeep(8, 4, buf_hsv);
            break;
        case 5:
            tmp.setDeep(8, 4, buf_binary);
            break;
        case 6:
            tmp.setDeep(8, 4, buf_yranib);
            break;
        case 7:
            tmp.setDeep(8, 4, buf_galaxy);
            break;
        case 8:
            tmp.setDeep(8, 4, buf_white);
            break;
        case 9:
            tmp.setDeep(8, 4, buf_black);
            break;
        case 42:
            tmp.setDeep(8, 4, buf_white_contrast);
            break;
        default:
            tmp.setDeep(8, 4, buf_hot);
            break;
    }

    // Compute the alpha
    switch (alpha_style)
    {
        case 0:
            // Uniform alpha except for the first vertex
            tmp[3] = 0.0;
            for (int i = 4; i < 32; i+=4)
            {
                tmp[i+3] = 1.0;
            }
            break;
        case 1:
            // Linearly increasing alpha
            for (int i = 0; i < 32; i+=4)
            {
                tmp[i+3] = ((float)i/4)/7.0;
            }
            break;
        case 2:
            // Exponentially increasing data
            tmp[3] = 0.0;
            for (int i = 4; i < 32; i+=4)
            {
                tmp[i+3] = std::exp(-(1.0 - (float)i/4/7.0)*3.0);
            }
            break;
        case 3:
            // Opaque
            for (int i = 0; i < 32; i+=4)
            {
                tmp[i+3] = 1.0;
            }
            break;
        default:
            for (int i = 0; i < 32; i+=4)
            {
                tmp[i+3] = 1.0;
            }
            break;
    }

    transfer_function->setDeep(4, 8, tmp.getColMajor().data());
    transfer_function->setSpline(256);
    transfer_function->setPreIntegrated();

    //~ tmp.getColMajor().print(2, "The matrix sent to be splined:");
    //~ transfer_function->getSpline().print(2,"Splined");
    //~ transfer_function->getPreIntegrated().print(2,"Integrated");
}
