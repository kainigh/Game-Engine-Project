#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"


// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <functional>


// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
//#include <common/my_texture.cpp>
#include <common/texture.hpp>

#include "model.h"
#include "shader.h"

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Transform.h"
#include "Quadtree.h"

void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

unsigned int VAO;

float yVelocity = 0.0f;
bool isJumping = false;
const float gravity = -200.0f;
const float jumpStrength = 80.0f;
const float jumpOffset = 3.0f;  // height above terrain

// Camera settings
glm::vec3 camera_position   = glm::vec3(0.0f, 2.0f, 80.0f);
glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);  // Initially facing forward
glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 lastValidDirection(0.0f, 0.0f, 1.0f);


// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

//rotation
float angle = 0.;
float zoom = 1.;

// Mouse tracking variables
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float yaw = -90.0f;  // Facing along -Z initially
float pitch = 0.0f;
float sensitivity = 0.1f;

float carSpeed = 0.0f;
float carAcceleration = 50.0f; // units per second squared
float carFriction = 30.0f;
float maxSpeed = 100.0f;
float carYaw = 0.0f; // car's current heading in degrees
float turnSpeed = 90.0f; // degrees per second


int resolution = 1; 

std::vector<float> vertices;
std::vector<float> texCoords;
std::vector<unsigned short> indices;

int width, height, nrChannels;
float yScale = 64.0f / 256.0f, yShift = 16.0f;

 // ORBIT ANIMATIONS
 static float planetOrbitAngle = 0.0f;
 static float moonOrbitAngle = 0.0f;

 //float getTerrainHeightAt(float x, float z, std::vector<glm::vec3>& terrainVertices);
 float getTerrainHeightAt(float x, float z, const std::vector<glm::vec3>& terrainVertices, int width, int height);
 glm::vec3 getTerrainNormal(float x, float z, const std::vector<glm::vec3>& terrainVertices, int width, int height);
 float barycentricInterpolation(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float x, float z);


glm::vec3 rotateAroundParent(glm::vec3 position, float angleDegrees) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angleDegrees), glm::vec3(0, 1, 0));
    glm::vec4 rotatedPos = rotation * glm::vec4(position, 1.0f);
    return glm::vec3(rotatedPos);
}

/*******************************************************************************/

