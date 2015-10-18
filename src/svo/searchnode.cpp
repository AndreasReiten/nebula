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
    p_is_empty(true),
    p_is_max_resolved(false),
    p_is_interpolation_node(false),
    p_voxels_per_side(7),
    p_node_grid_side(6),
    p_voxel_grid_side(8),
    p_max_points(1024)
{


//    p_min_data_interdistance = 0;

    p_mutex = new QMutex;
}

SearchNode::~SearchNode()
{

}


void SearchNode::insert2(xyzwd32 &point)
{
    // Place a data point into this node, or a deeper node if required
    // If the current node is not a leaf (but a branch), then it's private variables will not be changed on further insertion, and thus it does not need a mutex lock
    if (p_is_leaf)
    {
        QMutexLocker lock(p_mutex);
        p_insert2(point);
    }
    else
    {
        p_insert2(point);
    }
}

void SearchNode::p_insert2(xyzwd32 & point)
{
    // Descend if branch node (even if the data interdistance does not require it. A shortcut...)
    if (!p_is_leaf)
    {
        int id = ntant(point, 2);
        if (id >= 0)
        {
            p_children[id].insert(point);
        }
    }
    // Split or add if leaf node
    else
    {
        // Split if the data interdistance, d, is smaller than the voxel size of the node
        if (point.d < voxelside())
        {
            split2();
            p_cloud.clear();

            int id = ntant(point, 2);

            if (id >= 0)
            {
                p_children[id].insert(point);
            }

            p_is_empty = true;
            p_is_leaf = false;
        }
        else
        {
            // Add point to cloud or rebin
            if (p_cloud.size() <= p_max_points)
            {
                p_cloud << point;
                p_is_empty = false;
            }
            else
            {
                rebin2();
            }
        }
    }
}

void SearchNode::rebin2()
{
    // Place points in a resampling grid
    QVector<QVector<xyzwd32>> grid = cloudbins();

    p_cloud.clear();

    for (int i = 0; i < grid.size(); i++)
    {
        if (grid[i].size() <= 0) continue;

        // Calculate a single average point
        xyzwd32 agglomerate = {0.0,0.0,0.0,0.0,0.0};

        for (int j = 0; j < grid[i].size(); j++)
        {
            agglomerate.x += grid[i][j].x;
            agglomerate.y += grid[i][j].y;
            agglomerate.z += grid[i][j].z;
            agglomerate.w += grid[i][j].w;
            agglomerate.d += grid[i][j].d;
        }

        agglomerate.x /= (float) grid[i].size();
        agglomerate.y /= (float) grid[i].size();
        agglomerate.z /= (float) grid[i].size();
        agglomerate.w /= (float) grid[i].size();
        agglomerate.d /= (float) grid[i].size();


        p_cloud << agglomerate;
    }
}

QVector<QVector<xyzwd32>> SearchNode::cloudbins()
{
    QVector<QVector<xyzwd32>> grid(p_node_grid_side*p_node_grid_side*p_node_grid_side);

    for (int i = 0; i < p_cloud.size(); i++)
    {
        int id = ntant(p_cloud[i], p_node_grid_side);

        if (id >= 0)
        {
            grid[i] << p_cloud[i];
        }
    }

    return grid;
}

QVector<xyzwd32> SearchNode::cloudgrid(QVector<QVector<xyzwd32>> & bins)
{
    QVector<xyzwd32> grid;

    for (int i = 0; i < bins.size(); i++)
    {
        // Calculate a single average point
        xyzwd32 agglomerate = {0.0,0.0,0.0,0.0,0.0};

        if (bins[i].size() > 0)
        {
            for (int j = 0; j < bins[i].size(); j++)
            {
                agglomerate.x += bins[i][j].x;
                agglomerate.y += bins[i][j].y;
                agglomerate.z += bins[i][j].z;
                agglomerate.w += bins[i][j].w;
                agglomerate.d += bins[i][j].d;
            }

            agglomerate.x /= (float) bins[i].size();
            agglomerate.y /= (float) bins[i].size();
            agglomerate.z /= (float) bins[i].size();
            agglomerate.w /= (float) bins[i].size();
            agglomerate.d /= (float) bins[i].size();
        }

        grid[i] = agglomerate;
    }

    return grid;
}

void SearchNode::split2()
{
    // Return if there are in fact children
    if (!p_children.isEmpty()) return;

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

                p_children[linear_id].setMaxPoints(p_max_points);
                p_children[linear_id].setBinsPerSide(p_node_grid_side);
                p_children[linear_id].setId(child_id_x, child_id_y, child_id_z);
            }
        }
    }

    // If this node has data points, flush them down. Ideally let them be and use them together with node grid interpolation later
    for (int i = 0; i < p_cloud.size(); i++)
    {
        int id = ntant(p_cloud[i], 2);

        if (id >= 0)
        {
            p_children[id].insert2(p_cloud[i]);
        }
    }
}

