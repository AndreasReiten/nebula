#ifndef VOL_DATA_SET_H
#define VOL_DATA_SET_H

#ifdef _win32
#include "HDF/hdf5.h"
#endif
#ifdef __linux__
#include "hdf5.h"
#endif

#include <CL/opencl.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <fstream>
#include <cmath>
#include <cerrno>
#include <vector>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <functional>
#include <iterator>

#include <QDebug>
#include <QStringList>
#include <QObject>
#include <QWidget>
#include <QPainter>
#include <QFileInfo>
#include <QSyntaxHighlighter>
#include <QHash>
#include <QChar>
#include <QString>
#include <QTextCharFormat>
#include <QList>
#include <QVector>
#include <QVarLengthArray>
#include <QProgressBar>
#include <QFileDialog>
#include <QElapsedTimer>

#include "tools.h"
#include "miniarray.h"
#include "file_formats.h"
#include "matrix.h"
#include "octnode.h"
#include "node.h"
#include "imagerender.h"

class VolumeDataSet : public QObject
{
    Q_OBJECT;

    public:
        VolumeDataSet(cl_device * device, cl_context * context, cl_command_queue * queue);
        ~VolumeDataSet();
        bool OPEN_CL;
        int currentDisplayFrame;
        float suggested_q;
        float suggested_search_radius_low;
        float suggested_search_radius_high;

        size_t size_raw;
        size_t size_svo;
        size_t dim_x, dim_y, dim_z;
        float alpha;
        float beta;
        float tilt_y;
        float tilt_z;
        int active_angle;

        int detector_int;
        QString detector;
        QString format;
        float hint_wavelength;

        void setImageRenderWidget(ImageRenderGLWidget * widget);

        int funcSetFiles();
        int funcReadFiles();
        int funcProjectFiles();
        int funcGenerateSvo();
        int funcAllInOne();

        MiniArray<float> * getBRICKS();
        MiniArray<unsigned int> * getOCT_INDEX();
        MiniArray<unsigned int> * getOCT_BRICK();
        float * getExtent();
        size_t getBPP();
        size_t getLEVELS();
        size_t getN_BRICKS();
        size_t getBRICK_DIM_TOT();

        //~ MiniArray<float> final_intensity;
        MiniArray<float> POINTS;
        QList<PilatusFile> RAWFILE;
        QList<PilatusFile> background;
        Matrix<float> testBackground;

    protected:

    public slots:
        void setFormatGenericProgress(QString str);
        void setMessageString(QString str);
        void setValueGenericProgress(int value);
        void setLowThresholdReduce(double value);
        void setLowThresholdProject(double value);
        void setHighThresholdReduce(double value);
        void setHighThresholdProject(double value);
        void setFormat(int value);
        void saveSVO();
        void setSvoLevels(int value);
        void incrementDisplayFrame1();
        void decrementDisplayFrame1();
        void incrementDisplayFrame5();
        void decrementDisplayFrame5();
        void setDisplayFrame(int value);
        void setPaths(QStringList strlist);

    signals:
        void changedGenericProgress(int value);
        void changedMessageString(QString str);
        void changedFormatGenericProgress(QString str);
        void changedRawImage(PilatusFile * file);
        void changedCorrectedImage(PilatusFile * file);
        void repaintRequest();
        void displayFrameChanged(int value);

    private:
        ImageRenderGLWidget * imageRenderWidget;

        cl_device * device;
        cl_context * context;
        cl_command_queue * queue;
        cl_program program;
        cl_kernel K_FRAME_FILTER;
        cl_int err;
        int initCL();

        int GENERATE_SVO_OCTTREE();

        QElapsedTimer timer;

        MiniArray<float> BRICKS;
        MiniArray<int> BRICK_INDICES;
        MiniArray<int> BRICK_INDEX_OFFSETS;
        MiniArray<int> BRICK_INDEX_LENGTHS;
        MiniArray<unsigned int> OCT_INDEX;
        MiniArray<unsigned int> OCT_BRICK;

        size_t N_VOX_BRICK;
        size_t BRICK_POOL_POWER;
        size_t MAX_BRICKS;
        size_t n_rs_box;
        size_t n_rs_slice;
        size_t n_rs_row;
        size_t n_final;
        size_t BRICK_DIM;
        size_t BRICK_BORDER;
        size_t LEVELS;
        size_t N_BRICKS;
        size_t BRICK_DIM_TOT;
        size_t final_resolution;

        int rs_resolution;
        int sub_box_resolution;
        int n_sub_box;
        int genprog_value;

        float srchrad;
        float volume_extent[8];
        float threshold_reduce[2];
        float threshold_project[2];
        float threshold_voxelize[2];

        QStringList paths;
};
#endif
