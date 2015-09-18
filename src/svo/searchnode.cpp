#include "searchnode.h"

#include <iostream>
#include <iomanip>
#include <cmath>

#include <QString>
#include <QDebug>
#include <QElapsedTimer>

static const unsigned int MAX_POINTS = 32;
static const unsigned int MAX_LEVELS = 16;
static const unsigned int CL_MAX_ITEMS = 1024 * 2 * 2 * 2 * 2 * 2 * 2 * 2;
static const unsigned int CL_LEVEL = 2;

SearchNode::SearchNode()
{
    p_is_msd = true;
    p_is_empty = true;
//    n_points = 0;
    p_n_children = 0;
    p_extent.reserve(1, 6);
}

SearchNode::SearchNode(SearchNode * parent, double * extent)
{
    p_is_msd = true;
    p_is_empty = true;
    p_parent = parent;
//    n_points = 0;
    p_n_children = 0;
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
        p_tree_level = parent->getLevel() + 1;
    }
}

SearchNode::~SearchNode()
{
    clear();
}

void SearchNode::clear()
{
    if (p_n_children > 0) // This clause really needed?
    {
        for (unsigned int i = 0; i < p_n_children; i++)
        {
            p_children[i]->clear();
        }
    }

    p_children.clear();
    p_data_points.clear();
    p_is_msd = true;
    p_is_empty = true;
    p_n_children = 0;
}

//void SearchNode::clearPoints()
//{
//    p_data_points.clear();
//}

void SearchNode::setExtent(Matrix<double> extent)
{
    p_extent = extent;
}

void SearchNode::setParent(SearchNode * parent)
{
//    parent->print();
    p_parent = parent;

    if (p_parent == NULL)
    {
        p_tree_level = 0;
    }
    else
    {
        p_tree_level = parent->getLevel() + 1;
    }
}

unsigned int SearchNode::getLevel()
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

    std::cout << "L" << p_tree_level << "-> n: " << p_data_points.size() << " c: " << p_n_children << " empt: " << p_is_empty << " msd: = " << p_is_msd << " Ext = [ " << std::setprecision(2) << std::fixed << p_extent[0] << " " << p_extent[1] << " " << p_extent[2] << " " << p_extent[3] << " " << p_extent[4] << " " << p_extent[5] << " ]" << std::endl;

    /* Print children */
    if (p_n_children > 0)
    {
        for (int i = 0; i < 8; i++)
        {
            p_children[i]->print();
        }
    }
}

// Functions that rely on recursive action such as this one should not be declared as a member function. Rather make a function external to the node. (?)
void SearchNode::insert(xyzw32 & point)
{
    if (!p_is_msd)
    {
        bool isOutofBounds = false;

        unsigned int id = getOctant(point, &isOutofBounds);

        if (!isOutofBounds)
        {
            p_children[id]->insert(point);
        }
    }
    else if (p_data_points.isEmpty() && p_is_msd)
    {
        p_data_points << point;
//        n_points++;
        p_is_empty = false;
    }
    else if ((p_data_points.size() >= MAX_POINTS - 1) && (p_tree_level < MAX_LEVELS - 1))
    {
        p_data_points << point;
//        n_points++;
        split();
        p_data_points.clear();
        p_is_msd = false;
    }
    else if ((p_data_points.size() > MAX_POINTS - 1))
    {
        p_data_points << point;
//        n_points++;
    }
    else
    {
        p_data_points << point;
//        n_points++;
    }

}

void SearchNode::weighSamples(xyzw32 & sample, double * sample_extent, float * sum_w, float * sum_wu, float p, float search_radius)
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
    else if (p_n_children > 0)
    {
        for (unsigned int i = 0; i < 8; i++)
        {
            if (p_children[i]->isIntersected(sample_extent))
            {
                p_children[i]->weighSamples(sample, sample_extent, sum_w, sum_wu, p, search_radius);
            }
        }
    }
}

bool SearchNode::isIntersected(double * sample_extent)
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

    weighSamples(sample, sample_extent.data(), &sum_w, &sum_wu, p, search_radius);

    if (sum_w > 0.0)
    {
        return sum_wu / sum_w;
    }
    else
    {
        return 0;
    }
}

bool SearchNode::getIntersectedItems(Matrix<double> * effective_extent, size_t * accumulated_points, size_t max_points, QList<xyzw32> * point_data)
{
    if ((p_is_msd) && (!p_is_empty))
    {
        for (unsigned int i = 0; i < p_data_points.size(); i++)
        {
            if (
                ((p_data_points[i].x >= effective_extent->at(0)) && (p_data_points[i].x <= effective_extent->at(1))) &&
                ((p_data_points[i].y >= effective_extent->at(2)) && (p_data_points[i].y <= effective_extent->at(3))) &&
                ((p_data_points[i].z >= effective_extent->at(4)) && (p_data_points[i].z <= effective_extent->at(5))))
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
    else if ((p_n_children > 0))
    {
        for (unsigned int i = 0; i < 8; i++)
        {
            if (p_children[i]->isIntersected(effective_extent->data()))
            {
                if (p_children[i]->getIntersectedItems(effective_extent, accumulated_points, max_points, point_data))
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

    return getIntersectedItems(&effective_extent, accumulated_points, max_points, &point_data);
}

float SearchNode::distance(xyzw32 & a, xyzw32 & b)
{
    return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z));
}

unsigned int SearchNode::getOctant(xyzw32 & point, bool * isOutofBounds)
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

void SearchNode::split()
{
    /* The octants are assumed to be cubic. First create eight new
     * children. Then insert the nodes in the children according to
     * octant */

    p_n_children = 8;
    p_children.resize(8);

    // For each child
    for (int i = 0; i < 8; i++)
    {
        int id_x = (i % 4) % 2;
        int id_y = (i % 4) / 2;
        int id_z = i / 4;

        double half_side = (p_extent[1] - p_extent[0]) * 0.5;

        double child_extent[6];
        child_extent[0] = p_extent[0] + half_side * id_x;
        child_extent[1] = p_extent[1] - half_side * (1 - id_x);
        child_extent[2] = p_extent[2] + half_side * id_y;
        child_extent[3] = p_extent[3] - half_side * (1 - id_y);
        child_extent[4] = p_extent[4] + half_side * id_z;
        child_extent[5] = p_extent[5] - half_side * (1 - id_z);

        p_children[i] = SearchNode(this, child_extent);
    }

    // For each point
    for (size_t i = 0; i < p_data_points.size(); i++)
    {
        bool isOutofBounds = false;

        unsigned int id = getOctant(p_data_points[i], &isOutofBounds);

        if (!isOutofBounds)
        {
            p_children[id]->insert(p_data_points[i]);
        }
    }
}

double * SearchNode::getExtent()
{
    return p_extent.data();
}
