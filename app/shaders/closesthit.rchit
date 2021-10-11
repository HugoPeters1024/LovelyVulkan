#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#include "math.glsl"

struct Vertex {
    vec4 pos;
};

struct TriangleData {
    vec4 vs[3];
};

uint primitiveId() { return gl_PrimitiveID + gl_InstanceCustomIndexEXT; }

layout(binding = 3, set = 0) readonly buffer Indices { uint i[]; } indices;
layout(binding = 4, set = 0) readonly buffer Vertices { Vertex v[]; } vertices;
layout(binding = 5, set = 0) readonly buffer TriangleDatas { TriangleData triangleData[]; };

layout(location = 0) rayPayloadInEXT Payload {
    vec3 normal;
    bool hit;
    vec3 materialColor;
    float d;
    vec3 emission;
    uint customIndex;
    vec3 direction;
} payload;

hitAttributeEXT vec3 attribs;

vec3 getNormal() {
    const TriangleData td = triangleData[primitiveId()];
    const vec3 v0 = td.vs[0].xyz;
    const vec3 v1 = td.vs[1].xyz;
    const vec3 v2 = td.vs[2].xyz;
    const vec3 v0v1 = v1 - v0;
    const vec3 v0v2 = v2 - v0;
    return normalize(cross(v0v1, v0v2));
}

vec3 getEmission() {
    const TriangleData td = triangleData[primitiveId()];
    return vec3(td.vs[0].w, td.vs[1].w, td.vs[2].w);
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}


void main() {
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    payload.emission = getEmission();
    payload.materialColor = vec3(0.4f);
    payload.normal = getNormal();
    payload.d = gl_RayTmaxEXT;
    payload.hit = true;
    payload.customIndex = gl_InstanceCustomIndexEXT;
}
