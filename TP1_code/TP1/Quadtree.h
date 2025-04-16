// Terrain LOD with Quadtree
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <cmath>

struct QuadtreeNode {
    glm::vec2 center;
    float halfSize;
    int depth;
    bool isLeaf = true;
    std::vector<QuadtreeNode*> children;

    QuadtreeNode(glm::vec2 center, float halfSize, int depth)
        : center(center), halfSize(halfSize), depth(depth) {}

    void subdivide(int maxDepth) {
        if (depth >= maxDepth) return;
        float h = halfSize / 2.0f;
        for (int dx = -1; dx <= 1; dx += 2) {
            for (int dz = -1; dz <= 1; dz += 2) {
                glm::vec2 childCenter = center + glm::vec2(dx * h, dz * h);
                QuadtreeNode* child = new QuadtreeNode(childCenter, h, depth + 1);
                children.push_back(child);
                child->subdivide(maxDepth);
            }
        }
        isLeaf = false;
    }

    void drawBounds() {
        float x = center.x, z = center.y;
        float s = halfSize;
        glBegin(GL_LINE_LOOP);
        glVertex3f(x - s, 0, z - s);
        glVertex3f(x + s, 0, z - s);
        glVertex3f(x + s, 0, z + s);
        glVertex3f(x - s, 0, z + s);
        glEnd();

        for (auto* child : children)
            child->drawBounds();
    }

    void drawLOD(const glm::vec3& camPos) {
        float dist = glm::length(glm::vec3(center.x, 0, center.y) - glm::vec3(camPos.x, 0, camPos.z));

        if (children.empty()) {
            // Draw tile based on LOD (stubbed here)
            if (dist < 50.0f) drawHighResTile();
            else if (dist < 150.0f) drawMediumResTile();
            else drawLowResTile();
            return;
        }

        for (auto* child : children)
            child->drawLOD(camPos);
    }

    void drawHighResTile() {
        // Replace with actual VBO draw call
        glColor3f(1.0f, 1.0f, 1.0f);
        drawTileMesh();
    }

    void drawMediumResTile() {
        glColor3f(0.6f, 0.6f, 0.6f);
        drawTileMesh();
    }

    void drawLowResTile() {
        glColor3f(0.3f, 0.3f, 0.3f);
        drawTileMesh();
    }

    void drawTileMesh() {
        float x = center.x, z = center.y;
        float s = halfSize;
        glBegin(GL_QUADS);
        glVertex3f(x - s, 0, z - s);
        glVertex3f(x + s, 0, z - s);
        glVertex3f(x + s, 0, z + s);
        glVertex3f(x - s, 0, z + s);
        glEnd();
    }
};