#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <iomanip>
#include <cmath>
#include "miniarray.h"

class SearchNode {
    /* This class represents a node in the "search octtree" data structure. It is used in order to create bricks for the GPU octtree. */
    public:
        SearchNode();
        SearchNode(SearchNode * parent, double * extent);
        ~SearchNode();

        void clearChildren();
        void clearPoints();
        void insert(float * point);
        void split();
        void print();
        void weighSamples(float * sample, double * sample_extent, float * sum_w, float * sum_wu, float p, float search_radius);
        bool isIntersected(double * sample_extent);

        /* gets and sets */
        void setParent(SearchNode * parent);

        bool getBrick(float * dst, double * brick_extent, float p, float search_radius, unsigned int dimension);
        float getIDW(float * sample, float p, float search_radius);
        unsigned int getLevel();
        unsigned int getOctant(float * point, bool * isOutofBounds);
        double * getExtent();

    private:
        SearchNode * parent;
        SearchNode ** children;
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
