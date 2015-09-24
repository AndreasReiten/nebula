#include "searchnode.h"

#include <iostream>
#include <iomanip>
#include <cmath>

#include <QString>
#include <QDebug>
#include <QElapsedTimer>

static const unsigned int MAX_LEVELS = 16;

SearchNode::SearchNode()
{
    p_is_msd = true;
    p_is_empty = true;
    p_extent.reserve(1, 6);
    p_mutex = new QMutex;
}

SearchNode::SearchNode(SearchNode * parent, double * extent)
{
    p_is_msd = true;
    p_is_empty = true;
    p_parent = parent;
    p_extent.reserve(1, 6);

    for (int i = 0; i < 6; i++)
    {
        p_extent[i] = extent[i];
    }

    if (p_parent == NULL)
    {
        p_tree_level = 0;
    }
    else
    {
        p_tree_level = parent->level() + 1;
    }

    p_mutex = new QMutex;
}

SearchNode::~SearchNode()
{
    clear();
}

void SearchNode::clear()
{
//    p_linked_points.clear();
    p_children.clear();
    p_data_points.clear();
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

void SearchNode::setMinDataInterdistance(double value)
{
    p_min_data_interdistance = value;
}

void SearchNode::setExtent(Matrix<double> extent)
{
    p_extent = extent;
}

void SearchNode::setParent(SearchNode * parent)
{
    p_parent = parent;

    if (p_parent == NULL)
    {
        p_tree_level = 0;
    }
    else
    {
        p_tree_level = parent->level() + 1;
    }
}

unsigned int SearchNode::level()
{
    return p_tree_level;
}


void SearchNode::print()
{
    /* Print self */
    for (unsigned int i = 0; i < p_tree_level; i++)
    {
        std::cout << " ";
    }

//    std::cout << "L" << p_tree_level << "-> n bins: " << p_linked_points.size() << " c: " << 8 << " empt: " << p_is_empty << " msd: = " << p_is_msd << " Ext = [ " << std::setprecision(2) << std::fixed << p_extent[0] << " " << p_extent[1] << " " << p_extent[2] << " " << p_extent[3] << " " << p_extent[4] << " " << p_extent[5] << " ]" << std::endl;

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
    // If this is not the maximum subdivision, then proceed to the next octree level
    if (!p_is_msd)
    {
        bool isOutofBounds = false;
        unsigned int id = octant(point, &isOutofBounds);

        if (!isOutofBounds)
        {
            p_children[id].insert(point);
        }
    }
    // Else if this is the first data point to be inserted
    else if (p_data_points.isEmpty() && p_is_msd)
    {

//        subnode node;
//        node.data_points << point;
//        node.index =
//        p_linked_points << point;

        p_data_points.reserve(p_max_points);
        p_data_points << point;
        p_is_empty = false;
    }
    // Else if there will be too many points and a split is required, and the split wont result in too deep an octree level
    else if ((p_data_points.size() >= p_max_points - 1) && (p_tree_level < MAX_LEVELS - 1))
    {
        p_data_points << point;
        split();
        p_data_points.clear();
        p_is_msd = false;
    }
    // Else if there will be too many points
//    else if ((p_data_points.size() > MAX_POINTS - 1))
//    {
//        p_data_points << point;
//    }
    // Else, just insert the point
    else
    {
        p_data_points << point;
    }

}

void SearchNode::weighSamples(xyzw32 & sample, Matrix<double> & sample_extent, float * sum_w, float * sum_wu, float p, float search_radius)
{
    if ((p_is_msd) && (!p_is_empty))
    {
        float d, w;

        for (unsigned int i = 0; i < p_data_points.size(); i++)
        {
            d = distance(p_data_points[i], sample);

            if (d <= search_radius)
            {
                w = 1.0 / d;
                *sum_w += w;
                *sum_wu += w * (p_data_points[i].w);
            }
        }
    }
    else //if (p_n_children > 0)
    {
        for (int i = 0; i < 8; i++)
        {
            if (p_children[i].isIntersected(sample_extent))
            {
                p_children[i].weighSamples(sample, sample_extent, sum_w, sum_wu, p, search_radius);
            }
        }
    }
}

bool SearchNode::isIntersected(Matrix<double> & sample_extent)
{
    // Box box intersection by checking for each dimension if there is an overlap. If there is an overlap for all three dimensions, the node intersects the sampling extent

    double tmp[6];

    for (int i = 0; i < 3; i++)
    {
        tmp[i * 2] = std::max(sample_extent[i * 2], p_extent[i * 2]);
        tmp[i * 2 + 1] = std::min(sample_extent[i * 2 + 1], p_extent[i * 2 + 1]);

        if (tmp[i * 2 + 0] >= tmp[i * 2 + 1])
        {
            return false;
        }
    }

    return true;
}

float SearchNode::getIDW(xyzw32 & sample, float p, float search_radius)
{
    float sum_w = 0;
    float sum_wu = 0;

    Matrix<double> sample_extent(1, 6);
    sample_extent[0] = sample.x - search_radius;
    sample_extent[1] = sample.x + search_radius;
    sample_extent[2] = sample.y - search_radius;
    sample_extent[3] = sample.y + search_radius;
    sample_extent[4] = sample.z - search_radius;
    sample_extent[5] = sample.z + search_radius;

    weighSamples(sample, sample_extent, &sum_w, &sum_wu, p, search_radius);

    if (sum_w > 0.0)
    {
        return sum_wu / sum_w;
    }
    else
    {
        return 0;
    }
}

bool SearchNode::intersectedItems(Matrix<double> & effective_extent, size_t * accumulated_points, size_t max_points, QList<xyzw32> * point_data)
{
    if ((p_is_msd) && (!p_is_empty))
    {
        for (unsigned int i = 0; i < p_data_points.size(); i++)
        {
            if (
                ((p_data_points[i].x >= effective_extent.at(0)) && (p_data_points[i].x <= effective_extent.at(1))) &&
                ((p_data_points[i].y >= effective_extent.at(2)) && (p_data_points[i].y <= effective_extent.at(3))) &&
                ((p_data_points[i].z >= effective_extent.at(4)) && (p_data_points[i].z <= effective_extent.at(5))))
            {
                *point_data << p_data_points[i];
                (*accumulated_points)++;

                if (max_points <= *accumulated_points)
                {
                    return true;
                }
            }
        }
    }
    else //if ((p_n_children > 0))
    {
        for (int i = 0; i < p_children.size(); i++)
        {
            if (p_children[i].isIntersected(effective_extent))
            {
                if (p_children[i].intersectedItems(effective_extent, accumulated_points, max_points, point_data))
                {
                    return true;
                }
            }
        }
    }

    return false;
}


bool SearchNode::getData(
    size_t max_points,
    double * brick_extent,
    QList<xyzw32> point_data,
    size_t * accumulated_points,
    float search_radius)
{
    // First check if max bytes is reached
    if (max_points <= *accumulated_points)
    {
        return true;
    }

    Matrix<double> effective_extent(1, 6);
    effective_extent[0] = brick_extent[0] - search_radius;
    effective_extent[1] = brick_extent[1] + search_radius;
    effective_extent[2] = brick_extent[2] - search_radius;
    effective_extent[3] = brick_extent[3] + search_radius;
    effective_extent[4] = brick_extent[4] - search_radius;
    effective_extent[5] = brick_extent[5] + search_radius;

    return intersectedItems(effective_extent, accumulated_points, max_points, &point_data);
}

float SearchNode::distance(xyzw32 & a, xyzw32 & b)
{
    return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z));
}

