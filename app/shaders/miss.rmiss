#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT Payload {
    vec3 normal;
    bool hit;
    vec3 materialColor;
    float d;
    vec3 emission;
} payload;

void main() {
    payload.hit = false;
}
