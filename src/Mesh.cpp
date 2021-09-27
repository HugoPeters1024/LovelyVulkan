#include "Mesh.h"

namespace lv {

void Mesh::load(const char* filename) {
    tinyobj::ObjReader reader;
    reader.ParseFromFile(filename);
    auto& attrib = reader.GetAttrib();
    auto& shape = reader.GetShapes()[0];
    for(size_t v=0; v<attrib.GetVertices().size(); v+=3) {
        vertices.push_back(lv::Vertex { glm::vec4(attrib.vertices[v+0], attrib.vertices[v+1], attrib.vertices[v+2], 0) });
    }

    for(const auto& idx : shape.mesh.indices) {
        indices.push_back(idx.vertex_index);
    }
}

}