void SearchNode::recombine2()
{
    // Return if leaf
    if (p_is_leaf) return;

    // Count the number of recombinable 2x2x2 bins
    int n_recombinable = 0;

    for (int i = 0; i < p_children.size(); i++)
    {
        n_recombinable += p_children[i].cloudRecombinable(0.1, 0.0); // IMPORTANT INPUT PARAMS
    }

    // Recombine if children are recombinable enough
    if ((double) n_recombinable / (double)(p_node_grid_side*p_node_grid_side*p_node_grid_side*8) <= 0) // IMPORTANT INPUT PARAMS
    {
        if (p_cloud.size() > 0) qFatal("oh shit");

        for (int i = 0; i < p_children.size(); i++)
        {
            p_cloud << p_children[i].cloud();
        }

        p_children.clear();
        p_is_leaf = true;
        p_is_empty = false;

        // Rebin if needed
        if (p_cloud.size() <= p_max_points)
        {
            rebin2();
        }
    }
}

int SearchNode::cloudRecombinable(double req_avg_prct, double noise)
{
    // There is plenty of room for optimization here if needed
    // Place points in a resampling grid
    QVector<QVector<xyzwd32>> bins = cloudbins();
    QVector<xyzwd32> grid = cloudgrid(bins);

    // Metrics for each 2x2x2 point cube, used primarily to assess if the node is exceedingly self-similar
    int n_self_similar = 0;

    for (int i = 0; i < p_node_grid_side; i+=2)
    {
        for (int j = 0; j < p_node_grid_side; j+=2)
        {
            for (int k = 0; k < p_node_grid_side; k+=2)
            {
                // Average
                double avg = 0;
                avg += grid[(i+0) + (j+0) * p_node_grid_side + (k+0) * p_node_grid_side * p_node_grid_side].w;
                avg += grid[(i+1) + (j+0) * p_node_grid_side + (k+0) * p_node_grid_side * p_node_grid_side].w;
                avg += grid[(i+0) + (j+1) * p_node_grid_side + (k+0) * p_node_grid_side * p_node_grid_side].w;
                avg += grid[(i+0) + (j+0) * p_node_grid_side + (k+1) * p_node_grid_side * p_node_grid_side].w;
                avg += grid[(i+1) + (j+1) * p_node_grid_side + (k+0) * p_node_grid_side * p_node_grid_side].w;
                avg += grid[(i+0) + (j+1) * p_node_grid_side + (k+1) * p_node_grid_side * p_node_grid_side].w;
                avg += grid[(i+1) + (j+0) * p_node_grid_side + (k+1) * p_node_grid_side * p_node_grid_side].w;
                avg += grid[(i+1) + (j+1) * p_node_grid_side + (k+1) * p_node_grid_side * p_node_grid_side].w;
                avg /= 8.0;

                // Standard deviation
                double sigma = 0;
                sigma += std::pow(grid[(i+0) + (j+0) * p_node_grid_side + (k+0) * p_node_grid_side * p_node_grid_side].w - avg,2.0);
                sigma += std::pow(grid[(i+1) + (j+0) * p_node_grid_side + (k+0) * p_node_grid_side * p_node_grid_side].w - avg,2.0);
                sigma += std::pow(grid[(i+0) + (j+1) * p_node_grid_side + (k+0) * p_node_grid_side * p_node_grid_side].w - avg,2.0);
                sigma += std::pow(grid[(i+0) + (j+0) * p_node_grid_side + (k+1) * p_node_grid_side * p_node_grid_side].w - avg,2.0);
                sigma += std::pow(grid[(i+1) + (j+1) * p_node_grid_side + (k+0) * p_node_grid_side * p_node_grid_side].w - avg,2.0);
                sigma += std::pow(grid[(i+0) + (j+1) * p_node_grid_side + (k+1) * p_node_grid_side * p_node_grid_side].w - avg,2.0);
                sigma += std::pow(grid[(i+1) + (j+0) * p_node_grid_side + (k+1) * p_node_grid_side * p_node_grid_side].w - avg,2.0);
                sigma += std::pow(grid[(i+1) + (j+1) * p_node_grid_side + (k+1) * p_node_grid_side * p_node_grid_side].w - avg,2.0);
                sigma /= 8.0;
                sigma = std::sqrt(sigma);

                if (sigma < avg*req_avg_prct + noise) n_self_similar++;
            }
        }
    }

    return n_self_similar;
}

QVector<xyzwd32> & SearchNode::cloud()
{
    return p_cloud;
}

QVector<float> & SearchNode::nodegrid()
{
    return p_node_grid;
}

