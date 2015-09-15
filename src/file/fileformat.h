#ifndef FILE_FORMATS_H
#define FILE_FORMATS_H

#include <QSizeF>
#include <QVector>
#include <QString>

#include "../math/matrix.h"

class DetectorFile
{
public:
    DetectorFile();
    DetectorFile(const DetectorFile &other);
    DetectorFile(DetectorFile && other);
    DetectorFile& operator=(DetectorFile other);
    DetectorFile(QString p_file_path);
    ~DetectorFile();

    QString filePath() const;
    QString fileName() const;
    QString dir() const;
    QString info();
    QString detector() const;

    void setPath(QString p_file_path);

    int readBody();
    int readHeader();
    bool isValid();
    bool isDataRead();
    bool isHeaderRead();

    void setAlpha(float value);
    void setBeta(float value);

    const QVector<float> &data() const;
    int width() const;
    int height() const;
    QSizeF size() const;
    size_t bytes() const;
    float alpha() const;
    float beta() const;
    float getSearchRadiusLowSuggestion() const;
    float getSearchRadiusHighSuggestion() const;
    float getQSuggestion();
    float maxCount() const;
    float startAngle() const;
    float angleIncrement() const;
    float phi() const;
    float omega() const;
    float kappa() const;
    void clearData();
    float flux() const;
    float expTime() const;
    float wavelength() const;
    float detectorDist() const;
    float beamX() const;
    float beamY() const;
    float pixSizeX() const;
    float pixSizeY() const;

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
    int p_n_excluded_pixels;
    QString p_gain_setting;
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
    float p_chi;
    float p_omega;
    int p_n_oscillations;
    float p_start_position;
    float p_position_increment;
    float p_shutter_time;
    float p_background_flux;
    float p_backgroundExpTime;
    float p_beta;
    float p_max_counts;
    bool p_is_header_read;
    bool p_is_data_read;
    float p_srchrad_sugg_low, p_srchrad_sugg_high;
    QString p_file_path;
    QString p_file_name;
    QString p_dir;
    size_t p_fast_dimension, p_slow_dimension;
    QVector<float> p_data_buf;

    // Misc
    void swap(DetectorFile & other);
    void setSearchRadiusHint();
    QString regExp(QString & str, QString & source, size_t offset, size_t i);
};

Q_DECLARE_METATYPE(DetectorFile);

QDebug operator<<(QDebug dbg, const DetectorFile &file);

#endif
