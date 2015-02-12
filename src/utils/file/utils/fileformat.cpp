#include "fileformat.h"

#include <QFileInfo>
#include <QRegExp>
#include <QString>
#include <QDebug>

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
DetectorFile::DetectorFile() :
    active_angle(2),
    fast_dimension(0),
    slow_dimension(0),
    isFileValid(false),
    isFileDataRead(false),
    isFileHeaderRead(false),
    p_isNaive(false)
{
    srchrad_sugg_low = std::numeric_limits<float>::max();
    srchrad_sugg_high = std::numeric_limits<float>::min();
    max_counts = 0;
//    STATUS_OK = 0;
}
DetectorFile::DetectorFile(QString path):
    active_angle(2),
    isFileValid(false),
    isFileDataRead(false),
    isFileHeaderRead(false),
    p_isNaive(false)
{
    srchrad_sugg_low = std::numeric_limits<float>::max();
    srchrad_sugg_high = std::numeric_limits<float>::min();
    max_counts = 0;
    this->setPath(path);
    isValid();
}

void DetectorFile::setNaive()
{
    fast_dimension = 11;
    slow_dimension = 6;
    
    
    float buf[] = {
        0.1, 0.1, 1.0, 1.0, 0.1, 0.1, 1.0, 0.1, 0.1, 0.1, 0.1,
        0.1, 1.0, 0.1, 0.1, 1.0, 0.1, 1.0, 0.1, 0.1, 0.1, 0.1,
        0.1, 1.0, 0.1, 0.1, 0.1, 0.1, 1.0, 0.1, 0.1, 0.1, 0.1,
        0.1, 1.0, 0.1, 1.0, 1.0, 0.1, 1.0, 0.1, 0.1, 0.1, 0.1,
        0.1, 1.0, 0.1, 0.1, 1.0, 0.1, 1.0, 0.1, 0.1, 0.1, 0.1,
        0.1, 0.1, 1.0, 1.0, 0.1, 0.1, 1.0, 1.0, 1.0, 1.0, 0.1,
    };
    
    data_buf.setDeep(slow_dimension, fast_dimension, buf);

//    float value = 0;

//    for (int i = 0; i < data_buf.size(); i++)
//    {
//        data_buf[i] = value;

//        value += 1;
//    }

//    STATUS_OK = 1;
    
    p_isNaive = true;
    isFileValid = true;
    isFileDataRead = true;
    isFileHeaderRead = true;
}

float DetectorFile::intensity(int x, int y)
{
    if ((x >= 0) && (x < fast_dimension) && (y >= 0) && (y < slow_dimension))
    {
        return data_buf[x + y*fast_dimension];
    }
    else return 0;
}

float DetectorFile::getWavelength()
{
    return wavelength;
}

QString DetectorFile::getHeaderText()
{
    std::stringstream ss;
    ss << "__Detector___________" << std::endl;
    ss << "Detector:    " << detector.toStdString().c_str() << std::endl;
    ss << "Pixel size:  " << pixel_size_x << " x " << pixel_size_y << " m"<<  std::endl;
    ss << "Exposure time:   "<< exposure_time << " s" << std::endl;
    ss << "Count cutoff:    "<< count_cutoff << std::endl << std::endl;
    
    
    ss << "__Beam_______________" << std::endl;
    ss << "Wavelength:  " << wavelength << " Å"<< std::endl;
    ss << "Flux:        "<< flux << std::endl << std::endl;

    
    ss << "__Geometry___________" << std::endl;
    ss << "Detector distance:   " << detector_distance << " m" << std::endl;
    ss << "Beam x y:        "<< beam_x << ", "<< beam_y << " pixels" << std::endl;
    ss << "Start angle:     "<< start_angle*180.0/pi << " deg" << std::endl;
    ss << "Angle increment: "<< angle_increment*180.0/pi << " deg"<< std::endl;
    ss << "Omega:           "<< omega*180.0/pi   << " deg" << std::endl;
    ss << "Kappa:           "<< kappa*180.0/pi   << " deg" << std::endl;
    ss << "Phi:             "<< phi*180.0/pi << " deg" << std::endl << std::endl;
    
    QString text(ss.str().c_str());

    return text;
}

bool DetectorFile::isValid()
{
    if (!isFileValid)
    {
        QFileInfo file_info(path);
        isFileValid =  (file_info.exists() && file_info.isReadable() && file_info.isFile());
    }
    return isFileValid;
}

bool DetectorFile::isNaive()
{
    return p_isNaive;
}

bool DetectorFile::isDataRead()
{
    return isFileDataRead;
}

bool DetectorFile::isHeaderRead()
{
    return isFileHeaderRead;
}