void SearchNode::interpolate2(SearchNode * root, bool check_neighbours)
{
    // Return if already interpolated
    if (!p_voxel_grid.isEmpty()) return;

    // This is needed later
    double x_center, y_center, z_center;
    center(x_center, y_center, z_center);

    // If leaf
    if (p_is_leaf)
    {
        // Use the statistics of the cloud to approximate the sample interdistance
        double data_interdist_min = 0;
        double data_interdist_max = 0;
        double data_interdist_avg = 0;

        interdistMetrics2(data_interdist_min, data_interdist_max, data_interdist_avg);

        double extra_extent = std::max(binside(), voxelside())*0.5 + data_interdist_avg;

        QVector<double> extent(6);
        extent[0] = x_center - (side() * 0.5 + extra_extent);
        extent[1] = x_center + (side() * 0.5 + extra_extent);
        extent[2] = y_center - (side() * 0.5 + extra_extent);
        extent[3] = y_center + (side() * 0.5 + extra_extent);
        extent[4] = z_center - (side() * 0.5 + extra_extent);
        extent[5] = z_center + (side() * 0.5 + extra_extent);

        // Get all relevant data
        // Fetch data from up to 3x3x3 - 1 surrounding bins (can be fewer). Start by finding their pointers
        // Use a QSet to ensure zero duplicate nodes
        QSet<SearchNode *> nodes;

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    double x = x_center + (double)(i-1)*side();
                    double y = y_center + (double)(j-1)*side();
                    double z = z_center + (double)(k-1)*side();

                    if ((x < 0) || (x >= 1) || (y < 0) || (y >= 1) || (z < 0) || (z >= 1))
                    {
                        nodes << nodeAt(x, y, z, p_level);
                    }
                }
            }
        }

        // Iterate through the data, keeping only data that lies within the sampling volume
        QVector<xyzwd32> data; // It might be beneficial to use a k-d tree for this, or using GPGPU and local memory

        QSetIterator<SearchNode *> k(nodes);
        while (k.hasNext())
        {
            SearchNode * node = k.next();

            for (int i = 0; i < node->cloud().size(); i++)
            {
                if (
                        (node->cloud()[i].x > extent[0]) && (node->cloud()[i].x < extent[1]) &&
                        (node->cloud()[i].y > extent[2]) && (node->cloud()[i].y < extent[3]) &&
                        (node->cloud()[i].z > extent[4]) && (node->cloud()[i].z < extent[5]))
                {
                    data << node->cloud()[i];
                }
            }
        }

        // Return if there is no relevant data
        if (data.isEmpty())
        {
            p_is_empty = true;
            return;
        }

        // Interpolate voxel grid from cloud
        p_voxel_grid.resize(p_voxel_grid_side*p_voxel_grid_side*p_voxel_grid_side);

        for (int i = 0; i < p_voxel_grid_side; i++)
        {
            for (int j = 0; j < p_voxel_grid_side; j++)
            {
                for (int k = 0; k < p_voxel_grid_side; k++)
                {
                    int id =
                            i +
                            j * p_voxel_grid_side +
                            k * p_voxel_grid_side * p_voxel_grid_side;


                    // Do a lookup of the value
                    double x = (double) (p_id_node_x + ((double)i/(double)(p_voxel_grid_side-1))) * side();
                    double y = (double) (p_id_node_y + ((double)j/(double)(p_voxel_grid_side-1))) * side();
                    double z = (double) (p_id_node_z + ((double)k/(double)(p_voxel_grid_side-1))) * side();

                    // If requested value lies outside of octtree, set to zero
                    if ((x < 0) || (x >= 1) || (y < 0) || (y >= 1) || (z < 0) || (z >= 1))
                    {
                        p_voxel_grid[id] = 0;
                    }
                    // Else use grid value
                    else
                    {
                        // Multivariate interpolation, IDW with Gaussian value scaling


                        double p = 1.0; // IMPORTANT INPUT PARAMS
                        double nominator = 0;
                        double denominator = 0;

                        for (int l = 0; l < data.size(); l++)
                        {
                            double d = distance(x, y, z, data[l].x, data[l].y, data[l].z);
                            double w = 1.0/std::pow(d, p);
                            double r = std::max((double)voxelside()*0.5, (double)data[l].d);


                            nominator += data[l].w*std::exp(-(d * d / (2.0 * r))) * w;
                            denominator += w;

                        }

                        p_voxel_grid[id] = nominator /  denominator;
                    }
                }
            }
        }

        // Interpolate rebuild grid from cloud
        p_node_grid.resize(p_node_grid_side*p_node_grid_side*p_node_grid_side);

        for (int i = 0; i < p_node_grid_side; i++)
        {
            for (int j = 0; j < p_node_grid_side; j++)
            {
                for (int k = 0; k < p_node_grid_side; k++)
                {
                    int id =
                            i +
                            j * p_node_grid_side +
                            k * p_node_grid_side * p_node_grid_side;


                    // Do a lookup of the value
                    double x = (double) (p_id_node_x + (((double)i+0.5)/(double)(p_node_grid_side))) * side();
                    double y = (double) (p_id_node_y + (((double)j+0.5)/(double)(p_node_grid_side))) * side();
                    double z = (double) (p_id_node_z + (((double)k+0.5)/(double)(p_node_grid_side))) * side();

                    // If requested value lies outside of octtree, set to zero
                    if ((x < 0) || (x >= 1) || (y < 0) || (y >= 1) || (z < 0) || (z >= 1))
                    {
                        p_node_grid[id] = 0;
                    }
                    // Else use grid value
                    else
                    {
                        // Multivariate interpolation, IDW with Gaussian value scaling
                        double p = 1.0; // IMPORTANT INPUT PARAMS
                        double nominator = 0;
                        double denominator = 0;

                        for (int l = 0; l < data.size(); l++)
                        {
                            double d = distance(x, y, z, data[l].x, data[l].y, data[l].z);
                            double w = 1.0/std::pow(d, p);
                            double r = std::max((double)binside2()*0.5, (double)data[l].d);


                            nominator += data[l].w*std::exp(-(d * d / (2.0 * r))) * w;
                            denominator += w;

                        }

                        p_node_grid[id] = nominator /  denominator;
                    }
                }
            }
        }
    }
    // If branch
    else
    {
        // Clear point clouds of children
        for (int i = 0; i < p_children.size(); i++)
        {
            p_children[i].cloud().clear();
        }

        // Interpolate node grid from node grid of last level
        p_node_grid.resize(p_node_grid_side*p_node_grid_side*p_node_grid_side);

        for (int i = 0; i < p_node_grid_side*2; i++)
        {
            for (int j = 0; j < p_node_grid_side*2; j++)
            {
                for (int k = 0; k < p_node_grid_side*2; k++)
                {
                    double x = (double) (p_id_node_x + (((double)i+0.5)/(double)(p_node_grid_side*2))) * side();
                    double y = (double) (p_id_node_y + (((double)j+0.5)/(double)(p_node_grid_side*2))) * side();
                    double z = (double) (p_id_node_z + (((double)k+0.5)/(double)(p_node_grid_side*2))) * side();

                    p_node_grid[i/2 + j/2*p_node_grid_side + k/2*p_node_grid_side*p_node_grid_side] += this->nodeValueAt(x,y,z, p_level+1);
                }
            }
        }

        for (int i = 0; i < p_node_grid.size(); i++)
        {
            p_node_grid[i] /= 2*2*2;
        }

        // Interpolate voxel grid from node grid of this level
        // Use trilinear interpolation
        for (int i = 0; i < p_voxel_grid_side; i++)
        {
            for (int j = 0; j < p_voxel_grid_side; j++)
            {
                for (int k = 0; k < p_voxel_grid_side; k++)
                {
                    double x = (double) (p_id_node_x + ((double)i/(double)(p_voxel_grid_side-1))) * side();
                    double y = (double) (p_id_node_y + ((double)j/(double)(p_voxel_grid_side-1))) * side();
                    double z = (double) (p_id_node_z + ((double)k/(double)(p_voxel_grid_side-1))) * side();

                    p_voxel_grid[i + j*p_voxel_grid_side + k*p_voxel_grid_side*p_voxel_grid_side] += root->nodeValueAt_Linear(x,y,z, p_level, root);
                }
            }
        }

        // Clear node grids of children
        for (int i = 0; i < p_children.size(); i++)
        {
            p_children[i].nodegrid().clear();
        }
    }

    /* If the outer layer of the voxel grid has nonzero values, ensure that there are neighbour nodes
     * of appreciable LOD so that there will be no visual artefacts due to interpolation. In some
     * cases this will mean a new nodes must be created, and that these nodes also must be
     * interpolated, possibly in the same function call
     * */
    if (check_neighbours)
    {
        QVector<bool> neighbours(3*3*3, false);

        for (int i = 0; i < p_voxel_grid_side; i++)
        {
            for (int j = 0; j < p_voxel_grid_side; j++)
            {
                for (int k = 0; k < p_voxel_grid_side; k++)
                {
                    // If voxel is on face
                    if (    (i <= 0) || (i >= p_voxel_grid_side-1) ||
                            (j <= 0) || (j >= p_voxel_grid_side-1) ||
                            (k <= 0) || (k >= p_voxel_grid_side-1))
                    {
                        int id_voxel =
                                i +
                                j * p_voxel_grid_side +
                                k * p_voxel_grid_side * p_voxel_grid_side;

                        // If voxel value is greater than zero, tag the neighbouring nodes
                        if (p_voxel_grid[id_voxel] > 0)
                        {
                            int id_neighbour_x = (i <= 0 ? 0 : (i < p_voxel_grid_side - 1 ? 1 : 2));
                            int id_neighbour_y = (j <= 0 ? 0 : (j < p_voxel_grid_side - 1 ? 1 : 2));
                            int id_neighbour_z = (k <= 0 ? 0 : (k < p_voxel_grid_side - 1 ? 1 : 2));

                            int id_neighbour =
                                    id_neighbour_x +
                                    id_neighbour_y * 3 +
                                    id_neighbour_z * 3 * 3;

                            neighbours[id_neighbour] = true;
                        }
                    }
                }
            }
        }


        // For each tagged neighbour, check if said neighbour exists. If not, create it (mutex lock)
        for (int i = 0; i < neighbours.size(); i++)
        {
            if (neighbours[i])
            {
                int id_neighbour_z = i / (3 * 3);
                int id_neighbour_y = (i - 3 * 3 * id_neighbour_z) / 3;
                int id_neighbour_x = i - 3 * 3 * id_neighbour_z - 3 * id_neighbour_y;

                double x = x_center + (double)(id_neighbour_x - 1) * side();
                double y = y_center + (double)(id_neighbour_y - 1) * side();
                double z = z_center + (double)(id_neighbour_z - 1) * side();

                root->ensureNodeAt(x, y, z, p_level, root);
            }
        }
    }
}

