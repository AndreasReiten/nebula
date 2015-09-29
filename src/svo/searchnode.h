#ifndef NODE_H
#define NODE_H

#include <QList>
#include <QLinkedList>
#include <QMutex>
#include <QMutexLocker>

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
// - Other interpolation modes than IDW to get rid of blobbyness. Especially fro non-msd leaves. IDW great unless sparsely populated.
// - In the intermediate tree, when a certain conditions are met, points are merged rather than added.
// - Conditional rebinning could work. Placing values directly into intermediate octree and either rebin or split octnodes depending
//   on data properties when they exceed a given number of points. If the contained data is smooth, then rebinning to n**3 samples is ok.
// - In lack of an easy-to-implement way of generating a both lossless and moderate intermediate data interpolation structure, it
//   might be that values must simply be added to a "giant" octree, and only let compression ensue when the tree is deeper (more resolved)
//   than the resolution dictated by the intersected data points (strict rebinning), or a size limit is reached  (conditional upwards
//   rebinning, first relaxed then strict).
// - Resolution is governed entirely by point density, but splits cannot occur over a certain resolution.

class SearchNode
{
        /* This class represents a node in the "search octree" data structure. It is used in order to create bricks for the GPU octree. */
    public:
        SearchNode();
//        SearchNode(SearchNode * p_parent, float * extent);
        ~SearchNode();

        bool isEmpty();

        bool isRoot();

        bool isMsd();

        QVector<SearchNode> & children();

        void setRoot(bool value);

        void rebin(bool relaxed);

        void estimate();

        void estimateRecursive();

        void brick();

        QVector<QVector<xyzw32> > &bins();

        void hierarchy(QVector<QList<SearchNode *> > &nodes, bool branches_only);

        void clear();
        void print();
        void insert(xyzw32 &point);
        void setBinsPerSide(int value);
        void setParent(SearchNode * p_parent);
        void setMaxPoints(int value);
        void setMinDataInterdistance(float value);
        bool isIntersected(Matrix<float> &sample_extent);
//        bool getData(size_t max_points,
//                     float * brick_extent,
//                     QList<xyzw32> point_data,
//                     size_t * accumulated_points,
//                     float search_radius);


//        float getIDW(xyzw32 &sample, float p, float search_radius);
        void setId(int id_x, int id_y, int id_z);


    private:
        int p_id_x, p_id_y, p_id_z;
        int num_points();
        void p_insert(xyzw32 &point);
//        void weighSamples(xyzw32 & sample, Matrix<float> &sample_extent, float * sum_w, float * sum_wu, float p, float search_radius);
//        bool intersectedItems(Matrix<float> &effective_extent, size_t * accumulated_points, size_t max_points, QList<xyzw32> * point_data);
//        float distance(xyzw32 &a, xyzw32 &b);
        void split();
        int level();
        int ntant(xyzw32 &point, int n);


        SearchNode * p_parent;
        QVector<SearchNode> p_children;
        QVector<QVector<xyzw32>> p_data_binned; // Binned data
        int p_level;
        bool p_bins_empty; // No children, no data in bins
        bool p_is_leaf;
        bool p_is_root;

        float p_min_data_interdistance;

        int p_bins_per_side;
        int p_max_points;

        QMutex * p_mutex;
};
#endif