int DetectorFile::setPath(QString path)
{
//    this->context_cl = context;
    if (this->path == path) return 1;
    else this->path = path;
    
    isFileValid = false;
    isFileHeaderRead = false;
    isFileDataRead = false;
            
    if (!isValid()) return 0;
    
    
    if (!this->readHeader()) return 0;

    if (detector == "PILATUS 1M")
    {
        fast_dimension = 981;
        slow_dimension = 1043;
    }
    else if (detector == "PILATUS 2M")
    {
        fast_dimension = 1475;
        slow_dimension = 1679;
    }
    else if (detector == "PILATUS 6M")
    {
        fast_dimension = 2463;
        slow_dimension = 2527;
    }
    else
    {
        std::cout << "Unknown detector: " << detector.toStdString().c_str() << std::endl;
        return 0;
    }
    this->setSearchRadiusHint();

    return 1;
}



float DetectorFile::getSearchRadiusLowSuggestion()
{
    return srchrad_sugg_low;
}
float DetectorFile::getSearchRadiusHighSuggestion()
{
    return srchrad_sugg_high;
}
float DetectorFile::getQSuggestion()
{
    return 1.0/wavelength;
}

int DetectorFile::getFastDimension() const
{
    return fast_dimension;
}
int DetectorFile::getSlowDimension() const
{
    return slow_dimension;
}

size_t DetectorFile::getBytes() const
{
    return data_buf.bytes();
}

Matrix<float> & DetectorFile::data()
{
    return data_buf;
}

float DetectorFile::getMaxCount()
{
    return max_counts;
}


float DetectorFile::getFlux()
{
    return flux;
}
float DetectorFile::getExpTime()
{
    return exposure_time;
}

int DetectorFile::readHeader()
{
    if (isFileHeaderRead) return isFileHeaderRead;
    
    const float pi = 4.0*atan(1.0);

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
    QFileInfo file_info(path);
    if (!file_info.exists())
    {
        qDebug() << "File does not exist: " << path.toStdString().c_str();
        return 0;
    }
    else if (file_info.size() <= 0)
    {
        qDebug() << "File does not exist: " << path.toStdString().c_str();
        return 0;
    }
    // Read file
    QFile file(path.toStdString().c_str());
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Error reading file: " << path.toStdString().c_str();
        return 0;
    }
    QString header = file.read(4096);
    file.close();


    // Fetch keywords!
    /* Non-optional keywords */
    detector = regExp(&reqExpDetector, &header, 0, 1);
    pixel_size_x = regExp(&reqPixSizex, &header, 0, 1).toFloat();
    pixel_size_y = regExp(&reqPixSizey, &header, 0, 1).toFloat();
    //~ silicon_sensor_thickness = regExp(&, &header, 0, 1).toFloat();
    exposure_time = regExp(&reqExpTime, &header, 0, 1).toFloat();
    //~ exposure_period = regExp(&, &header, 0, 1).toFloat();
    //~ tau = regExp(&, &header, 0, 1).toFloat();
    //~ count_cutoff = regExp(&, &header, 0, 1).toInt();
    //~ threshold_setting = regExp(&, &header, 0, 1).toInt();
    //~ gain_setting = regExp(&, &header, 0, 1);
    //~ n_excluded_pixels = regExp(&, &header, 0, 1).toInt();
    //~ excluded_pixels = regExp(&, &header, 0, 1);
    //~ flat_field = regExp(&, &header, 0, 1);
    //~ time_file = regExp(&, &header, 0, 1);
    //~ image_path = regExp(&, &header, 0, 1);

    /* Optional keywords */
    wavelength = regExp(&optExpWl, &header, 0, 1).toFloat();
    //~ energy_range_low = regExp(&, &header, 0, 1).toInt();
    //~ energy_range_high = regExp(&, &header, 0, 1).toInt();
    detector_distance = regExp(&optExpDd, &header, 0, 1).toFloat();
    //~ detector_voffset = regExp(&, &header, 0, 1).toFloat();
    beam_x = regExp(&optExpBeamx, &header, 0, 1).toFloat();
    beam_y = regExp(&optExpBeamy, &header, 0, 1).toFloat();
    flux = regExp(&optExpFlux, &header, 0, 1).toFloat();
    //~ filter_transmission = regExp(&, &header, 0, 1).toFloat();
    start_angle = regExp(&optExpStAng, &header, 0, 1).toFloat()*pi/180.0;
    angle_increment = regExp(&optExpAngInc, &header, 0, 1).toFloat()*pi/180.0;
    //~ detector_2theta = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    //~ polarization = regExp(&, &header, 0, 1).toFloat();
