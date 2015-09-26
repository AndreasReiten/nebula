#include "searchnode.h"

//#include <iostream>
//#include <iomanip>
//#include <cmath>

//#include <QString>
//#include <QDebug>
//#include <QElapsedTimer>

//static const int MAX_LEVELS = 16;

SearchNode::SearchNode()
{
    p_is_root = false;
    p_is_msd = true;
    p_is_empty = true;
    p_id_x = 0;
    p_id_y = 0;
    p_id_z = 0;

//    p_extent.reserve(1, 6);
    p_mutex = new QMutex;
}

//SearchNode::SearchNode(SearchNode * parent, float * extent)
//{
//    p_is_root = false;
//    p_is_msd = true;
//    p_is_empty = true;
//    p_parent = parent;
//    p_extent.reserve(1, 6);

//    for (int i = 0; i < 6; i++)
//    {
//        p_extent[i] = extent[i];
//    }

//    p_level = parent->level() + 1;

//    p_mutex = new QMutex;
//}

SearchNode::~SearchNode()
{
//    clear();
}

void SearchNode::rebin(bool relaxed)
{
    // Regenerate data points within bins. For each grid cell, the xyz position of the data point is the average
    // position of the sample sub-set (with possible weighing).
    // If there are no points, then no data point is generated. In some cases, the points might not be rebinned at all.

    if (relaxed)
    {

    }
    else
    {
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
}

void SearchNode::estimate()
{
    // Non-recursively generate a point cloud on an irregular grid based on the child nodes
    if(p_is_msd || p_is_empty) return;

    // Get the relevant data points, and place them in bins
    p_data_binned.resize(p_bins_per_side*p_bins_per_side*p_bins_per_side);
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < p_bins_per_side*p_bins_per_side*p_bins_per_side; j++)
        {
            for (int k = 0; k < p_children[i].bins()[j].size(); k++)
            {
                int id = ntant(p_children[i].bins()[j][k], p_bins_per_side);
                p_data_binned[id] << p_children[i].bins()[j][k];
            }
        }
    }

    rebin(0);
}

QVector<QList<xyzw32>> & SearchNode::bins()
{
    return p_data_binned;
}

bool SearchNode::isEmpty()
{
    return p_is_empty;
}

bool SearchNode::isRoot()
{
    return p_is_root;
}

bool SearchNode::isMsd()
{
    return p_is_msd;
}

QVector<SearchNode> & SearchNode::children()
{
    return p_children;
}


void SearchNode::hierarchy(QVector<QList<SearchNode *>> & nodes)
{
    // Add self to hierarchy
    if (nodes.size() <= p_level)
    {
        nodes.resize(p_level+1);
    }

    nodes[p_level] << this;

    // Return if there are no children
    if(p_is_msd || p_is_empty) return;

    // Call for children
    for (int i = 0; i < 8; i++)
    {
        p_children[i].hierarchy(nodes);
    }
}

void SearchNode::brick()
{
    // Return a brick corresponding to the given extent based on the octree data
}


