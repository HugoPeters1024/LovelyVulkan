#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT Payload {
    vec3 normal;
    bool hit;
    vec3 materialColor;
    float d;
    vec3 emission;
    uint customIndex;
    vec3 direction;
} payload;

void main() {
    payload.hit = false;
    const vec3 sunDir = normalize(vec3(1,1,0));
    payload.emission = vec3(0, 0, 0.01f) + vec3(max(0.0f, 14 * pow(dot(payload.direction, sunDir), 80)));
    payload.emission = vec3(5,3,1);
}
