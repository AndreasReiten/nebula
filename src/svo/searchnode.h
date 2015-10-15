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
        ~SearchNode();

        bool isLeafBranch();
        bool isEmpty();
        bool isRoot();
        bool isLeaf();

        QVector<SearchNode> & children();

        // The most essential reconstruction functions
        void insert(xyzwd32 &point);
        void squeeze();
        void rebuild();
        void reorganize();
        void recombine();
        void voxelize(int octant, QVector<unsigned int> & index , QVector<unsigned int> & brick, QVector<int> & level_offsets, QVector<int> & level_progress,  int & pool_dim_x, int & pool_dim_y, int & pool_dim_z, int & num_bricks);
        void nodeToPool(QVector<float> &pool, int dim_x, int dim_y, int dim_z, SearchNode &root);

        void rebuildRecursive();

        void hierarchy(QVector<QList<SearchNode *> > &nodes, int branch_leaf_both, int empty_nonempty_both);
        void nodelist(QVector<SearchNode*> &nodes, bool unresolved_leaves_only);
        void countbricks(int &count);


        void clear();
        void print();

        void setBinsPerSide(int value);
        void setParent(SearchNode * p_parent);
        void setRoot(bool value);
        void setMaxPoints(int value);

        int level();

    private:
        int nodesPerSide();
        double side();
        double binside();
        void center(double & x, double & y, double & z);
//        void gridcenter(float &x, float &y, float &z, float &w);

        QVector<QVector<xyzwd32>> &bins();

        void interdistMetrics(double & data_interdist_min, double & data_interdist_max, double & data_interdist_avg);
//        double gridValueAt_Nearest(double x, double y, double z);
        double gridValueAt_Nearest(double x, double y, double z, int max_level = 1000);
        double gridValueAt_Linear(double x, double y, double z, int max_level, SearchNode &root);
        void gridRecombineMetrics(QVector<double> & averages, QVector<double> & sigmas);
        void gridMetrics(double & average, double & sigma);

        double binsum();
        double gridsum();

        void rebin();
        void makeGridFromBins();
        void p_insert(xyzwd32 &point);
        void split();
        void setId(int id_x, int id_y, int id_z);

        int expendable();
        int num_points();
        int ntant(xyzwd32 &point, int n);
//        int ntant(double x, double y, double z, int n, QList<nodeinfo> &trace);
        int ntant(double x, double y, double z, int n);


        QVector<QVector<xyzwd32>> p_data_binned; // Binned data
        QVector<float> p_grid;
        QVector<SearchNode> p_children;
        SearchNode * p_parent;

//        float p_min_data_interdistance;

        unsigned int p_id_pool_x, p_id_pool_y, p_id_pool_z;
        int p_id_node_x, p_id_node_y, p_id_node_z;
        int p_level;
        int p_bins_per_side;
        int p_max_points;

        bool p_is_empty; // No data
        bool p_is_leaf;
        bool p_is_root;
        bool p_is_max_resolved;

        QMutex * p_mutex;
};
#endif