/*******************************************************************************/
int main( void )
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


    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "TP1 - GLFW", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
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
    

    int maxQuadtreeDepth = 7; // initial depth (you can modify this via keyboard later)

    QuadtreeNode* root = new QuadtreeNode(glm::vec2(width / 2.0f, height / 2.0f), width / 2.0f, 0);
    root->subdivide(maxQuadtreeDepth);


    // Cull triangles which normal is not towards the camera
    //glEnable(GL_CULL_FACE);
    //glDisable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    
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




    unsigned char* heightData = stbi_load("../heightmaps/island_heightmap.jpg", &width, &height, &nrChannels, 1);
    if (!heightData) {
        std::cerr << "Failed to load heightmap!" << std::endl;
        return -1;
    }

    // Create a heightmap mesh from the height data
    std::vector<glm::vec3> terrainVertices;
    std::vector<glm::vec2> terrainUVs;
    std::vector<glm::vec3> terrainNormals;
    std::vector<unsigned int> terrainIndices;

    float heightScale = 80.0f;
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


    
    /*****************TODO***********************/
    // Get a handle for our "Model View Projection" matrices uniforms

   
    /****************************************/
    //SphereMesh sphere("../backpack/backpack.obj");

    
    const float scale = 5.75f;

    
    
    // Get a handle for our "LightPosition" uniform
    //glUseProgram(programID);
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

    

    
    
    std::function<void(Entity&)> drawEntity = [&](Entity& e) {
        glm::mat4 model = e.m_transform.getModelMatrix();
        glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &model[0][0]);

        glUniform1i(glGetUniformLocation(programID, "myTextureSampler"), 3);
        
        
    
        e.mesh->bind();
    
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, e.mesh->vertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, e.mesh->uvBuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, e.mesh->normalBuffer);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
        e.mesh->draw();
    
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    
        for (auto& child : e.children) {
            drawEntity(*child);

        }


    };
    
    //Shader ourShader("vertex_shader.glsl", "fragment_shader.glsl");

    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");

    Model carModel("../formula_1/Formula_1_mesh.obj");
    //Model ourModel("planet.obj");
    glm::vec3 carPosition(0.0f, 0.0f, 50.0f); // start somewhere safe on the terrain
    glm::vec3 previousCarPosition = carPosition;

    

    do{

        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        

            // UP = accelerate
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                carSpeed += carAcceleration * deltaTime;
                if (carSpeed > maxSpeed) carSpeed = maxSpeed;
            }

            // DOWN = brake
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                carSpeed -= carAcceleration * deltaTime * 2.0f; // braking is stronger
                if (carSpeed < 0) carSpeed = 0;
            }

            // LEFT/RIGHT = steering
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                carYaw += turnSpeed * deltaTime;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                carYaw -= turnSpeed * deltaTime;
            }

            // Apply friction if no input
            if (glfwGetKey(window, GLFW_KEY_UP) != GLFW_PRESS &&
                glfwGetKey(window, GLFW_KEY_DOWN) != GLFW_PRESS) {
                carSpeed -= carFriction * deltaTime;
                if (carSpeed < 0) carSpeed = 0;
            }

            // Convert heading to direction vector
            glm::vec3 forwardDir = glm::vec3(
                sin(glm::radians(carYaw)),
                0.0f,
                cos(glm::radians(carYaw))
            );

            // Update position
            carPosition += forwardDir * carSpeed * deltaTime;


            carPosition.y = getTerrainHeightAt(carPosition.x, carPosition.z, terrainVertices, width, height);

            glm::vec3 normal = getTerrainNormal(carPosition.x, carPosition.z, terrainVertices, width, height);


            //glm::vec3 moveDir = carPosition - previousCarPosition;
            glm::vec3 moveDir = glm::normalize(forwardDir);



            if (glm::length(moveDir) > 0.01f) {
                moveDir = glm::normalize(moveDir);
                lastValidDirection = moveDir; // update stored direction
            } else {
                moveDir = lastValidDirection; // use last good direction
            }

            // Make sure forward and normal are orthogonal
            glm::vec3 right = glm::normalize(glm::cross(moveDir, normal));
            glm::vec3 adjustedForward = glm::normalize(glm::cross(normal, right));

            // Construct rotation matrix
            glm::mat4 rotationMatrix = glm::mat4(1.0f);
            rotationMatrix[0] = glm::vec4(right, 0.0f);
            rotationMatrix[1] = glm::vec4(normal, 0.0f);
            rotationMatrix[2] = glm::vec4(adjustedForward, 0.0f);

            previousCarPosition = carPosition;


        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 lightPos = glm::vec3(0, 3, 5); // You can tweak this
        //glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(LightID, -4.0f, 4.0f, -4.0f);

        //float lightTime = glfwGetTime();
        //glm::vec3 lightPos = glm::vec3(150 * cos(lightTime), 100, 150 * sin(lightTime));
        //glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);




        /*****************TODO***********************/
        // Model matrix : an identity matrix (model will be at the origin) then change

        // View matrix : camera/view transformation lookat() utiliser camera_position camera_target camera_up

        // Projection matrix : 45 Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units

        // Send our transformation to the currently bound shader,
        // in the "Model View Projection" to the shader uniforms

        /****************************************/
    

        
    

        // Set terrain model matrix
        //glm::mat4 terrainModel = glm::translate(glm::mat4(1.0f), glm::vec3(-width / 2.0f, -20.0f, -height / 2.0f));
        //glm::mat4 terrainModel = glm::mat4(1.0f);
        //glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &terrainModel[0][0]);

        

        // don't forget to enable shader before setting uniforms
        //ourShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000.0f);
        //glm::mat4 view = glm::lookAt(camera_position, camera_target, camera_up);

        // Camera settings
        float cameraDistance = 10.0f;  // How far behind the car
        float cameraHeight = 1.0f;    // How high above the car

        // Offset the camera behind the car
        glm::vec3 cameraOffset = -adjustedForward * cameraDistance + glm::vec3(0.0f, cameraHeight, 0.0f);
        glm::vec3 dynamicCameraPos = carPosition + cameraOffset;
        glm::vec3 dynamicCameraTarget = carPosition + adjustedForward * 5.0f; // Look slightly ahead

        glm::mat4 view = glm::lookAt(dynamicCameraPos, dynamicCameraTarget, camera_up);

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glUseProgram(programID);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 MVP = projection * view * model;

       
        glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &model[0][0]);
        
        // Texture already bound:
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, terrainTexture);
        glUniform1i(glGetUniformLocation(programID, "myTextureSampler"), 0);

        // Draw terrain
        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        ourShader.use();

        model = glm::translate(model, carPosition);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // rotate around Y axis
        model *= rotationMatrix; // apply tilt
        model = glm::scale(model, glm::vec3(0.02f)); // scale after rotation
        ourShader.setMat4("model", model);
        carModel.Draw(ourShader);
        
            
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

 
    glDeleteProgram(programID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    // Recursive delete if you want to manage memory
    std::function<void(QuadtreeNode*)> deleteQuadtree = [&](QuadtreeNode* node) {
        for (auto* child : node->children) deleteQuadtree(child);
        delete node;
    };
    deleteQuadtree(root);


    return 0;

}

