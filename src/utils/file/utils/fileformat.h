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

        int set(QString path);
        int readData();

        void setNaive();
        
        float intensity(int x, int y);

        Matrix<float> & data();
        int getFastDimension() const;
        int getSlowDimension() const;
        size_t getBytes() const;
        float getSearchRadiusLowSuggestion();
        float getSearchRadiusHighSuggestion();
        float getQSuggestion();
        float getMaxCount();
        void clearData();
        float getFlux();
        float getExpTime();
        float getWavelength();
        float getDetectorDist();
        float getBeamX();
        float getBeamY();
        float getPixSizeX();
        float getPixSizeY();
        void print();
        QString getHeaderText();
        Matrix<float> &getData();
        
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
        float background_flux;
        float backgroundExpTime;
        float beta;
        
        float max_counts;
        
    private:
        // Misc
        int active_angle;
        Matrix<float> data_buf;
        

        QString path;
        size_t fast_dimension, slow_dimension;

        int STATUS_OK;

        float srchrad_sugg_low, srchrad_sugg_high;

        void suggestSearchRadius();
        int readHeader();
        QString regExp(QString * regular_expression, QString * source, size_t offset, size_t i);

};

Q_DECLARE_METATYPE(DetectorFile);

QDebug operator<<(QDebug dbg, const DetectorFile &file);

#endif
