#include "searchnode.h"

static const unsigned int MAX_POINTS = 32;
static const unsigned int MAX_LEVELS = 16;
static const unsigned int CL_MAX_ITEMS = 256;
static const unsigned int CL_LEVEL = 2;

SearchNode::SearchNode()
{
    this->isMsd = true;
    this->isEmpty = true;
    this->n_points = 0;
    this->n_children = 0;
    this->verbosity = 1;
    this->extent.reserve(6);
}

SearchNode::SearchNode(SearchNode * parent, double * extent)
{
    this->isMsd = true;
    this->isEmpty = true;
    this->parent = parent;
    this->n_points = 0;
    this->n_children = 0;
    this->verbosity = 1;
    this->extent.reserve(6);

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

// Functions that rely on recursive action such as this one should not be declared as an object function. Rather make a function external to the node.
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
                w = 1.0 / std::pow(d,p);
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

    double sample_extent[6];
    sample_extent[0] = sample[0] - search_radius;
    sample_extent[1] = sample[0] + search_radius;
    sample_extent[2] = sample[1] - search_radius;
    sample_extent[3] = sample[1] + search_radius;
    sample_extent[4] = sample[2] - search_radius;
    sample_extent[5] = sample[2] + search_radius;

    weighSamples(sample, sample_extent, &sum_w, &sum_wu, p, search_radius);

    if (sum_w > 0.0) return sum_wu / sum_w;
    else return 0;
}

