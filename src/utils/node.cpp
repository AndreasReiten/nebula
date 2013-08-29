#include "node.h"

unsigned int MAX_POINTS = 8;
unsigned int MAX_LEVELS = 16;

Node::Node()
{
    this->isMsd = true;
    this->isEmpty = true;
    this->n_points = 0;
    this->n_children = 0;
}

Node::Node(Node * parent, double * extent)
{
    this->isMsd = true;
    this->isEmpty = true;
    this->parent = parent;
    this->n_points = 0;
    this->n_children = 0;
    
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

Node::~Node()
{
    //~ std::cout << "L" << level << ": Destroying node!" << std::endl;
    this->clearChildren();
    this->clearPoints();
}

void Node::clearChildren()
{
    if (n_children > 0)
    {
        //~ std::cout << "Killing children: "<< n_children << std::endl;
        for (unsigned int i = 0; i < n_children; i++)
        {
            //~ std::cout << "Destroying child " << i << std::endl;
            delete children[i];
        } 
        delete[] children;
        n_children = 0;
    }
}

void Node::clearPoints()
{
    if (n_points > 0)
    {
        //~ std::cout << "Killing " << n_points << " points at L" << level << std::endl;
        //~ 
        //~ for (unsigned int i = 0; i < n_points; i++)
        //~ {
            //~ for (unsigned int j = 0; j < level; j++)
            //~ {
                //~ std::cout << " ";
            //~ }
            //~ 
            //~ std::cout << "[ "<< points[i*4+0] << " " << points[i*4+1] << " " << points[i*4+2] << " ]"<< std::endl;
        //~ }
        //~ std::cout<< "delete (before): " << points <<std::endl;
        delete[] this->points;
        //~ std::cout<< "delete (after)" << points <<std::endl;
        n_points = 0;
        //~ std::cout << "clearPoints: n_points: " << n_points << std::endl;
    }
}

void Node::setParent(Node * parent)
{
    //~ std::cout << "setParent" << std::endl;
    parent->print();
    this->parent = parent;
    //~ std::cout << "setParent" << std::endl;
    if (this->parent == NULL)
    { 
        this->level = 0;
    }
    else
    {
        this->level = parent->getLevel() + 1;
    }
    //~ std::cout << "setParent" << std::endl;
}

unsigned int Node::getLevel()
{
    return level;
}

void Node::print()
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
    //~ else if (n_points > 0)
    //~ {
        //~ for (unsigned int i = 0; i < n_points; i++)
        //~ {
            //~ for (unsigned int j = 0; j < level+1; j++)
            //~ {
                //~ std::cout << " ";
            //~ }
            //~ 
            //~ std::cout << "[ "<< points[i*4+0] << " " << points[i*4+1] << " " << points[i*4+2] << " ]"<< std::endl;
        //~ }
    //~ }
}

// Functions that rely on recursive action such as this one should not be declared as an object function. Rather make a function external to the node.
void Node::insert(float * point)
{
    //~ std::cout << "Inserting at L" << level << std::endl;
    if (!isMsd)
    {
        //~ std::cout << "-> Insert 'go to next level'" << std::endl;
        bool isOutofBounds = false;
        
        unsigned int id = getOctant(point, &isOutofBounds);
        //~ std::cout << "  Inserting into octant: " << id << std::endl;
        if (!isOutofBounds) this->children[id]->insert(point);
        //~ if (isOutofBounds) std::cout << "insert marker" << std::endl;
    }
    else if ((n_points == 0) && isMsd)
    {
        //~ std::cout << "-> Insert 'new node'" << std::endl;
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
        //~ std::cout << "-> Insert 'split'" << std::endl;
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
        //~ std::cout << "-> Insert 'appending insert'" << std::endl;
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
        //~ std::cout << "-> L"<<level<<": Insert 'normal insert'" << std::endl;
        //~ std::cout << "      point: [" << std::setprecision(2) << std::fixed << point[0] << " "<< point[1] << " "<< point[2] << "]" << std::endl;
        //~ std::cout << "        ext: [" << std::setprecision(2) << std::fixed << extent[0] << " "<< extent[1] << " "<< extent[2] << " "<< extent[3] << " "<< extent[4] << " "<< extent[5] << "]" << std::endl;
        points[n_points*4+0]  = point[0];
        points[n_points*4+1]  = point[1];
        points[n_points*4+2]  = point[2];
        points[n_points*4+3]  = point[3];
        n_points++;
    }
    //~ std::cout << "Done Inserting" << std::endl;
}

void Node::weighSamples(float * sample, double * sample_extent, float * sum_w, float * sum_wu, float p, float search_radius)
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

bool Node::isIntersected(double * sample_extent)
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

float Node::getIDW(float * sample, float p, float search_radius)
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

bool Node::getBrick(float * dst, double * brick_extent, float p, float search_radius, unsigned int dimension)
{
    float idw; 
    MiniArray<float> sample(3);
    float brick_step = (brick_extent[1] - brick_extent[0]) / ((float)(dimension-1));
    bool isEmptyBrick = true;
    
    for (unsigned int z = 0; z < dimension; z++)
    {
        for (unsigned int y = 0; y < dimension; y++)
        {
            for (unsigned int x = 0; x < dimension; x++)
            {
                sample[0] = brick_extent[0] + brick_step * x;
                sample[1] = brick_extent[2] + brick_step * y;
                sample[2] = brick_extent[4] + brick_step * z;
                idw = this->getIDW(sample.data(), p, search_radius);
                
                dst[x + y*dimension + z*dimension*dimension] = idw;
                
                if (idw > 0) isEmptyBrick = false;
            }
        }
    }
    
    return isEmptyBrick;
}

