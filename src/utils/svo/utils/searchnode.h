#ifndef NODE_H
#define NODE_H

#include "../../math/qxmathlib.h"

class SearchNode {
    /* This class represents a node in the "search octtree" data structure. It is used in order to create bricks for the GPU octtree. */
    public:
        SearchNode();
        SearchNode(SearchNode * parent, double * extent);
        ~SearchNode();

//        void setOpenCLContext(OpenCLContext *context);
        void clearChildren();
        void clearPoints();
        void insert(float * point);
        void split();
        void print();
        void weighSamples(float * sample, double * sample_extent, float * sum_w, float * sum_wu, float p, float search_radius);
        bool isIntersected(double * sample_extent);

        /* gets and sets */
        void setParent(SearchNode * parent);

        bool getData(size_t max_points,
                     double *brick_extent,
                     float * point_data,
                     size_t *accumulated_points,
                     float search_radius);
        
        
        float getIDW(float * sample, float p, float search_radius);
        unsigned int getLevel();
        unsigned int getOctant(float * point, bool * isOutofBounds);
        double * getExtent();

    private:
        bool getIntersectedItems(Matrix<double> * effective_extent, size_t * accumulated_points, size_t max_points, float * point_data);
        SearchNode * parent;
        SearchNode ** children;
        float * points;
        Matrix<double> extent;

        float distance(float * a, float * b);

        unsigned int level;
        unsigned int n_children;
        unsigned int n_points;

        bool isEmpty;
        bool isMsd;
//        cl_int err;

//        OpenCLContext * context;
};
#endif
