#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT Payload {
    vec3 direction;
    vec3 color;
} payload;

void main() {
    //payload.color = vec3(pow(dot(payload.direction, vec3(0,0,1)),4));
    payload.color += vec3(0, 0, 0.045f);
}
