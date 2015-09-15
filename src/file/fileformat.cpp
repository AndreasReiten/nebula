#include "fileformat.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QByteArray>
#include <QElapsedTimer>

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <fstream>
#include <cmath>
#include <limits>

QDebug operator<<(QDebug dbg, const DetectorFile &file)
{
    dbg.nospace() << "DetectorFile()";
    return dbg.maybeSpace();
}

DetectorFile::~DetectorFile()
{

}

void DetectorFile::swap(DetectorFile & other)
{
    std::swap(this->p_alpha, other.p_alpha);
    std::swap(this->p_beta, other.p_beta);

    std::swap(this->p_detector, other.p_detector);
    std::swap(this->p_pixel_size_x, other.p_pixel_size_x);
    std::swap(this->p_pixel_size_y, other.p_pixel_size_y);
    std::swap(this->p_silicon_sensor_thickness, other.p_silicon_sensor_thickness);
    std::swap(this->p_exposure_time, other.p_exposure_time);
    std::swap(this->p_exposure_period, other.p_exposure_period);
    std::swap(this->p_tau, other.p_tau);
    std::swap(this->p_count_cutoff, other.p_count_cutoff);
    std::swap(this->p_threshold_setting, other.p_threshold_setting);
    std::swap(this->p_n_excluded_pixels, other.p_n_excluded_pixels);
    std::swap(this->p_gain_setting, other.p_gain_setting);
    std::swap(this->p_excluded_pixels, other.p_excluded_pixels);
    std::swap(this->p_flat_field, other.p_flat_field);
    std::swap(this->p_time_file, other.p_time_file);
    std::swap(this->p_image_path, other.p_image_path);

    // Optional keywords
    std::swap(this->p_wavelength, other.p_wavelength);
    std::swap(this->p_energy_range_low, other.p_energy_range_low);
    std::swap(this->p_energy_range_high, other.p_energy_range_high);
    std::swap(this->p_detector_distance, other.p_detector_distance);
    std::swap(this->p_detector_voffset, other.p_detector_voffset);
    std::swap(this->p_beam_x, other.p_beam_x);
    std::swap(this->p_beam_y, other.p_beam_y);
    std::swap(this->p_flux, other.p_flux);
    std::swap(this->p_filter_transmission, other.p_filter_transmission);
    std::swap(this->p_start_angle, other.p_start_angle);
    std::swap(this->p_angle_increment, other.p_angle_increment);
    std::swap(this->p_detector_2theta, other.p_detector_2theta);
    std::swap(this->p_polarization, other.p_polarization);
    std::swap(this->p_alpha, other.p_alpha);
    std::swap(this->p_kappa, other.p_kappa);
    std::swap(this->p_phi, other.p_phi);
    std::swap(this->p_chi, other.p_chi);
    std::swap(this->p_omega, other.p_omega);
    std::swap(this->p_n_oscillations, other.p_n_oscillations);
    std::swap(this->p_start_position, other.p_start_position);
    std::swap(this->p_position_increment, other.p_position_increment);
    std::swap(this->p_shutter_time, other.p_shutter_time);
    std::swap(this->p_background_flux, other.p_background_flux);
    std::swap(this->p_backgroundExpTime, other.p_backgroundExpTime);
    std::swap(this->p_max_counts, other.p_max_counts);
    std::swap(this->p_is_header_read, other.p_is_header_read);
    std::swap(this->p_is_data_read, other.p_is_data_read);
    std::swap(this->p_srchrad_sugg_low, other.p_srchrad_sugg_low);
    std::swap(this->p_srchrad_sugg_high, other.p_srchrad_sugg_high);
    std::swap(this->p_file_path, other.p_file_path);
    std::swap(this->p_file_name, other.p_file_name);
    std::swap(this->p_dir, other.p_dir);
    std::swap(this->p_fast_dimension, other.p_fast_dimension);
    std::swap(this->p_slow_dimension, other.p_slow_dimension);
    std::swap(this->p_data_buf, other.p_data_buf);
}

DetectorFile::DetectorFile() :
    p_fast_dimension(0),
    p_slow_dimension(0),
    p_alpha(0.8735582),
    p_beta(0.000891863),
    p_is_data_read(false),
    p_is_header_read(false),
    p_max_counts(0)
{

}

