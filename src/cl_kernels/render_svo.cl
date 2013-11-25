__kernel void svoRayTrace(
    __write_only image2d_t ray_tex,
    __read_only image2d_t tsf_tex,
    __read_only image3d_t bricks,
    __global uint * oct_index,
    __global uint * oct_brick,
    sampler_t brick_sampler,
    sampler_t tsf_sampler,
    __constant float * data_view_matrix,
    __constant float * data_extent,
    __constant float * data_view_extent,
    __constant float * tsf_var,
    __constant int * misc_int,
    __constant float * scalebar_rotation,
    __write_only image2d_t integration_tex
    )
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));

    int2 ray_tex_dim = get_image_dim(ray_tex);
    int2 tsf_tex_dim = get_image_dim(tsf_tex);
    int4 bricks_dim = get_image_dim(bricks);

    int numOctLevels = misc_int[0];
    int brickSize = misc_int[1];
    int islog3DActive = misc_int[2];
    int isDsActive = misc_int[3];
    int isSlicingActive = misc_int[4];
    int isIntegration2DActive = misc_int[5];

    float tsfOffsetLow = tsf_var[0];
    float tsfOffsetHigh = tsf_var[1];
    float data_offset_low = tsf_var[2];
    float data_offset_high = tsf_var[3];
    float alpha = tsf_var[4];
    float brightness = tsf_var[5];

    if (islog3DActive)
    {
        if (data_offset_low <= 0.0f) data_offset_low = 0.01f;
        if (data_offset_high <= 0.0f) data_offset_high = 0.01f;
        data_offset_low = log10(data_offset_low);
        data_offset_high = log10(data_offset_high);
    }

    // If the global id corresponds to a texel, then check if its associated ray hits our cubic bounding box. If it does - traverse along the intersecing ray segment and accumulate color
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        /*
         * Find the geometry of the ray
         * */
        float4 ray_near, ray_far;
        float3 ray_delta;
        float cone_diameter_increment;
        float cone_diameter_near;
        float integrated_intensity = 0.0f;
        {
            float4 rayNearEdge, rayFarEdge;
            float3 pixel_radius_near, pixel_radius_far;

            // Normalized device coordinates (ndc) of the pixel and its edge (in screen coordinates)
            float2 ndc = (float2)(2.0f * (( convert_float2(id_glb) + 0.5f)/convert_float2(ray_tex_dim)) - 1.0f);
            float2 ndc_edge = (float2)(2.0f * (( convert_float2(id_glb) + (float2)(1.0f, 1.0f))/convert_float2(ray_tex_dim)) - 1.0f);

            // Ray origin and exit point (screen coordinates)
            // z = 1 corresponds to far plane
            // z = -1 corresponds to near plane
            float4 ray_near_ndc = (float4)(ndc, -1.0f, 1.0f);
            float4 ray_far_ndc = (float4)(ndc, 1.0f, 1.0f);

            float4 ray_near_ndc_edge = (float4)(ndc_edge, -1.0f, 1.0f);
            float4 ray_far_ndc_edge = (float4)(ndc_edge, 1.0f, 1.0f);

            // Ray entry point at near and far plane
            ray_near = sc2xyz(data_view_matrix, ray_near_ndc);
            ray_far = sc2xyz(data_view_matrix, ray_far_ndc);
            rayNearEdge = sc2xyz(data_view_matrix, ray_near_ndc_edge);
            rayFarEdge = sc2xyz(data_view_matrix, ray_far_ndc_edge);

            ray_delta = ray_far.xyz - ray_near.xyz;
            pixel_radius_near = rayNearEdge.xyz - ray_near.xyz;
            pixel_radius_far = rayFarEdge.xyz - ray_far.xyz;

            // The ray is treated as a cone of a certain diameter. In a perspective projection, this diameter typically increases along the direction of ray propagation. We calculate the diameter width incrementation per unit length by rejection of the pixel_radius vector onto the central ray_delta vector
            float3 a1Near = native_divide(dot(pixel_radius_near, ray_delta),dot(ray_delta,ray_delta))*ray_delta;
            float3 a2Near = pixel_radius_near - a1Near;

            float3 a1Far = native_divide(dot(pixel_radius_far, ray_delta),dot(ray_delta,ray_delta))*ray_delta;
            float3 a2Far = pixel_radius_far - a1Far;

            // The geometry of the cone
            cone_diameter_increment = 2.0f*native_divide( length(a2Far - a2Near), length(ray_delta - a1Near + a1Far) );
            cone_diameter_near = 2.0f*length(a2Near); // small approximation
        }

        // To limit resource spending we limit the ray to the intersection between itself and a bounding box
        int hit;
        float t_near, t_far;
        {
            // Construct a bounding box from the intersect between data_view_extent and data_extent
            float bbox[6];

            bbox[0] = fmax(data_extent[0],data_view_extent[0]);
            bbox[1] = fmin(data_extent[1],data_view_extent[1]);
            bbox[2] = fmax(data_extent[2],data_view_extent[2]);
            bbox[3] = fmin(data_extent[3],data_view_extent[3]);
            bbox[4] = fmax(data_extent[4],data_view_extent[4]);
            bbox[5] = fmin(data_extent[5],data_view_extent[5]);

            // Does the ray intersect?
            if (!((bbox[0] >= bbox[1]) || (bbox[2] >= bbox[3]) || (bbox[4] >= bbox[5])))
            {
                hit = boundingBoxIntersect(ray_near.xyz, ray_delta.xyz, bbox, &t_near, &t_far);
            }
        }
        float4 sample = (float4)(0.0f);
        float4 color = (float4)(0.0f);
        float step_length = 1.0f;
        
        // In the case that the ray actually hits the bounding box, prepare for volume sampling and color accumulation
        if(hit)
        {
            // The geometry of the intersecting part of the ray
            float3 rayBoxOrigin = ray_near.xyz + t_near * ray_delta.xyz;
            float3 rayBoxEnd = ray_near.xyz + t_far * ray_delta.xyz;
            float3 rayBoxDelta = rayBoxEnd - rayBoxOrigin;

            float3 direction = normalize(rayBoxDelta);

            // Some variables we will need during ray traversal
            float skip_length, intensity, voxelSize, voxelSizeUp;
            float cone_diameter;
            float cone_diameter_low = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (numOctLevels-1))));
            float cone_diameter_high = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (0))));
            uint index, index_prev, brick, isMsd, isLowEnough, isEmpty;
            float3 ray_xyz_box, rayBoxXyzPrev, ray_add_box;
            float3 norm_xyz, norm_xyz_prev;
            float3 tmp_a, tmp_b;
            float4 lookup_pos;
            uint4 brickId;
            int3 normIndex;

            // The traversal coordinate. We keep track of the previous position as well
            ray_xyz_box = rayBoxOrigin;
            rayBoxXyzPrev = rayBoxOrigin;

            // Merged bits in the octtree can be read using these bitmasks:
            uint mask_msd_flag = ((1u << 1u) - 1u) << 31u;
            uint mask_data_flag = ((1 << 1) - 1) << 30;
            uint mask_child_index = ((1 << 30) - 1) << 0;
            uint mask_brick_id_x = ((1 << 10) - 1) << 20;
            uint mask_brick_id_y = ((1 << 10) - 1) << 10;
            uint mask_brick_id_z = ((1 << 10) - 1) << 0;

            /* Traverse the octtree. For each step, descend into the octree until a) The resolution is appreciable or b) The final level of the octree is reached. During any descent, empty nodes might be found. In such case, the ray is advanced forward without sampling to the next sample that is not in said node. Stuff inside this while loop is what really takes time and therefore should be optimized
             * */
            
            if (isSlicingActive)
            {
                // Ray-plane intersection
                float4 center = (float4)(
                    data_view_extent[0] + 0.5*(data_view_extent[1] - data_view_extent[0]),
                    data_view_extent[2] + 0.5*(data_view_extent[3] - data_view_extent[2]),
                    data_view_extent[4] + 0.5*(data_view_extent[5] - data_view_extent[4]),
                    0);

                // Plane normals
                float4 normal[3];
                normal[0] = (float4)(1.0f, 0.0f, 0.0f, 0.0f);
                normal[1] = (float4)(0.0f, 1.0f, 0.0f, 0.0f);
                normal[2] = (float4)(0.0f, 0.0f, 1.0f, 0.0f);

                float d[3];

                for (int i = 0; i < 3; i++)
                {
                    // Rotate plane normals in accordance with the relative scalebar
                    normal[i] = matrixMultiply4x4X1x4(scalebar_rotation, normal[i]);

                    // Compute number of meaningful intersections (i.e. not parallel to plane, intersection, and within bounding box)
                    float nominator = dot(center - (float4)(rayBoxOrigin, 0.0f), normal[i]);
                    float denominator = dot((float4)(rayBoxDelta, 0.0f), normal[i]);

                    if (denominator != 0.0f)  d[i] = nominator / denominator;
                    else d[i] = -1.0f;
                }


                // Sort intersections along ray
                float d_sorted[3];
                selectionSort(d, 3);

                // Accumulate color
                for (int i = 0; i < 3; i++)
                {
                    if ((d[i] >= 0.0f) && (d[i] <= 1.0f))
                    {
                        ray_xyz_box = rayBoxOrigin + d[i] * rayBoxDelta;

                        // Descend into the octtree data structure
                        // Index trackers for the traversal.
                        index = 0;
                        index_prev = 0;

                        // Calculate the cone diameter at the current ray position
                        cone_diameter = (cone_diameter_near + length(ray_xyz_box - ray_near.xyz) * cone_diameter_increment);
                        cone_diameter = clamp(cone_diameter, cone_diameter_low, cone_diameter_high);

                        // The step length is chosen such that there is roughly a set number of samples (4) per voxel. This number changes based on the pseudo-level the ray is currently traversing (interpolation between two octtree levels)
                        step_length = cone_diameter * 0.25f;
                        ray_add_box = direction * step_length;

                         // We use a normalized convention during octtree traversal. The normalized convention makes it easier to think about the octtree traversal.
                        norm_xyz = native_divide( (float3)(ray_xyz_box.x - data_extent[0], ray_xyz_box.y - data_extent[2], ray_xyz_box.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;

                        normIndex = convert_int3(norm_xyz);
                        normIndex = clamp(normIndex, 0, 1);

                        // Traverse the octtree
                        for (int j = 0; j < numOctLevels; j++)
                        {
                            voxelSize = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << j)));
                            if (j > 0) voxelSizeUp = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (j-1))));

                            isMsd = (oct_index[index] & mask_msd_flag) >> 31;
                            isEmpty = !((oct_index[index] & mask_data_flag) >> 30);
                            isLowEnough = (cone_diameter > voxelSize);

                            if (isEmpty)
                            {
                                // Skip forward by calculating how many steps can be advanced before reaching the next node. This is done by finding the intersect between the ray and a box of sides two. The number of steps to increment by is readily given by the length of the corresponding ray segment;

                                if (isDsActive)
                                {
                                    sample = (float4)(1.0f,1.0f,1.0f, 0.5f);
                                    color.xyz = color.xyz +(1.0f - color.w)*sample.xyz*sample.w;
                                    color.w = color.w +(1.0f - color.w)*sample.w;
                                }

                                // This ugly shit needs to go
                                tmp_a = norm_xyz - 5.0f*direction;
                                tmp_b = 15.0f*direction;
                                hit = boundingBoxIntersectNorm(tmp_a, tmp_b, &t_near, &t_far);

                                if (hit)
                                {
                                    break;
                                }
                            }
                            else if (isMsd || isLowEnough)
                            {
                                // Sample brick
                                if (isLowEnough && (j >= 1))
                                {
                                    /* Quadrilinear interpolation between two bricks */

                                    // The brick in the level above
                                    brick = oct_brick[index_prev];
                                    brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                                    lookup_pos = native_divide(0.5f + convert_float4(brickId * brickSize)  + (float4)(norm_xyz_prev, 0.0f)*3.5f , convert_float4(bricks_dim));

                                    float intensity_prev = read_imagef(bricks, brick_sampler, lookup_pos).w;

                                    // The brick in the current level
                                    brick = oct_brick[index];
                                    brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                                    lookup_pos = native_divide(0.5f + convert_float4(brickId * brickSize)  + (float4)(norm_xyz, 0.0f)*3.5f , convert_float4(bricks_dim));

                                    float intensity_now = read_imagef(bricks, brick_sampler, lookup_pos).w;

                                    // Linear interpolation between the two intensities
                                    intensity = intensity_prev + (intensity_now - intensity_prev)*native_divide(cone_diameter - voxelSizeUp, voxelSize - voxelSizeUp);
                                }
                                else
                                {
                                    brick = oct_brick[index];
                                    brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                                    lookup_pos = native_divide(0.5f + convert_float4(brickId * brickSize)  + (float4)(norm_xyz, 0.0f)*3.5f , convert_float4(bricks_dim));

                                    intensity = read_imagef(bricks, brick_sampler, lookup_pos).w;
                                }

                                if (isDsActive)
                                {

                                    if (j == 0) sample = (float4)(0.2f,0.3f,3.0f, 1.00f);
                                    else if (j == 1) sample = (float4)(1.0f,0.3f,0.2f, 1.00f);
                                    else if (j == 2) sample = (float4)(0.2f,1.0f,0.3f, 1.00f);
                                    else if (j == 3) sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                                    else if (j == 4) sample = (float4)(1.0f,0.3f,0.2f, 1.00f);
                                    else if (j == 5) sample = (float4)(0.2f,1.0f,0.3f, 1.00f);
                                    else if (j == 6) sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                                    else if (j == 7) sample = (float4)(1.0f,0.3f,0.2f, 1.00f);
                                    else if (j == 8) sample = (float4)(0.2f,1.0f,0.3f, 1.00f);
                                    else if (j == 9) sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                                    else if (j == 10) sample = (float4)(1.0f,0.3f,0.2f, 1.00f);
                                    else if (j == 11) sample = (float4)(0.2f,1.0f,0.3f, 1.00f);
                                    else if (j == 12) sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                                    else sample = (float4)(0.2f,0.3f,1.0f, 1.00f);

                                    color.xyz = color.xyz +(1.f - color.w)*sample.xyz*sample.w;
                                    color.w = color.w +(1.f - color.w)*sample.w;
                                    break;
                                }


                                // Sample color
                                if(islog3DActive)
                                {
                                    intensity = log10(intensity);
                                }
                                
                                float2 tsfPosition = (float2)(tsfOffsetLow + (tsfOffsetHigh - tsfOffsetLow) * ((intensity - data_offset_low)/(data_offset_high - data_offset_low)), 0.5f);
                                
                                sample = read_imagef(tsf_tex, tsf_sampler, tsfPosition);

                                color.xyz += (1.f - color.w)*sample.xyz*sample.w;
                                color.w += (1.f - color.w)*sample.w;

                                break;
                            }
                            else
                            {
                                // Save values from this level to enable quadrilinear interpolation between levels
                                index_prev = index;
                                norm_xyz_prev = norm_xyz;

                                // Descend to the next level
                                index = (oct_index[index] & mask_child_index);
                                index += normIndex.x + normIndex.y*2 + normIndex.z*4;

                                //norm_xyz = (norm_xyz - convert_float(normIndex))*2.0f;
                    norm_xyz = (norm_xyz - (float3)((float)normIndex.x, (float)normIndex.y, (float)normIndex.z))*2.0f;
                                normIndex = convert_int3(norm_xyz);
                                normIndex = clamp(normIndex, 0, 1);
                            }
                        }
                    }
                }
            }
            else
            {
                // Ray-volume intersection
                while ( fast_length(ray_xyz_box - rayBoxOrigin) < fast_length(rayBoxDelta) )
                {
                    // Break off early if the accumulated alpha is high enough
                    if (color.w > 0.995f) break;

                    // Index trackers for the traversal.
                    index = 0;
                    index_prev = 0;

                    // Calculate the cone diameter at the current ray position
                    cone_diameter = (cone_diameter_near + length(ray_xyz_box - ray_near.xyz) * cone_diameter_increment);
                    cone_diameter = clamp(cone_diameter, cone_diameter_low, cone_diameter_high);

                    // The step length is chosen such that there is roughly a set number of samples (4) per voxel. This number changes based on the pseudo-level the ray is currently traversing (interpolation between two octtree levels)
                    step_length = cone_diameter * 0.25f;
                    ray_add_box = direction * step_length;

                     // We use a normalized convention during octtree traversal. The normalized convention makes it easier to think about the octtree traversal.
                    norm_xyz = native_divide( (float3)(ray_xyz_box.x - data_extent[0], ray_xyz_box.y - data_extent[2], ray_xyz_box.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;

                    normIndex = convert_int3(norm_xyz);
                    normIndex = clamp(normIndex, 0, 1);

                    // Traverse the octtree
                    for (int j = 0; j < numOctLevels; j++)
                    {
                        voxelSize = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << j)));
                        if (j > 0) voxelSizeUp = (data_extent[1] - data_extent[0])/((float)((brickSize-1) * (1 << (j-1))));

                        isMsd = (oct_index[index] & mask_msd_flag) >> 31;
                        isEmpty = !((oct_index[index] & mask_data_flag) >> 30);
                        isLowEnough = (cone_diameter > voxelSize);

                        if (isEmpty)
                        {
                            // Skip forward by calculating how many steps can be advanced before reaching the next node. This is done by finding the intersect between the ray and a box of sides two. The number of steps to increment by is readily given by the length of the corresponding ray segment;

                            if (isDsActive)
                            {
                                sample = (float4)(1.0f,1.0f,1.0f, 0.08f);
                                color.xyz = color.xyz +(1.0f - color.w)*sample.xyz*sample.w;
                                color.w = color.w +(1.0f - color.w)*sample.w;
                            }

                            tmp_a = norm_xyz - 5.0f*direction;
                            tmp_b = 15.0f*direction;
                            hit = boundingBoxIntersectNorm(tmp_a, tmp_b, &t_near, &t_far);

                            if (hit)
                            {
                                skip_length =  ceil(native_divide(0.5f * fast_length((tmp_a + t_far*tmp_b) - norm_xyz) * (brickSize-1) * voxelSize, step_length))* step_length;
                                ray_xyz_box += skip_length * direction;
                                break;
                            }
                        }
                        else if (isMsd || isLowEnough)
                        {
                            // Sample brick
                            if (isDsActive)
                            {

                                if (j == 0) sample = (float4)(0.2f,0.3f,3.0f, 1.00f);
                                else if (j == 1) sample = (float4)(1.0f,0.3f,0.2f, 1.00f);
                                else if (j == 2) sample = (float4)(0.2f,1.0f,0.3f, 1.00f);
                                else if (j == 3) sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                                else if (j == 4) sample = (float4)(1.0f,0.3f,0.2f, 1.00f);
                                else if (j == 5) sample = (float4)(0.2f,1.0f,0.3f, 1.00f);
                                else if (j == 6) sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                                else if (j == 7) sample = (float4)(1.0f,0.3f,0.2f, 1.00f);
                                else if (j == 8) sample = (float4)(0.2f,1.0f,0.3f, 1.00f);
                                else if (j == 9) sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                                else if (j == 10) sample = (float4)(1.0f,0.3f,0.2f, 1.00f);
                                else if (j == 11) sample = (float4)(0.2f,1.0f,0.3f, 1.00f);
                                else if (j == 12) sample = (float4)(0.2f,0.3f,1.0f, 1.00f);
                                else sample = (float4)(0.2f,0.3f,1.0f, 1.00f);

                                color.xyz = color.xyz +(1.f - color.w)*sample.xyz*sample.w;
                                color.w = color.w +(1.f - color.w)*sample.w;
                                ray_xyz_box += ray_add_box;
                                break;
                            }

                            if (isLowEnough && (j >= 1))
                            {
                                /* Quadrilinear interpolation between two bricks */

                                // The brick in the level above
                                brick = oct_brick[index_prev];
                                brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                                lookup_pos = native_divide(0.5f + convert_float4(brickId * brickSize)  + (float4)(norm_xyz_prev, 0.0f)*3.5f , convert_float4(bricks_dim));

                                float intensity_prev = read_imagef(bricks, brick_sampler, lookup_pos).w;

                                // The brick in the current level
                                brick = oct_brick[index];
                                brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                                lookup_pos = native_divide(0.5f + convert_float4(brickId * brickSize)  + (float4)(norm_xyz, 0.0f)*3.5f , convert_float4(bricks_dim));

                                float intensity_now = read_imagef(bricks, brick_sampler, lookup_pos).w;

                                // Linear interpolation between the two intensities
                                intensity = intensity_prev + (intensity_now - intensity_prev)*native_divide(cone_diameter - voxelSizeUp, voxelSize - voxelSizeUp);
                            }
                            else
                            {
                                brick = oct_brick[index];
                                brickId = (uint4)((brick & mask_brick_id_x) >> 20, (brick & mask_brick_id_y) >> 10, brick & mask_brick_id_z, 0);

                                lookup_pos = native_divide(0.5f + convert_float4(brickId * brickSize)  + (float4)(norm_xyz, 0.0f)*3.5f , convert_float4(bricks_dim));

                                intensity = read_imagef(bricks, brick_sampler, lookup_pos).w;
                            }


                            // Sample color
//                            if (isIntegration2DActive && !isDsActive)
//                            {
                                integrated_intensity += intensity * step_length;
//                            }
//                            else
//                            {
                                if(islog3DActive)
                                {
                                    intensity = log10(intensity);
                                }
                            
                                float2 tsfPosition = (float2)(tsfOffsetLow + (tsfOffsetHigh - tsfOffsetLow) * ((intensity - data_offset_low)/(data_offset_high - data_offset_low)), 0.5f);
    
                                sample = read_imagef(tsf_tex, tsf_sampler, tsfPosition);
    
                                sample.w *= alpha*native_divide(cone_diameter, cone_diameter_low);
    
                                color.xyz += (1.f - color.w)*sample.xyz*sample.w;
                                color.w += (1.f - color.w)*sample.w;
//                            }
                            
                            ray_xyz_box += ray_add_box;
                            break;
                        }
                        else
                        {
                            // Save values from this level to enable quadrilinear interpolation between levels
                            index_prev = index;
                            norm_xyz_prev = norm_xyz;

                            // Descend to the next level
                            index = (oct_index[index] & mask_child_index);
                            index += normIndex.x + normIndex.y*2 + normIndex.z*4;

                            //norm_xyz = (norm_xyz - convert_float(normIndex))*2.0f;
                            norm_xyz = (norm_xyz - (float3)((float)normIndex.x, (float)normIndex.y, (float)normIndex.z))*2.0f;
                            normIndex = convert_int3(norm_xyz);
                            normIndex = clamp(normIndex, 0, 1);
                        }
                    }

                    // Help the ray progress in case it gets stuck between two bricks
                    if (fast_distance(ray_xyz_box, rayBoxXyzPrev) < step_length*0.5f) ray_xyz_box += ray_add_box;

                    rayBoxXyzPrev = ray_xyz_box;
                }
            }
            if (!isDsActive)color *= brightness;
        }
        write_imagef(integration_tex, id_glb, (float4)(integrated_intensity*cone_diameter_near));
        write_imagef(ray_tex, id_glb, clamp(color, 0.0f, 1.0f));
        
//        if (isIntegration2DActive && !isSlicingActive && !isDsActive)
//        {
//            if(islog3DActive) 
//            {
//                if (integrated_intensity < 1.f) integrated_intensity = 1.f;            
//                integrated_intensity = log10(integrated_intensity);
//            }
            
//            float2 tsfPosition = (float2)(tsfOffsetLow + (tsfOffsetHigh - tsfOffsetLow) * ((integrated_intensity - data_offset_low)/(data_offset_high - data_offset_low)), 0.5f);
    
//            sample = read_imagef(tsf_tex, tsf_sampler, tsfPosition);       
    
//            write_imagef(ray_tex, id_glb, clamp(sample, 0.0f, 1.0f));
//        }        
//        else
//        {
//            write_imagef(ray_tex, id_glb, clamp(color, 0.0f, 1.0f));
//        }
    }
}
