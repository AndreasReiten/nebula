#include "searchnode.h"

#include <iostream>
#include <iomanip>
#include <cmath>

#include <QString>
#include <QDebug>
#include <QElapsedTimer>

static const unsigned int MAX_POINTS = 32;
static const unsigned int MAX_LEVELS = 16;
static const unsigned int CL_MAX_ITEMS = 1024*2*2*2*2*2*2*2;
static const unsigned int CL_LEVEL = 2;

SearchNode::SearchNode()
{
    this->isMsd = true;
    this->isEmpty = true;
    this->n_points = 0;
    this->n_children = 0;
    this->extent.reserve(1,6);
}

SearchNode::SearchNode(SearchNode * parent, double * extent)
{
    this->isMsd = true;
    this->isEmpty = true;
    this->parent = parent;
    this->n_points = 0;
    this->n_children = 0;
    this->extent.reserve(1,6);

    for (int i = 0; i < 6; i++)
    {
        this->extent[i] = extent[i];
    }

    if (this->parent == NULL)
    {
        this->level = 0;
    }
    else
    {
        this->level = parent->getLevel() + 1;
    }
}

SearchNode::~SearchNode()
{
    this->clearChildren();
    this->clearPoints();
}

void SearchNode::clearChildren()
{
    if (n_children > 0)
    {
        for (unsigned int i = 0; i < n_children; i++)
        {
            delete children[i];
        }
        delete[] children;
        n_children = 0;
    }
}

void SearchNode::clearPoints()
{
    if (n_points > 0)
    {
        delete[] this->points;
        n_points = 0;
    }
}

void SearchNode::setParent(SearchNode * parent)
{
    parent->print();
    this->parent = parent;
    if (this->parent == NULL)
    {
        this->level = 0;
    }
    else
    {
        this->level = parent->getLevel() + 1;
    }
}

unsigned int SearchNode::getLevel()
{
    return level;
}

void SearchNode::print()
{
    /* Print self */
    for (unsigned int i = 0; i < level; i++)
    {
        std::cout << " ";
    }
    std::cout << "L" << level << "-> n: " << n_points << " c: " << n_children << " empt: " << isEmpty << " msd: = " << isMsd << " Ext = [ " << std::setprecision(2) << std::fixed << extent[0] << " " << extent[1] << " " << extent[2] << " " << extent[3] << " " << extent[4] << " " << extent[5] << " ]"<< std::endl;

    /* Print children */
    if (n_children > 0)
    {
        for (int i = 0; i < 8; i++)
        {
            children[i]->print();
        }
    }
}

//void SearchNode::setOpenCLContext(OpenCLContext *context)
//{
//    this->context = context;
//}

// Functions that rely on recursive action such as this one should not be declared as a member function. Rather make a function external to the node. (?)
void SearchNode::insert(float * point)
{
    if (!isMsd)
    {
        bool isOutofBounds = false;

        unsigned int id = getOctant(point, &isOutofBounds);
        if (!isOutofBounds) this->children[id]->insert(point);
    }
    else if ((n_points == 0) && isMsd)
    {
        points = new float[MAX_POINTS*4];
        points[n_points*4+0]  = point[0];
        points[n_points*4+1]  = point[1];
        points[n_points*4+2]  = point[2];
        points[n_points*4+3]  = point[3];
        n_points++;
        isEmpty = false;
    }
    else if ((n_points >= MAX_POINTS - 1) && (level < MAX_LEVELS - 1))
    {
        points[n_points*4+0]  = point[0];
        points[n_points*4+1]  = point[1];
        points[n_points*4+2]  = point[2];
        points[n_points*4+3]  = point[3];
        n_points++;
        this->split();
        this->clearPoints();
        isMsd = false;
    }
    else if ((n_points > MAX_POINTS - 1))
    {
        // Expand allocated array by 1
        float * tmp = new float[n_points*4];

        for (unsigned int i = 0; i < n_points*4; i++)
        {
            tmp[i] = points[i];
        }

        delete[] points;
        points = new float[(n_points+1)*4];

        for (unsigned int i = 0; i < n_points*4; i++)
        {
            points[i] = tmp[i];
        }

        delete[] tmp;

        points[n_points*4+0]  = point[0];
        points[n_points*4+1]  = point[1];
        points[n_points*4+2]  = point[2];
        points[n_points*4+3]  = point[3];
        n_points++;
    }
    else
    {
        points[n_points*4+0]  = point[0];
        points[n_points*4+1]  = point[1];
        points[n_points*4+2]  = point[2];
        points[n_points*4+3]  = point[3];
        n_points++;
    }
    
}

