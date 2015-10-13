#include "searchnode.h"

SearchNode::SearchNode() :
    p_id_pool_x(0),
    p_id_pool_y(0),
    p_id_pool_z(0),
    p_id_node_x(0),
    p_id_node_y(0),
    p_id_node_z(0),
    p_is_root(false),
    p_is_leaf(true),
    p_is_empty(true)

{


//    p_min_data_interdistance = 0;

    p_mutex = new QMutex;
}

SearchNode::~SearchNode()
{

}

void SearchNode::squeeze()
{
    // Perform a conditional split of a leaf node in an attempt to increase resolution
//    binMetrics(); // Metrics for the point bins


//    bool is_max_resolved = ;
//    bool is_self_similar = ;

//    if (!is_max_resolved && !is_self_similar) split();
}

void SearchNode::recombine()
{
    // Do not recombine anything but a branch with leaves
    if (!isLeafBranch()) return;

    int expendables = 0;

    for (int i = 0; i < p_children.size(); i++)
    {
        expendables += p_children[i].expendable();

//        qDebug() << "Child" << i << "expendables" << expendables;

    }


    if (expendables >= p_bins_per_side*p_bins_per_side*p_bins_per_side)
    {
        p_children.clear();
        p_is_leaf = true;

//        qDebug() << "Recombined node in level" << p_level;
    }
}

int SearchNode::expendable()
{
    /* Dermine if the points in the grid are so self-similar that the node can be discarded in favour of the parent node.
     * Not to be called directly, but rather from a parent, since if all children can be discarded, then the parent will be the new leaf.
     * */

    // If empty, with no relevant data
    if (p_is_empty) return p_bins_per_side*p_bins_per_side*p_bins_per_side/8;

    // Evaluate if and how the data points can be reduced
    int n_expendable_bins = 0;

    QVector<double> averages(p_bins_per_side*p_bins_per_side*p_bins_per_side/8);
    QVector<double> sigmas(p_bins_per_side*p_bins_per_side*p_bins_per_side/8);

    gridMetrics(averages, sigmas);

    for (int i = 0; i < p_bins_per_side*p_bins_per_side*p_bins_per_side/8; i+=2)
    {
        if (sigmas[i] < averages[i]*0.1 + 5.0) n_expendable_bins++;
    }

    return n_expendable_bins;
}

