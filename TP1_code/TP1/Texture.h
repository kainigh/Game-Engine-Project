// Texture.h
#pragma once

#include <GL/glew.h>
#include <string>
#include <iostream>

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

class Textures {
public:
    GLuint id = 0;

    Textures(const std::string& path) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        // Settings
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Load image
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true); // Common for OpenGL
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            std::cerr << "Failed to load texture at: " << path << std::endl;
        }

        stbi_image_free(data);
    }

    void bind(GLuint unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    ~Textures() {
        glDeleteTextures(1, &id);
    }
};