void SearchNode::weighSamples(float * sample, double * sample_extent, float * sum_w, float * sum_wu, float p, float search_radius)
{
    if ((this->isMsd) && (!this->isEmpty))
    {
        float d, w;

        for (unsigned int i = 0; i < n_points; i++)
        {
            d = distance(points + i*4, sample);
            if (d <= search_radius)
            {
                w = 1.0 / d;
                *sum_w += w;
                *sum_wu += w * (points[i*4 + 3]);
            }

        }
    }
    else if (n_children > 0)
    {
        for (unsigned int i = 0; i < 8; i++)
        {
            if(children[i]->isIntersected(sample_extent))
            {
                children[i]->weighSamples(sample, sample_extent, sum_w, sum_wu, p, search_radius);
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
        tmp[i*2] = std::max(sample_extent[i*2], this->extent[i*2]);
        tmp[i*2+1] = std::min(sample_extent[i*2+1], this->extent[i*2+1]);
        if (tmp[i*2+0] >= tmp[i*2+1]) return false;
    }
    return true;
}

float SearchNode::getIDW(float * sample, float p, float search_radius)
{
    float sum_w = 0;
    float sum_wu = 0;

    Matrix<double> sample_extent(1,6);
    sample_extent[0] = sample[0] - search_radius;
    sample_extent[1] = sample[0] + search_radius;
    sample_extent[2] = sample[1] - search_radius;
    sample_extent[3] = sample[1] + search_radius;
    sample_extent[4] = sample[2] - search_radius;
    sample_extent[5] = sample[2] + search_radius;

    weighSamples(sample, sample_extent.data(), &sum_w, &sum_wu, p, search_radius);

    if (sum_w > 0.0) return sum_wu / sum_w;
    else return 0;
}

bool SearchNode::getIntersectedItems(Matrix<double> * effective_extent, size_t * accumulated_points, size_t max_points, float * point_data)
{
    if ((this->isMsd) && (!this->isEmpty))
    {
        for (unsigned int i = 0; i < n_points; i++)
        {
            if (
            ((points[i*4+0] >= effective_extent->at(0)) && (points[i*4+0] <= effective_extent->at(1))) &&
            ((points[i*4+1] >= effective_extent->at(2)) && (points[i*4+1] <= effective_extent->at(3))) &&
            ((points[i*4+2] >= effective_extent->at(4)) && (points[i*4+2] <= effective_extent->at(5))))
            {
                point_data[*accumulated_points*4+0] = points[i*4+0];
                point_data[*accumulated_points*4+1] = points[i*4+1];
                point_data[*accumulated_points*4+2] = points[i*4+2];
                point_data[*accumulated_points*4+3] = points[i*4+3];
                
                (*accumulated_points)++;

                if (max_points <= *accumulated_points)
                {
                    return true;
                }
            }
        }
    }
    else if ((n_children > 0))
    {
        for (unsigned int i = 0; i < 8; i++)
        {
            if(children[i]->isIntersected(effective_extent->data()))
            {
                if(children[i]->getIntersectedItems(effective_extent, accumulated_points, max_points, point_data)) return true;
            }
        }
    }

    return false;
}


bool SearchNode::getData(
        size_t max_points,
        double * brick_extent,
        float * point_data,
        size_t * accumulated_points,
        float search_radius)
{
    // First check if max bytes is reached
    if (max_points <= *accumulated_points)
    {
        return true;
    }

    Matrix<double> effective_extent(1,6);
    effective_extent[0] = brick_extent[0] - search_radius;
    effective_extent[1] = brick_extent[1] + search_radius;
    effective_extent[2] = brick_extent[2] - search_radius;
    effective_extent[3] = brick_extent[3] + search_radius;
    effective_extent[4] = brick_extent[4] - search_radius;
    effective_extent[5] = brick_extent[5] + search_radius;
    
    return getIntersectedItems(&effective_extent, accumulated_points, max_points, point_data);
}

float SearchNode::distance(float * a, float * b)
{
    return std::sqrt((b[0]-a[0])*(b[0]-a[0]) + (b[1]-a[1])*(b[1]-a[1]) + (b[2]-a[2])*(b[2]-a[2]));
}

unsigned int SearchNode::getOctant(float * point, bool * isOutofBounds)
{
    // Find 3D octant id
    int oct_x =  (point[0] - extent[0])*2.0 / (extent[1] - extent[0]);
    int oct_y =  (point[1] - extent[2])*2.0 / (extent[3] - extent[2]);
    int oct_z =  (point[2] - extent[4])*2.0 / (extent[5] - extent[4]);

    // Clamp
    if ((oct_x >= 2) || (oct_x < 0)) *isOutofBounds = true;
    if ((oct_y >= 2) || (oct_y < 0)) *isOutofBounds = true;
    if ((oct_z >= 2) || (oct_z < 0)) *isOutofBounds = true;

    unsigned int id = oct_x + 2*oct_y + 4*oct_z;

    // Find 1D octant id
    return id;
}

void SearchNode::split()
{
    /* The octants are assumed to be cubic. First create eight new
     * children. Then insert the nodes in the children according to
     * octant */

    n_children = 8;
    children = new SearchNode*[8];

    // For each child
    for (int i = 0; i < 8; i++)
    {
        int id_x = (i%4)%2;
        int id_y = (i%4)/2;
        int id_z = i/4;

        double half_side = (extent[1] - extent[0]) * 0.5;

        double child_extent[6];
        child_extent[0] = extent[0] + half_side*id_x;
        child_extent[1] = extent[1] - half_side*(1-id_x);
        child_extent[2] = extent[2] + half_side*id_y;
        child_extent[3] = extent[3] - half_side*(1-id_y);
        child_extent[4] = extent[4] + half_side*id_z;
        child_extent[5] = extent[5] - half_side*(1-id_z);

        children[i] = new SearchNode(this, child_extent);
//        children[i]->setOpenCLContext(context);
    }

    // For each point
    for (size_t i = 0; i < n_points; i++)
    {
        bool isOutofBounds = false;

        unsigned int id = getOctant(points + i*4, &isOutofBounds);
        if (!isOutofBounds)
        {

            this->children[id]->insert(points + i*4);
        }
    }
}

double * SearchNode::getExtent()
{
    return this->extent.data();
}