unsigned int SearchNode::octant(xyzw32 & point, bool * isOutofBounds)
{
    // Find 3D octant id
    int oct_x =  (point.x - p_extent[0]) * 2.0 / (p_extent[1] - p_extent[0]);
    int oct_y =  (point.y - p_extent[2]) * 2.0 / (p_extent[3] - p_extent[2]);
    int oct_z =  (point.z - p_extent[4]) * 2.0 / (p_extent[5] - p_extent[4]);

    // Clamp
    if ((oct_x >= 2) || (oct_x < 0))
    {
        *isOutofBounds = true;
    }

    if ((oct_y >= 2) || (oct_y < 0))
    {
        *isOutofBounds = true;
    }

    if ((oct_z >= 2) || (oct_z < 0))
    {
        *isOutofBounds = true;
    }

    // Find 1D octant id
    unsigned int id = oct_x + 2 * oct_y + 4 * oct_z;

    return id;
}

void SearchNode::rebin()
{
    // Regenerate data points on a grid. For each grid cell, the xyz position of the data point is the average position of the sample sub-set (with possible weighing).
    // If there are no points, then no data point is generated. In some cases, the points might not be rebinned at all.

}

void SearchNode::split()
{
    /* The octants are assumed to be cubic. First create eight new
     * children. Then insert the nodes in the children according to
     * octant */

    p_children.resize(8);

    // For each child
    for (int i = 0; i < 8; i++)
    {
        int id_x = (i % 4) % 2;
        int id_y = (i % 4) / 2;
        int id_z = i / 4;

        double half_side = (p_extent[1] - p_extent[0]) * 0.5;

        Matrix<double> child_extent(1,6);
        child_extent[0] = p_extent[0] + half_side * id_x;
        child_extent[1] = p_extent[1] - half_side * (1 - id_x);
        child_extent[2] = p_extent[2] + half_side * id_y;
        child_extent[3] = p_extent[3] - half_side * (1 - id_y);
        child_extent[4] = p_extent[4] + half_side * id_z;
        child_extent[5] = p_extent[5] - half_side * (1 - id_z);

        p_children[i].setParent(this);
        p_children[i].setExtent(child_extent);
        p_children[i].setMaxPoints(p_max_points);
        p_children[i].setBinsPerSide(p_bins_per_side);
        p_children[i].setMinDataInterdistance(p_min_data_interdistance);
    }

    // For each point
    for (size_t i = 0; i < p_data_points.size(); i++)
    {
        bool isOutofBounds = false;

        unsigned int id = octant(p_data_points[i], &isOutofBounds);

        if (!isOutofBounds)
        {
            p_children[id].insert(p_data_points[i]);
        }
    }
}

//double * SearchNode::getExtent()
//{
//    return p_extent.data();
//}