void SearchNode::getIntersectedItems(MiniArray<double> * effective_extent, unsigned int * item_counter, cl_mem * items, cl_command_queue * queue)
{
    if ((this->isMsd) && (!this->isEmpty))// && (*item_counter <= CL_MAX_ITEMS))
    {
        //~if (*item_counter <= CL_MAX_ITEMS)
        //~{
            //~effective_extent->print(2, "eff_extent");
            //~extent.print(2, "extent");
            int tmp = 0;
            for (unsigned int i = 0; i < n_points; i++)
            {

                if (
                ((points[i*4+0] >= effective_extent->at(0)) && (points[i*4+0] <= effective_extent->at(1))) &&
                ((points[i*4+1] >= effective_extent->at(2)) && (points[i*4+1] <= effective_extent->at(3))) &&
                ((points[i*4+2] >= effective_extent->at(4)) && (points[i*4+2] <= effective_extent->at(5))))
                {
                    if (*item_counter < CL_MAX_ITEMS)
                    {
                        err = clEnqueueWriteBuffer(*queue, *items,
                            CL_TRUE, // try CL_FALSE
                            (*item_counter)*sizeof(cl_float4),
                            sizeof(cl_float4),
                            points + tmp*4,
                            0, NULL, NULL);
                        if (err != CL_SUCCESS)
                        {
                            writeLog("[!][SearchNode] Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
                            return;
                        }
                        tmp++;
                    }
                    (*item_counter)++;
                }
            }
            //~err = clEnqueueWriteBuffer(*queue, *items,
                //~CL_TRUE, // try CL_FALSE
                //~(*item_counter)*sizeof(cl_float4),
                //~n_points*sizeof(cl_float4),
                //~points,
                //~0, NULL, NULL);
            //~if (err != CL_SUCCESS)
            //~{
                //~writeLog("[!][SearchNode] Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
                //~return;
            //~}
            //~*item_counter += n_points;
        //~}
        //~else
        //~{
            //~*item_counter+= 9000; // "a lot"
        //~}
    }
    else if ((n_children > 0))// && (*item_counter < CL_MAX_ITEMS))
    {
        for (unsigned int i = 0; i < 8; i++)
        {
            if(children[i]->isIntersected(effective_extent->data()))
            {
                children[i]->getIntersectedItems(effective_extent, item_counter, items, queue);
            }
        }
    }
}

void SearchNode::writeLog(QString str)
{
    writeToLogAndPrint(str.toStdString().c_str(), "riv.log", 1);
}

int SearchNode::getBrick(float * target, MiniArray<double> * brick_extent, float p, float search_radius, unsigned int brick_outer_dimension, unsigned int level, cl_mem * items_cl, cl_mem * brick_extent_cl, cl_mem * target_cl, cl_kernel * voxelize_kernel, cl_command_queue * queue, int * method)
{
    int isEmptyBrick = 1;

    MiniArray<double> effective_extent(6);
    effective_extent[0] = brick_extent->at(0) - search_radius;
    effective_extent[1] = brick_extent->at(1) + search_radius;
    effective_extent[2] = brick_extent->at(2) - search_radius;
    effective_extent[3] = brick_extent->at(3) + search_radius;
    effective_extent[4] = brick_extent->at(4) - search_radius;
    effective_extent[5] = brick_extent->at(5) + search_radius;

    // Here we count the number of items in the volume intersected by the brick and the octtree. If this number is less than CL_MAX_ITEMS the calculations are carried out in parallel by OpenCL employing fast shared memory. Also, to be realistic, the check is only done for bricks deeper than CL_LEVEL

    unsigned int item_counter = 0;

    getIntersectedItems(&effective_extent, &item_counter, items_cl, queue);

    std::cout << "lvl "<< level << " item_counter" << item_counter << std::endl;
     //~effective_extent.print(2, "effective_extent");
    if (item_counter == 0)
    {
        isEmptyBrick = 1;
        *method = 2;
    }
    else if ((level >= CL_LEVEL) && (item_counter <= CL_MAX_ITEMS))
    {
        clFinish(*queue);
        // Prepare buffers
        err = clEnqueueWriteBuffer(*queue, *brick_extent_cl,
            CL_TRUE,
            0,
            6*sizeof(cl_float),
            brick_extent->toFloat().data(),
            0, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            writeLog("[!][SearchNode] Error before line "+QString::number(__LINE__)+": "+QString(cl_error_cstring(err)));
        }

        // Set kernel arguments
        err = clSetKernelArg(*voxelize_kernel, 0, sizeof(cl_mem), (void *) items_cl);
        err |= clSetKernelArg(*voxelize_kernel, 1, sizeof(cl_mem), (void *) brick_extent_cl);
        err |= clSetKernelArg(*voxelize_kernel, 2, sizeof(cl_mem), (void *) target_cl);
        err |= clSetKernelArg(*voxelize_kernel, 3, sizeof(cl_int), &brick_outer_dimension);
        err |= clSetKernelArg(*voxelize_kernel, 4, sizeof(cl_int), &item_counter);
        err |= clSetKernelArg(*voxelize_kernel, 5, sizeof(cl_float), &search_radius);
        err |= clSetKernelArg(*voxelize_kernel, 6, 512*sizeof(cl_float), NULL);
        if (err != CL_SUCCESS)
        {
            writeLog("[!][SearchNode]: Error before line "+QString::number(__LINE__)+QString(cl_error_cstring(err)));
        }

        // Launch kernel
        size_t loc_ws[3] = {8,8,8};
        size_t glb_ws[3] = {8,8,8};
        err = clEnqueueNDRangeKernel(*queue, *voxelize_kernel, 3, NULL, glb_ws, loc_ws, 0, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            writeLog("[!][SearchNode]: Error before line "+QString::number(__LINE__)+QString(cl_error_cstring(err)));
        }

        clFinish(*queue);
        std::cout << "### NEW BRICK ### brick_outer_dimension = " << brick_outer_dimension <<  ", search_radius = " << search_radius << std::endl;

        Matrix<float> extentz(1, 6);
        err = clEnqueueReadBuffer ( *queue,
            *brick_extent_cl,
            CL_TRUE,
            0,
            6*sizeof(cl_float),
            extentz.data(),
            0,
            NULL,
            NULL);
        if (err != CL_SUCCESS)
        {
            writeLog("[!][SearchNode]: Error before line "+QString::number(__LINE__)+QString(cl_error_cstring(err)));
        }
        extentz.print(2, "extentz");


        Matrix<float> itamz(item_counter, 4);
        err = clEnqueueReadBuffer ( *queue,
            *items_cl,
            CL_TRUE,
            0,
            item_counter*sizeof(cl_float4),
            itamz.data(),
            0,
            NULL,
            NULL);
        if (err != CL_SUCCESS)
        {
            writeLog("[!][SearchNode]: Error before line "+QString::number(__LINE__)+QString(cl_error_cstring(err)));
        }
        itamz.print(2, "itamz");

        // Read results
        err = clEnqueueReadBuffer ( *queue,
            *target_cl,
            CL_TRUE,
            0,
            513*sizeof(cl_float),
            target,
            0,
            NULL,
            NULL);
        if (err != CL_SUCCESS)
        {
            writeLog("[!][SearchNode]: Error before line "+QString::number(__LINE__)+QString(cl_error_cstring(err)));
        }

        if (target[512] > 0.0) isEmptyBrick = 0;
        else isEmptyBrick = 1;

        Matrix<float> targetz(32,16);
        targetz.setDeep(32,16, target);
        targetz.print(2,"targetz");
        std::cout << "sum " << target[512] << std::endl;

        *method = 0;
    }
    else
    {
        float idw;
        MiniArray<float> sample(3);
        float brick_step = (brick_extent->at(1) - brick_extent->at(0)) / ((float)(brick_outer_dimension-1));

        // If not, however, the calculations are simply carried out on the CPU instead
        for (unsigned int z = 0; z < brick_outer_dimension; z++)
        {
            for (unsigned int y = 0; y < brick_outer_dimension; y++)
            {
                for (unsigned int x = 0; x < brick_outer_dimension; x++)
                {
                    sample[0] = brick_extent->at(0) + brick_step * x;
                    sample[1] = brick_extent->at(2) + brick_step * y;
                    sample[2] = brick_extent->at(4) + brick_step * z;
                    idw = this->getIDW(sample.data(), p, search_radius);

                    target[x + y*brick_outer_dimension + z*brick_outer_dimension*brick_outer_dimension] = idw;

                    if (idw > 0) isEmptyBrick = 0;
                }
            }
        }
        *method = 1;
    }
    //~if (verbosity == 1) writeLog("[SearchNode] Line "+QString::number(__LINE__));
    return isEmptyBrick;
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
