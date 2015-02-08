#include "framecontainer.h"

int ImageInfo::MAX_LSQ_SAMPLES = 20;
int ImageInfo::LSQ_SAMPLE_SIZE = 32;

ImageInfo::ImageInfo()
{
    p_selection = Selection(0,0,1e5,1e5);
    
    for (int i = 0; i < MAX_LSQ_SAMPLES; i++)
    {
        plane_sample <<  Selection(40*(i%2), (i/2)*(LSQ_SAMPLE_SIZE+8), LSQ_SAMPLE_SIZE, LSQ_SAMPLE_SIZE);
    }
    //    p_background = Selection(0,0,1e5,1e5);
}

ImageInfo::ImageInfo(const ImageInfo & other)
{
    p_selection = other.selection();
    plane_sample = other.planeMarker();
//    p_background = other.background();
    p_path = other.path();
}

ImageInfo::~ImageInfo()
{
    ;
}

void ImageInfo::setPath(QString str)
{
    p_path = str;
}

const QString ImageInfo::path() const
{
    return p_path;
}

void ImageInfo::setSelection(Selection rect)
{
    p_selection = rect;
}

//void ImageInfo::setBackground(Selection rect)
//{
//    p_background = rect;
//}

Selection ImageInfo::selection() const
{
    return p_selection;
}

Selection * ImageInfo::selectionPtr() 
{
    return &p_selection;
}

//Selection ImageInfo::background() const
//{
//    return p_background;
//}

ImageInfo& ImageInfo::operator = (ImageInfo other)
{
    p_path = other.path();
    p_selection = other.selection();
//    p_background = other.background();

    return * this;
}

void ImageInfo::setPlaneMarker(QList<Selection> marker)
{
    plane_sample = marker;
}


void ImageInfo::setPlaneMarkerTest(QList<Selection> marker)
{
//    qDebug() << "And so we copy";

//    for (int i = 0; i < marker.size(); i++)
//    {
//        qDebug() << marker[i].selected() << marker[i].integral();
//    }

    plane_sample = marker;

//    for (int i = 0; i < marker.size(); i++)
//    {
//        qDebug() << plane_marker[i].selected();
//    }
}
//    void setBackground(Selection rect);
QList<Selection> ImageInfo::planeMarker() const
{
    return  plane_sample;
}

QList<Selection> * ImageInfo::planeMarkerPtr()
{
    return  &plane_sample;
}

QDebug operator<<(QDebug dbg, const ImageInfo &image)
{
    dbg.nospace() << "Image()" << image.path() << image.selection();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const ImageInfo &image)
{
    out << image.path() << image.selection() << image.planeMarker();

    return out;
}

QDataStream &operator>>(QDataStream &in, ImageInfo &image)
{
    QString path;
    Selection selection;
//    Selection background;
    QList<Selection> plane_marker;

    in >> path >> selection >> plane_marker;
    image.setPath(path);
    image.setSelection(selection);
    image.setPlaneMarker(plane_marker);
//    image.setBackground(background);

    return in;
}


ImageSeries::ImageSeries()
{
    p_i = 0;
    p_i_memory = 0;
}
ImageSeries::ImageSeries(const ImageSeries & other)
{
    p_images = other.images();
    p_path = other.path();
    p_i = 0;
    p_i_memory = 0;
    
}
ImageSeries::~ImageSeries()
{
    ;
}

void ImageSeries::setPath(QString str)
{
    p_path = str;
}

const QString ImageSeries::path() const
{
    return p_path;
}

int ImageSeries::size() const
{
    return p_images.size();
}

int ImageSeries::i() const
{
    return p_i;
}


QStringList ImageSeries::paths()
{
    QStringList paths;

    for (int j = 0; j < p_images.size(); j++)
    {
        paths << p_images[j].path();
    }

    return paths;
}

void ImageSeries::setPlaneMarker(QList<Selection> marker)
{
    for (int i = 0; i < p_images.size(); i++)
    {
        p_images[i].setPlaneMarker(marker);
    }
}

void ImageSeries::setSelection(Selection rect)
{
    for (int i = 0; i < p_images.size(); i++)
    {
        p_images[i].setSelection(rect);
    }
}

void ImageSeries::setImages(QList<ImageInfo> list)
{
    p_images = list;
    p_i = 0;
    p_i_memory = 0;
}

void ImageSeries::clear()
{
    p_images.clear();

    p_i = 0;
    p_i_memory = 0;
}

void ImageSeries::removeCurrent()
{
    p_images.removeAt(p_i);

    if (p_i > 0)
    {
        p_i--;
    }
}

void ImageSeries::saveCurrentIndex()
{
    p_i_memory = p_i;
}

void ImageSeries::loadSavedIndex()
{
    if (p_i_memory < images().size())
    {
        p_i = p_i_memory;
    }
}

void ImageSeries::append(ImageInfo image)
{
    p_images.append(image);
}

ImageInfo * ImageSeries::current()
{
    return &p_images[p_i];
}

