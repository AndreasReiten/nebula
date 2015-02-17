#ifndef FILE_FORMATS_H
#define FILE_FORMATS_H

/*
 * The DetectorFile class.
 * */

/* GL and CL*/
// It might be cleaner to exclude opencl from this lib and rather use opencl as a method in a parent
//#include <QMetaType>

/* QT */
//class QString;

/* User libs */
#include "../../opencl/qxopencllib.h"
#include "../../math/qxmathlib.h"




/* This is the file to edit/append if you would like to add your own file formats */

class DetectorFile
{
    /* Refer to PILATUS CBF header specs for details */
    public:
        ~DetectorFile();
        DetectorFile();
        DetectorFile(QString path);

        QString getPath() const;
        QString info();

        int setPath(QString path);
        int readData();
        bool isNaive();
        bool isValid();
        bool isDataRead();
        bool isHeaderRead();

        void setNaive();
        void setAlpha(float value);
        void setBeta(float value);
        
        float intensity(int x, int y);

        Matrix<float> & data();
        int fastDimension() const;
        int slowDimension() const;
        size_t bytes() const;
        float alpha();
        float beta();
        float getSearchRadiusLowSuggestion();
        float getSearchRadiusHighSuggestion();
        float getQSuggestion();
        float maxCount();
        float startAngle();
        float angleIncrement();
        float phi();
        float omega();
        float kappa();
        void clearData();
        float flux();
        float expTime();
        float wavelength();
        float detectorDist();
        float beamX();
        float beamY();
        float pixSizeX();
        float pixSizeY();
        void print();
        QString getHeaderText();
        Matrix<float> &getData();
        
    private:
        /* Non-optional keywords */
        QString p_detector;
        float p_pixel_size_x, p_pixel_size_y;
        float p_silicon_sensor_thickness;
        float p_exposure_time;
        float p_exposure_period;
        float p_tau;
        int p_count_cutoff;
        int p_threshold_setting;
        QString p_gain_setting;
        int p_n_excluded_pixels;
        QString p_excluded_pixels;
        QString p_flat_field;
        QString p_time_file;
        QString p_image_path;

        /* Optional keywords */
        float p_wavelength;
        int p_energy_range_low, p_energy_range_high;
        float p_detector_distance;
        float p_detector_voffset;
        float p_beam_x, p_beam_y;
        float p_flux;
        float p_filter_transmission;
        float p_start_angle;
        float p_angle_increment;
        float p_detector_2theta;
        float p_polarization;
        float p_alpha;
        float p_kappa;
        float p_phi;
        float p_phi_increment;
        float p_chi;
        float p_chi_increment;
        float p_omega;
        float p_omega_increment;
        QString p_oscillation_axis;
        int p_n_oscillations;
        float p_start_position;
        float p_position_increment;
        float p_shutter_time;
        float p_background_flux;
        float p_backgroundExpTime;
        float p_beta;
        
        float p_max_counts;
        
        // Misc
        int active_angle;
        Matrix<float> data_buf;
        

        QString path;
        size_t fast_dimension, slow_dimension;
        
        bool p_isNaive;
        bool isFileValid;
        bool isFileHeaderRead;
        bool isFileDataRead;
//        int STATUS_OK;

        float srchrad_sugg_low, srchrad_sugg_high;

        void setSearchRadiusHint();
        int readHeader();
        QString regExp(QString * regular_expression, QString * source, size_t offset, size_t i);

};

Q_DECLARE_METATYPE(DetectorFile);

QDebug operator<<(QDebug dbg, const DetectorFile &file);

#endif
