#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

struct Vertex {
    vec4 pos;
    vec4 emission;
};

layout(binding = 3, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 4, set = 0) buffer Vertices { Vertex v[]; } vertices;

layout(location = 0) rayPayloadInEXT Payload {
    vec3 normal;
    bool hit;
    vec3 materialColor;
    float d;
    vec3 emission;
} payload;

hitAttributeEXT vec3 attribs;

vec3 getNormal() {
    vec3 v0 = vertices.v[indices.i[gl_PrimitiveID * 3 + 0]].pos.xyz;
    vec3 v1 = vertices.v[indices.i[gl_PrimitiveID * 3 + 1]].pos.xyz;
    vec3 v2 = vertices.v[indices.i[gl_PrimitiveID * 3 + 2]].pos.xyz;

    return normalize(cross(v0 - v1, v0 - v2));
}

vec3 getEmission() {
    vec3 e0 = vertices.v[indices.i[gl_PrimitiveID * 3 + 0]].emission.xyz;
    vec3 e1 = vertices.v[indices.i[gl_PrimitiveID * 3 + 1]].emission.xyz;
    vec3 e2 = vertices.v[indices.i[gl_PrimitiveID * 3 + 2]].emission.xyz;
    return (1.0f - attribs.x - attribs.y) * e0 + attribs.x * e1 + attribs.y * e2;
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}


void main() {
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    payload.emission = getEmission();
    payload.materialColor = payload.emission;
    //payload.materialColor = vec3(0.5f * max(0.0f, abs(dot(getNormal(), vec3(0,1,0)))));
    payload.normal = getNormal();
    payload.d = gl_RayTmaxEXT;
    payload.hit = true;
}
