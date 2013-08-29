#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <iomanip>
#include <cmath>
#include "miniarray.h"

class Node {
    public:
        Node();
        Node(Node * parent, double * extent);
        ~Node();
        
        void clearChildren();
        void clearPoints();
        void insert(float * point);
        void split();
        void print();
        void weighSamples(float * sample, double * sample_extent, float * sum_w, float * sum_wu, float p, float search_radius);
        bool isIntersected(double * sample_extent);
        
        /* gets and sets */
        void setParent(Node * parent);
        
        bool getBrick(float * dst, double * brick_extent, float p, float search_radius, unsigned int dimension);
        float getIDW(float * sample, float p, float search_radius);
        unsigned int getLevel();
        unsigned int getOctant(float * point, bool * isOutofBounds);
        double * getExtent();
    private:
        Node * parent;
        Node ** children;
        float * points;
        double extent[6];
        
        float distance(float * a, float * b);
        
        
        unsigned int level;
        unsigned int n_children;
        unsigned int n_points;
        
        bool isEmpty;
        bool isMsd;
};
#endif
