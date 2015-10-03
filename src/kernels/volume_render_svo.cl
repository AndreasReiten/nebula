float4 brickColor2(float value)
{
    return (float4)(0.0, 1.0f-value, value, 1.0f);
}

float4 brickColor(int value)
{
    if (value == 0)
    {
        return (float4)(0.2f, 0.3f, 3.0f, 1.00f);
    }
    else if (value == 1)
    {
        return (float4)(1.0f, 0.3f, 0.2f, 1.00f);
    }
    else if (value == 2)
    {
        return (float4)(0.2f, 1.0f, 0.3f, 1.00f);
    }
    else if (value == 3)
    {
        return (float4)(0.2f, 0.3f, 1.0f, 1.00f);
    }
    else if (value == 4)
    {
        return (float4)(1.0f, 0.3f, 0.2f, 1.00f);
    }
    else if (value == 5)
    {
        return (float4)(0.2f, 1.0f, 0.3f, 1.00f);
    }
    else if (value == 6)
    {
        return (float4)(0.2f, 0.3f, 1.0f, 1.00f);
    }
    else if (value == 7)
    {
        return (float4)(1.0f, 0.3f, 0.2f, 1.00f);
    }
    else if (value == 8)
    {
        return (float4)(0.2f, 1.0f, 0.3f, 1.00f);
    }
    else if (value == 9)
    {
        return (float4)(0.2f, 0.3f, 1.0f, 1.00f);
    }
    else if (value == 10)
    {
        return (float4)(1.0f, 0.3f, 0.2f, 1.00f);
    }
    else if (value == 11)
    {
        return (float4)(0.2f, 1.0f, 0.3f, 1.00f);
    }
    else if (value == 12)
    {
        return (float4)(0.2f, 0.3f, 1.0f, 1.00f);
    }
    else
    {
        return (float4)(0.2f, 0.3f, 1.0f, 1.00f);
    }
}

uint4 brickId(uint value)
{
    // Merged bits in the octree can be read using these bitmasks:
    uint mask_brick_id_x = ((1 << 10) - 1) << 20;
    uint mask_brick_id_y = ((1 << 10) - 1) << 10;
    uint mask_brick_id_z = ((1 << 10) - 1) << 0;

    return (uint4)((value & mask_brick_id_x) >> 20, (value & mask_brick_id_y) >> 10, value & mask_brick_id_z, 0);
}

uint isMsd(uint value)
{
    uint mask_msd_flag = ((1u << 1u) - 1u) << 31u;

    return (value & mask_msd_flag) >> 31;
}

uint isEmpty(uint value)
{
    uint mask_data_flag = ((1 << 1) - 1) << 30;

    return !((value & mask_data_flag) >> 30);
}

uint child(uint value)
{
    uint mask_child_index = ((1 << 30) - 1) << 0;

    return value & mask_child_index;
}


