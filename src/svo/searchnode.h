#ifndef NODE_H
#define NODE_H

#include <QList>

#include "../math/matrix.h"
#include "../misc/smallstuff.h"

class SearchNode
{
        /* This class represents a node in the "search octree" data structure. It is used in order to create bricks for the GPU octree. */
    public:
        SearchNode();
        SearchNode(SearchNode * p_parent, double * extent);
        ~SearchNode();

        void clear();
//        void clearPoints();
        void insert(xyzw32 &point);
        void split();
        void print();
        void weighSamples(xyzw32 & sample, double * sample_extent, float * sum_w, float * sum_wu, float p, float search_radius);
        bool isIntersected(double * sample_extent);
        void setExtent(Matrix<double> extent);
        void setParent(SearchNode * p_parent);

        bool getData(size_t max_points,
                     double * brick_extent,
                     QList<xyzw32> point_data,
                     size_t * accumulated_points,
                     float search_radius);


        float getIDW(xyzw32 &sample, float p, float search_radius);
        unsigned int getLevel();
        unsigned int getOctant(xyzw32 &point, bool * isOutofBounds);
        double * getExtent();

    private:
        bool getIntersectedItems(Matrix<double> * effective_extent, size_t * accumulated_points, size_t max_points, QList<xyzw32> * point_data);
        float distance(xyzw32 &a, xyzw32 &b);

        SearchNode * p_parent;
        QVector<SearchNode> p_children;
        QList<xyzw32> p_data_points;
        Matrix<double> p_extent;
        unsigned int p_tree_level;
        unsigned int p_n_children;
        bool p_is_empty;
        bool p_is_msd;
};
#endif