DetectorFile::DetectorFile(const DetectorFile &other) :
    p_is_header_read(other.p_is_header_read),
    p_is_data_read(other.p_is_data_read),
    p_alpha(other.p_alpha),
    p_beta(other.p_beta),
    p_detector(other.p_detector),
    p_pixel_size_x(other.p_pixel_size_x),
    p_pixel_size_y(other.p_pixel_size_y),
    p_exposure_time(other.p_exposure_time),
    p_wavelength(other.p_wavelength),
    p_detector_distance(other.p_detector_distance),
    p_beam_x(other.p_beam_x),
    p_beam_y(other.p_beam_y),
    p_flux(other.p_flux),
    p_start_angle(other.p_start_angle),
    p_angle_increment(other.p_angle_increment),
    p_kappa(other.p_kappa),
    p_phi(other.p_phi),
    p_omega(other.p_omega),
    p_srchrad_sugg_low(other.p_srchrad_sugg_low),
    p_srchrad_sugg_high(other.p_srchrad_sugg_high),
    p_file_path(other.p_file_path),
    p_file_name(other.p_file_name),
    p_dir(other.p_dir),
    p_fast_dimension(other.p_fast_dimension),
    p_slow_dimension(other.p_slow_dimension),
    p_data_buf(other.p_data_buf)
{
}

DetectorFile::DetectorFile(DetectorFile &&other) :
    p_is_header_read(std::move(other.p_is_header_read)),
    p_is_data_read(std::move(other.p_is_data_read)),
    p_alpha(std::move(other.p_alpha)),
    p_beta(std::move(other.p_beta)),
    p_detector(std::move(other.p_detector)),
    p_pixel_size_x(std::move(other.p_pixel_size_x)),
    p_pixel_size_y(std::move(other.p_pixel_size_y)),
    p_exposure_time(std::move(other.p_exposure_time)),
    p_wavelength(std::move(other.p_wavelength)),
    p_detector_distance(std::move(other.p_detector_distance)),
    p_beam_x(std::move(other.p_beam_x)),
    p_beam_y(std::move(other.p_beam_y)),
    p_flux(std::move(other.p_flux)),
    p_start_angle(std::move(other.p_start_angle)),
    p_angle_increment(std::move(other.p_angle_increment)),
    p_kappa(std::move(other.p_kappa)),
    p_phi(std::move(other.p_phi)),
    p_omega(std::move(other.p_omega)),
    p_srchrad_sugg_low(std::move(other.p_srchrad_sugg_low)),
    p_srchrad_sugg_high(std::move(other.p_srchrad_sugg_high)),
    p_file_path(std::move(other.p_file_path)),
    p_file_name(std::move(other.p_file_name)),
    p_dir(std::move(other.p_dir)),
    p_fast_dimension(std::move(other.p_fast_dimension)),
    p_slow_dimension(std::move(other.p_slow_dimension)),
    p_data_buf(std::move(other.p_data_buf))
{
}

DetectorFile& DetectorFile::operator=(DetectorFile other)
{
    this->swap(other);
    return *this;
}



DetectorFile::DetectorFile(QString path):
    p_fast_dimension(0),
    p_slow_dimension(0),
    p_alpha(0.8735582),
    p_beta(0.000891863),
    p_is_data_read(false),
    p_is_header_read(false),
    p_max_counts(0)
{
    this->setPath(path);
}

QString DetectorFile::detector() const
{
    return p_detector;
}

float DetectorFile::wavelength()  const
{
    return p_wavelength;
}

bool DetectorFile::isValid()
{
    QFileInfo info(p_file_path);
    return  (info.exists() && info.isReadable() && info.isFile());
}

bool DetectorFile::isDataRead()
{
    return p_is_data_read;
}

bool DetectorFile::isHeaderRead()
{
    return p_is_header_read;
}

void DetectorFile::setPath(QString path)
{
    if (p_file_path != path)
    {
        p_is_data_read = false;
        p_is_header_read = false;

        QFileInfo info(path);

        p_dir = info.path();
        p_file_path = info.filePath();
        p_file_name = info.fileName();
    }
}

