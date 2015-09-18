#ifndef NODE_H
#define NODE_H

#include <QList>

#include "../math/matrix.h"
#include "../misc/smallstuff.h"


// Stop subdivision if the following conditions are met:
// - The node reaches a minimum volume
// - Recursion has reached a maximum number of subdivisions

// Ways of reducing a node given appropriate reqs:
// - Values in a bottom node can be reevaluated on a regular grid
// - Values in any node can be reevaluated on a regular grid if they have a sufficiently high degree of self similarity
// - A re-evaluation should result in considerably fewer points than MAX_POINTS, unless it is a bottom node
// - Sufficiently close points can be merged


class SearchNode
{
        /* This class represents a node in the "search octree" data structure. It is used in order to create bricks for the GPU octree. */
    public:
        SearchNode();
        SearchNode(SearchNode * p_parent, double * extent);
        ~SearchNode();

        void clear();
        void print();
        void insert(xyzw32 &point);
        void setExtent(Matrix<double> extent);
        void setParent(SearchNode * p_parent);
        bool isIntersected(Matrix<double> &sample_extent);
        bool getData(size_t max_points,
                     double * brick_extent,
                     QList<xyzw32> point_data,
                     size_t * accumulated_points,
                     float search_radius);


        float getIDW(xyzw32 &sample, float p, float search_radius);


    private:
        void weighSamples(xyzw32 & sample, Matrix<double> &sample_extent, float * sum_w, float * sum_wu, float p, float search_radius);
        bool intersectedItems(Matrix<double> &effective_extent, size_t * accumulated_points, size_t max_points, QList<xyzw32> * point_data);
        float distance(xyzw32 &a, xyzw32 &b);
        void split();
        unsigned int level();
        unsigned int octant(xyzw32 &point, bool * isOutofBounds);

        SearchNode * p_parent;
        QVector<SearchNode> p_children;
        QVector<xyzw32> p_data_points;
        Matrix<double> p_extent;
        unsigned int p_tree_level;
//        unsigned int p_n_children;
        bool p_is_empty;
        bool p_is_msd;
};
#endif
