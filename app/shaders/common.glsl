const float PI = 3.141592653589793;
const float INVPI = 1.0f / PI;

struct Vertex {
    vec4 pos;
};

struct TriangleData {
    vec4 vs[3];
};

uint rand_xorshift(in uint seed)
{
    seed ^= (seed << 13);
    seed ^= (seed >> 17);
    seed ^= (seed << 5);
    return seed;
}

uint wang_hash(in uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float rand(inout uint seed)
{
    seed = rand_xorshift(seed);
    return seed * 2.3283064365387e-10f;
}

vec3 SampleHemisphereCosine(in vec3 normal, in float r0, in float r1)
{
    const float r = sqrt(r0);
    const float theta = 2.0f * PI * r1;
    const float x = r * cos(theta);
    const float y = r * sin(theta);
    const vec3 s = vec3(x, y, sqrt(1.0f - r0));

    const vec3 w = normal;
    const vec3 u = normalize(cross((abs(w.x) > .1f ? vec3(0, 1, 0) : vec3(1, 0, 0)), w));
    const vec3 v = normalize(cross(w,u));

    return normalize(vec3(
            dot(s, vec3(u.x, v.x, w.x)),
            dot(s, vec3(u.y, v.y, w.y)),
            dot(s, vec3(u.z, v.z, w.z))));
}

// Special version of sign that never returns 0
float sign_(in float x) {
    return uintBitsToFloat(floatBitsToUint(sign(x)) | 0x3f800000);
}


vec3 SampleHemisphereCosine(in vec3 normal, inout uint seed)
{
    const float u1 = rand(seed);
    const float u2 = rand(seed);
    return SampleHemisphereCosine(normal, u1, u2);
}



float max3(in vec3 v) { return max(v.x, max(v.y, v.z)); }