double SearchNode::distance(double x0, double y0, double z0, double x1, double y1, double z1)
{
    return std::sqrt((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0) + (z1-z0)*(z1-z0));
}

void SearchNode::ensureNodeAt(double x, double y, double z, int level, SearchNode *root)
{
    if (p_level < level)
    {
        if (p_is_leaf)
        {
            QMutexLocker lock(p_mutex);

            split2();
            p_cloud.clear();

            p_is_empty = true;
            p_is_leaf = false;
        }

        int id = ntant(x, y, z, 2);
        if (id >= 0)
        {
            p_children[id].ensureNodeAt(x, y, z, level, root);
        }

    }
    else
    {
        QMutexLocker lock(p_mutex);

        this->interpolate2(root, false);
    }
}

double SearchNode::nodeValueAt(double x, double y, double z, int level)
{
    if (p_is_leaf || (p_level >= level))
    {
        int id = ntant(x, y, z, p_node_grid_side);
        if (id >= 0)
        {
            return p_node_grid[id];
        }
    }
    else
    {
        int id = ntant(x, y, z, 2);
        if (id >= 0)
        {
            return p_children[id].nodeValueAt(x, y, z, level);
        }
    }
}

double SearchNode::nodeValueAt_Linear(double x, double y, double z, int max_level, SearchNode * root)
{
    // If leaf, search the grid
    if (p_is_leaf || (p_level >= max_level))
    {
        // Find the xyz values for the surrounding eight interpolation points, positions of which are given by the bin structure
        QVector<xyzwd32> points(8);

        double sample_spacing = voxelside();

        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    // The position used to retrieve the interpolation value at this position
                    double x_interpol = x + (double)(i*2-1)*sample_spacing*0.5;
                    double y_interpol = y + (double)(j*2-1)*sample_spacing*0.5;
                    double z_interpol = z + (double)(k*2-1)*sample_spacing*0.5;

                    // The center position of the bin from which the interpolation will be read
                    // Note that this bin can be in a different node, and could be empty or at another level
                    // The possible change in level of detail is not currently allowed to interfere with said position
                    // The situation might be remedied by weighing shallow nodes more lightly
                    points[i + j*2 + k*4].x = ((double) std::floor(x_interpol * (double) (nodesPerSide() * p_bins_per_side)) + 0.5 ) * binside();
                    points[i + j*2 + k*4].y = ((double) std::floor(y_interpol * (double) (nodesPerSide() * p_bins_per_side)) + 0.5 ) * binside();
                    points[i + j*2 + k*4].z = ((double) std::floor(z_interpol * (double) (nodesPerSide() * p_bins_per_side)) + 0.5 ) * binside();
                    points[i + j*2 + k*4].w = root->nodeValueAt(x_interpol, y_interpol, z_interpol, p_level);
                }
            }
        }

        // Carry out trilinear interpolation
        double xd = (x - points[0].x)/(points[7].x - points[0].x);
        double yd = (y - points[0].y)/(points[7].y - points[0].y);
        double zd = (z - points[0].z)/(points[7].z - points[0].z);

        // Interpolate along x
        double c00 = points[0].w * (1.0-xd) + points[1].w * xd;
        double c10 = points[2].w * (1.0-xd) + points[3].w * xd;
        double c01 = points[4].w * (1.0-xd) + points[5].w * xd;
        double c11 = points[6].w * (1.0-xd) + points[7].w * xd;

        // Interpolate along y
        double c0 = c00 * (1.0 - yd) + c10 * yd;
        double c1 = c01 * (1.0 - yd) + c11 * yd;

        // Interpolate along z
        double c = c0 * (1.0 - zd) + c1 * zd;

        return c;
    }
    // Else if branch, descend to the next level
    else
    {
        int id = ntant(x,y,z, 2);
        return p_children[id].nodeValueAt_Linear(x, y, z, max_level, root);
    }
}

