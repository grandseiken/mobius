#version 330
layout(location = 0) in vec4 position;

uniform mat4 perspective_matrix;
uniform mat4 transform_matrix;

void main()
{
  vec4 camera_space = transform_matrix * position;
  vec4 clip_space = perspective_matrix * camera_space;
  gl_Position = clip_space;
}
