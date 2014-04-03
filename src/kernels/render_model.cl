float model(float3 k, __constant float * param)
{
    // Calculate the model
    float pi = 3.14159; 
//float pi = 0.5;


    float a = param[0] * (2.0f - native_cos(k.x*2*pi)*(native_cos(k.y*2*pi) + native_cos(k.z*2*pi))) + (2.0f*param[2] - param[0])*(1.0f - native_cos(k.y*2*pi)*native_cos(k.z*2*pi));

    float b = param[0] * (2.0f - native_cos(k.y*2*pi)*(native_cos(k.x*2*pi) + native_cos(k.z*2*pi))) + (2.0f*param[2] - param[0])*(1.0f - native_cos(k.x*2*pi)*native_cos(k.z*2*pi));

    float c = param[0] * (2.0f - native_cos(k.z*2*pi)*(native_cos(k.y*2*pi) + native_cos(k.x*2*pi))) + (2.0f*param[2] - param[0])*(1.0f - native_cos(k.y*2*pi)*native_cos(k.x*2*pi));

    float d = (param[1] + param[2]) * native_sin(k.x*2*pi) * native_sin(k.y*2*pi);

    float e = (param[1] + param[2]) * native_sin(k.z*2*pi) * native_sin(k.y*2*pi);

    float f = (param[1] + param[2]) * native_sin(k.x*2*pi) * native_sin(k.z*2*pi);


    float3 Ak = (float3)(
        (-k.x*e*e + f*k.y*e + d*k.z*e + b*c*k.x - c*d*k.y - b*f*k.z),
        (-k.y*f*f + e*k.x*f + d*k.z*f - c*d*k.x + a*c*k.y - a*e*k.z),
        (-k.z*d*d + e*k.x*d + f*k.y*d - b*f*k.x - a*e*k.y + a*b*k.z));

    return exp(-2.0f*(k.x*k.x + k.y*k.y + k.z*k.z)*param[3])*native_divide(1.0f,(a*b*c - a*e*e - b*f*f - c*d*d + 2.0f*d*e*f))*dot(k, Ak);
}


