__kernel void FRAME_TO_IMAGE(
    __write_only image2d_t target,
    __read_only image2d_t source,
    __read_only image2d_t tsf_tex, 
    sampler_t tsf_sampler,
    sampler_t source_sampler,
    float max)
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));

    int2 target_dim = get_image_dim(target);
    int2 source_dim = get_image_dim(source);
    int2 tsf_tex_dim = get_image_dim(tsf_tex);
    
    if ((id_glb.x < target_dim.x) && (id_glb.y < target_dim.y))
    {
        float intensity = read_imagef(source, source_sampler, id_glb).w;

        if (intensity < 1) intensity = 1;
        
        float2 tsf_position = (float2)(native_divide(log10(intensity), log10(max)), 0.5f);
                    
        float4 sample = read_imagef(tsf_tex, tsf_sampler, tsf_position);

        
        write_imagef(target, id_glb, sample);
    }
}