float DetectorFile::getSearchRadiusLowSuggestion() const
{
    return p_srchrad_sugg_low;
}
float DetectorFile::getSearchRadiusHighSuggestion() const
{
    return p_srchrad_sugg_high;
}
float DetectorFile::getQSuggestion()
{
    return 1.0 / p_wavelength; // Might double
}

int DetectorFile::width() const
{
    return p_fast_dimension;
}
int DetectorFile::height() const
{
    return p_slow_dimension;
}

QSizeF DetectorFile::size() const
{
    return QSizeF(p_fast_dimension,p_slow_dimension);;
}

size_t DetectorFile::bytes() const
{
    return p_data_buf.size() * sizeof(float);
}

const QVector<float> &DetectorFile::data() const
{
    return p_data_buf;
}

float DetectorFile::maxCount() const
{
    return p_max_counts;
}

void DetectorFile::setAlpha(float value)
{
    p_alpha = value;
}

void DetectorFile::setBeta(float value)
{
    p_beta = value;
}

float DetectorFile::alpha() const
{
    return p_alpha;
}

float DetectorFile::beta() const
{
    return p_beta;
}

float DetectorFile::angleIncrement() const
{
    return p_angle_increment;
}

float DetectorFile::startAngle() const
{
    return p_start_angle;
}

float DetectorFile::phi() const
{
    return p_phi;
}

float DetectorFile::omega() const
{
    return p_omega;
}

float DetectorFile::kappa() const
{
    return p_kappa;
}

float DetectorFile::flux()  const
{
    return p_flux;
}
float DetectorFile::expTime() const
{
    return p_exposure_time;
}

