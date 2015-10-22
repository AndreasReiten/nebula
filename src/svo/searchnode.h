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




//    bool isRoot();


//

//    // The most essential reconstruction functions
//    void insert(xyzwd32 &point);
//    void squeeze();
//    void rebuild();
//    void reorganize();
//    void recombine();
//    void voxelize(int octant, QVector<unsigned int> & index , QVector<unsigned int> & brick, QVector<int> & level_offsets, QVector<int> & level_progress,  int & pool_dim_x, int & pool_dim_y, int & pool_dim_z, int & num_bricks);
//    void nodeToPool(QVector<float> &pool, int dim_x, int dim_y, int dim_z, SearchNode &root);

//    void rebuildRecursive();

//
//



//    void print();

//    void setBinsPerSide(int value);
//
//    void setRoot(bool value);


//

    // Public funcs still used in new meta

    SearchNode();
    ~SearchNode();

    void setOriginal(bool value);
    void setElusive(bool value);
    void rebin();
    void countbricks(int &count);
    void setLevel(int value);
//    int level();
    bool isElusive();
    bool isVoxelized();
    bool isOriginal();
    bool isEmpty();
    bool isLeaf();
    bool isLeafBranch();
    QVector<SearchNode> & children();
    void insert(xyzwd32 &point);
    void recombine();

    void voxelizePassOne(SearchNode &root);
    void voxelizePassTwo(SearchNode &root);
    void voxelizePassThree(SearchNode & root);

//    void interpolate(SearchNode *root, bool check_neighbours = true);
//    void interpolate3(SearchNode *root);


    void gpuVoxels(int octant, QVector<float> &pool, QVector<unsigned int> &index, QVector<unsigned int> &brick, QVector<int> &array_level_offsets, QVector<int> &array_child_offsets, int &pool_dim_x, int &pool_dim_y, int &pool_dim_z, int &num_bricks);
    void hierarchy(QVector<QList<SearchNode *> > &nodes, int branch_leaf_both, int empty_nonempty_both, int finished_unfinished_both, int nonelusive_elusive_both, bool exclude_empty_leaves);
    void nodelist(QList<SearchNode *> &nodes, bool rebinned_only = false);

//    void setMaxPoints(int value);
    void clear();
    QVector<xyzwd32> & cloud();
    QVector<float> & nodegrid();

private:
//
//    double binside();


////        void gridcenter(float &x, float &y, float &z, float &w);

//    QVector<QVector<xyzwd32>> &bins();

//    void interdistMetrics(double & data_interdist_min, double & data_interdist_max, double & data_interdist_avg);
////        double gridValueAt_Nearest(double x, double y, double z);
//    double gridValueAt_Nearest(double x, double y, double z, int max_level = 1000);
//    double gridValueAt_Linear(double x, double y, double z, int max_level, SearchNode &root);
//    void gridRecombineMetrics(QVector<double> & averages, QVector<double> & sigmas);
//    void gridMetrics(double & average, double & sigma);

//    double binsum();
//    double gridsum();

//
//    void rebin();
//    void makeGridFromBins_Nearest();
//    void makeGridFromBins_Linear();
//    void p_insert(xyzwd32 &point);
//    void split();
//

//    int expendable();
//    int num_points();

//    QVector<QVector<xyzwd32>> p_data_binned; // Binned data
//    QVector<float> p_grid;

//    SearchNode * p_parent;

//    unsigned int p_id_pool_x, p_id_pool_y, p_id_pool_z;
//    int p_bins_per_side;
//    int p_voxels_per_side;

//    bool p_is_interpolation_node;
//    bool p_is_root;
//    bool p_is_max_resolved;


    // Private funcs still used in new meta
    void getSurroundingClouds(QVector<xyzwd32> & data, SearchNode &root);
    void interpolateElusiveNeighbours(SearchNode &root);
    double cloudValue(QVector<xyzwd32> & data, double x, double y, double z);
    int nodesPerSide();
    SearchNode * nodeAt(double x, double y, double z, int max_level);
    void setId(int id_x, int id_y, int id_z);
    void center(double & x, double & y, double & z);
    double nodeValueAt_Linear(double x, double y, double z, SearchNode &root);
    double nodeValueAt(double x, double y, double z, int level);
    void ensureNodeAt(double x, double y, double z, int level, SearchNode &root);
    double distance(double x0, double y0, double z0, double x1, double y1, double z1);
    double side();
    double binside();
    void interdistMetrics(double & data_interdist_min, double & data_interdist_max, double & data_interdist_avg);
    QVector<QVector<xyzwd32>> cloudbins(int grid_side);
    QVector<xyzwd32> cloudgrid(QVector<QVector<xyzwd32>> & bins);
    int cloudRecombinable(double req_avg_prct, double noise);
    int ntant(xyzwd32 &point, int n);
    int ntant(double x, double y, double z, int n);
    double voxelside();
    void p_insert(xyzwd32 &point);
    void split();

    // Data still used in new meta
    int p_max_points;
    int p_level;
    int p_id_node_x, p_id_node_y, p_id_node_z;

    QVector<SearchNode> p_children;
    QVector<xyzwd32> p_cloud;
    QVector<float> p_node_grid;
    QVector<float> p_voxel_grid;

    int p_node_grid_side;
    int p_voxel_grid_side;

    bool p_is_leaf;
    bool p_is_empty; // No node-grid or cloud data, the node is a zero-field
    bool p_is_voxelized; // The node has a voxel grid
    bool p_is_rebinned; // The node has been rebinned at some point
    bool p_is_elusive; // The node was constructed at a later stage when it was discovered that it potentially needs to be voxelized to avoid interpolation artefacts
    bool p_is_original; // The node was constructed during data insertion into the tree

    QMutex * p_mutex;
};
#endif