void SearchNode::gridMetrics(QVector<double> & averages, QVector<double> & sigmas)
{
    // Metrics for each 2x2x2 point cube, used primarily to assess if the node is exceedingly self-similar
    int counter = 0;


    for (int i = 0; i < p_bins_per_side; i+=2)
    {
        for (int j = 0; j < p_bins_per_side; j+=2)
        {
            for (int k = 0; k < p_bins_per_side; k+=2)
            {
                // Average
                double avg = 0;
                avg += p_grid[(i+0) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side];
                avg += p_grid[(i+1) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side];
                avg += p_grid[(i+0) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side];
                avg += p_grid[(i+0) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side];
                avg += p_grid[(i+1) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side];
                avg += p_grid[(i+0) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side];
                avg += p_grid[(i+1) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side];
                avg += p_grid[(i+1) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side];
                avg /= 8.0;

                // Standard deviation
                double sigma = 0;
                sigma += std::powf(p_grid[(i+0) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::powf(p_grid[(i+1) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::powf(p_grid[(i+0) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::powf(p_grid[(i+0) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::powf(p_grid[(i+1) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::powf(p_grid[(i+0) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::powf(p_grid[(i+1) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::powf(p_grid[(i+1) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma /= 8.0;
                sigma = std::sqrt(sigma);

                averages[counter] = avg;
                sigmas[counter] = sigma;

                counter++;
            }
        }
    }
}



void SearchNode::reorganize()
{
    // If empty, with no relevant data
    if (p_is_empty) return;

    makeGridFromBins();

    p_data_binned.clear();
}

void SearchNode::makeGridFromBins()
{
    // Reduce a node from bins with irregular xyzw points to a simple equi-distanced grid with w-values
    p_grid.resize(p_bins_per_side*p_bins_per_side*p_bins_per_side);

    // For each bin
    for (int i = 0; i < p_data_binned.size(); i++)
    {
        if (p_data_binned[i].size() <= 0)
        {
            p_grid[i] = 0;
        }
        else
        {
            // Calculate a single average point
            double tmp = 0;

            for (int j = 0; j < p_data_binned[i].size(); j++)
            {
                tmp += p_data_binned[i][j].w;
            }

            tmp /= (double) p_data_binned[i].size();

            p_grid[i] = tmp;
        }
    }
}


double SearchNode::gridValueAt_Nearest(double x, double y, double z)
{
    // If leaf, search the grid
    if (isLeaf())
    {
        if (isEmpty())
        {
            return 0.0;
        }
        else
        {
            int id = ntant(x,y,z, p_bins_per_side);
            if (id < 0)
            {
                return 0.0;
            }
            else
            {
                return p_grid[id];
            }
        }
    }
    // Else if branch, descend to the next level
    else
    {
        int id = ntant(x,y,z, 2);
        if (id < 0)
        {
            return 0.0;
        }
        else
        {
            return p_children[id].gridValueAt_Nearest(x, y, z);
        }
    }
}

double SearchNode::gridValueAt_Linear(double x, double y, double z)
{
    // If leaf, search the grid
    if (isLeaf())
    {
        if (isEmpty())
        {
            return 0.0;
        }
        else
        {
            int id = ntant(x,y,z, p_bins_per_side);
            if (id < 0)
            {
                return 0.0;
            }
            else
            {
                return p_grid[id];
            }
        }
    }
    // Else if branch, descend to the next level
    else
    {
        int id = ntant(x,y,z, 2);
        if (id < 0)
        {
            return 0.0;
        }
        else
        {
            return p_children[id].gridValueAt_Nearest(x, y, z);
        }
    }
}




double SearchNode::binsum()
{
    double sum = 0;
    for (int j = 0; j < p_data_binned.size(); j++)
    {
        for (int k = 0; k < p_data_binned[j].size(); k++)
        {
            sum += p_data_binned[j][k].w;
        }
    }

    return sum;
}

double SearchNode::gridsum()
{
    double sum = 0;
    for (int j = 0; j < p_grid.size(); j++)
    {
        sum += p_grid[j];
    }

    return sum;
}

void SearchNode::rebin()
{
    // Regenerate data points within bins. For each grid cell, the xyz position of the data point is the average
    // position of the sample sub-set (with possible weighing).
    // If there are no points, then no data point is generated. In some cases, the points might not be rebinned at all.
    // For each bin
    for (int i = 0; i < p_data_binned.size(); i++)
    {
        if (p_data_binned[i].size() <= 1) continue;

        // Calculate a single average point
        xyzwd32 tmp = {0.0,0.0,0.0,0.0,0.0};

        for (int j = 0; j < p_data_binned[i].size(); j++)
        {
            tmp.x += p_data_binned[i][j].x;
            tmp.y += p_data_binned[i][j].y;
            tmp.z += p_data_binned[i][j].z;
            tmp.w += p_data_binned[i][j].w;
            tmp.d += p_data_binned[i][j].d;
        }

        tmp.x /= (float) p_data_binned[i].size();
        tmp.y /= (float) p_data_binned[i].size();
        tmp.z /= (float) p_data_binned[i].size();
        tmp.w /= (float) p_data_binned[i].size();
        tmp.d /= (float) p_data_binned[i].size();

        p_data_binned[i].clear();

        p_data_binned[i] << tmp;
    }
}


void SearchNode::rebuild()
{
    // Non-recursively generate a point cloud on an irregular grid based on the child nodes
    if(p_is_leaf) return;

    // Get the relevant data points, and place them in bins
    p_data_binned.resize(p_bins_per_side*p_bins_per_side*p_bins_per_side);

    for (int i = 0; i < 8; i++)
    {
        if (p_children[i].isEmpty()) continue;

        for (int j = 0; j < p_children[i].bins().size(); j++)
        {
            for (int k = 0; k < p_children[i].bins()[j].size(); k++)
            {
                int id = ntant(p_children[i].bins()[j][k], p_bins_per_side);
                p_data_binned[id] << p_children[i].bins()[j][k];
            }
        }
    }

    rebin();

    p_is_empty = false;
}

void SearchNode::rebuildRecursive()
{
    // Recursively generate a point cloud on an irregular grid based on the child nodes
    if(p_is_leaf) return;

    // For all children
    for (int i = 0; i < 8; i++)
    {
        p_children[i].rebuildRecursive();
    }


    // Get the relevant data points, and place them in bins
    p_data_binned.resize(p_bins_per_side*p_bins_per_side*p_bins_per_side);

    for (int i = 0; i < 8; i++)
    {
        if (p_children[i].isEmpty()) continue;

        for (int j = 0; j < p_bins_per_side*p_bins_per_side*p_bins_per_side; j++)
        {
            for (int k = 0; k < p_children[i].bins()[j].size(); k++)
            {
                int id = ntant(p_children[i].bins()[j][k], p_bins_per_side);
                p_data_binned[id] << p_children[i].bins()[j][k];
            }
        }
    }

    rebin();

    p_is_empty = false;
}

QVector<QVector<xyzwd32> > &SearchNode::bins()
{
    return p_data_binned;
}

bool SearchNode::isLeafBranch()
{
    // Return true if this is a branch with leaves
    if (p_is_leaf || p_is_empty) return false;

    for (int i = 0; i < 8; i++)
    {
        if (!p_children[i].isLeaf()) return false;
    }

    return true;
}

bool SearchNode::isEmpty()
{
    return p_is_empty;
}

bool SearchNode::isRoot()
{
    return p_is_root;
}

bool SearchNode::isLeaf()
{
    return p_is_leaf;
}

QVector<SearchNode> & SearchNode::children()
{
    return p_children;
}


void SearchNode::countbricks(int &  count)
{
    if (!p_is_empty) count++;

    for (int i = 0; i < p_children.size(); i++)
    {
        p_children[i].countbricks(count);
    }
}

void SearchNode::nodelist(QVector<SearchNode*> &nodes)
{
    // Add self to list
    nodes << this;

    // Add children to list
    if(!p_is_leaf)
    {
        // Call for children
        for (int i = 0; i < 8; i++)
        {
            p_children[i].nodelist(nodes);
        }
    }
}

void SearchNode::hierarchy(QVector<QList<SearchNode *>> & nodes, int branch_leaf_both, int empty_nonempty_both)
{
    // Return a data structure representing the node hierarchy, including branches, leaves, or both, but only if empty, nonempty, or both
    bool add = false;

    if ((branch_leaf_both == 0) && !p_is_leaf) // Branch
    {
        if ((empty_nonempty_both == 0) && p_is_empty) add = true;
        if ((empty_nonempty_both == 1) && !p_is_empty) add = true;
        if (empty_nonempty_both == 2) add = true;
    }
    else if ((branch_leaf_both == 1) && p_is_leaf) // Leaf
    {
        if ((empty_nonempty_both == 0) && p_is_empty) add = true;
        if ((empty_nonempty_both == 1) && !p_is_empty) add = true;
        if (empty_nonempty_both == 2) add = true;
    }
    else if (branch_leaf_both == 2) // Both
    {
        if ((empty_nonempty_both == 0) && p_is_empty) add = true;
        if ((empty_nonempty_both == 1) && !p_is_empty) add = true;
        if (empty_nonempty_both == 2) add = true;
    }

    if (add)
    {
        // Add self to hierarchy
        if (nodes.size() <= p_level)
        {
            nodes.resize(p_level+1);
        }

        nodes[p_level] << this;
    }

    // Add children to hierarchy
    for (int i = 0; i < p_children.size(); i++)
    {
        p_children[i].hierarchy(nodes, branch_leaf_both, empty_nonempty_both);
    }
}

void SearchNode::voxelize(int octant, QVector<unsigned int> & index , QVector<unsigned int> & brick, QVector<int> & array_level_offsets, QVector<int> & array_child_offsets,  int & pool_dim_x, int & pool_dim_y, int & pool_dim_z, int & num_bricks)
{
    // Add this node to a voxel octree

    // Generate a 3D pool ID unless the node is empty
    if (!p_is_empty)
    {
        p_id_pool_z = num_bricks / (pool_dim_x * pool_dim_y);
        p_id_pool_y = (num_bricks - p_id_pool_z * (pool_dim_x * pool_dim_y)) / pool_dim_x;
        p_id_pool_x = num_bricks - p_id_pool_z * (pool_dim_x * pool_dim_y) - p_id_pool_y * pool_dim_x;

        num_bricks++;
    }

    // Prepare the voxel metadata for this node
    unsigned int leaf_flag = p_is_leaf;
    unsigned int populated_flag = !p_is_empty;
    unsigned int child_index = 0;

    if (!p_is_leaf)
    {
        child_index = array_level_offsets[p_level+1] + array_child_offsets[p_level+1];
    }

//    int arr_id = array_level_offsets[p_level]+array_child_offsets[p_level];
//    unsigned int arr_index = (leaf_flag << 31) | (populated_flag << 30) | child_index;
//    unsigned int arr_brick = (p_pool_x << 20) | (p_pool_y << 10) | (p_pool_z << 0);

    // Assign the metadata, making sure it is stored at the appropriate index
    index[array_level_offsets[p_level] + array_child_offsets[p_level] + octant] = (leaf_flag << 31) | (populated_flag << 30) | child_index;
    brick[array_level_offsets[p_level] + array_child_offsets[p_level] + octant] = (p_id_pool_x << 20) | (p_id_pool_y << 10) | (p_id_pool_z << 0);

    if (!p_is_leaf)
    {
        // Add children
        for (int i = 0; i < 8; i++)
        {
            p_children[i].voxelize(/*Specify index at which to place new node?*/i, index , brick, array_level_offsets, array_child_offsets,  pool_dim_x, pool_dim_y, pool_dim_z, num_bricks);
        }

        array_child_offsets[p_level+1] += 8;
    }
}


void SearchNode::brickToPool(QVector<float> & pool, int dim_x, int dim_y, int dim_z, SearchNode & root)
{
    if (p_is_empty) return;

    // Return a brick corresponding to the given extent based on the octree data
    int side = 1 + p_bins_per_side + 1;

    QVector<float> tmp(side*side*side);

    for (int i = 0; i < side; i++)
    {
        for (int j = 0; j < side; j++)
        {
            for (int k = 0; k < side; k++)
            {
                int id_tmp =
                        (i) +
                        (j) * ( side)  +
                        (k) * ( side * side);

                // The linear id the current value should have in the brick pool
                int id_pool =
                        (p_id_pool_x * side + i) +
                        (p_id_pool_y * side + j) * (dim_x * side)  +
                        (p_id_pool_z * side + k) * (dim_x * side * dim_y * side);

                // If the id is associated with the face of the brick
                if ((i < 1) || (i >= side - 1) || (j < 1) || (j >= side - 1) || (k < 1) || (k >= side - 1))
                {
                    // Do a lookup of the value
                    double x = (double) (p_id_node_x * p_bins_per_side + (double)((i-1) + 0.5 )) / (double) ((1 << p_level) * p_bins_per_side);
                    double y = (double) (p_id_node_y * p_bins_per_side + (double)((j-1) + 0.5 )) / (double) ((1 << p_level) * p_bins_per_side);
                    double z = (double) (p_id_node_z * p_bins_per_side + (double)((k-1) + 0.5 )) / (double) ((1 << p_level) * p_bins_per_side);

                    // If requested value lies outside of octtree, set to zero
                    if ((x < 0) || (x >= 1) || (y < 0) || (y >= 1) || (z < 0) || (z >= 1))
                    {
                        pool[id_pool] = 0;
                        tmp[id_tmp] = 0;
//                        qDebug() << "Requested value lies outside of octtree" << x << y << z;
                    }
                    // Else use grid value
                    else
                    {
                        tmp[id_tmp] = root.gridValueAt_Nearest(x, y, z);
                        pool[id_pool] = root.gridValueAt_Nearest(x, y, z);
                    }
                }
                // Else find the data in this grid
                else
                {
                    int id_grid = (i-1) + (j-1) * p_bins_per_side + (k-1) * p_bins_per_side * p_bins_per_side;
                    pool[id_pool] = p_grid[id_grid];
                    tmp[id_tmp] = p_grid[id_grid];
                }
            }
        }
    }


}


void SearchNode::clear()
{
    p_children.clear();
    p_data_binned.clear();
    p_grid.clear();
    p_is_leaf = true;
    p_is_empty = true;
}

void SearchNode::setBinsPerSide(int value)
{
    p_bins_per_side = value;
}

void SearchNode::setMaxPoints(int value)
{
    p_max_points = value;
}

void SearchNode::setRoot(bool value)
{
    p_is_root = value;
    p_level = 0;
}

void SearchNode::setParent(SearchNode * parent)
{
    p_parent = parent;
    p_level = parent->level() + 1;
}

int SearchNode::level()
{
    return p_level;
}


void SearchNode::print()
{
    /* Print self */
    for (int i = 0; i < p_level; i++)
    {
        std::cout << " ";
    }

//    std::cout << "L" << p_tree_level << "-> n bins: " << p_bins.size() << " c: " << 8 << " empt: " << p_is_empty << " msd: = " << p_is_msd << " Ext = [ " << std::setprecision(2) << std::fixed << p_extent[0] << " " << p_extent[1] << " " << p_extent[2] << " " << p_extent[3] << " " << p_extent[4] << " " << p_extent[5] << " ]" << std::endl;

    /* Print children */
    for (int i = 0; i < 8; i++)
    {
        p_children[i].print();
    }
}

void SearchNode::insert(xyzwd32 & point)
{
    // If the current node is not a leaf (but a branch), then it's private variables will not be changed on further insertion, and thus it does not need a mutex lock
    if (p_is_leaf)
    {
        QMutexLocker lock(p_mutex);
        p_insert(point);
    }
    else
    {
        p_insert(point);
    }
}

void SearchNode::p_insert(xyzwd32 & point)
{
    // If this is a branch node, then proceed to the next octree level
    if (!p_is_leaf)
    {
        int id = ntant(point, 2);
        if (id >= 0)
        {
//            point.w = (double) (id+1.0) * point.w;
            p_children[id].insert(point);
        }
    }
    // Else if this is the first data point to be inserted into a leaf node
    else if (p_is_empty && p_is_leaf)
    {
        int id = ntant(point, p_bins_per_side);
        if (id >= 0)
        {
            p_data_binned.resize(p_bins_per_side*p_bins_per_side*p_bins_per_side);
            p_data_binned[id] << point;
            p_is_empty = false;
        }

    }
    // Else, just insert the point
    else
    {
        int id = ntant(point, p_bins_per_side);
        if (id >= 0)
        {
            p_data_binned[id] << point;

            // If there are too many data point, split or rebin
            if ((num_points() >= p_max_points ))
            {
                double data_interdist_min;
                double data_interdist_max;
                double data_interdist_avg;
                interdistMetrics(data_interdist_min, data_interdist_max, data_interdist_avg);

                if (data_interdist_min > 1.0/(double)((1 << p_level) * p_bins_per_side))
                {
//                    qDebug() << "Rebinning because" << data_interdist_hint << ">" <<  1.0/(double)((1 << p_level) * p_bins_per_side);
                    rebin();
                }
                else
                {
                    split();
                    p_data_binned.clear();
                    p_is_leaf = false;
                    p_is_empty = true;
                }
            }
        }
    }
}

void SearchNode::interdistMetrics(double & data_interdist_min, double & data_interdist_max, double & data_interdist_avg)
{
    data_interdist_min = 1e9;
    data_interdist_max = 0;
    data_interdist_avg = 0;

    int counter = 0;

    for (int i = 0; i < p_data_binned.size(); i++)
    {
        for (int j = 0; j < p_data_binned[i].size(); j++)
        {
            if (p_data_binned[i][j].d > data_interdist_max) data_interdist_max = p_data_binned[i][j].d;
            if (p_data_binned[i][j].d < data_interdist_min) data_interdist_min = p_data_binned[i][j].d;
            data_interdist_avg += p_data_binned[i][j].d;
            counter++;
        }
    }

    data_interdist_avg /= (double) counter;
}

int SearchNode::num_points()
{
    int num = 0;

    for (int i = 0; i < p_data_binned.size(); i++)
    {
        num += p_data_binned[i].size();
    }

    return num;
}

//int SearchNode::ntant(double x, double y, double z, int n, QList<nodeinfo> & trace)
//{
//    // Find 3D ntant id for the next subdivision
//    int id_x = (x * (double) (1 << p_level) - (double) p_id_x) * (double) n;
//    int id_y = (y * (double) (1 << p_level) - (double) p_id_y) * (double) n;
//    int id_z = (z * (double) (1 << p_level) - (double) p_id_z) * (double) n;

//    nodeinfo ni;
//    ni.level = p_level;
//    ni.id_x = p_id_x;
//    ni.id_y = p_id_y;
//    ni.id_z = p_id_z;
//    ni.side = (1 << p_level);
//    ni.id_nxt_x = (x * (double) (1 << p_level) - (double) p_id_x) * (double) n;
//    ni.id_nxt_y = (y * (double) (1 << p_level) - (double) p_id_y) * (double) n;
//    ni.id_nxt_z = (z * (double) (1 << p_level) - (double) p_id_z) * (double) n;
//    ni.n = n;

//    trace << ni;

//    // (0.375 * 1 - 0)*2 = 0.75
//    // (0.375 * 2 - 0*2+0.75=0)*2 = 1.5
//    // (0.375 * 4 - 0*2+1=1)*2 = 1.0
//    // (0.375 * 8 - 1*2+1=3)*2 = 0.0
//    // (0.375 * 16 - 3*2+0)*2 = 0.0

//    // Example n = 2 -> octant:
//    // 0 <= relative pos < 1 : Maps to 0
//    // 1 <= relative pos < 2 : Maps to 1
//    // Values should not map to 2 or greater

//    if ((id_x >= n) || (id_x < 0) || (id_y >= n) || (id_y < 0) || (id_z >= n) || (id_z < 0))
//    {
//        // This should really not happen for a point that with position values between 0 and 1
//        qDebug() << "Point does not fit in level:" << p_level << ", " << x << y << z;


//        return -1;
//    }

//    // Return 1D ntant id
//    int id = id_x + n * id_y + n*n * id_z;

//    return id;
//}

int SearchNode::ntant(double x, double y, double z, int n)
{
    // Find 3D ntant id
    int id_ntant_x = (x * (double) (1 << p_level) - (double) p_id_node_x) * (double) n;
    int id_ntant_y = (y * (double) (1 << p_level) - (double) p_id_node_y) * (double) n;
    int id_ntant_z = (z * (double) (1 << p_level) - (double) p_id_node_z) * (double) n;

    // Example n = 2 -> octant:
    // 0 <= relative pos < 1 : Maps to 0
    // 1 <= relative pos < 2 : Maps to 1
    // Values should not map to 2 or greater

    if ((id_ntant_x >= n) || (id_ntant_x < 0) || (id_ntant_y >= n) || (id_ntant_y < 0) || (id_ntant_z >= n) || (id_ntant_z < 0))
    {
        // This should really not happen for a point that with position values between 0 and 1
        qDebug() << "Point does not fit in level:" << p_level << ", xyz =" << x << y << z << "ntant id =" << id_ntant_x << id_ntant_y << id_ntant_z << "node id =" << p_id_node_x << p_id_node_y << p_id_node_z;

        return -1;
    }

    // Return 1D ntant id
    int id = id_ntant_x + id_ntant_y * n + id_ntant_z * n * n;

    return id;
}


int SearchNode::ntant(xyzwd32 & point, int n)
{
    return ntant(point.x, point.y, point.z, n);
}

void SearchNode::setId(int id_x, int id_y, int id_z)
{
    p_id_node_x = id_x;
    p_id_node_y = id_y;
    p_id_node_z = id_z;
}

void SearchNode::split()
{
    /* The octants are assumed to be cubic. First create eight new
     * children. Then insert the nodes in the children according to
     * octant */

    p_children.resize(8);

    // For each child
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                int child_id_x = p_id_node_x*2+i;
                int child_id_y = p_id_node_y*2+j;
                int child_id_z = p_id_node_z*2+k;

                int linear_id = i + 2*j + 4*k;

                p_children[linear_id].setParent(this);
                p_children[linear_id].setMaxPoints(p_max_points);
                p_children[linear_id].setBinsPerSide(p_bins_per_side);
                p_children[linear_id].setId(child_id_x, child_id_y, child_id_z);
            }
        }
    }

    // For each point
    for (int i = 0; i < p_data_binned.size(); i++)
    {
        for (int j = 0; j < p_data_binned[i].size(); j++)
        {
            int id = ntant(p_data_binned[i][j], 2);

            if (id > 0)
            {
                p_children[id].insert(p_data_binned[i][j]);
            }
        }
    }
}