ImageInfo * ImageSeries::next()
{
    if (p_i < p_images.size() - 1)
    {
        p_i++;
    }

    return &p_images[p_i];
}

ImageInfo * ImageSeries::at(int value)
{
    if (value >= p_images.size())
    {
        value = p_images.size() - 1;
    }
    if (value < 0)
    {
        p_i = 0;
    }

    p_i = value;
    
    return &p_images[p_i];
}


ImageInfo * ImageSeries::previous()
{
    if ((p_i > 0) && (p_images.size() > 0))
    {
        p_i--;
    }

    return &p_images[p_i];
}

ImageInfo * ImageSeries::begin()
{
    p_i = 0;
    return &p_images[p_i];
}

const QList<ImageInfo> & ImageSeries::images() const
{
    return p_images;
}

bool ImageSeries::operator ==(const ImageSeries& other)
{
    if (this->path() == other.path()) return true;
    else return false;
}

QDebug operator<<(QDebug dbg, const ImageSeries &image_series)
{
    dbg.nospace() << "ImageSeries()" << image_series.path();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const ImageSeries &image_series)
{
    out << image_series.path() << image_series.images();

    return out;
}

QDataStream &operator>>(QDataStream &in, ImageSeries &image_series)
{
    QString path;
    QList<ImageInfo> images;

    in >> path >> images;
    image_series.setPath(path);
    image_series.setImages(images);

    return in;
}

ImageSeries &operator<<(ImageSeries &image_series, const ImageInfo &image)
{
    image_series.append(image);

    return image_series;
}

SeriesSet::SeriesSet()
{
    p_i = 0;
    p_i_memory = 0;
}
SeriesSet::SeriesSet(const SeriesSet & other)
{
    p_series = other.series();
    p_i = 0;
    p_i_memory = 0;
    
}
SeriesSet::~SeriesSet()
{

}
int SeriesSet::i() const
{
    return p_i;
}

ImageSeries * SeriesSet::current()
{
    return &p_series[p_i];
}
ImageSeries * SeriesSet::next()
{
    if (p_i < p_series.size() - 1)
    {
        p_i++;
    }

    return &p_series[p_i];
}
ImageSeries * SeriesSet::previous()
{
    if ((p_i > 0) && (p_series.size() > 0))
    {
        p_i--;
    }

    return &p_series[p_i];
}

ImageSeries * SeriesSet::begin()
{
    p_i = 0;
    return &p_series[p_i];
}

void SeriesSet::setFolders(QList<ImageSeries> list)
{
    p_series = list;
    p_i = 0;
    p_i_memory = 0;
}

bool SeriesSet::isEmpty()
{
    for (int i = 0; i < p_series.size(); i++)
    {
        if (p_series[i].size()) return false;
    }

    return true;
}

void SeriesSet::setPlaneMarker(QList<Selection> marker)
{
    for (int i = 0; i < p_series.size(); i++)
    {
        p_series[i].setPlaneMarker(marker);
    }
}

void SeriesSet::setSelection(Selection rect)
{
    for (int i = 0; i < p_series.size(); i++)
    {
        p_series[i].setSelection(rect);
    }
}

void SeriesSet::saveCurrentIndex()
{
    p_i_memory = p_i;
}

void SeriesSet::loadSavedIndex()
{
    if (p_i_memory < series().size())
    {
        p_i = p_i_memory;
    }
}

void SeriesSet::clear()
{
    p_series.clear();

    p_i = 0;
    p_i_memory = 0;
}

void SeriesSet::removeCurrent()
{
    p_series.removeAt(p_i);

    if (p_i > 0)
    {
        p_i--;
    }
}

QStringList SeriesSet::paths()
{
    QStringList paths;

    for (int i = 0; i < p_series.size(); i++)
    {
        for (int j = 0; j < p_series[i].images().size(); j++)
        {
            paths << p_series[i].images()[j].path();
        }
    }

    return paths;
}

ImageSeries SeriesSet::oneSeries()
{
    ImageSeries series;
    
    for (int i = 0; i < p_series.size(); i++)
    {
        for (int j = 0; j < p_series[i].images().size(); j++)
        {
            series << p_series[i].images()[j];
        }
    }

    return series;
}


int SeriesSet::size() const
{
    return p_series.size();
}

const QList<ImageSeries> &SeriesSet::series() const
{
    return p_series;
}

void SeriesSet::append(ImageSeries image_series)
{
    p_series.append(image_series);
}

QDebug operator<<(QDebug dbg, const SeriesSet &series_set)
{
    dbg.nospace() << "SeriesSet()" << series_set.size();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const SeriesSet &series_set)
{
    out << series_set.series();

    return out;
}

QDataStream &operator>>(QDataStream &in, SeriesSet &series_set)
{
    QList<ImageSeries> series_list;

    in >> series_list;
    series_set.setFolders(series_list);

    return in;
}

SeriesSet &operator<<(SeriesSet &series_set, const ImageSeries &image_series)
{
    series_set.append(image_series);

    return series_set;
}
