#include "Mloader.h"
#include <iostream>

// Definiujemy implementacjê tinyobjloader TYLKO w tym pliku
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

ChessModel loadModel(const std::string& objPath, const std::string& baseDir) {
    ChessModel model;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.c_str(), baseDir.c_str());

    if (!warn.empty()) std::cout << "WARN: " << warn << std::endl;
    if (!err.empty()) std::cerr << "ERR: " << err << std::endl;
    if (!ret) {
        std::cerr << "Nie udalo sie wczytac modelu: " << objPath << std::endl;
        return model;
    }

    for (size_t s = 0; s < shapes.size(); s++) {
        for (size_t f = 0; f < shapes[s].mesh.indices.size(); f++) {
            tinyobj::index_t idx = shapes[s].mesh.indices[f];

            // Wierzcho³ki (x, y, z, w)
            model.vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
            model.vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
            model.vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
            model.vertices.push_back(1.0f);

            // Wektory normalne
            if (idx.normal_index >= 0) {
                model.normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
                model.normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
                model.normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
                model.normals.push_back(0.0f);
            }

            // Koordynaty tekstur
            if (idx.texcoord_index >= 0) {
                model.texCoords.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                model.texCoords.push_back(1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]);
            }

            model.vertexCount++;
        }
    }
    std::cout << "Wczytano model " << objPath << " (wierzcholkow: " << model.vertexCount << ")" << std::endl;
    return model;
}