#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <vector>
#include <string>

// Struktura przechowuj¹ca dane pojedynczej figury
struct ChessModel {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    int vertexCount = 0;
};

// Deklaracja funkcji wczytuj¹cej
ChessModel loadModel(const std::string& objPath, const std::string& baseDir = "");

#endif // MODELLOADER_H