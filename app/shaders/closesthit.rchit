#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT Payload {
    vec3 direction;
    vec3 color;
} payload;

hitAttributeEXT vec3 attribs;

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
    const float r = float(rand_xorshift(gl_PrimitiveID) % 256) / 255.0f;
    payload.color += hsv2rgb(vec3(r, 1.0f, 1.0f));
}
