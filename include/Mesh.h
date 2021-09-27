#include "precomp.h"

namespace lv {

struct Vertex { glm::vec4 v; };

class Mesh : NoCopy {
public:
    Mesh() = default;

    void load(const char* filename);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

}

