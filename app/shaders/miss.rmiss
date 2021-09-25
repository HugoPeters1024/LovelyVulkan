#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT Payload {
    vec4 normal;
    vec3 color;
    float d;
} payload;

void main() {
}