//    ~ alpha = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    kappa = regExp(&optExpKappa, &header, 0, 1).toFloat()*pi/180.0;
    phi = regExp(&optExpPhi, &header, 0, 1).toFloat()*pi/180.0;
    //~ phi_increment = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    //~ chi = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    //~ chi_increment = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    omega = regExp(&optExpOmega, &header, 0, 1).toFloat()*pi/180.0;
    //~ omega_increment = regExp(&, &header, 0, 1).toFloat()*pi/180.0;
    //~ oscillation_axis = regExp(&, &header, 0, 1);
    //~ n_oscillations = regExp(&, &header, 0, 1).toInt();
    //~ start_position = regExp(&, &header, 0, 1).toFloat();
    //~ position_increment = regExp(&, &header, 0, 1).toFloat();
    //~ shutter_time = regExp(&, &header, 0, 1).toFloat();



    isFileHeaderRead = true;
    return isFileHeaderRead;
}

QString DetectorFile::getPath() const
{
    return path;
}

QString DetectorFile::regExp(QString * regular_expression, QString * source, size_t offset, size_t i)
{
    QRegExp tmp(*regular_expression);
    int pos = tmp.indexIn(*source, offset);
    if (pos > -1)
    {
        QString value = tmp.cap(i);
        return value;
    }
    else
    {
        return QString("");
    }
}

Matrix<float> & DetectorFile::getData()
{
    return data_buf;
}


void DetectorFile::clearData()
{
    data_buf.clear();
}

void DetectorFile::print()
{
    std::stringstream ss;
    ss << "__________ PILATUS FILE __________" << std::endl;
    ss << "Path: " << path.toStdString().c_str() << std::endl;
    ss << "Data elements: " << data_buf.size() << std::endl;
    ss << "Dimensions: " << fast_dimension << " x " << slow_dimension << std::endl;
    ss << "Max counts: " << max_counts << std::endl;
    ss << "Search radius: " << srchrad_sugg_low << " " << srchrad_sugg_high << std::endl; 
    ss << "..." << std::endl;
    ss << "Detector: " << detector.toStdString().c_str() << std::endl;
    ss << "Pixel size: " << pixel_size_x << " x " << pixel_size_y << std::endl;
    ss << "Exposure time: " << exposure_time << std::endl;
    ss << "Exposure period: " << exposure_period << std::endl;
    ss << "Count cutoff: " << count_cutoff << std::endl;    
    ss << "Wavelength: " << wavelength << std::endl;
    ss << "Detector distance: " << detector_distance << std::endl;
    ss << "Beam position: " << beam_x << " x " << beam_y << std::endl;
    ss << "Flux: " << flux << std::endl;
    ss << "Start angle: " << start_angle << std::endl;
    ss << "Angle increment: " << angle_increment << std::endl;
    ss << "Alpha: " << alpha << std::endl;
    ss << "Beta: " << beta << std::endl;
    ss << "Kappa: " << kappa << std::endl;
    ss << "Phi: " << phi << std::endl;
    ss << "Omega: " << omega << std::endl;
    
    qDebug() << ss.str().c_str();
}

QString DetectorFile::info()
{
    std::stringstream ss;
    ss << "#__________ PILATUS FILE __________" << std::endl;
    ss << "# Path: " << path.toStdString().c_str() << std::endl;
    ss << "# Data elements: " << data_buf.size() << std::endl;
    ss << "# Dimensions: " << fast_dimension << " x " << slow_dimension << std::endl;
    ss << "# Max counts: " << max_counts << std::endl;
    ss << "# ..." << std::endl;
    ss << "# Detector: " << detector.toStdString().c_str() << std::endl;
    ss << "# Pixel size: " << pixel_size_x << " x " << pixel_size_y << std::endl;
    ss << "# Exposure time: " << exposure_time << std::endl;
    ss << "# Exposure period: " << exposure_period << std::endl;
    ss << "# Count cutoff: " << count_cutoff << std::endl;    
    ss << "# Wavelength: " << wavelength << std::endl;
    ss << "# Detector distance: " << detector_distance << std::endl;
    ss << "# Beam position: " << beam_x << " x " << beam_y << std::endl;
    ss << "# Flux: " << flux << std::endl;
    ss << "# Start angle: " << start_angle << std::endl;
    ss << "# Angle increment: " << angle_increment << std::endl;
    ss << "# Alpha: " << alpha << std::endl;
    ss << "# Beta: " << beta << std::endl;
    ss << "# Kappa: " << kappa << std::endl;
    ss << "# Phi: " << phi << std::endl;
    ss << "# Omega: " << omega << std::endl;
    
    return QString(ss.str().c_str());
}



float DetectorFile::getDetectorDist()
{
    return detector_distance;
}