float barycentricInterpolation(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float x, float z) {
    float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);
    float l1 = ((p2.z - p3.z) * (x - p3.x) + (p3.x - p2.x) * (z - p3.z)) / det;
    float l2 = ((p3.z - p1.z) * (x - p3.x) + (p1.x - p3.x) * (z - p3.z)) / det;
    float l3 = 1.0f - l1 - l2;

    return l1 * p1.y + l2 * p2.y + l3 * p3.y;
}

glm::vec3 getTerrainNormal(float x, float z, const std::vector<glm::vec3>& terrainVertices, int width, int height) {
    float epsilon = 1.0f; // Small delta for sampling

    float hL = getTerrainHeightAt(x - epsilon, z, terrainVertices, width, height);
    float hR = getTerrainHeightAt(x + epsilon, z, terrainVertices, width, height);
    float hD = getTerrainHeightAt(x, z - epsilon, terrainVertices, width, height);
    float hU = getTerrainHeightAt(x, z + epsilon, terrainVertices, width, height);

    glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f, hD - hU));
    return normal;
}

float getTerrainHeightAt(float x, float z, const std::vector<glm::vec3>& terrainVertices, int width, int height) {
    float size = 2.0f;
    int gridX = static_cast<int>(x / size);
    int gridZ = static_cast<int>(z / size);

    if (gridX < 0 || gridX >= width - 1 || gridZ < 0 || gridZ >= height - 1)
        return 0.0f;

    int vertexIndex = gridZ * width + gridX;
    glm::vec3 v1 = terrainVertices[vertexIndex];
    glm::vec3 v2 = terrainVertices[vertexIndex + 1];
    glm::vec3 v3 = terrainVertices[vertexIndex + width];
    glm::vec3 v4 = terrainVertices[vertexIndex + width + 1];

    float xCoord = fmod(x, size) / size;
    float zCoord = fmod(z, size) / size;

    if (xCoord + zCoord < 1.0f)
        return barycentricInterpolation(v1, v2, v3, x, z);
    else
        return barycentricInterpolation(v2, v4, v3, x, z);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

   
        float cameraSpeed = 100 * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            camera_position.y += cameraSpeed;
        }
            //camera_position += cameraSpeed * camera_target;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera_position.y -= cameraSpeed;
            //std::cout << camera_position.x << " " << camera_position.y << " " << camera_position.z << std::endl;
        }   
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera_position.x -= cameraSpeed;
        }
            //camera_position -= glm::normalize(glm::cross(camera_target, camera_up)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera_position.x += cameraSpeed;
        }
            //camera_position += glm::normalize(glm::cross(camera_target, camera_up)) * cameraSpeed;

            extern bool isJumping;
            extern float yVelocity;
            
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping) {
                isJumping = true;
                yVelocity = jumpStrength;
                std::cout << "Jumping!" << std::endl;
            }
            
}

// Mouse callback function
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw   += xOffset;
    pitch += yOffset;

    // Constrain the pitch to prevent screen flipping
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Calculate the new camera direction
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camera_target = glm::normalize(front);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