int DetectorFile::readHeader()
{
    if (p_is_header_read)
    {
        return p_is_header_read;
    }
    const float pi = 4.0 * atan(1.0);

    // Based on the PILATUS 1.2 header convention. Regular expressions are used to fetch header values
    QString reqExpDetector("(?:Detector:\\s+)(\\S+\\s+\\S+),");
    QString reqExpTime("(?:Exposure_time\\s+)(\\d+(?:\\.\\d+)?)");
    QString reqPixSizex("(?:Pixel_size\\s+)(\\d+(?:\\.\\d+)?e[+-]\\d+)");
    QString reqPixSizey("(?:Pixel_size\\s+)(?:\\d+(?:\\.\\d+)?e[+-]\\d+)\\sm\\sx\\s(\\d+(?:\\.\\d+)?e[+-]\\d+)");

    QString optExpWl( "Wavelength\\s+(\\d+(?:\\.\\d+)?)" );
    QString optExpStAng( "Start_angle\\s+(\\d+(?:\\.\\d+)?)" );
    QString optExpAngInc( "Angle_increment\\s+(\\d+(?:\\.\\d+)?)" );
    QString optExpFlux( "Flux\\s+(\\d+(?:\\.\\d+)?)" );
    QString optExpDd( "Detector_distance\\s+(-?\\d+(?:\\.\\d+)?)" );
    QString optExpBeamx( "Beam_xy\\s+\\((-?\\d+(?:\\.\\d+)?)" );
    QString optExpBeamy( "Beam_xy\\s+\\((?:-?\\d+(?:\\.\\d+)?)(?:\\s*\\,\\s*)(-?\\d+(?:\\.\\d+)?)" );
    QString optExpPhi( "Phi\\s+(-?\\d+(?:\\.\\d+)?)" );
    QString optExpKappa( "Kappa\\s+(-?\\d+(?:\\.\\d+)?)" );
    QString optExpOmega( "Omega\\s+(-?\\d+(?:\\.\\d+)?)" );

    // Open file
    QFileInfo file_info(p_file_path);

    if (!file_info.exists())
    {
        qDebug() << "File does not exist: " << p_file_path.toStdString().c_str();
        return 0;
    }
    else if (file_info.size() <= 0)
    {
        qDebug() << "File does not exist: " << p_file_path;
        return 0;
    }

    // Read file
    QFile file(p_file_path.toStdString().c_str());

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Error reading file: " << p_file_path;
        return 0;
    }

    QString header = file.read(4096);
    file.close();

    // Fetch keywords
    p_detector = regExp(reqExpDetector, header, 0, 1);
    p_pixel_size_x = regExp(reqPixSizex, header, 0, 1).toFloat();
    p_pixel_size_y = regExp(reqPixSizey, header, 0, 1).toFloat();
    p_exposure_time = regExp(reqExpTime, header, 0, 1).toFloat();
    p_wavelength = regExp(optExpWl, header, 0, 1).toFloat();
    p_detector_distance = regExp(optExpDd, header, 0, 1).toFloat();
    p_beam_x = regExp(optExpBeamx, header, 0, 1).toFloat();
    p_beam_y = regExp(optExpBeamy, header, 0, 1).toFloat();
    p_flux = regExp(optExpFlux, header, 0, 1).toFloat();
    p_start_angle = regExp(optExpStAng, header, 0, 1).toFloat() * pi / 180.0;
    p_angle_increment = regExp(optExpAngInc, header, 0, 1).toFloat() * pi / 180.0;

    // Check if defined. If two are defined, the third is active. In any other case, omega is active. Unless the active angle is directly specified.
    QString omega_str = regExp(optExpOmega, header, 0, 1);
    QString kappa_str = regExp(optExpKappa, header, 0, 1);
    QString phi_str = regExp(optExpPhi, header, 0, 1);

    p_omega = omega_str.toFloat() * pi / 180.0;
    p_kappa = kappa_str.toFloat() * pi / 180.0;
    p_phi = phi_str.toFloat() * pi / 180.0;

    int num_defined_angles = ((omega_str != "")? 1 : 0) + ((kappa_str != "")? 1 : 0) + ((phi_str != "")? 1 : 0);

    if (num_defined_angles == 2)
    {
        if (omega_str == "") p_omega = (p_start_angle+0.5*p_angle_increment);
        else if (kappa_str == "") p_kappa = (p_start_angle+0.5*p_angle_increment);
        else if (phi_str == "") p_phi = (p_start_angle+0.5*p_angle_increment);
    }
    else
    {
        p_omega = (p_start_angle+0.5*p_angle_increment);
    }

    //~ silicon_sensor_thickness = regExp(, header, 0, 1).toFloat();
    //~ exposure_period = regExp(, header, 0, 1).toFloat();
    //~ tau = regExp(, header, 0, 1).toFloat();
    //~ count_cutoff = regExp(, header, 0, 1).toInt();
    //~ threshold_setting = regExp(, header, 0, 1).toInt();
    //~ gain_setting = regExp(, header, 0, 1);
    //~ n_excluded_pixels = regExp(, header, 0, 1).toInt();
    //~ excluded_pixels = regExp(, header, 0, 1);
    //~ flat_field = regExp(, header, 0, 1);
    //~ time_file = regExp(, header, 0, 1);
    //~ image_path = regExp(, header, 0, 1);
    //~ energy_range_low = regExp(, header, 0, 1).toInt();
    //~ energy_range_high = regExp(, header, 0, 1).toInt();
    //~ detector_voffset = regExp(, header, 0, 1).toFloat();
    //~ filter_transmission = regExp(, header, 0, 1).toFloat();
    //~ detector_2theta = regExp(, header, 0, 1).toFloat()*pi/180.0;
    //~ polarization = regExp(, header, 0, 1).toFloat();
    //~ alpha = regExp(, header, 0, 1).toFloat()*pi/180.0;
    //~ phi_increment = regExp(, header, 0, 1).toFloat()*pi/180.0;
    //~ chi = regExp(, header, 0, 1).toFloat()*pi/180.0;
    //~ chi_increment = regExp(, header, 0, 1).toFloat()*pi/180.0;
    //~ omega_increment = regExp(, header, 0, 1).toFloat()*pi/180.0;
    //~ oscillation_axis = regExp(, header, 0, 1);
    //~ n_oscillations = regExp(, header, 0, 1).toInt();
    //~ start_position = regExp(, header, 0, 1).toFloat();
    //~ position_increment = regExp(, header, 0, 1).toFloat();
    //~ shutter_time = regExp(, header, 0, 1).toFloat();

    if (p_detector == "PILATUS 1M")
    {
        p_fast_dimension = 981;
        p_slow_dimension = 1043;
    }
    else if (p_detector == "PILATUS 2M")
    {
        p_fast_dimension = 1475;
        p_slow_dimension = 1679;
    }
    else if (p_detector == "PILATUS 6M")
    {
        p_fast_dimension = 2463;
        p_slow_dimension = 2527;
    }
    else
    {
        qDebug() << "Unknown detector: " << p_detector;
        return 0;
    }

    this->setSearchRadiusHint();

    p_is_header_read = true;
    return p_is_header_read;
}

