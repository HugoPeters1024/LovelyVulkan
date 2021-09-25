#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 3, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 4, set = 0) buffer Vertices { vec3 v[]; } vertices;

layout(location = 0) rayPayloadInEXT Payload {
    vec4 normal;
    vec3 accumulator;
    float d;
    vec3 mask;
} payload;

hitAttributeEXT vec3 attribs;

vec3 getNormal() {
    vec3 v0 = vertices.v[indices.i[gl_PrimitiveID * 3 + 0]].xyz;
    vec3 v1 = vertices.v[indices.i[gl_PrimitiveID * 3 + 1]].xyz;
    vec3 v2 = vertices.v[indices.i[gl_PrimitiveID * 3 + 2]].xyz;

    return normalize(cross(v0 - v1, v0 - v2));
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

int rand_xorshift(int seed)
{
    seed ^= (seed << 13);
    seed ^= (seed >> 17);
    seed ^= (seed << 5);
    return seed;
}

void main() {
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - -attribs.y, attribs.x, attribs.y);
    //const float r = float(rand_xorshift(gl_PrimitiveID) % 256) / 255.0f;
    //payload.color += hsv2rgb(vec3(r, 1.0f, 0.2f));
    payload.accumulator += payload.mask * 0.5f * max(0.0f, abs(dot(getNormal(), vec3(0,1,0))));
    payload.normal = vec4(getNormal(),1);
    payload.d = gl_RayTmaxEXT;
}
