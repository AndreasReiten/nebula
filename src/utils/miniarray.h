#ifndef MINIARRAY_H
#define MINIARRAY_H


#include <QVector>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstring>

template <class T>
class MiniArray{
    public:
        MiniArray();
        MiniArray(size_t length, T * buffer);
        MiniArray(size_t length, T value);
        MiniArray(size_t length);
        MiniArray(const MiniArray & other);
        MiniArray(MiniArray && other);
        ~MiniArray();

        /* Operators */
        T& operator[] (const size_t index);
        const T& operator[] (const size_t index) const;
        MiniArray& operator = (MiniArray other);

        /* Management */
        MiniArray<float> toFloat() const;
        QVector<T> toQVector() const;
        void setShallow(size_t length, T * buffer);
        void setDeep(size_t length, T * buffer);
        void set(size_t length, T value);
        void resize(size_t new_length);
        void reserve(size_t length);
        T at(size_t index);
        void clear();
        T * data();
        T * data() const;

        /* Utility */
        void print(int precision = 0, const char * id = "", size_t cap = 0) const;
        size_t bytes() const;
        size_t size() const;
        double * histogram(size_t bins, T min, T max, bool log_x = 0, bool log_y = 0);
        T min();
        T max();

    protected:
        T * buffer;
        size_t length;

        /* Swap function as per C++11 idiom */
        void swap(MiniArray& first, MiniArray& second);
};




template <class T>
MiniArray<T>::MiniArray()
{
    this->length = 0;
    this->buffer = NULL;
}

template <class T>
MiniArray<T>::MiniArray(const MiniArray & other)
{
    this->length = other.size();
    this->buffer = new T[this->length];
    for (size_t i = 0; i < this->length; i++)
    {
        this->buffer[i] = other[i];
    }
}

template <class T>
MiniArray<T>::MiniArray(MiniArray && other)
{
    swap(*this, other);
}


template <class T>
MiniArray<T>::MiniArray(size_t length, T * buffer)
{
    this->length = length;
    this->buffer = buffer;
}

template <class T>
MiniArray<T>::MiniArray(size_t length, T value)
{
    this->length = length;
    this->buffer = new T[length];
    for (size_t i = 0; i < this->length; i++)
    {
        this->buffer[i] = value;
    }
}

template <class T>
MiniArray<T>::MiniArray(size_t length)
{
    this->length = length;
    this->buffer = new T[length];
}

template <class T>
MiniArray<T>::~MiniArray()
{
    //~ std::cout << "MiniArray::~ for length " << this->length << " buffer!" << std::endl;
    if (this->length > 0)
    {
        delete[] this->buffer;
    }
}

template <class T>
void MiniArray<T>::swap(MiniArray& first, MiniArray& second)
{
    std::swap(first.length, second.length);
    std::swap(first.buffer, second.buffer);
}


template <class T>
MiniArray<T>& MiniArray<T>::operator = (MiniArray other)
{
    swap(*this, other);

    return * this;
}

template <class T>
MiniArray<float> MiniArray<T>::toFloat() const
{
    MiniArray<float> buf(this->size());

    for (size_t i = 0; i < this->size(); i++)
    {
        buf[i] = (float) this->buffer[i];
    }

    return buf;
}

template <class T>
QVector<T> MiniArray<T>::toQVector() const
{
    QVector<T> buf;
    buf.resize(this->size());

    for (size_t i = 0; i < this->size(); i++)
    {
        buf[i] = this->buffer[i];
    }

    return buf;
}


template <class T>
T MiniArray<T>::min()
{
    if (length > 0)
    {
        T min = buffer[0];

        for (size_t i = 1; i < length; i++)
        {
            if (buffer[i] < min) min = buffer[i];
        }

        return min;
    }
    else return 0;
}

template <class T>
T MiniArray<T>::max()
{
    if (length > 0)
    {
        T max = buffer[0];

        for (size_t i = 1; i < length; i++)
        {
            if (buffer[i] > max) max = buffer[i];
        }

        return max;
    }
    else return 0;
}

template <class T>
size_t MiniArray<T>::bytes() const
{
    return length*sizeof(T);
}

template <class T>
void MiniArray<T>::print(int precision, const char * id, size_t cap) const
{
    if (strlen(id) > 0) std::cout << id << std::endl;
    if (cap == 0) cap = this->length;

    std::cout <<"{";
    for (size_t i = 0; i < cap; i++)
    {
        std::cout << std::setprecision(precision) << std::fixed << (double) this->buffer[i] ;
        if (i < length - 1)std::cout << ", ";
    }
    std::cout <<"}" << std::endl;
}