QString DetectorFile::filePath() const
{
    return p_file_path;
}

QString DetectorFile::fileName() const
{
    return p_file_name;
}
QString DetectorFile::dir() const
{
    return p_dir;
}

QString DetectorFile::regExp(QString &str, QString &source, size_t offset, size_t i)
{
    QRegularExpression expr(str);

    QRegularExpressionMatch match = expr.match(source, offset);

    if (match.hasMatch())
    {
        return match.captured(i);
    }
    else
    {
        return QString("");
    }
}

void DetectorFile::clearData()
{
    p_data_buf.clear();
}

QString DetectorFile::info()
{
    QString str;
    str += "#__________ PILATUS FILE __________\n";
    str +="# Path: " + p_file_path + "\n";
    str +="# Data elements: " + QString::number(p_data_buf.size()) + "\n";
    str +="# Dimensions: " + QString::number(p_fast_dimension) + " x " + p_slow_dimension + "\n";
    str +="# Max counts: " + QString::number(p_max_counts) + "\n";
    str +="# Detector: " + p_detector + "\n";
    str +="# Pixel size: " + QString::number(p_pixel_size_x) + " x " +  QString::number(p_pixel_size_y) + "\n";
    str +="# Exposure time: " + QString::number(p_exposure_time) + "\n";
    str +="# Exposure period: " + QString::number(p_exposure_period) + "\n";
    str +="# Count cutoff: " + QString::number(p_count_cutoff) + "\n";
    str +="# Wavelength: " + QString::number(p_wavelength) + "\n";
    str +="# Detector distance: " + QString::number(p_detector_distance) + "\n";
    str +="# Beam position: " + QString::number(p_beam_x) + " x " + QString::number(p_beam_y) + "\n";
    str +="# Flux: " + QString::number(p_flux) + "\n";
    str +="# Start angle: " + QString::number(p_start_angle) + "\n";
    str +="# Angle increment: " + QString::number(p_angle_increment) + "\n";
    str +="# Alpha: " + QString::number(p_alpha) + "\n";
    str +="# Beta: " + QString::number(p_beta) + "\n";
    str +="# Kappa: " + QString::number(p_kappa) + "\n";
    str +="# Phi: " + QString::number(p_phi) + "\n";
    str +="# Omega: " + QString::number(p_omega) + "\n";

    return str;
}



float DetectorFile::detectorDist() const
{
    return p_detector_distance;
}

float DetectorFile::beamX() const
{
    return p_beam_x;
}
float DetectorFile::beamY() const
{
    return p_beam_y;
}
float DetectorFile::pixSizeX() const
{
    return p_pixel_size_x;
}
float DetectorFile::pixSizeY() const
{
    return p_pixel_size_y;
}


