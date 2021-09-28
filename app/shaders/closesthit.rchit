#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#include "math.glsl"

struct Vertex {
    vec4 pos;
};

struct TriangleData {
    vec4 normals[3];
};

layout(binding = 3, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 4, set = 0) buffer Vertices { Vertex v[]; } vertices;
layout(binding = 5, set = 0) buffer TriangleDatas { TriangleData td[]; } triangleData;

layout(location = 0) rayPayloadInEXT Payload {
    vec3 normal;
    bool hit;
    vec3 materialColor;
    float d;
    vec3 emission;
    uint customIndex;
} payload;

hitAttributeEXT vec3 attribs;

vec3 getNormal() {
    const TriangleData td = triangleData.td[gl_PrimitiveID + gl_InstanceCustomIndexEXT];
    return td.normals[0].xyz * (1.0f - attribs.x - attribs.y) + td.normals[1].xyz * attribs.x + td.normals[2].xyz * attribs.y;
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}


void main() {
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    payload.emission = (gl_InstanceCustomIndexEXT > 0) ? 10 * vec3(1, 1, 1) : vec3(0);
    payload.materialColor = vec3(0.3f);
    payload.normal = getNormal();
    payload.d = gl_RayTmaxEXT;
    payload.hit = true;
    payload.customIndex = gl_InstanceCustomIndexEXT;
}