kernel void svoRayTrace(
    write_only image2d_t ray_tex,
    read_only image2d_t tsf_tex,
    read_only image3d_t bricks,
    global uint * oct_index,
    global uint * oct_brick,
    sampler_t brick_sampler,
    sampler_t tsf_sampler,
    constant float * data_view_matrix,
    constant float * data_extent,
    constant float * data_view_extent,
    constant float * tsf_var,
    constant int * misc_int,
    constant float * scalebar_rotation,
    write_only image2d_t integration_tex
//    constant uint * oct_index_const,
//    constant local uint * oct_brick_const,
//    int const_size;
)
{
    int2 id_glb = (int2)(get_global_id(0), get_global_id(1));

    int2 ray_tex_dim = get_image_dim(ray_tex);
    int2 tsf_tex_dim = get_image_dim(tsf_tex);
    int4 pool_dim = get_image_dim(bricks); //This might as well be stored as float

    int n_tree_levels = misc_int[0];
    int brick_dim = misc_int[1];
    int isLogActive = misc_int[2];
    int isDsActive = misc_int[3];
    int isSlicingActive = misc_int[4];
    int isIntegration2DActive = misc_int[5];
    int isIntegration3DActive = misc_int[7];

    float tsf_offset_low = tsf_var[0];
    float tsf_offset_high = tsf_var[1];
    float data_offset_low = tsf_var[2];
    float data_offset_high = tsf_var[3];
    float alpha = tsf_var[4];
    float brightness = tsf_var[5];

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
            float4 ray_near_corner, ray_far_corner;
            float3 pixel_radius_near, pixel_radius_far;

            // Normalized device coordinates (pixel_gl_screen_pos) of the pixel and its edge (in screen coordinates)
            float2 pixel_gl_screen_pos = (float2)(2.0f * (( convert_float2(id_glb) + 0.5f) / convert_float2(ray_tex_dim)) - 1.0f);
            float2 pixel_corner_gl_screen_pos = (float2)(2.0f * (( convert_float2(id_glb) + (float2)(1.0f, 1.0f)) / convert_float2(ray_tex_dim)) - 1.0f);

            // Ray origin and exit point (screen coordinates)
            // z = 1 corresponds to far plane
            // z = -1 corresponds to near plane
            float4 ray_near_ndc = (float4)(pixel_gl_screen_pos, -1.0f, 1.0f);
            float4 ray_far_ndc = (float4)(pixel_gl_screen_pos, 1.0f, 1.0f);

            float4 ray_near_ndc_corner = (float4)(pixel_corner_gl_screen_pos, -1.0f, 1.0f);
            float4 ray_far_ndc_corner = (float4)(pixel_corner_gl_screen_pos, 1.0f, 1.0f);

            // Ray entry point at near and far plane
            ray_near = glScreenPosToEuclidean(data_view_matrix, ray_near_ndc);
            ray_far = glScreenPosToEuclidean(data_view_matrix, ray_far_ndc);
            ray_near_corner = glScreenPosToEuclidean(data_view_matrix, ray_near_ndc_corner);
            ray_far_corner = glScreenPosToEuclidean(data_view_matrix, ray_far_ndc_corner);

            ray_delta = ray_far.xyz - ray_near.xyz;
            pixel_radius_near = ray_near_corner.xyz - ray_near.xyz;
            pixel_radius_far = ray_far_corner.xyz - ray_far.xyz;

            // The ray is treated as a cone of a certain diameter. In a perspective projection, this diameter typically increases along the direction of ray propagation. We calculate the diameter width incrementation per unit length by rejection of the pixel_radius vector onto the central ray_delta vector
            float3 a1_near = dot(pixel_radius_near, normalize(ray_delta)) * normalize(ray_delta);
            float3 a2_near = pixel_radius_near - a1_near;

            float3 a1_far = dot(pixel_radius_far, normalize(ray_delta)) * normalize(ray_delta);
            float3 a2_far = pixel_radius_far - a1_far;

            // The geometry of the cone
            cone_diameter_increment = 2.0f * ( length(a2_far - a2_near)/ length(ray_delta - a1_near + a1_far) );
            cone_diameter_near = 2.0f * length(a2_near) - cone_diameter_increment*length(a1_near);
        }

        // To limit resource spending we limit the ray to the intersection between itself and a bounding box
        int hit = 0;
        float t_near, t_far;
        {
            float bbox[6];

            // Does the viewable bounding box intersect the extent of the data?
            if (boxIntersect(bbox, data_extent, data_view_extent))
            {
                // Does the ray intersect the viewable bounding box?
                hit = boundingBoxIntersect(ray_near.xyz, ray_delta.xyz, bbox, &t_near, &t_far);
            }
        }
        float4 sample = (float4)(0.0f);
        float4 color = (float4)(0.0f);
        float step_length = 1.0f;

        // In the case that the ray actually hits the bounding box, prepare for volume sampling and color accumulation
        if (hit)
        {
            // The geometry of the intersecting part of the ray
            float3 box_ray_origin = ray_near.xyz + t_near * ray_delta.xyz;
            float3 box_ray_end = ray_near.xyz + t_far * ray_delta.xyz;
            float3 box_ray_delta = box_ray_end - box_ray_origin;

            float3 direction = normalize(box_ray_delta);

            // Some variables we will need during ray traversal
            float skip_length, intensity, voxel_size_this_lvl, voxel_size_prev_lvl;
            float cone_diameter;
            float cone_diameter_low = (data_extent[1] - data_extent[0]) / ((float)((brick_dim - 1) * (1 << (n_tree_levels - 1))));
            float cone_diameter_high = (data_extent[1] - data_extent[0]) / ((float)((brick_dim - 1) * (1 << (0))));
            uint index_this_lvl, index_prev_lvl, brick, is_msd, is_low_enough, is_empty;
            float3 box_ray_xyz, box_ray_xyz_prev, ray_add_box;
            float3 norm_pos_this_lvl, norm_pos_prev_lvl;
            float3 tmp_a, tmp_b;
            float4 lookup_pos;
            uint4 brick_id;
            int3 norm_index;
            float intensity_this_lvl, intensity_prev_lvl;

            // The traversal coordinate. We keep track of the previous position as well
            box_ray_xyz = box_ray_origin;
            box_ray_xyz_prev = box_ray_origin;

            /* Traverse the octree. For each step, descend into the octree until a) The resolution is appreciable or b) The final level of the octree is reached. During any descent, empty nodes might be found. In such case, the ray is advanced forward without sampling to the next sample that is not in said node. Stuff inside this while loop is what really takes time and therefore should be optimized
             * */

            if (isSlicingActive)
            {
                // Ray-plane intersection
                float4 center = (float4)(
                                    data_view_extent[0] + 0.5f * (data_view_extent[1] - data_view_extent[0]),
                                    data_view_extent[2] + 0.5f * (data_view_extent[3] - data_view_extent[2]),
                                    data_view_extent[4] + 0.5f * (data_view_extent[5] - data_view_extent[4]),
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
                    float nominator = dot(center - (float4)(box_ray_origin, 0.0f), normal[i]);
                    float denominator = dot((float4)(box_ray_delta, 0.0f), normal[i]);

                    if (denominator != 0.0f)
                    {
                        d[i] = nominator / denominator;
                    }
                    else
                    {
                        d[i] = -1.0f;
                    }
                }


                // Sort intersections along ray
                float d_sorted[3];
                selectionSort(d, 3);

                // Accumulate color
                for (int i = 0; i < 3; i++)
                {
                    if ((d[i] >= 0.0f) && (d[i] <= 1.0f))
                    {
                        box_ray_xyz = box_ray_origin + d[i] * box_ray_delta;

                        // Descend into the octree data structure
                        // Index trackers for the traversal.
                        index_this_lvl = 0;
                        index_prev_lvl = 0;

                        // Calculate the cone diameter at the current ray position
                        cone_diameter = (cone_diameter_near + length(box_ray_xyz - ray_near.xyz) * cone_diameter_increment);
                        cone_diameter = clamp(cone_diameter, cone_diameter_low, cone_diameter_high);

                        // The step length is chosen such that there is roughly a set number of samples (4) per voxel. This number changes based on the pseudo-level the ray is currently traversing (interpolation between two octree levels)
                        step_length = cone_diameter * 0.25f;
                        ray_add_box = direction * step_length;

                        // We use a normalized convention during octree traversal. The normalized convention makes it easier to think about the octree traversal.
                        norm_pos_this_lvl = native_divide( (float3)(box_ray_xyz.x - data_extent[0], box_ray_xyz.y - data_extent[2], box_ray_xyz.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;

                        norm_index = convert_int3(norm_pos_this_lvl);
                        norm_index = clamp(norm_index, 0, 1);

                        // Traverse the octree
                        for (int j = 0; j < n_tree_levels; j++)
                        {
                            voxel_size_this_lvl = (data_extent[1] - data_extent[0]) / ((float)((brick_dim - 1) * (1 << j)));

                            if (j > 0)
                            {
                                voxel_size_prev_lvl = (data_extent[1] - data_extent[0]) / ((float)((brick_dim - 1) * (1 << (j - 1))));
                            }

                            brick = oct_index[index_this_lvl];
                            is_msd = isMsd(brick);
                            is_empty = isEmpty(brick);
                            is_low_enough = (cone_diameter > voxel_size_this_lvl);

                            if (is_empty)
                            {
                                // Skip forward by calculating how many steps can be advanced before reaching the next node. This is done by finding the intersect between the ray and a box of sides two. The number of steps to increment by is readily given by the length of the corresponding ray segment;

                                if (isDsActive)
                                {
                                    sample = (float4)(1.0f, 1.0f, 1.0f, 0.5f);
                                    color.xyz = color.xyz + (1.0f - color.w) * sample.xyz * sample.w;
                                    color.w = color.w + (1.0f - color.w) * sample.w;
                                }

                                // This ugly shit needs to go
                                tmp_a = norm_pos_this_lvl - 5.0f * direction;
                                tmp_b = 15.0f * direction;
                                hit = boundingBoxIntersectNorm(tmp_a, tmp_b, &t_near, &t_far);

                                if (hit)
                                {
                                    break;
                                }
                            }
                            else if (is_msd || is_low_enough)
                            {
                                // Sample brick
                                if (is_low_enough && (j >= 1))
                                {
                                    /* Quadrilinear interpolation between two bricks */

                                    // The brick in the level above
                                    brick_id = brickId(oct_brick[index_prev_lvl]);

                                    lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_prev_lvl, 0.0f) * 3.5f , convert_float4(pool_dim));

                                    intensity_prev_lvl = read_imagef(bricks, brick_sampler, lookup_pos).w;

                                    // The brick in the current level
                                    brick_id = brickId(oct_brick[index_this_lvl]);

                                    lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_this_lvl, 0.0f) * 3.5f , convert_float4(pool_dim));

                                    intensity_this_lvl = read_imagef(bricks, brick_sampler, lookup_pos).w;

                                    // Linear interpolation between the two intensities
                                    intensity = intensity_prev_lvl + (intensity_this_lvl - intensity_prev_lvl) * native_divide(cone_diameter - voxel_size_prev_lvl, voxel_size_this_lvl - voxel_size_prev_lvl);
                                }
                                else
                                {
                                    brick_id = brickId(oct_brick[index_this_lvl]);

                                    lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_this_lvl, 0.0f) * 3.5f , convert_float4(pool_dim));

                                    intensity = read_imagef(bricks, brick_sampler, lookup_pos).w;
                                }

                                if (isDsActive)
                                {
                                    sample = brickColor(j);

                                    float dist = fast_length(norm_pos_this_lvl - (float3)(1.0f));
                                    sample = mix(sample, (float4)(0.0f, 0.0f, 0.0f, 1.0f), dist * dist - 0.7f);


                                    color.xyz = color.xyz + (1.f - color.w) * sample.xyz * sample.w;
                                    color.w = color.w + (1.f - color.w) * sample.w;
                                    break;
                                }


                                // Sample color
                                float2 tsf_position;

                                if (isLogActive)
                                {
                                    float value = log10(max(intensity + 1.0f - data_offset_low, 1.0f)) / log10(data_offset_high - data_offset_low + 1.0f);

                                    if (value >= 0.0)
                                    {
                                        tsf_position = (float2)((tsf_offset_low + (tsf_offset_high - tsf_offset_low) * value), 0.5f);
                                    }

                                    else
                                    {
                                        tsf_position = (float2)(tsf_offset_low, 0.5f);
                                    }
                                }
                                else
                                {
                                    tsf_position = (float2)(tsf_offset_low + (tsf_offset_high - tsf_offset_low) * ((intensity - data_offset_low) / (data_offset_high - data_offset_low)), 0.5f);
                                }

                                sample = read_imagef(tsf_tex, tsf_sampler, tsf_position);

                                //sample = read_imagef(tsf_tex, tsf_sampler, tsfPos(intensity, data_offset_low, data_offset_high, tsf_offset_low, tsf_offset_high, isLogActive, 1.0e1, 1.0));


//                                sample = read_imagef(tsf_tex, tsf_sampler, tsfPos3(intensity, data_offset_low, data_offset_high, isLogActive));

                                color.xyz += (1.f - color.w) * sample.xyz * sample.w;
                                color.w += (1.f - color.w) * sample.w;

                                break;
                            }
                            else
                            {
                                // Save values from this level to enable quadrilinear interpolation between levels
                                index_prev_lvl = index_this_lvl;
                                norm_pos_prev_lvl = norm_pos_this_lvl;

                                // Descend to the next level
                                index_this_lvl = child(brick);
                                index_this_lvl += norm_index.x + norm_index.y * 2 + norm_index.z * 4;

                                //norm_pos_this_lvl = (norm_pos_this_lvl - convert_float(norm_index))*2.0f;
                                norm_pos_this_lvl = (norm_pos_this_lvl - (float3)((float)norm_index.x, (float)norm_index.y, (float)norm_index.z)) * 2.0f;
                                norm_index = convert_int3(norm_pos_this_lvl);
                                norm_index = clamp(norm_index, 0, 1);
                            }
                        }
                    }
                }
            }
            else
            {
                // Ray-volume intersection
                while ( fast_length(box_ray_xyz - box_ray_origin) < fast_length(box_ray_delta) )
                {
                    // Break off early if the accumulated alpha is high enough
                    if (!isIntegration3DActive)
                    {
                        if (color.w > 0.995f)
                        {
                            break;
                        }
                    }

                    // Index trackers for the traversal.
                    index_this_lvl = 0;
                    index_prev_lvl = 0;

                    // Calculate the cone diameter at the current ray position
                    cone_diameter = (cone_diameter_near + length(box_ray_xyz - ray_near.xyz) * cone_diameter_increment);
                    cone_diameter = clamp(cone_diameter, cone_diameter_low, cone_diameter_high);

                    // The step length is chosen such that there is roughly a set number of samples (4) per voxel. This number changes based on the pseudo-level the ray is currently traversing (interpolation between two octree levels)
                    step_length = cone_diameter * 0.25f;
                    ray_add_box = direction * step_length;

                    // We use a normalized convention. The normalized convention makes it easier to think about the octree traversal.
                    norm_pos_this_lvl = native_divide( (float3)(box_ray_xyz.x - data_extent[0], box_ray_xyz.y - data_extent[2], box_ray_xyz.z - data_extent[4]), (float3)(data_extent[1] - data_extent[0], data_extent[3] - data_extent[2], data_extent[5] - data_extent[4])) * 2.0f;

                    norm_index = convert_int3(norm_pos_this_lvl);
                    norm_index = clamp(norm_index, 0, 1);

                    // Traverse the octree
                    for (int j = 0; j < n_tree_levels; j++)
                    {
                        voxel_size_this_lvl = (data_extent[1] - data_extent[0]) / ((float)((brick_dim - 1) * (1 << j)));

                        if (j > 0)
                        {
                            voxel_size_prev_lvl = (data_extent[1] - data_extent[0]) / ((float)((brick_dim - 1) * (1 << (j - 1))));
                        }

                        brick = oct_index[index_this_lvl];

                        is_msd = isMsd(brick);
                        is_empty = isEmpty(brick);
                        is_low_enough = (cone_diameter > voxel_size_this_lvl);

                        if (is_msd || is_low_enough || is_empty)
                        {
                            // Sample brick
                            if (isDsActive)
                            {
                                /* Color the data structure*/
                                if (!is_empty)
                                {
                                    sample = brickColor(j);

                                    float dist = fast_length(norm_pos_this_lvl - (float3)(1.0f));
                                    sample = mix(sample, (float4)(0.0f, 0.0f, 0.0f, 1.0f), dist * dist - 0.7f);

                                    color.xyz = color.xyz + (1.f - color.w) * sample.xyz * sample.w;
                                    color.w = color.w + (1.f - color.w) * sample.w;
                                    box_ray_xyz += ray_add_box;
                                }
                                else
                                {
                                    sample = (float4)(1.0f, 1.0f, 1.0f, 0.08f);
                                    color.xyz = color.xyz + (1.0f - color.w) * sample.xyz * sample.w;
                                    color.w = color.w + (1.0f - color.w) * sample.w;
                                }
                            }
                            else if (j >= 1)
                            {
                                /* Quadrilinear interpolation between two bricks */

                                // The brick in the level above
                                brick_id = brickId(oct_brick[index_prev_lvl]);

                                lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_prev_lvl, 0.0f) * 3.5f , convert_float4(pool_dim));
                                intensity_prev_lvl = read_imagef(bricks, brick_sampler, lookup_pos).w;

                                // The brick in the current level
                                if (is_empty)
                                {
                                    intensity_this_lvl = 0;
                                }
                                else
                                {
                                    brick_id = brickId(oct_brick[index_this_lvl]);

                                    lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_this_lvl, 0.0f) * 3.5f , convert_float4(pool_dim));
                                    intensity_this_lvl = read_imagef(bricks, brick_sampler, lookup_pos).w;
                                }

                                // Linear interpolation between the two intensities
                                clamp(cone_diameter, voxel_size_this_lvl, voxel_size_prev_lvl);

                                intensity = intensity_prev_lvl + (intensity_this_lvl - intensity_prev_lvl) * native_divide(cone_diameter - voxel_size_prev_lvl, voxel_size_this_lvl - voxel_size_prev_lvl);
                            }
                            else
                            {
                                brick_id = brickId(oct_brick[index_this_lvl]);

                                lookup_pos = native_divide(0.5f + convert_float4(brick_id * brick_dim)  + (float4)(norm_pos_this_lvl, 0.0f) * 3.5f , convert_float4(pool_dim));

                                intensity = read_imagef(bricks, brick_sampler, lookup_pos).w;
                            }


                            integrated_intensity += intensity * step_length;

                            if (!isIntegration3DActive)
                            {
                                float2 tsf_position;

                                if (isLogActive)
                                {
                                    float value = log10(max(intensity + 1.0f - data_offset_low, 1.0f)) / log10(data_offset_high - data_offset_low + 1.0f);

                                    if (value >= 0.0f)
                                    {
                                        tsf_position = (float2)((tsf_offset_low + (tsf_offset_high - tsf_offset_low) * value), 0.5f);
                                    }

                                    else
                                    {
                                        tsf_position = (float2)(tsf_offset_low, 0.5f);
                                    }
                                }
                                else
                                {
                                    tsf_position = (float2)(tsf_offset_low + (tsf_offset_high - tsf_offset_low) * ((intensity - data_offset_low) / (data_offset_high - data_offset_low)), 0.5f);
                                }

                                sample = read_imagef(tsf_tex, tsf_sampler, tsf_position);

//                                sample = read_imagef(tsf_tex, tsf_sampler, tsfPos3(integrated_intensity, data_offset_low, data_offset_high, isLogActive, 1, 0, 1.0e6f));

                                //                                sample = read_imagef(tsf_tex, tsf_sampler, tsfPos(integrated_intensity, data_offset_low, data_offset_high, tsf_offset_low, tsf_offset_high, isLogActive, 10, 1.0));

//                                sample = read_imagef(tsf_tex, tsf_sampler, tsfPos3(integrated_intensity, data_offset_low, data_offset_high, isLogActive));
                                sample.w *= alpha * native_divide(cone_diameter, cone_diameter_low);

                                color.xyz += (1.f - color.w) * sample.xyz * sample.w;
                                color.w += (1.f - color.w) * sample.w;
                            }


                            /* Traverse to the next sample along the ray */
                            if (is_empty)
                            {
                                // Skip forward by calculating how many steps can be advanced before reaching the next node. This is done by finding the intersect between the ray and a box of sides two. The number of steps to increment by is readily given by the length of the corresponding ray segment;

                                tmp_a = norm_pos_this_lvl - 5.0f * direction;
                                tmp_b = 15.0f * direction;
                                hit = boundingBoxIntersectNorm(tmp_a, tmp_b, &t_near, &t_far);

                                if (hit)
                                {
                                    skip_length =  ceil(native_divide(0.5f * fast_length((tmp_a + t_far * tmp_b) - norm_pos_this_lvl) * (brick_dim - 1) * voxel_size_this_lvl, step_length)) * step_length;
                                    box_ray_xyz += skip_length * direction;
                                }
                            }
                            else
                            {
                                box_ray_xyz += ray_add_box;
                            }

                            break;
                        }
                        else // Descend to the next level
                        {
                            // Save values from this level to enable quadrilinear interpolation between levels
                            index_prev_lvl = index_this_lvl;
                            norm_pos_prev_lvl = norm_pos_this_lvl;

                            index_this_lvl = child(oct_index[index_this_lvl]);
                            index_this_lvl += norm_index.x + norm_index.y * 2 + norm_index.z * 4;

                            //norm_pos_this_lvl = (norm_pos_this_lvl - convert_float(norm_index))*2.0f;
                            norm_pos_this_lvl = (norm_pos_this_lvl - (float3)((float)norm_index.x, (float)norm_index.y, (float)norm_index.z)) * 2.0f;
                            norm_index = convert_int3(norm_pos_this_lvl);
                            norm_index = clamp(norm_index, 0, 1);
                        }
                    }

                    // Help the ray progress in case it gets stuck between two bricks
                    if (fast_distance(box_ray_xyz, box_ray_xyz_prev) < step_length * 0.5f)
                    {
                        box_ray_xyz += ray_add_box;
                    }

                    box_ray_xyz_prev = box_ray_xyz;
                }
            }

            if (!isDsActive)
            {
                color *= brightness;
            }
        }

        write_imagef(integration_tex, id_glb, (float4)(integrated_intensity * cone_diameter_near));

        if (isIntegration3DActive  && !isSlicingActive && !isDsActive)
        {
//            sample = read_imagef(tsf_tex, tsf_sampler, tsfPos2(integrated_intensity, data_offset_low, data_offset_high, tsf_offset_low, tsf_offset_high, isLogActive, 1.0e6f, 0.0f));
            sample = read_imagef(tsf_tex, tsf_sampler, tsfPos3(integrated_intensity, data_offset_low, data_offset_high, isLogActive, 1, 0, 1.0e6f));
            write_imagef(ray_tex, id_glb, clamp(sample, 0.0f, 1.0f)); // Can be multiplied by brightness
        }
        else
        {
            write_imagef(ray_tex, id_glb, clamp(color, 0.0f, 1.0f));
        }
    }
}

