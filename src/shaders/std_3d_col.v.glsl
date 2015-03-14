#version 400

in  vec3 fragpos;
uniform  mat4 model_transform;
uniform  mat4 projection_transform;

uniform vec4 clip_plane0 = vec4(0,-1,0,0);
uniform vec4 clip_plane1 = vec4(0,-1,0,0);
uniform vec4 clip_plane2 = vec4(0,-1,0,0);
uniform vec4 clip_plane3 = vec4(0,-1,0,0);
uniform vec4 clip_plane4 = vec4(0,-1,0,0);
uniform vec4 clip_plane5 = vec4(0,-1,0,0);

void main(void)
{
    vec4 xyz =  vec4(fragpos, 1.0);
    gl_Position = projection_transform * model_transform * xyz;

    gl_ClipDistance[0] = dot(clip_plane0, xyz);
    gl_ClipDistance[1] = dot(clip_plane1, xyz);
    gl_ClipDistance[2] = dot(clip_plane2, xyz);
    gl_ClipDistance[3] = dot(clip_plane3, xyz);
    gl_ClipDistance[4] = dot(clip_plane4, xyz);
    gl_ClipDistance[5] = dot(clip_plane5, xyz);
}

