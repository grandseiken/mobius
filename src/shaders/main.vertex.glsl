#version 330
layout(location = 0) in vec4 position;

uniform mat4 perspective_matrix;

void main()
{
  vec4 camera_space = position + vec4(0.5, 0.5, -2, 0);
  vec4 clip_space = perspective_matrix * camera_space;
  gl_Position = clip_space;
}
