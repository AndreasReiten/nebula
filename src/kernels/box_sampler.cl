__kernel void boxSample(
    __read_only image3d_t bricks,
    __global uint * oct_index,
    __global uint * oct_brick,
    sampler_t brick_sampler,
    __constant float * data_view_extent,
    __global float * output)
{
    int2 id_glb = (int2)(get_global_id(0),get_global_id(1));

}
