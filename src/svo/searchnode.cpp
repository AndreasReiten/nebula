#include "searchnode.h"

SearchNode::SearchNode()
{
    p_is_root = false;
    p_is_leaf = true;
    p_is_empty = true;
    p_id_x = 0;
    p_id_y = 0;
    p_id_z = 0;
//    p_min_data_interdistance = 0;

    p_mutex = new QMutex;
}

SearchNode::~SearchNode()
{

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

//    qDebug() << "The values are";

//    for (int i = 0; i < p_grid.size(); i++)
//    {
//        qDebug() << i << p_grid[i];
//    }


    for (int i = 0; i < p_bins_per_side; i+=2)
    {
        for (int j = 0; j < p_bins_per_side; j+=2)
        {
            for (int k = 0; k < p_bins_per_side; k+=2)
            {
//                qDebug() << "Index" << i << j << k;
//                qDebug() << (i+0) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side << p_grid[(i+0) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side];
//                qDebug() << (i+1) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side << p_grid[(i+1) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side];
//                qDebug() << (i+0) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side << p_grid[(i+0) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side];
//                qDebug() << (i+0) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side << p_grid[(i+0) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side];
//                qDebug() << (i+1) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side << p_grid[(i+1) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side];
//                qDebug() << (i+0) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side << p_grid[(i+0) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side];
//                qDebug() << (i+1) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side << p_grid[(i+1) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side];
//                qDebug() << (i+1) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side << p_grid[(i+1) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side];


                // Note: Method does not discriminate zero

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

//                if (std::fabs(w - avg) < sigma*0.1)

                if (sigma < avg*0.1 + 5.0) n_expendable_bins++;
//                else
//                {
//                    qDebug() << "Wow, a node that made it through!";
//                    qDebug() << "Node lvl" << p_level << "avg" << avg << "sigma" << sigma << "< ?" << avg*0.1 + 5.0;

//                    qDebug() << "The values are";

//                    for (int i = 0; i < p_grid.size(); i++)
//                    {
//                        qDebug() << i << p_grid[i];
//                    }
//                }

//                qDebug() << "avg" << avg << "sigma" << sigma << "< ?" << avg*0.1 + 5.0;
            }
        }
    }

    return n_expendable_bins;
}

void SearchNode::reorganize()
{
    // If empty, with no relevant data
    if (p_is_empty) return;

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

    p_data_binned.clear();
}