template <class T>
void MiniArray<T>::resize(size_t new_length)
{
    if(this->length == 0)
    {
        this->set(new_length, 0.0f);
    }
    else if(this->length > new_length)
    {
        T * temp = new T[new_length];
        for (size_t i = 0; i < new_length; i++)
        {
            temp[i] =  buffer[i];
        }
        this->clear();
        this->setShallow(new_length, temp);
    }
    else if(this->length < new_length)
    {
        T * temp = new T[new_length];
        for (size_t i = 0; i < this->length; i++)
        {
            temp[i] =  buffer[i];
        }
        for (size_t i = this->length; i < new_length; i++)
        {
            temp[i] =  0.0f;
        }
        this->clear();
        this->setShallow(new_length, temp);
    }
}

template <class T>
void MiniArray<T>::reserve(size_t length)
{
    std::cout << "reserve - length: " << this->length << std::endl;
    std::cout << "reserve - clear: " << length << std::endl;
    this->clear();
    this->length = length;
    std::cout << "reserve - new: " << length << std::endl;
    this->buffer = new T[length];
    std::cout << "reserve - done: " << length << std::endl;
}

template <class T>
T& MiniArray<T>::operator[] (const size_t index)
{
    assert(index < this->length);

    return buffer[index];
}

template <class T>
const T& MiniArray<T>::operator[] (const size_t index) const
{
    assert(index < this->length);

    return buffer[index];
}

template <class T>
void MiniArray<T>::setShallow(size_t length, T * buffer)
{
    //~ std::cout << "Set shallow!!" <<std::endl;
    this->clear();
    this->length = length;
    this->buffer = buffer;
}

template <class T>
void MiniArray<T>::setDeep(size_t length, T * buffer)
{
    this->clear();
    this->length = length;
    this->buffer = new T[length];
    for (size_t i = 0; i < this->length; i++)
    {
        this->buffer[i] = buffer[i];
    }
}

template <class T>
void MiniArray<T>::set(size_t length, T value)
{
    this->clear();
    this->length = length;
    this->buffer = new T[length];
    for (size_t i = 0; i < this->length; i++)
    {
        this->buffer[i] = value;
    }
}

template <class T>
void MiniArray<T>::clear()
{
    std::cout << "Attempting clear of " << length << " elements" << std::endl;
    if (this->length > 0)
    {
        delete[] this->buffer;
        this->length = 0;
        std::cout << "Cleared , new length: " << length << " elements" << std::endl;
    }
}

template <class T>
T * MiniArray<T>::data()
{
    return buffer;
}

template <class T>
T * MiniArray<T>::data() const
{
    return buffer;
}

template <class T>
T MiniArray<T>::at(size_t index)
{
    assert(index < this->length);

    return buffer[index];
}

template <class T>
size_t MiniArray<T>::size() const
{
    return this->length;
}

template <class T>
double * MiniArray<T>::histogram(size_t bins, T min, T max, bool log_x, bool log_y)
{
    // Place values into the bins. A value translates to the index of a bin. If the index is outside [0, bins], it is omitted
    if (length > 0)
    {
        double * histogram = new double[bins];

        for (size_t i = 0; i < bins; i++) histogram[i] = 0;

        if (log_x)
        {
            if (min < 1) min = 0;
            else min = std::log10(min);

            if (max < 1) max = 0;
            else max = std::log10(max);
        }

        size_t id;
        for (size_t i = 0; i < length; i++)
        {
            if (log_x)
            {
                if (buffer[i] < 1) continue;
                id = (std::log10(buffer[i]) - min)/ (max - min)*(bins - 1);
            }
            else
            {
                //~ if (buffer[i] == 0.0) continue;
                id = (buffer[i] - min)/ (max - min)*(bins - 1);
            }
            if ( (id >= 0) && (id < bins))
            {
                histogram[id] += 1;
            }
        }

        if (log_y)
        {
            for (size_t i = 0; i < bins; i++)
            {
                if (histogram[i] < 1) histogram[i] = 1;
                histogram[i] = std::log10(histogram[i]);
            }
        }


        return histogram;
    }
    else
    {
        return NULL;
    }
}


#endif