__kernel void modelRayTrace(
    __write_only image2d_t ray_tex,
    __read_only image2d_t tsf_tex,
    sampler_t tsf_sampler,
    __constant float * data_view_matrix,
    __constant float * data_extent,
    __constant float * data_view_extent,
    __constant float * tsf_var,
    __constant float * parameters,
    __constant int * misc_int,
    __constant float * scalebar_rotation,
    __write_only image2d_t integration_tex,
    float4 shadow_vector)
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));

    int2 ray_tex_dim = get_image_dim(ray_tex);
    int2 tsf_tex_dim = get_image_dim(tsf_tex);

    float tsfOffsetLow = tsf_var[0];
    float tsfOffsetHigh = tsf_var[1];
    float data_offset_low = tsf_var[2];
    float data_offset_high = tsf_var[3];
    float alpha = tsf_var[4];
    float brightness = tsf_var[5];

    int isLogActive = misc_int[2];
    int isSlicingActive = misc_int[4];
    int isIntegration2DActive = misc_int[5];
    int isShadowActive = misc_int[6];
    int isIntegration3DActive = misc_int[7];

    if (isLogActive)
    {
        if (data_offset_low <= 0) data_offset_low = 0.01f;
        if (data_offset_high <= 0) data_offset_high = 0.01f;
        data_offset_low = log10(data_offset_low);
        data_offset_high = log10(data_offset_high);
    }

    // The color of "shadow", or rather the color associated with gradient matching
    float4 shadow_color = (float4)(0.0f,0.0f,0.0f,1.0f);
    shadow_vector = normalize(shadow_vector);

    // If the global id corresponds to a texel
    if ((id_glb.x < ray_tex_dim.x) && (id_glb.y < ray_tex_dim.y))
    {
        float4 ray_near, ray_far;
        float3 ray_delta;
        float cone_diameter_increment;
        float cone_diameter_near;
        float integrated_intensity = 0.0;

        {
            float4 rayNearEdge, rayFarEdge;
            float3 pixel_radius_near, pixel_radius_far;

            // Normalized device coordinates (ndc) of the pixel and its edge (in screen coordinates)
            float2 ndc = (float2)(2.0f * (( convert_float2(id_glb) + 0.5f)/convert_float2(ray_tex_dim)) - 1.0f);
            float2 ndc_edge = (float2)(2.0f * (( convert_float2(id_glb) + (float2)(1.0f, 1.0f))/convert_float2(ray_tex_dim)) - 1.0f);

            // Ray origin and exit point (screen coordinates)
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
        
        // Bug: For some configurations of the view matrix, it seems that the software struggles to compute the correct geometry of the rays. Debug this by painting ray parameters

        int hit;
        float t_near, t_far;
        {
            // Construct a bounding box from the intersect between data_view_extent and data_extent
            float bbox[6];

            bbox[0] = data_view_extent[0];
            bbox[1] = data_view_extent[1];
            bbox[2] = data_view_extent[2];
            bbox[3] = data_view_extent[3];
            bbox[4] = data_view_extent[4];
            bbox[5] = data_view_extent[5];

            // Does the ray for this pixel intersect bbox?
            if (!((bbox[0] >= bbox[1]) || (bbox[2] >= bbox[3]) || (bbox[4] >= bbox[5])))
            {
                hit = boundingBoxIntersect(ray_near.xyz, ray_delta.xyz, bbox, &t_near, &t_far);
            }
        }

        float4 color = (float4)(0.0f);
        float4 sample = (float4)(0.0f);

//        float4 max_sample = read_imagef(tsf_tex, tsf_sampler, (float2)(1.0f, 0.5f));
//        float4 min_sample = read_imagef(tsf_tex, tsf_sampler, (float2)(0.0f, 0.5f));
        
float3 rayBoxDelta;

        if(hit)
        {
            // The geometry of the intersecting part of the ray
            float cone_diameter, step_length;
            float cone_diameter_low = 0.05; // Only acts as a scaling factor

            float3 rayBoxOrigin = ray_near.xyz + t_near * ray_delta.xyz;
            float3 rayBoxEnd = ray_near.xyz + t_far * ray_delta.xyz;
            rayBoxDelta = rayBoxEnd - rayBoxOrigin;
            float3 direction = normalize(rayBoxDelta);
            float3 ray_add_box;
            float rayBoxLength = fast_length(rayBoxDelta);

            float3 ray_xyz_box = rayBoxOrigin;
            float intensity;

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

                        intensity = model(ray_xyz_box, parameters);

                        if(isLogActive)
                        {
                            if (intensity < 1.f) intensity = 1.f;
                            intensity = log10(intensity);
                        }

                        float2 tsfPosition = (float2)(tsfOffsetLow + (tsfOffsetHigh - tsfOffsetLow) * ((intensity - data_offset_low)/(data_offset_high - data_offset_low)), 0.5f);
                        sample = read_imagef(tsf_tex, tsf_sampler, tsfPosition);

                        color.xyz += (1.0f - color.w)*sample.xyz*sample.w;
                        color.w += (1.0f - color.w)*sample.w;
                    }
                }
            }
            else
            {
                // Ray-volume intersection
                while ( fast_length(ray_xyz_box - rayBoxOrigin) < rayBoxLength )
                {
                    // Calculate the cone diameter at the current ray position
                    cone_diameter = (cone_diameter_near + length(ray_xyz_box - ray_near.xyz) * cone_diameter_increment);

                    // The step length is chosen such that there is roughly a set number of samples (4) per voxel. This number changes based on the pseudo-level the ray is currently traversing (interpolation between two octtree levels)
                    step_length = cone_diameter;
                    ray_add_box = direction * step_length;

                    intensity = model(ray_xyz_box, parameters);

                    integrated_intensity += intensity * step_length;
                    
                    if (!isIntegration3DActive)
                    {
                        if(isLogActive)
                        {
                            if (intensity < 0.f) intensity = 0.f;
                            intensity = log10(intensity);
                        }
    
                        float2 tsfPosition = (float2)(tsfOffsetLow + (tsfOffsetHigh - tsfOffsetLow) * ((intensity - data_offset_low)/(data_offset_high - data_offset_low)), 0.5f);
    
                        sample = read_imagef(tsf_tex, tsf_sampler, tsfPosition);
                        sample.w *= alpha;
    
                        if (isShadowActive)
                        {
                            float3 gradient_vector = (float3)(
                                native_divide(
                                    - model((float3)(ray_xyz_box.x - step_length, ray_xyz_box.y, ray_xyz_box.z), parameters)
                                    + model((float3)(ray_xyz_box.x + step_length, ray_xyz_box.y, ray_xyz_box.z), parameters),
                                    step_length),
                                native_divide(
                                    - model((float3)(ray_xyz_box.x, ray_xyz_box.y - step_length, ray_xyz_box.z), parameters)
                                    + model((float3)(ray_xyz_box.x, ray_xyz_box.y + step_length, ray_xyz_box.z), parameters),
                                    step_length),
                                native_divide(
                                    - model((float3)(ray_xyz_box.x, ray_xyz_box.y, ray_xyz_box.z - step_length), parameters)
                                    + model((float3)(ray_xyz_box.x, ray_xyz_box.y, ray_xyz_box.z + step_length), parameters),
                                    step_length));
                            float strength = (dot(shadow_vector.xyz, normalize(gradient_vector)) + 1.0f) * 0.5f; // Simplest form. Can multiply with shadow_magnitude*(1.0f - color.w)*sample.w or the like, but can it be justified, and does it provide better results? Thus far I am inclined to say no.
    //                        float strength = native_powr((dot(shadow_vector.xyz, normalize(gradient_vector)) + 1.0f) * 0.5f, 2.0f); // Scaled with some power to increase contrast between shadow and no shadow
                            sample.xyz = mix(sample.xyz, shadow_color.xyz, strength);
                        }
    
                        // Scale the alpha channel in accordance with the cone diameter
                        sample.w *= native_divide(cone_diameter, cone_diameter_low);
    
                        // MIX COLORS
                        color.xyz += (1.0f - color.w)*sample.xyz*sample.w;
    //                    color.xyz += (1.0f - color.w)*sample.xyz; //(Crassin)
                        color.w += (1.0f - color.w)*sample.w;
    
                        if (color.w > 0.999f) break;
                    }
                    ray_xyz_box += ray_add_box;
                }
                color *= brightness;
            }
        }
