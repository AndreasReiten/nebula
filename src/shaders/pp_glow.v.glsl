uniform int orientation;
uniform vec2 pixel_size;
uniform float scale;
uniform float deviation;
uniform int samples;
in vec2 position;
in vec2 texpos;
out vec2 f_texpos;
out vec2 f_pixel_size;
out float f_scale;
out float f_deviation;
flat out int f_samples;
flat out int f_orientation;

void main(void)
{
    gl_Position =  vec4(position, 0.0, 1.0);
    f_texpos = texpos;
    f_scale = scale;
    f_deviation = deviation;
    f_samples = samples;
    f_pixel_size = pixel_size;
    f_orientation = orientation;
}