float DetectorFile::getBeamX()
{
    return beam_x;
}
float DetectorFile::getBeamY()
{
    return beam_y;
}
float DetectorFile::getPixSizeX()
{
    return pixel_size_x;
}
float DetectorFile::getPixSizeY()
{
    return pixel_size_y;
}


int DetectorFile::readData()
{
    if (isFileDataRead) return isFileDataRead;
    
    // Open file
    std::ifstream in(path.toStdString().c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
        std::cout << "Error reading file: " << path.toStdString().c_str() << std::endl;
        return 0;
    }

    in.seekg (0, in.end);
    int length = in.tellg();
    in.seekg (0, in.beg);
    int offset = 0;
    int header_length_max = 2000;

    // Read file
    char * buf = new char[length];
    in.read(buf, length);
    in.close();

    this->data_buf.reserve(1, fast_dimension*slow_dimension);

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
    size_t id = offset;
    int counts;
    int int16;
    int int32;

    int i, j;
    for (i = 0; i < (int) slow_dimension; i++)
    {
        for (j = 0; j < (int) fast_dimension; j++)
        {
            // Get value
            if (buf[id] == -128)
            {
                id++;
                int16 =  ((buf[id+1] << 8 ) | (buf[id] & 0xff));
                if (int16 == -32768)
                {
                    id+=2;
                    int32 = ((buf[id+3] << 24) | ((buf[id+2]& 0xff) << 16) | ((buf[id+1]& 0xff) << 8) | (buf[id]& 0xff));
                    counts = prev + int32;
                    id+=4;
                }
                else
                {
                    counts = prev + int16;
                    id+=2;
                }
            }
            else
            {
                counts = prev + (int) (buf[id]);
                id++;
            }
            
            
            
            prev = counts;
            
            
            if (counts < 0) counts = 0;
            
            data_buf[i*fast_dimension+j] = (float) counts;

            if (max_counts < counts) max_counts = counts;
        }
    }
    delete[] buf;
    
    isFileDataRead = true;
    return isFileDataRead;
}


void DetectorFile::setSearchRadiusHint()
{
    /* A search radius can be found based on the projected size of a pixel in reciprocal space. The following calculations assume a detector that can be translated but not rotated. Could give a fair estimate even for a rotating detector*/

    // For several pixel position extrema:
    float y_config[] = {(float) 0, (float) slow_dimension/2, (float) slow_dimension-1, (float) slow_dimension-1, (float) 0};
    float z_config[] = {(float) 0, (float) fast_dimension/2, (float) fast_dimension-1, (float) 0, (float) fast_dimension-1};

    for (int i = 0; i < 5; i++)
    {
        // Calculate the projected size of the pixel onto the Ewald sphere. In effect, we find the width along the diagonal of a rectangular pixel
        float xyz_a[3] = {
            (float)(-detector_distance),
            (float)(((y_config[i]-0.5) - beam_x) * pixel_size_x),
            (float)(((z_config[i]-0.5) - beam_y) * pixel_size_y)};

        float len_xyz_a = sqrt(xyz_a[0]*xyz_a[0] + xyz_a[1]*xyz_a[1] + xyz_a[2]*xyz_a[2]);

        xyz_a[0] /= len_xyz_a;
        xyz_a[1] /= len_xyz_a;
        xyz_a[2] /= len_xyz_a;

        float xyz_b[3] = {
            (float)(-detector_distance),
            (float)(((y_config[i]+0.5) - beam_x) * pixel_size_y),
            (float)(((z_config[i]+0.5) - beam_y) * pixel_size_x)};

        float len_xyz_b = sqrt(xyz_b[0]*xyz_b[0] + xyz_b[1]*xyz_b[1] + xyz_b[2]*xyz_b[2]);

        xyz_b[0] /= len_xyz_b;
        xyz_b[1] /= len_xyz_b;
        xyz_b[2] /= len_xyz_b;

        float k = 1/wavelength;

        float k_a[3] = {k*xyz_a[0], k*xyz_a[1], k*xyz_a[2]};
        float k_b[3] = {k*xyz_b[0], k*xyz_b[1], k*xyz_b[2]};

        float k_delta[3] = {k_b[0]-k_a[0], k_b[1]-k_a[1], k_b[2]-k_a[2]};
        float temp_search_radius = sqrt(k_delta[0]*k_delta[0] + k_delta[1]*k_delta[1] + k_delta[2]*k_delta[2]);

        if (temp_search_radius < srchrad_sugg_low) srchrad_sugg_low = temp_search_radius;
        if (temp_search_radius > srchrad_sugg_high) srchrad_sugg_high = temp_search_radius;
    }
}