//        if ( (id_glb.x == 5) || (id_glb.y == 15))color = (float4)(1.0,0.0,0.0,1.0);
        write_imagef(integration_tex, id_glb, (float4)(integrated_intensity*cone_diameter_near));
        if (isIntegration3DActive)
        {
            if(isLogActive)
            {
                if (integrated_intensity < 0.f) integrated_intensity = 0.f;
                integrated_intensity = log10(integrated_intensity);
            }
    
            float2 tsfPosition = (float2)(tsfOffsetLow + (tsfOffsetHigh - tsfOffsetLow) * ((integrated_intensity - data_offset_low)/(data_offset_high - data_offset_low)), 0.5f);
    
            sample = read_imagef(tsf_tex, tsf_sampler, tsfPosition);
    
            write_imagef(ray_tex, id_glb, clamp(sample, 0.0f, 1.0f));
        }
        else
        {
//            if (!(id_glb.x%16))
//            {
                color.x = length(rayBoxDelta)/(data_view_extent[1]-data_view_extent[0]);
                color.w = length(rayBoxDelta)/(data_view_extent[1]-data_view_extent[0]);
//            }
            write_imagef(ray_tex, id_glb, clamp(color, 0.0f, 1.0f));
        }
//        if (isIntegration2DActive && !isSlicingActive)
//        {
//            if(isLogActive)
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
