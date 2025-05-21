// ECS Racing Game with GLFW, Models, Shaders, and HUD
#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

#include "ft2build.h"
#include FT_FREETYPE_H

#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <functional>


// Include GLEW
//#include <GL/glew.h>

#include <glad/glad.h>
// Include GLEW

#include <GLFW/glfw3.h>
#include "model.h"
#include "shader.h"
//#include "text_renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using Entity = uint32_t;
const Entity MAX_ENTITIES = 5000;
// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

struct Transform {
    glm::vec3 position;
    float yaw = 0.0f;
    glm::vec3 scale = glm::vec3(1.0f);
};

struct Physics {
    glm::vec3 velocity;
    float yVelocity = 0.0f;
    bool isJumping = false;
};

struct RenderModel {
    Model* model;
    Shader* shader;
};

struct PlayerControl {
    float speed = 0.0f;
    float maxSpeed = 50.0f;
};

struct AIControl {
    int waypointIndex = 0;
};

struct Collider {
    float radius = 2.0f;
};

struct TextRender {
    std::string text;
    float x, y;
    float scale;
    glm::vec3 color;
};

std::unordered_map<Entity, Transform> transforms;
std::unordered_map<Entity, Physics> physics;
std::unordered_map<Entity, RenderModel> renderables;
std::unordered_map<Entity, PlayerControl> players;
std::unordered_map<Entity, AIControl> aiControllers;
std::unordered_map<Entity, Collider> colliders;
std::unordered_map<Entity, TextRender> textElements;

std::vector<glm::vec3> waypoints;
Entity playerCar;
Entity aiCar;
Entity hudLapText;
Entity hudSpeedText;
Entity hudStatusText;

Model carModel("models/car.obj");
Model aiModel("models/car_ai.obj");
Shader modelShader("shaders/model.vs", "shaders/model.fs");
Shader textShader("shaders/text.vs", "shaders/text.fs");
//TextRenderer textRenderer();

GLFWwindow* window;

void processInput(GLFWwindow* window, float dt) {
    auto& ctrl = players[playerCar];
    auto& tf = transforms[playerCar];

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) ctrl.speed += 50.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) ctrl.speed -= 100.0f * dt;
    if (ctrl.speed > ctrl.maxSpeed) ctrl.speed = ctrl.maxSpeed;
    if (ctrl.speed < 0.0f) ctrl.speed = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) tf.yaw += 90.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) tf.yaw -= 90.0f * dt;

    glm::vec3 forward = glm::vec3(sin(glm::radians(tf.yaw)), 0.0f, cos(glm::radians(tf.yaw)));
    physics[playerCar].velocity = forward * ctrl.speed;
}

void updateAI(float dt) {
    auto& tf = transforms[aiCar];
    auto& ai = aiControllers[aiCar];

    glm::vec3 target = waypoints[ai.waypointIndex];
    glm::vec3 toTarget = target - tf.position;
    toTarget.y = 0;

    if (glm::length(toTarget) < 5.0f) {
        ai.waypointIndex = (ai.waypointIndex + 1) % waypoints.size();
    }

    glm::vec3 direction = glm::normalize(toTarget);
    float desiredYaw = glm::degrees(atan2(direction.x, direction.z));
    tf.yaw = glm::mix(tf.yaw, desiredYaw, dt * 5.0f);

    physics[aiCar].velocity = direction * 30.0f;
}

void updatePhysics(float dt) {
    for (auto& [e, phys] : physics) {
        auto& tf = transforms[e];
        tf.position += phys.velocity * dt;
    }
}

void updateHUD() {
    int lap = 1;
    float speed = players[playerCar].speed;
    std::string status = "Race On";

    textElements[hudLapText].text = "Lap: " + std::to_string(lap);
    textElements[hudSpeedText].text = "Speed: " + std::to_string((int)speed) + " km/h";
    textElements[hudStatusText].text = status;
}

void renderAll(const glm::mat4& view, const glm::mat4& projection) {
    for (auto& [e, rend] : renderables) {
        auto& tf = transforms[e];

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, tf.position);
        model = glm::rotate(model, glm::radians(tf.yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, tf.scale);

        rend.shader->use();
        rend.shader->setMat4("model", model);
        rend.shader->setMat4("view", view);
        rend.shader->setMat4("projection", projection);

        rend.model->Draw(*rend.shader);
    }
}

void renderTextHUD() {
    for (auto& [e, txt] : textElements) {
        //textRenderer.RenderText(textShader, txt.text, txt.x, txt.y, txt.scale, txt.color);
    }
}

int main() {
    if (!glfwInit()) return -1;
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ECS Racing", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    //TextRenderer textRenderer;
    //textRenderer.Load("fonts/arial.ttf", 48, SCR_WIDTH, SCR_HEIGHT);


    playerCar = 1;
    aiCar = 2;
    hudLapText = 3;
    hudSpeedText = 4;
    hudStatusText = 5;

    transforms[playerCar] = {{0, 5, 0}, 180, glm::vec3(0.4f)};
    players[playerCar] = {};
    physics[playerCar] = {};
    renderables[playerCar] = {&carModel, &modelShader};
    colliders[playerCar] = {};

    transforms[aiCar] = {{10, 5, 0}, 0, glm::vec3(0.4f)};
    aiControllers[aiCar] = {};
    physics[aiCar] = {};
    renderables[aiCar] = {&aiModel, &modelShader};
    colliders[aiCar] = {};

    textElements[hudLapText] = {"Lap: 1", 25.0f, 550.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f)};
    textElements[hudSpeedText] = {"Speed: 0 km/h", 25.0f, 520.0f, 1.0f, glm::vec3(0.8f, 0.8f, 1.0f)};
    textElements[hudStatusText] = {"Race On", 25.0f, 490.0f, 1.0f, glm::vec3(0.5f, 1.0f, 0.5f)};

    waypoints = {
        {10, 0, 100}, {90, 0, 340}, {300, 0, 360}, {275, 0, 100}, {40, 0, 40}
    };

    while (!glfwWindowShouldClose(window)) {
        float dt = 0.016f;  // Replace with actual deltaTime if needed
        glfwPollEvents();

        processInput(window, dt);
        updateAI(dt);
        updatePhysics(dt);
        updateHUD();

        renderAll(glm::mat4(1.0f), glm::mat4(1.0f));
        renderTextHUD();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
