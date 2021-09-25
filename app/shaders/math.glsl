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
    seed = wang_hash(seed);
    return seed * 2.3283064365387e-10f;
}

vec3 SampleHemisphereCosine(in vec3 normal, inout uint seed)
{
    const float u1 = rand(seed);
    const float u2 = rand(seed);
    const float r = sqrt(1.0f - u1 * u1);
    const float phi = 2.0f * 3.1415926535f * u2;
    const vec3 s = vec3(cos(phi)*r, sin(phi)*r, u1);

    const vec3 w = normal;
    const vec3 u = normalize(cross((abs(w.x) > .1f ? vec3(0, 1, 0) : vec3(1, 0, 0)), w));
    const vec3 v = normalize(cross(w,u));

    return normalize(vec3(
            dot(s, vec3(u.x, v.x, w.x)),
            dot(s, vec3(u.y, v.y, w.y)),
            dot(s, vec3(u.z, v.z, w.z))));
}

