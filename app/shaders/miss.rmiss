#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadEXT vec3 hitValue;

void main() {
    hitValue = vec3(0.0f, 0.0f, 0.2f);
}