float Node::distance(float * a, float * b)
{
    //~ std::cout << "      Measuring between [" << a[0] << " " << a[1] << " " << a[2] << "] and [" << b[0] << " " << b[1] << " " << b[2] << "]" << std::endl;
    
    return std::sqrt((b[0]-a[0])*(b[0]-a[0]) + (b[1]-a[1])*(b[1]-a[1]) + (b[2]-a[2])*(b[2]-a[2]));
}

unsigned int Node::getOctant(float * point, bool * isOutofBounds)
{
    //~ std::cout << "Node::getOctant" << std::endl;
    // Find 3D octant id
    int oct_x =  (point[0] - extent[0])*2.0 / (extent[1] - extent[0]);
    int oct_y =  (point[1] - extent[2])*2.0 / (extent[3] - extent[2]);
    int oct_z =  (point[2] - extent[4])*2.0 / (extent[5] - extent[4]);
    
    //~ std::cout << "oct_y = (" << points[1] <<"-"<< extent[2] <<")*2.0 / (" << extent[3] << "-" << extent[2] << ")" << std::endl;
    
    // Clamp
    if ((oct_x >= 2) || (oct_x < 0)) *isOutofBounds = true;
    if ((oct_y >= 2) || (oct_y < 0)) *isOutofBounds = true;
    if ((oct_z >= 2) || (oct_z < 0)) *isOutofBounds = true;
    
    //~ if (*isOutofBounds && (level > 0))
    //~ {
        //~ std::cout << "<< L"<<level<<": Out of bounds and sub node: " << oct_x << " "<< oct_y << " "<< oct_z  << std::endl;
        //~ std::cout << "      point: [" << std::setprecision(2) << std::fixed << point[0] << " "<< point[1] << " "<< point[2] << "]" << std::endl;
        //~ std::cout << "        ext: [" << std::setprecision(2) << std::fixed << extent[0] << " "<< extent[1] << " "<< extent[2] << " "<< extent[3] << " "<< extent[4] << " "<< extent[5] << "]" << std::endl;
    //~ }
    unsigned int id = oct_x + 2*oct_y + 4*oct_z;
    
    //~ std::cout << "id: " << id << "cuz: [x,y,z] = [" << oct_x << ", "<< oct_y << ", "<< oct_z << "]"<< std::endl;
    //~ std::cout << "ext: [x,x,y,y,z,z] = [" << extent[0] << ", "<< extent[1] << ", "<< extent[2] << ", "<< extent[3]<< ", "<< extent[4]<< ", "<< extent[5]<< "]"<< std::endl;
    //~ std::cout << "point: [x,y,z] = [" << point[0] << ", "<< point[1] << ", "<< point[2] << "]"<< std::endl;
    //~ std::cout << "Node::getOctant (end)" << std::endl;
    // Find 1D octant id
    return id;
    
    
}
    
void Node::split()
{
    /* The octants are assumed to be cubic. First create eight new
     * children. Then insert the nodes in the children according to
     * octant */
     
     
    //~ std::cout << "Splitting" << std::endl;
    n_children = 8;
    children = new Node*[8];
    
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
        
        children[i] = new Node(this, child_extent);
    }

    // For each point
    //~ std::cout << n_points << std::endl;
    for (size_t i = 0; i < n_points; i++)
    {
        bool isOutofBounds = false;
        
        unsigned int id = getOctant(points + i*4, &isOutofBounds);
        //~ if (isOutofBounds) std::cout << "split marker" << std::endl;
        //~ std::cout << "inserting ( "<<id<<" ) [" << std::setprecision(2) << std::fixed << points[i*4] << " " << points[i*4+1] << " " << points[i*4+2] << "] into 
        //~ std::cout<<"["<< children[id]->getExtent()[0] << " "<< children[id]->getExtent()[1] << " "<< children[id]->getExtent()[2] << " "<< children[id]->getExtent()[3] << " "<< children[id]->getExtent()[4] << " "<< children[id]->getExtent()[5] << "]"<< std::endl;
        
        //~ std::cout << "going in (id = "<< id<<")" << std::endl;
        //~ double * lol = children[id]->getExtent();
        //~ std::cout << "going out" << std::endl;
        
        //~ std::cout << "  split inserting into octant: " << id << std::endl;
        if (!isOutofBounds)
        {
            //~ std::cout << "L" << level<<": inserting ( "<<id<<" ) [" << std::setprecision(2) << std::fixed << points[i*4] << " " << points[i*4+1] << " " << points[i*4+2] << "] into\n\t["<< children[id]->getExtent()[0] << " "<< children[id]->getExtent()[1] << " "<< children[id]->getExtent()[2] << " "<< children[id]->getExtent()[3] << " "<< children[id]->getExtent()[4] << " "<< children[id]->getExtent()[5] << "] from\n\t["<< extent[0] << " "<< extent[1] << " "<< extent[2] << " "<< extent[3] << " "<< extent[4] << " "<< extent[5] << "]"<<std::endl;
            
            this->children[id]->insert(points + i*4);
        }
    }
}

double * Node::getExtent()
{
    //~ std::cout << "returning extent" << std::endl;
    return this->extent;
}
