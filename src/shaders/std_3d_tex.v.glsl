uniform vec4 color;
uniform vec3 bbox_min;
uniform vec3 bbox_max;
uniform mat4 transform;
uniform mat4 U;
uniform int time;
in vec3 position;
in vec2 texpos;
out vec2 f_texpos;
out vec4 f_color;
flat out int f_time;

void main(void)
{
    gl_Position = transform * vec4(position, 1.0);
    
    vec4 pos = U * vec4(position, 1.0); 
    
    if (((pos.x >= bbox_min.x) && (pos.x <= bbox_max.x)) && ((pos.y >= bbox_min.y) && (pos.y <= bbox_max.y)) && ((pos.z >= bbox_min.z) && (pos.z <= bbox_max.z))) f_color = color;
    else f_color = vec4(color.xyz, 0.0);
    
    f_texpos = texpos;
    f_time = time;
}