void SearchNode::clear()
{
    p_children.clear();
    p_data_binned.clear();
    p_is_msd = true;
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

void SearchNode::setMinDataInterdistance(float value)
{
    p_min_data_interdistance = value;
}

//void SearchNode::setExtent(Matrix<float> extent)
//{
//    p_extent = extent;
//}

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

void SearchNode::insert(xyzw32 & point)
{
    // If the current node is not msd (max subdivision), then it's private variables will not be changed, and thus it does not need a mutex lock
    if (p_is_msd)
    {
        QMutexLocker lock(p_mutex);
        p_insert(point);
    }
    else
    {
        p_insert(point);
    }
}

void SearchNode::p_insert(xyzw32 & point)
{
    // If this is a branch node, then proceed to the next octree level
    if (!p_is_msd)
    {
        int id = ntant(point, 2);
        if (id >= 0)
        {
            p_children[id].insert(point);
        }
    }
    // Else if this is the first data point to be inserted into a leaf node
    else if (p_is_empty && p_is_msd)
    {
        int id = ntant(point, p_bins_per_side);
        if (id >= 0)
        {
            p_data_binned.resize(p_bins_per_side*p_bins_per_side*p_bins_per_side);
            p_data_binned[id] << point;
            p_is_empty = false;
        }

    }
    // Else if there will be too many points and a split is required
    else if ((num_points() >= p_max_points - 1))
    {
        int id = ntant(point, p_bins_per_side);
        if (id >= 0)
        {
            p_data_binned[id] << point;
            split();
            p_data_binned.clear();
            p_is_msd = false;
        }
    }
    // Else, just insert the point
    else
    {
        int id = ntant(point, p_bins_per_side);
        if (id >= 0)
        {
            p_data_binned[id] << point;
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

//void SearchNode::weighSamples(xyzw32 & sample, Matrix<float> & sample_extent, float * sum_w, float * sum_wu, float p, float search_radius)
//{
//    if ((p_is_msd) && (!p_is_empty))
//    {
//        float d, w;

//        for (int i = 0; i < num_points(); i++)
//        {
//            d = distance(p_data_points[i], sample);

//            if (d <= search_radius)
//            {
//                w = 1.0 / d;
//                *sum_w += w;
//                *sum_wu += w * (p_data_points[i].w);
//            }
//        }
//    }
//    else //if (p_n_children > 0)
//    {
//        for (int i = 0; i < 8; i++)
//        {
//            if (p_children[i].isIntersected(sample_extent))
//            {
//                p_children[i].weighSamples(sample, sample_extent, sum_w, sum_wu, p, search_radius);
//            }
//        }
//    }
//}

//bool SearchNode::isIntersected(Matrix<float> & sample_extent)
//{
    // Box box intersection by checking for each dimension if there is an overlap. If there is an overlap for all three dimensions, the node intersects the sampling extent

//    float tmp[6];

//    for (int i = 0; i < 3; i++)
//    {
//        tmp[i * 2] = std::max(sample_extent[i * 2], p_extent[i * 2]);
//        tmp[i * 2 + 1] = std::min(sample_extent[i * 2 + 1], p_extent[i * 2 + 1]);

//        if (tmp[i * 2 + 0] >= tmp[i * 2 + 1])
//        {
//            return false;
//        }
//    }

//    return true;
//}

//float SearchNode::getIDW(xyzw32 & sample, float p, float search_radius)
//{
//    float sum_w = 0;
//    float sum_wu = 0;

//    Matrix<float> sample_extent(1, 6);
//    sample_extent[0] = sample.x - search_radius;
//    sample_extent[1] = sample.x + search_radius;
//    sample_extent[2] = sample.y - search_radius;
//    sample_extent[3] = sample.y + search_radius;
//    sample_extent[4] = sample.z - search_radius;
//    sample_extent[5] = sample.z + search_radius;

//    weighSamples(sample, sample_extent, &sum_w, &sum_wu, p, search_radius);

//    if (sum_w > 0.0)
//    {
//        return sum_wu / sum_w;
//    }
//    else
//    {
//        return 0;
//    }
//}

//bool SearchNode::intersectedItems(Matrix<float> & effective_extent, size_t * accumulated_points, size_t max_points, QList<xyzw32> * point_data)
//{
//    if ((p_is_msd) && (!p_is_empty))
//    {
//        for (int i = 0; i < p_data_points.size(); i++)
//        {
//            if (
//                ((p_data_points[i].x >= effective_extent.at(0)) && (p_data_points[i].x <= effective_extent.at(1))) &&
//                ((p_data_points[i].y >= effective_extent.at(2)) && (p_data_points[i].y <= effective_extent.at(3))) &&
//                ((p_data_points[i].z >= effective_extent.at(4)) && (p_data_points[i].z <= effective_extent.at(5))))
//            {
//                *point_data << p_data_points[i];
//                (*accumulated_points)++;

//                if (max_points <= *accumulated_points)
//                {
//                    return true;
//                }
//            }
//        }
//    }
//    else //if ((p_n_children > 0))
//    {
//        for (int i = 0; i < p_children.size(); i++)
//        {
//            if (p_children[i].isIntersected(effective_extent))
//            {
//                if (p_children[i].intersectedItems(effective_extent, accumulated_points, max_points, point_data))
//                {
//                    return true;
//                }
//            }
//        }
//    }

//    return false;
//}


//bool SearchNode::getData(
//    size_t max_points,
//    float * brick_extent,
//    QList<xyzw32> point_data,
//    size_t * accumulated_points,
//    float search_radius)
//{
//    // First check if max bytes is reached
//    if (max_points <= *accumulated_points)
//    {
//        return true;
//    }

//    Matrix<float> effective_extent(1, 6);
//    effective_extent[0] = brick_extent[0] - search_radius;
//    effective_extent[1] = brick_extent[1] + search_radius;
//    effective_extent[2] = brick_extent[2] - search_radius;
//    effective_extent[3] = brick_extent[3] + search_radius;
//    effective_extent[4] = brick_extent[4] - search_radius;
//    effective_extent[5] = brick_extent[5] + search_radius;

//    return intersectedItems(effective_extent, accumulated_points, max_points, &point_data);
//}

//float SearchNode::distance(xyzw32 & a, xyzw32 & b)
//{
//    return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z));
//}

int SearchNode::ntant(xyzw32 & point, int n)
{
    // Find 3D ntant id
    int id_x = (point.x * (float) (1 << p_level) - (float) p_id_x) * n;
    int id_y = (point.y * (float) (1 << p_level) - (float) p_id_y) * n;
    int id_z = (point.z * (float) (1 << p_level) - (float) p_id_z) * n;

    // Example n = 2 -> octant:
    // 0 <= relative pos < 1 : Maps to 0
    // 1 <= relative pos < 2 : Maps to 1
    // Values should not map to 2 or greater

    if ((id_x >= n) || (id_x < 0) || (id_y >= n) || (id_y < 0) || (id_z >= n) || (id_z < 0))
    {
//        qDebug() << "Point does not fit in level:" << p_level << ", " << point.x << point.y << point.z << point.w;
        return -1;
    }

    // Find 1D ntant id
    int id = id_x + n * id_y + n*n * id_z;

    return id;
}

void SearchNode::setId(int id_x, int id_y, int id_z)
{
    p_id_x = id_x;
    p_id_y = id_y;
    p_id_z = id_z;
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
                int child_id_x = p_id_x*2+i;
                int child_id_y = p_id_y*2+j;
                int child_id_z = p_id_z*2+k;

                int linear_id = i + 2*j + 4*k;

                p_children[linear_id].setParent(this);
                p_children[linear_id].setMaxPoints(p_max_points);
                p_children[linear_id].setBinsPerSide(p_bins_per_side);
                p_children[linear_id].setMinDataInterdistance(p_min_data_interdistance);
                p_children[linear_id].setId(child_id_x, child_id_y, child_id_z);
            }
        }
    }

    // For each point
    for (size_t i = 0; i < p_data_binned.size(); i++)
    {
        for (size_t j = 0; j < p_data_binned[i].size(); j++)
        {
            int id = ntant(p_data_binned[i][j], 2);

            if (id > 0)
            {
                p_children[id].insert(p_data_binned[i][j]);
            }
        }
    }
}