void SearchNode::voxelize2()
{

}


































void SearchNode::squeeze()
{
    if (!isLeaf() || p_is_max_resolved) return;

    if (isEmpty())
    {
        p_is_max_resolved = true;
        return;
    }

    // Perform a conditional split of a leaf node in an attempt to increase resolution
    // Is the resolution of the node comparable to the theoretical resolution limit of the data (point interdistance)?
    double data_interdist_min;
    double data_interdist_max;
    double data_interdist_avg;

    interdistMetrics(data_interdist_min, data_interdist_max, data_interdist_avg);

    bool is_deep_enough = (voxelside() <= data_interdist_min);
    if (is_deep_enough)
    {
        p_is_max_resolved = true;
        return;
    }

    // Is the data in this node self-similar (low variance)?
    double average;
    double sigma;

    makeGridFromBins_Nearest();
    gridMetrics(average, sigma);
    p_grid.clear();

    bool is_self_similar = sigma < average*0.1 + 0.0; /* TODO: How to determine parameters */
    if (is_self_similar)
    {
        p_is_max_resolved = true;
        return;
    }

    if (!is_deep_enough && !is_self_similar)
    {
        split();
        p_data_binned.clear();
        p_is_leaf = false;
        p_is_empty = true;
    }
}

void SearchNode::gridMetrics(double & average, double & sigma)
{
    average = 0;
    sigma = 0;
    for (int i = 0; i < p_grid.size(); i++)
    {
        average += p_grid[i];
    }

    average /= (double) p_grid.size();

    for (int i = 0; i < p_grid.size(); i++)
    {
        sigma += std::pow(p_grid[i] - average, 2.0);
    }

    sigma  /= (double) p_grid.size();

    sigma = std::sqrt(sigma);
}

