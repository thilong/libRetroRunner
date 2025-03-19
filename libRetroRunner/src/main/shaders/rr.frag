#version 310 es

precision mediump float;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) in vec2 v_texcoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(tex, v_texcoord);
}
