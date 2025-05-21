#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

#include "ft2build.h"
#include FT_FREETYPE_H

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <functional>
#include <filesystem>
#include <string>


// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <common/shader.hpp>
using namespace glm;


#include "model.h"
#include "shader.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using Entity = uint32_t;
const Entity MAX_ENTITIES = 5000;
// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

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

void CreateTerrain()
{
      // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("vertex_shader.glsl", "fragment_shader.glsl");


      GLuint terrainTexture;
    int texWidth, texHeight, texChannels;
    unsigned char* image = stbi_load("../texture/grass.png", &texWidth, &texHeight, &texChannels, 0);
    if (!image) {
        std::cerr << "Failed to load terrain texture!" << std::endl;
    }

    glGenTextures(1, &terrainTexture);
    glBindTexture(GL_TEXTURE_2D, terrainTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);

    int width, height, nrChannels;
    float heightScale = 0.1f;
    unsigned char* heightData = stbi_load("../heightmaps/heightmap_circle.png", &width, &height, &nrChannels, 1);
    if (!heightData) {
        std::cerr << "Failed to load heightmap!" << std::endl;
        return;
    }

    // Create a heightmap mesh from the height data
    std::vector<glm::vec3> terrainVertices;
    std::vector<glm::vec2> terrainUVs;
    std::vector<glm::vec3> terrainNormals;
    std::vector<unsigned int> terrainIndices;

    
    float size = 2.0f;


    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            float y = heightData[z * width + x] * heightScale / 255.0f;
            terrainVertices.emplace_back(x * size, y, z * size);
            terrainUVs.emplace_back((float)x / width, (float)z / height);
            terrainNormals.emplace_back(0, 1, 0); // Basic upward normal
        }
    }

     // Generate indices
    for (int z = 0; z < height - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int topLeft = z * width + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * width + x;
            int bottomRight = bottomLeft + 1;

            terrainIndices.push_back(topLeft);
            terrainIndices.push_back(bottomLeft);
            terrainIndices.push_back(topRight);

            terrainIndices.push_back(topRight);
            terrainIndices.push_back(bottomLeft);
            terrainIndices.push_back(bottomRight);
        }
    }

    GLuint terrainVAO, terrainVBO, terrainEBO, terrainUVBO, terrainNormalBO;
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);
    glGenBuffers(1, &terrainEBO);
    glGenBuffers(1, &terrainUVBO);
    glGenBuffers(1, &terrainNormalBO);

    glBindVertexArray(terrainVAO);

    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(glm::vec3), &terrainVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, terrainUVBO);
    glBufferData(GL_ARRAY_BUFFER, terrainUVs.size() * sizeof(glm::vec2), &terrainUVs[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, terrainNormalBO);
    glBufferData(GL_ARRAY_BUFFER, terrainNormals.size() * sizeof(glm::vec3), &terrainNormals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrainIndices.size() * sizeof(unsigned int), &terrainIndices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);



    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);


    float previousHeightScale = heightScale;

     glUseProgram(programID);
            glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 10.0f);
            glm::vec3 cameraTarget = glm::vec3(0.0f, 5.0f, 0.0f);

            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000.0f);
            glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));


            glm::mat4 MVP = projection * view * model;
            glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &model[0][0]);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, terrainTexture);
            glUniform1i(glGetUniformLocation(programID, "myTextureSampler"), 0);

            glBindVertexArray(terrainVAO);
            glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);


}


int main()
{
  
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }


    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);


    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "E Racer", NULL, NULL);
    if (!window)
    {
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE; // Needed for core profile
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        getchar();
        glfwTerminate();
        return -1;
    }

     // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Dark blue background
    glClearColor(0.2f, 0.1f, 0.2f, 0.0f);

     // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);


 

    Model carModel("../formula_1/Formula_1_mesh.obj");
    Model aiModel("../formula_1/Formula_1_mesh.obj");
    Shader modelShader("../shaders/1.model_loading.vs", "../shaders/1.model_loading.fs");
    Shader trackShader("../shaders/track.vs", "../shaders/track.fs");  
    Shader textShader("../shaders/text.vs", "../shaders/text.fs");


    
    playerCar = 1;
    aiCar = 2;
    hudLapText = 3;
    hudSpeedText = 4;
    hudStatusText = 5;

    transforms[playerCar] = {{0, 5, 0}, 180, glm::vec3(0.01f)};
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

    do
    { 
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);
        updateAI(deltaTime);
        updatePhysics(deltaTime);
        updateHUD();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
        (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

        // Position the camera to see the track and cars
        glm::vec3 cameraPos = transforms[playerCar].position + glm::vec3(0.0f, 15.0f, 5.0f);
        glm::vec3 cameraTarget = transforms[playerCar].position;
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));


        renderAll(view, projection);
        renderTextHUD();


          // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);



}