void SearchNode::recombine()
{
//    return;

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

    gridRecombineMetrics(averages, sigmas);

    for (int i = 0; i < p_bins_per_side*p_bins_per_side*p_bins_per_side/8; i+=2)
    {
        if (sigmas[i] < averages[i]*0.1 + 5.0) n_expendable_bins++; /* TODO: How to determine parameters */
    }

    return n_expendable_bins;
}

void SearchNode::gridRecombineMetrics(QVector<double> & averages, QVector<double> & sigmas)
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
                sigma += std::pow(p_grid[(i+0) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::pow(p_grid[(i+1) + (j+0) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::pow(p_grid[(i+0) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::pow(p_grid[(i+0) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::pow(p_grid[(i+1) + (j+1) * p_bins_per_side + (k+0) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::pow(p_grid[(i+0) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::pow(p_grid[(i+1) + (j+0) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side] - avg,2.0);
                sigma += std::pow(p_grid[(i+1) + (j+1) * p_bins_per_side + (k+1) * p_bins_per_side * p_bins_per_side] - avg,2.0);
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

}

void SearchNode::makeGridFromBins_Nearest()
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

SearchNode * SearchNode::nodeAt(double x, double y, double z, int max_level)
{
    if (!p_is_leaf || p_level >= max_level)
    {
        return this;
    }
    else
    {
        int id = ntant(x, y, z, 2);
        if (id >= 0)
        {
            return p_children[id].nodeAt(x, y, z, max_level);
        }
    }
}

void SearchNode::makeGridFromBins_Linear()
{
    // Reduce a node from bins with irregular xyzw points to a simple equi-distanced grid with w-values
    // This requires the usage of nearby points from other nodes
    p_grid.resize(p_bins_per_side*p_bins_per_side*p_bins_per_side);

    // Fetch data from up to 3x3x3 - 1 surrounding bins as well (can be fewer). Start by finding their pointers
    // Use a QSet to ensure zero duplicate pointers
    QSet<SearchNode *> nodes;

    double x_center, y_center, z_center;
    center(x_center, y_center, z_center);

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                double x = x_center + (double)(i-1)*side();
                double y = y_center + (double)(j-1)*side();
                double z = z_center + (double)(k-1)*side();
                nodes << nodeAt(x, y, z, p_level);
            }
        }
    }

    // Determine the extent of the sampling volume.
    // The extent beyond that of the node is at least the the average sample interdistance + the bin size
    double data_interdist_min;
    double data_interdist_max;
    double data_interdist_avg;
    interdistMetrics(data_interdist_min, data_interdist_max, data_interdist_avg);

    double extra_extent = binside()*0.5 + data_interdist_avg;

    QVector<double> extent(6);
    extent[0] = x_center - (side() * 0.5 + extra_extent);
    extent[1] = x_center + (side() * 0.5 + extra_extent);
    extent[2] = y_center - (side() * 0.5 + extra_extent);
    extent[3] = y_center + (side() * 0.5 + extra_extent);
    extent[4] = z_center - (side() * 0.5 + extra_extent);
    extent[5] = z_center + (side() * 0.5 + extra_extent);

    // Iterate through the data, keeping only data that lies within the sampling volume
    QVector<xyzwd32> data;

    QSetIterator<SearchNode *> k(nodes);
    while (k.hasNext())
    {
        SearchNode * node = k.next();

        for (int i = 0; i < node->bins().size(); i++)
        {
            for (int j = 0; j < node->bins()[i].size(); j++)
            {
                xyzwd32 point = node->bins()[i][j];
                if (
                        (point.x > extent[0]) && (point.x < extent[1]) &&
                        (point.y > extent[2]) && (point.y < extent[3]) &&
                        (point.z > extent[4]) && (point.z < extent[5]))
                {
                    data << point;
                }
            }
        }
    }


}

double SearchNode::gridValueAt_Nearest(double x, double y, double z, int max_level)
{
    // If outside of octree, return zero
    if ((x < 0) || (x >= 1) || (y < 0) || (y >= 1) || (z < 0) || (z >= 1))
    {
        return 0.0;
    }
    // Else if leaf, or if the maximum allowed level of detail is reached, search the grid
    else if (isLeaf() || (p_level >= max_level))
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
            return 0;
        }
        else
        {
            return p_children[id].gridValueAt_Nearest(x, y, z, max_level);
        }
    }
}

double SearchNode::side()
{
    return 1.0 / (double) nodesPerSide();
}

double SearchNode::binside()
{
    return 1.0 / (double) (nodesPerSide() *  p_bins_per_side);
}

double SearchNode::binside2()
{
    return 1.0 / (double) (nodesPerSide() *  p_node_grid_side);
}


double SearchNode::voxelside()
{
    return 1.0 / (double) (nodesPerSide() *  (p_voxel_grid_side-1));
}

void SearchNode::center(double & x, double & y, double & z)
{
    x = (double) (p_id_node_x + 0.5 ) * side();
    y = (double) (p_id_node_y + 0.5 ) * side();
    z = (double) (p_id_node_z + 0.5 ) * side();
}

double SearchNode::gridValueAt_Linear(double x, double y, double z, int max_level, SearchNode & root)
{
    // If leaf, search the grid
    if (isLeaf() || (p_level >= max_level))
    {
        // Find the xyz values for the surrounding eight interpolation points, positions of which are given by the bin structure
        QVector<xyzwd32> points(8);

        double sample_spacing = voxelside();

        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    // The position used to retrieve the interpolation value at this position
                    double x_interpol = x + (double)(i*2-1)*sample_spacing*0.5;
                    double y_interpol = y + (double)(j*2-1)*sample_spacing*0.5;
                    double z_interpol = z + (double)(k*2-1)*sample_spacing*0.5;

                    // The center position of the bin from which the interpolation will be read
                    // Note that this bin can be in a different node, and could be empty or at another level
                    // The possible change in level of detail is not currently allowed to interfere with said position
                    // The situation might be remedied by weighing shallow nodes more lightly
                    points[i + j*2 + k*4].x = ((double) std::floor(x_interpol * (double) (nodesPerSide() * p_bins_per_side)) + 0.5 ) * binside();
                    points[i + j*2 + k*4].y = ((double) std::floor(y_interpol * (double) (nodesPerSide() * p_bins_per_side)) + 0.5 ) * binside();
                    points[i + j*2 + k*4].z = ((double) std::floor(z_interpol * (double) (nodesPerSide() * p_bins_per_side)) + 0.5 ) * binside();
                    points[i + j*2 + k*4].w = root.gridValueAt_Nearest(x_interpol, y_interpol, z_interpol, p_level);
                }
            }
        }

        // Carry out trilinear interpolation
        double xd = (x - points[0].x)/(points[7].x - points[0].x);
        double yd = (y - points[0].y)/(points[7].y - points[0].y);
        double zd = (z - points[0].z)/(points[7].z - points[0].z);

        // Interpolate along x
        double c00 = points[0].w * (1.0-xd) + points[1].w * xd;
        double c10 = points[2].w * (1.0-xd) + points[3].w * xd;
        double c01 = points[4].w * (1.0-xd) + points[5].w * xd;
        double c11 = points[6].w * (1.0-xd) + points[7].w * xd;

        // Interpolate along y
        double c0 = c00 * (1.0 - yd) + c10 * yd;
        double c1 = c01 * (1.0 - yd) + c11 * yd;

        // Interpolate along z
        double c = c0 * (1.0 - zd) + c1 * zd;

//        if (p_is_leaf && (p_id_node_x != 0) && (p_id_node_y != 0) && (p_id_node_z != 0))
//        {
//            qDebug() << "hi";
//        }

        return c;
    }
    // Else if branch, descend to the next level
    else
    {
        int id = ntant(x,y,z, 2);
        return p_children[id].gridValueAt_Linear(x, y, z, max_level, root);
    }
}


int SearchNode::nodesPerSide()
{
    return (1 << p_level);
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
    // If empty, with no relevant data
    // If leaf, use linear interpolation of points in bins to make grid
    if (p_is_leaf)
    {
        // Optionally try empty leaves as well, marking them as nonempty if the linear interpolation results in a nonzero grid
        makeGridFromBins_Linear();
    }
    // Else (if branch), reconstruct from children
    else
    {
        // Clear bins of children, as the data will not be used any further
        for (int i = 0; i < p_children.size(); i++)
        {
            p_children[i].bins().clear();
        }

        // Construct a grid based on child nodes
//        makeGridFromChildren();
    }
    p_is_empty = false;



//    // Non-recursively generate a point cloud on an irregular grid based on the child nodes
//    if(p_is_leaf) return;

//    // Get the relevant data points, and place them in bins
//    p_data_binned.resize(p_bins_per_side*p_bins_per_side*p_bins_per_side);

//    for (int i = 0; i < 8; i++)
//    {
//        if (p_children[i].isEmpty()) continue;

//        for (int j = 0; j < p_children[i].bins().size(); j++)
//        {
//            for (int k = 0; k < p_children[i].bins()[j].size(); k++)
//            {
//                int id = ntant(p_children[i].bins()[j][k], p_bins_per_side);
//                p_data_binned[id] << p_children[i].bins()[j][k];
//            }
//        }
//    }

//    rebin();

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

void SearchNode::nodelist(QVector<SearchNode*> &nodes, bool unresolved_leaves_only)
{
    // Add self to list
    if (unresolved_leaves_only)
    {
        if (p_is_leaf && !p_is_max_resolved && !p_is_empty) nodes << this;
    }
    else
    {
        nodes << this;
    }

    // Add children to list
    if(!p_is_leaf)
    {
        // Call for children
        for (int i = 0; i < 8; i++)
        {
            p_children[i].nodelist(nodes, unresolved_leaves_only);
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
    if (!p_is_empty || p_is_interpolation_node)
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


void SearchNode::nodeToPool(QVector<float> & pool, int dim_x, int dim_y, int dim_z, SearchNode & root)
{
    if (p_is_empty) return;

    // Return a brick corresponding to the extent of the node
    int brick_side = 8;

//    QVector<float> tmp(brick_side*brick_side*brick_side);

    for (int i = 0; i < brick_side; i++)
    {
        for (int j = 0; j < brick_side; j++)
        {
            for (int k = 0; k < brick_side; k++)
            {
//                int id_tmp =
//                        (i) +
//                        (j) * ( brick_side)  +
//                        (k) * ( brick_side * brick_side);

                // The linear id the current value should have in the brick pool
                int id_pool =
                    (p_id_pool_x * brick_side + i) +
                    (p_id_pool_y * brick_side + j) * (dim_x * brick_side)  +
                    (p_id_pool_z * brick_side + k) * (dim_x * brick_side * dim_y * brick_side);

                // Do a lookup of the value
                double x = (double) (p_id_node_x + ((double)i/(double)(brick_side-1))) * side();// / (double) (nodesPerSide() * p_bins_per_side);
                double y = (double) (p_id_node_y + ((double)j/(double)(brick_side-1))) * side();// / (double) (nodesPerSide() * p_bins_per_side);
                double z = (double) (p_id_node_z + ((double)k/(double)(brick_side-1))) * side();// / (double) (nodesPerSide() * p_bins_per_side);

                // If requested value lies outside of octtree, set to zero
                if ((x < 0) || (x >= 1) || (y < 0) || (y >= 1) || (z < 0) || (z >= 1))
                {
                    pool[id_pool] = 0;
//                    tmp[id_tmp] = 0;
//                        qDebug() << "Requested value lies outside of octtree" << x << y << z;
                }
                // Else use grid value
                else
                {
//                    tmp[id_tmp] = root.gridValueAt_Linear(x, y, z, p_level, root);

//                    double intetrpol_w = tmp[id_tmp];
//                    double interpol_x = x;

//                    if ((intetrpol_w > 1.1*interpol_x) || (intetrpol_w < 0.9*interpol_x))
//                    {
//                        qDebug() << "le fail";
//                    }

                    pool[id_pool] = root.gridValueAt_Linear(x, y, z, p_level, root);
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
    // Place a data point into this node, or a deeper node if required
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

                if (data_interdist_min > 1.0 * voxelside())// /(double)(nodesPerSide() * p_bins_per_side))
                {
//                    qDebug() << "Rebinning because" << data_interdist_hint << ">" <<  1.0/(double)(nodesPerSide() * p_bins_per_side);
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


void SearchNode::interdistMetrics2(double & data_interdist_min, double & data_interdist_max, double & data_interdist_avg)
{
    data_interdist_min = 1e9;
    data_interdist_max = 0;
    data_interdist_avg = 0;

    int counter = 0;

    for (int i = 0; i < p_cloud.size(); i++)
    {
        if (p_cloud[i].d > data_interdist_max) data_interdist_max = p_cloud[i].d;
        if (p_cloud[i].d < data_interdist_min) data_interdist_min = p_cloud[i].d;
        data_interdist_avg += p_cloud[i].d;
        counter++;
    }

    if (counter > 0) data_interdist_avg /= (double) counter;
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
//    int id_x = (x * (double) nodesPerSide() - (double) p_id_x) * (double) n;
//    int id_y = (y * (double) nodesPerSide() - (double) p_id_y) * (double) n;
//    int id_z = (z * (double) nodesPerSide() - (double) p_id_z) * (double) n;

//    nodeinfo ni;
//    ni.level = p_level;
//    ni.id_x = p_id_x;
//    ni.id_y = p_id_y;
//    ni.id_z = p_id_z;
//    ni.side = nodesPerSide();
//    ni.id_nxt_x = (x * (double) nodesPerSide() - (double) p_id_x) * (double) n;
//    ni.id_nxt_y = (y * (double) nodesPerSide() - (double) p_id_y) * (double) n;
//    ni.id_nxt_z = (z * (double) nodesPerSide() - (double) p_id_z) * (double) n;
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
    int id_ntant_x = (x * (double) nodesPerSide() - (double) p_id_node_x) * (double) n;
    int id_ntant_y = (y * (double) nodesPerSide() - (double) p_id_node_y) * (double) n;
    int id_ntant_z = (z * (double) nodesPerSide() - (double) p_id_node_z) * (double) n;

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

            if (id >= 0)
            {
                p_children[id].insert(p_data_binned[i][j]);
            }
        }
    }
}
