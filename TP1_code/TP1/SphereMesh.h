// SphereMesh.h
#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class SphereMesh {
public:
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLuint uvBuffer = 0;
    GLuint normalBuffer = 0;
    GLuint elementBuffer = 0;
    std::vector<unsigned short> indices;

    SphereMesh(const std::string& objPath) {
        loadFromOBJ(objPath);
        
    }

    ~SphereMesh() {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &uvBuffer);
        glDeleteBuffers(1, &normalBuffer);
        glDeleteBuffers(1, &elementBuffer);
        glDeleteVertexArrays(1, &vertexArray);
        //glDeleteTextures(1, &textureID);
    }

    void bind() const {
        glBindVertexArray(vertexArray);
        glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, textureID);
    }

    void draw() const {
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, (void*)0);
    }



private:
    void loadFromOBJ(const std::string& path) {
        std::vector<glm::vec3> vertices, indexed_vertices, normals, indexed_normals;
        std::vector<glm::vec2> uvs, indexed_uvs;

        if (!loadOBJ(path.c_str(), vertices, uvs, normals)) {
            throw std::runtime_error("Failed to load OBJ: " + path);
        }

        indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);

        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

        glGenBuffers(1, &uvBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

        glGenBuffers(1, &normalBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

        glGenBuffers(1, &elementBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
    }

};
