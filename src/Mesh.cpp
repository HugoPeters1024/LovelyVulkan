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

    normals.resize(indices.size());

    if (attrib.normals.size() == 0) {
        for(uint32_t i=0; i<indices.size(); i+=3) {
            const glm::vec3 v0 = vertices[indices[i+0]].v.xyz();
            const glm::vec3 v1 = vertices[indices[i+1]].v.xyz();
            const glm::vec3 v2 = vertices[indices[i+2]].v.xyz();
            const glm::vec3 normal = glm::normalize(glm::cross(v0 - v1, v0 - v2));
            normals[i+0] = normal;
            normals[i+1] = normal;
            normals[i+2] = normal;
        }
    } else {
        for(uint32_t i=0; i<shape.mesh.indices.size(); i+=3) {
            const auto& ni0 = shape.mesh.indices[i+0].normal_index;
            const auto& ni1 = shape.mesh.indices[i+1].normal_index;
            const auto& ni2 = shape.mesh.indices[i+2].normal_index;
            const auto n0 = glm::vec3(attrib.normals[ni0*3+0], attrib.normals[ni0*3+1], attrib.normals[ni0*3+2]);
            const auto n1 = glm::vec3(attrib.normals[ni1*3+0], attrib.normals[ni1*3+1], attrib.normals[ni1*3+2]);
            const auto n2 = glm::vec3(attrib.normals[ni2*3+0], attrib.normals[ni2*3+1], attrib.normals[ni2*3+2]);
            normals[i+0] = n0;
            normals[i+1] = n1;
            normals[i+2] = n2;
        }
    }
}

}