int DetectorFile::readBody()
{
    if (p_is_data_read)
    {
        return p_is_data_read;
    }

    QFile file(p_file_path);

    if (!file.open(QIODevice::ReadOnly)) qDebug() << "Failed to open file" << p_file_path;
    QByteArray blob = file.readAll();
    file.close();

    int offset = 0;
    int header_length_max = 2000;
    char * buf = blob.data();//new char[length];
    this->p_data_buf.resize(p_fast_dimension * p_slow_dimension);

    // Find beginning of binary section
    for (int i = 0; i < header_length_max; i++)
    {
        if ((int) buf[i] == -43)
        {
            offset = i + 1;
            break;
        }
    }

    // Decompress data and neglect data outside given thresholds
    int prev = 0;
    int id = offset;
    int counts;
    int int16;
    int int32;

    int i, j;

    for (i = 0; i < (int) p_slow_dimension; i++)
    {
        for (j = 0; j < (int) p_fast_dimension; j++)
        {
            // Get value
            if (buf[id] == (char) -128)
            {
                id++;
                int16 =  ((buf[id + 1] << 8 ) | (buf[id] & 0xff));

                if (int16 == -32768)
                {
                    id += 2;
                    int32 = ((buf[id + 3] << 24) | ((buf[id + 2] & 0xff) << 16) | ((buf[id + 1] & 0xff) << 8) | (buf[id] & 0xff));
                    counts = prev + int32;
                    id += 4;
                }
                else
                {
                    counts = prev + int16;
                    id += 2;
                }
            }
            else
            {
                counts = prev + (int) (buf[id]);
                id++;
            }



            prev = counts;


            if (counts < 0)
            {
                counts = 0;
            }


//            qDebug() << i * fast_dimension + j << data_buf.size();
            p_data_buf[i * p_fast_dimension + j] = (float) counts;

            if (p_max_counts < counts)
            {
                p_max_counts = counts;
            }
        }
    }

    p_is_data_read = true;
    return p_is_data_read;
}

//void DetectorFile::test()
//{
//    this->data_buf.resize(10);
//}


void DetectorFile::setSearchRadiusHint()
{
    /* A search radius can be found based on the projected size of a pixel in reciprocal space. The following calculations assume a detector that can be translated but not rotated. Could give a fair estimate even for a rotating detector */

    // TODO: This function should not be in this class, as it depends on the beam center override and is not contextually linked to this object
    p_srchrad_sugg_low = std::numeric_limits<float>::max();
    p_srchrad_sugg_high = std::numeric_limits<float>::min();

    // For several pixel position extrema:
    float y_config[] = {(float) 0, (float) p_slow_dimension / 2, (float) p_slow_dimension - 1, (float) p_slow_dimension - 1, (float) 0};
    float z_config[] = {(float) 0, (float) p_fast_dimension / 2, (float) p_fast_dimension - 1, (float) 0, (float) p_fast_dimension - 1};

    for (int i = 0; i < 5; i++)
    {
        // Calculate the projected size of the pixel onto the Ewald sphere. In effect, we find the width along the diagonal of a rectangular pixel
        float xyz_a[3] =
        {
            (float)(-p_detector_distance),
            (float)(((y_config[i] - 0.5) - p_beam_x) * p_pixel_size_x),
            (float)(((z_config[i] - 0.5) - p_beam_y) * p_pixel_size_y)
        };

        float len_xyz_a = sqrt(xyz_a[0] * xyz_a[0] + xyz_a[1] * xyz_a[1] + xyz_a[2] * xyz_a[2]);

        xyz_a[0] /= len_xyz_a;
        xyz_a[1] /= len_xyz_a;
        xyz_a[2] /= len_xyz_a;

        float xyz_b[3] =
        {
            (float)(-p_detector_distance),
            (float)(((y_config[i] + 0.5) - p_beam_x) * p_pixel_size_y),
            (float)(((z_config[i] + 0.5) - p_beam_y) * p_pixel_size_x)
        };

        float len_xyz_b = sqrt(xyz_b[0] * xyz_b[0] + xyz_b[1] * xyz_b[1] + xyz_b[2] * xyz_b[2]);

        xyz_b[0] /= len_xyz_b;
        xyz_b[1] /= len_xyz_b;
        xyz_b[2] /= len_xyz_b;

        float k = 1 / p_wavelength;

        float k_a[3] = {k * xyz_a[0], k * xyz_a[1], k * xyz_a[2]};
        float k_b[3] = {k * xyz_b[0], k * xyz_b[1], k * xyz_b[2]};

        float k_delta[3] = {k_b[0] - k_a[0], k_b[1] - k_a[1], k_b[2] - k_a[2]};
        float temp_search_radius = sqrt(k_delta[0] * k_delta[0] + k_delta[1] * k_delta[1] + k_delta[2] * k_delta[2]);

        if (temp_search_radius < p_srchrad_sugg_low)
        {
            p_srchrad_sugg_low = temp_search_radius;
        }

        if (temp_search_radius > p_srchrad_sugg_high)
        {
            p_srchrad_sugg_high = temp_search_radius;
        }
    }
}