double SearchNode::gridValueAt(double x, double y, double z)
{
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
    else
    {
        int id = ntant(x,y,z, 2);
        if (id < 0)
        {
            return 0.0;
        }
        else
        {
            return p_children[id].gridValueAt(x, y, z);
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
        xyzw32 tmp = {0.0,0.0,0.0,0.0};

        for (int j = 0; j < p_data_binned[i].size(); j++)
        {
            tmp.x += p_data_binned[i][j].x;
            tmp.y += p_data_binned[i][j].y;
            tmp.z += p_data_binned[i][j].z;
            tmp.w += p_data_binned[i][j].w;
        }

        tmp.x /= (float) p_data_binned[i].size();
        tmp.y /= (float) p_data_binned[i].size();
        tmp.z /= (float) p_data_binned[i].size();
        tmp.w /= (float) p_data_binned[i].size();

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

QVector<QVector<xyzw32> > &SearchNode::bins()
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


void SearchNode::brickcount(int &  count)
{
    if (!p_is_empty) count++;

    for (int i = 0; i < p_children.size(); i++)
    {
        p_children[i].brickcount(count);
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

void SearchNode::voxelize(QVector<unsigned int> & index , QVector<unsigned int> & brick, QVector<int> & level_offsets, QVector<int> & level_progress,  int & pool_dim_x, int & pool_dim_y, int & pool_dim_z, int & num_bricks)
{
    // Add this node, prepare

    if (!p_is_empty)
    {
        p_pool_z = num_bricks / (pool_dim_x * pool_dim_y);
        p_pool_y = (num_bricks - p_pool_z * (pool_dim_x * pool_dim_y)) / pool_dim_x;
        p_pool_x = num_bricks - p_pool_z * (pool_dim_x * pool_dim_y) - p_pool_y * pool_dim_x;

        num_bricks++;
    }

    unsigned int leaf_flag = p_is_leaf;
    unsigned int nonempty_flag = p_is_empty;
    unsigned int child_index = 0;

    if (!p_is_leaf)
    {
        child_index = level_offsets[p_level+1]+level_progress[p_level];
    }

    index[level_offsets[p_level]+level_progress[p_level]] = (leaf_flag << 31) | (nonempty_flag << 30) | child_index;
    brick[level_offsets[p_level]+level_progress[p_level]] = (p_pool_x << 20) | (p_pool_y << 10) | (p_pool_z << 0);

    level_progress[p_level]++;

    // Add children
    for (int i = 0; i < p_children.size(); i++)
    {
        p_children[i].voxelize(index , brick, level_offsets, level_progress,  pool_dim_x, pool_dim_y, pool_dim_z, num_bricks);
    }
}

void SearchNode::brickToPool(QVector<float> & pool, int dim_x, int dim_y, int dim_z)
{
    // Return a brick corresponding to the given extent based on the octree data
    int side = 1 + p_bins_per_side + 1;

    for (int i = 0; i < side; i++)
    {
        for (int j = 0; j < side; j++)
        {
            for (int k = 0; k < side; k++)
            {
                // The linear id the current value should have in the brick pool
                int id_pool = (p_pool_x * side + i) + (p_pool_y * side + j) * dim_x + (p_pool_z * side + k) * dim_x * dim_y;

                // If the id is associated with the face of the brick
                if ((i < 1) || (i >= side - 1) || (j < 1) || (j >= side - 1) || (k < 1) || (k >= side - 1))
                {
                    // Do a lookup of the value
                    double x = (double) (p_id_x + (double)((i-1) + 0.5 )/(double) (p_bins_per_side)) / (double) (1 << p_level);
                    double y = (double) (p_id_y + (double)((j-1) + 0.5 )/(double) (p_bins_per_side)) / (double) (1 << p_level);
                    double z = (double) (p_id_z + (double)((k-1) + 0.5 )/(double) (p_bins_per_side)) / (double) (1 << p_level);

                    if ((x < 0) || (x >= 1) || (y < 0) || (y >= 1) || (z < 0) || (z >= 1))
                    {
                        pool[id_pool] = 0;
                    }
                    else
                    {
                        pool[id_pool] = gridValueAt(x, y, z);
                    }
                }
                else
                {
                    // Find the data in the grid
                    int id_grid = (i-1) + (j-1) * p_bins_per_side + (k-1) * p_bins_per_side * p_bins_per_side;
                    pool[id_pool] = p_grid[id_grid];
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

void SearchNode::insert(xyzw32 & point, double data_interdist_hint)
{
    // If the current node is not msd (max subdivision), then it's private variables will not be changed, and thus it does not need a mutex lock
    if (p_is_leaf)
    {
        QMutexLocker lock(p_mutex);
        p_insert(point, data_interdist_hint);
    }
    else
    {
        p_insert(point, data_interdist_hint);
    }
}

void SearchNode::p_insert(xyzw32 & point, double data_interdist_hint)
{
    // If this is a branch node, then proceed to the next octree level
    if (!p_is_leaf)
    {
        int id = ntant(point, 2);
        if (id >= 0)
        {
            p_children[id].insert(point, data_interdist_hint);
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
                if (data_interdist_hint > 1.0/(double)((1 << p_level) * p_bins_per_side))
                {
//                    qDebug() << "Rebinning because" << data_interdist_hint << ">" <<  1.0/(double)((1 << p_level) * p_bins_per_side);
                    rebin();
                }
                else
                {
                    split(data_interdist_hint);
                    p_data_binned.clear();
                    p_is_leaf = false;
                    p_is_empty = true;
                }
            }
        }
    }
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

int SearchNode::ntant(double x, double y, double z, int n)
{
    // Find 3D ntant id
    int id_x = (x * (double) (1 << p_level) - (double) p_id_x) * n;
    int id_y = (y * (double) (1 << p_level) - (double) p_id_y) * n;
    int id_z = (z * (double) (1 << p_level) - (double) p_id_z) * n;

    // Example n = 2 -> octant:
    // 0 <= relative pos < 1 : Maps to 0
    // 1 <= relative pos < 2 : Maps to 1
    // Values should not map to 2 or greater

    if ((id_x >= n) || (id_x < 0) || (id_y >= n) || (id_y < 0) || (id_z >= n) || (id_z < 0))
    {
        // This should really not happen for a point that with position values between 0 and 1
        qDebug() << "Point does not fit in level:" << p_level << ", " << x << y << z;
        return -1;
    }

    // Return 1D ntant id
    int id = id_x + n * id_y + n*n * id_z;

    return id;
}

int SearchNode::ntant(xyzw32 & point, int n)
{
    return ntant(point.x, point.y, point.z, n);
}

void SearchNode::setId(int id_x, int id_y, int id_z)
{
    p_id_x = id_x;
    p_id_y = id_y;
    p_id_z = id_z;
}

void SearchNode::split(double data_interdist_hint)
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
                int child_id_x = p_id_x*2+i;
                int child_id_y = p_id_y*2+j;
                int child_id_z = p_id_z*2+k;

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
                p_children[id].insert(p_data_binned[i][j], data_interdist_hint);
            }
        }
    }
}
