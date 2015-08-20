#ifndef FRAMECONTAINER_H
#define FRAMECONTAINER_H

#include <QString>
#include <QRect>
#include <QDebug>
#include <QList>
#include "selection.h"


class ImageInfo
{
        static int MAX_LSQ_SAMPLES;
        static int LSQ_SAMPLE_SIZE;

    public:
        ImageInfo();
        ImageInfo(const ImageInfo &other);
        ~ImageInfo();

        void setPath(QString str);
        const QString path() const;

        void setSelection(Selection rect);
        void setPlaneMarker(QList<Selection> marker);
        void setPlaneMarkerTest(QList<Selection> marker);
        //    void setBackground(Selection rect);
        QList<Selection> planeMarker() const;
        QList<Selection> * planeMarkerPtr();
        Selection selection() const;
        Selection * selectionPtr();
        //    Selection background() const;

        ImageInfo &operator =(ImageInfo other);

    private:
        QString p_path;
        Selection p_selection;
        QList<Selection> plane_sample;
        //    Selection p_background;
};

Q_DECLARE_METATYPE(ImageInfo);

QDebug operator<<(QDebug dbg, const ImageInfo &image);

QDataStream &operator<<(QDataStream &out, const ImageInfo &image);
QDataStream &operator>>(QDataStream &in, ImageInfo &image);

class ImageSeries
{
    public:
        ImageSeries();
        ImageSeries(const ImageSeries &other);
        ~ImageSeries();

        void setPath(QString str);
        const QString path() const;
        int size() const;
        int i() const;

        QStringList paths();

        void setPlaneMarker(QList<Selection> marker);
        void setSelection(Selection rect);
        void setImages(QList<ImageInfo> list);
        void clear();
        void removeCurrent();
        void append(ImageInfo image);
        void saveCurrentIndex();
        void loadSavedIndex();
        const QList<ImageInfo> &images() const;

        ImageInfo * current();
        ImageInfo * next();
        ImageInfo * previous();
        ImageInfo * begin();
        ImageInfo * at(int value);

        bool operator == (const ImageSeries &);

    private:
        QString p_path;
        QList<ImageInfo> p_images;
        int p_i;
        int p_i_memory;

};

Q_DECLARE_METATYPE(ImageSeries);

QDebug operator<<(QDebug dbg, const ImageSeries &image_series);

QDataStream &operator<<(QDataStream &out, const ImageSeries &image_series);
QDataStream &operator>>(QDataStream &in, ImageSeries &image_series);

ImageSeries &operator<<(ImageSeries &image_series, const ImageInfo &image);

class SeriesSet
{
    public:
        SeriesSet();
        SeriesSet(const SeriesSet &other);
        ~SeriesSet();

        ImageSeries * current();
        ImageSeries * next();
        ImageSeries * previous();
        ImageSeries * begin();


        QStringList paths();
        ImageSeries oneSeries();

        bool isEmpty();

        void setPlaneMarker(QList<Selection> marker);
        void setSelection(Selection rect);
        void saveCurrentIndex();
        void loadSavedIndex();
        void clear();
        void append(ImageSeries image_series);
        void removeCurrent();
        void setFolders(QList<ImageSeries> list);
        const QList<ImageSeries> &series() const;
        int size() const;
        int i() const;

    private:
        QList<ImageSeries> p_series;
        int p_i;
        int p_i_memory;
};

Q_DECLARE_METATYPE(SeriesSet);

QDebug operator<<(QDebug dbg, const SeriesSet &series_set);

QDataStream &operator<<(QDataStream &out, const SeriesSet &series_set);
QDataStream &operator>>(QDataStream &in, SeriesSet &series_set);

SeriesSet &operator<<(SeriesSet &series_set, const ImageSeries &image_series);

#endif // FRAMECONTAINER_H
