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
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
//#include <common/my_texture.cpp>
#include <common/texture.hpp>

#include "model.h"
#include "shader.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Transform.h"
#include "Quadtree.h"


/*std::vector<glm::vec3> aiWaypoints = {
    glm::vec3(40.0f, 4.4f, 60.0f),
    glm::vec3(80.0f, 4.4f, 300.0f),
    glm::vec3(40.0f, 4.4f, 60.0f)
   
};
*/
std::vector<glm::vec3> aiWaypoints;

int currentWaypointIndex = 0;

std::vector<glm::vec3> ExtractAndSortTrackCenterline(
    const std::vector<unsigned int>& indices,
    const std::vector<glm::vec3>& vertices,
    const glm::mat4& modelMatrix,
    int step)
{
    std::vector<glm::vec3> rawCenters;

    for (size_t i = 0; i < indices.size(); i += 3 * step) {
        glm::vec3 v0 = glm::vec3(modelMatrix * glm::vec4(vertices[indices[i]], 1.0));
        glm::vec3 v1 = glm::vec3(modelMatrix * glm::vec4(vertices[indices[i + 1]], 1.0));
        glm::vec3 v2 = glm::vec3(modelMatrix * glm::vec4(vertices[indices[i + 2]], 1.0));

        glm::vec3 center = (v0 + v1 + v2) / 3.0f;
        rawCenters.push_back(center);
    }

    // === Nearest neighbor sorting ===
    std::vector<glm::vec3> sorted;
    std::vector<bool> used(rawCenters.size(), false);
    int currentIndex = 0;

    sorted.push_back(rawCenters[currentIndex]);
    used[currentIndex] = true;

    while (sorted.size() < rawCenters.size()) {
        float minDist = 1e9;
        int nextIndex = -1;
        for (int i = 0; i < rawCenters.size(); ++i) {
            if (used[i]) continue;
            float dist = glm::distance(sorted.back(), rawCenters[i]);
            if (dist < minDist) {
                minDist = dist;
                nextIndex = i;
            }
        }
        if (nextIndex != -1) {
            sorted.push_back(rawCenters[nextIndex]);
            used[nextIndex] = true;
        } else {
            break;
        }
    }

    return sorted;
}




void processInput(GLFWwindow* window, bool& isJumping, float& yVelocity);
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color);
void textInit();
void renderScene(Shader& ourShader, Shader& trackShader, GLuint programID, GLuint terrainVAO,
    GLuint terrainTexture, GLuint trackTexture, std::vector<unsigned int>& terrainIndices,
    glm::mat4& view, glm::mat4& projection, Model& trackModel);

void StayOnTrack(glm::vec3& carPosition, std::vector<unsigned int>& trackIndices, std::vector<glm::vec3>& trackVertices,
    glm::mat4& trackModelMatrix, std::vector<glm::vec3>& terrainVertices, float& yVelocity, bool& isJumping);

void DrawCar(Shader& shader, Model& carModel, glm::vec3& carPosition, glm::mat4& rotationMatrix,
            glm::mat4& view, glm::mat4& projection);

void renderUI(Shader& textShader, float carSpeed, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT);


bool rayIntersectsTriangle(
    const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
    float& outT);

float SphereTrackCollision(glm::vec3 wheelWorldPos, float radius,
    const std::vector<glm::vec3>& trackVertices,
    const std::vector<unsigned int>& trackIndices);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float yVelocity1 = 0.0f;
float yVelocity2 = 0.0f;
bool isJumping1 = false;
bool isJumping2 = false;
const float gravity = -300.0f;
const float jumpStrength = 80.0f;
const float jumpOffset = 3.0f;  // height above terrain

float heightScale = 0.0f;

// Camera settings
glm::vec3 camera_position   = glm::vec3(0.0f, 2.0f, 80.0f);
glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);  // Initially facing forward
glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 lastValidDirection(0.0f, 0.0f, 1.0f);

glm::vec3 carVelocity(0.0f, 0.0f, 0.0f);
bool collisionDetected = false;


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
float maxSpeed = 50.0f;
float car1Yaw = 180.0f; // car's current heading in degrees
float turnSpeed = 90.0f; // degrees per second

float car2Speed = 0.0f;
glm::vec3 car2Velocity(0.0f);
glm::vec3 car2ForwardDir(0.0f, 0.0f, 1.0f); // default forward
float car2Yaw = 0.0f; 
float lastValidTrackY_car2 = 4.2f; 

const float carCollisionRadius = 2.0f; // Adjust based on car model size

int resolution = 1; 

std::vector<float> vertices;
std::vector<float> texCoords;
std::vector<unsigned short> indices;

int width, height, nrChannels;
float yScale = 64.0f / 256.0f, yShift = 16.0f;

 float getTerrainHeightAt(float x, float z, const std::vector<glm::vec3>& terrainVertices, int width, int height);
 glm::vec3 getTerrainNormal(float x, float z, const std::vector<glm::vec3>& terrainVertices, int width, int height);
 float barycentricInterpolation(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float x, float z);

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};

bool checkAABBCollision(const BoundingBox& box1, const BoundingBox& box2);
const float wheelRadius = 0.3f; // Adjust depending on model



struct Plane {
    glm::vec3 normal;   // usually (0,1,0) for flat ground
    float d;            // plane equation: ax + by + cz + d = 0
};



/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
unsigned int VAO, VBO;


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

    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


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

    
    GLuint trackTexture;
    int tracktexWidth, tracktexHeight, tracktexChannels;
    unsigned char* trackImage = stbi_load("../texture/roadtexture.jpg", &tracktexWidth, &tracktexHeight, &tracktexChannels, 0);
    if (!trackImage) {
        std::cerr << "Failed to load track texture!" << std::endl;
    }

    glGenTextures(1, &trackTexture);
    glBindTexture(GL_TEXTURE_2D, trackTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tracktexWidth, tracktexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, trackImage);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(trackImage);



    unsigned char* heightData = stbi_load("../heightmaps/heightmap_circle.png", &width, &height, &nrChannels, 1);
    if (!heightData) {
        std::cerr << "Failed to load heightmap!" << std::endl;
        return -1;
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

    
    /*****************TODO***********************/
    // Get a handle for our "Model View Projection" matrices uniforms

   
    /****************************************/
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");



    
    Shader ourShader("../shaders/1.model_loading.vs", "../shaders/1.model_loading.fs");
    Shader trackShader("../shaders/track.vs", "../shaders/track.fs");  
    Shader textShader("../shaders/text.vs", "../shaders/text.fs");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    textShader.use();
    glUniformMatrix4fv(glGetUniformLocation(textShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    //Initialize Free-Type Text
    textInit();

    //Model carModel("truck.obj");
    Model carModel("../formula_1/Formula_1_mesh.obj");
    Model carModel2("../formula_1/Formula_1_mesh.obj");
    Model trackModel("../tracks/trackbanking.obj");



    std::vector<glm::vec3> trackVertices; // all vertices
    std::vector<unsigned int> trackIndices; // all indices

    // Example: load all vertices/indices from the loaded Model
    for (Mesh mesh : trackModel.meshes) {
        for (Vertex vertex : mesh.vertices) {
            trackVertices.push_back(vertex.Position);
        }
        for (unsigned int index : mesh.indices) {
            trackIndices.push_back(index);
        }
    }

        glm::mat4 trackModelMatrix = glm::mat4(1.0f);
        trackModelMatrix = glm::translate(trackModelMatrix, glm::vec3(180.0f, 3.0f, 180.0f));
        trackModelMatrix = glm::scale(trackModelMatrix, glm::vec3(1.0f));
        trackModelMatrix = glm::rotate(trackModelMatrix, glm::radians(100.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        std::vector<glm::vec3> sorted = ExtractAndSortTrackCenterline(trackIndices, trackVertices, trackModelMatrix, 500);
        aiWaypoints = sorted;
            
    //Print aiWaypoints
    for(const auto& waypoint : aiWaypoints) {
       std::cout << "Waypoint: " << waypoint.x << ", " << waypoint.y << ", " << waypoint.z << std::endl;
    }

    glm::vec3 carPosition(33.0f, 5.0f, 60.0f); // start somewhere safe on the track
    glm::vec3 previousCarPosition = carPosition;

        //glm::vec3 carPosition2(33.0f, 5.0f, 60.0f);
        glm::vec3 carPosition2 = aiWaypoints[0];
        //carPosition2.y += 0.05f; 

       
    car2Yaw = glm::degrees(atan2((aiWaypoints[1] - aiWaypoints[0]).x,
                             (aiWaypoints[1] - aiWaypoints[0]).z)) + 180.0f;



        // Set up ImGui context

    

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    
   
    do{

        //print carPosition2
        //std::cout << carPosition2.x << ", " << carPosition2.y << ", " << carPosition2.z << std::endl;
        


        BoundingBox car1Box;
        car1Box.min = carPosition + glm::vec3(-2.0f, 0.0f, -1.0f);
        car1Box.max = carPosition + glm::vec3(2.0f, 1.0f, 1.0f);
        
        BoundingBox car2Box;
        car2Box.min = carPosition2 + glm::vec3(-2.0f, -1.0f, -1.0f);
        car2Box.max = carPosition2 + glm::vec3(2.0f, 1.0f, 1.0f);    
    
        //std::cout << "Car 1 Position: " << carPosition.x << ", " << carPosition.y << ", " << carPosition.z << std::endl;
        

        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        //processInput(window, isJumping1, yVelocity1);

        

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
                car1Yaw += turnSpeed * deltaTime;
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                car1Yaw -= turnSpeed * deltaTime;
            }

            // Apply friction if no input
            if (glfwGetKey(window, GLFW_KEY_UP) != GLFW_PRESS &&
                glfwGetKey(window, GLFW_KEY_DOWN) != GLFW_PRESS) {
                carSpeed -= carFriction * deltaTime;
                if (carSpeed < 0) carSpeed = 0;
            }

            // Convert heading to direction vector
            glm::vec3 forwardDir1 = glm::vec3(sin(glm::radians(car1Yaw)), 0.0f, cos(glm::radians(car1Yaw)));

            // === AI Movement ===
            glm::vec3 target = aiWaypoints[currentWaypointIndex];
            glm::vec3 toTarget = target - carPosition2;
            toTarget.y = 0.0f;

            float distance = glm::length(toTarget);
            glm::vec3 directionToTarget = glm::normalize(toTarget);

            // Turn car2 toward waypoint
            float targetYaw = glm::degrees(atan2(directionToTarget.x, directionToTarget.z));
            float yawDiff = glm::mod(targetYaw - car2Yaw + 540.0f, 360.0f) - 180.0f;
            car2Yaw += glm::clamp(yawDiff, -turnSpeed * deltaTime, turnSpeed * deltaTime);
            

            // Move forward
            car2Speed = glm::mix(car2Speed, maxSpeed, 1.0f * deltaTime);  // smooth acceleration
            glm::vec3 forwardDir2 = glm::vec3(sin(glm::radians(car2Yaw)), 0.0f, cos(glm::radians(car2Yaw)));
            car2Velocity = (forwardDir2 * car2Speed) * 0.08f; // slow down AI car
    
            carPosition2 += car2Velocity * deltaTime;
             

           

            // If close to waypoint, move to next
            if (distance < 5.0f) {
                currentWaypointIndex = (currentWaypointIndex + 1) % aiWaypoints.size();
            }


            // Update velocity if no collision
            if (!collisionDetected) {
                carVelocity = forwardDir1 * carSpeed;
            }

            // Move the car
            carPosition += carVelocity * deltaTime;

    

            if (checkAABBCollision(car1Box, car2Box)) {
                std::cout << "AABB COLLISION DETECTED!" << std::endl;
            
                glm::vec3 collisionNormal = glm::normalize(carPosition - carPosition2);
            
                // Reflect the car's velocity vector over the collision normal
                carVelocity = glm::reflect(carVelocity, collisionNormal);
            
                // Optional: Dampen the speed a bit after collision (like energy loss)
                carVelocity *= 0.7f; // lose 30% speed
            
                // Update carSpeed based on new velocity
                carSpeed = glm::length(glm::vec2(carVelocity.x, carVelocity.z));

                // Update car position to avoid getting stuck inside the other car
                carPosition += collisionNormal * (carCollisionRadius + carCollisionRadius); // move out of collision
      
            } else {
                collisionDetected = false;
            }

         
            
            glm::vec3 normal = getTerrainNormal(carPosition.x, carPosition.z, terrainVertices, width, height);


            //glm::vec3 moveDir = carPosition - previousCarPosition;
            glm::vec3 moveDir = glm::normalize(forwardDir1);

            //glm::vec3 forwardDir2 = glm::vec3(sin(glm::radians(car2Yaw)), 0.0f, cos(glm::radians(car2Yaw)));

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

            // Get terrain normal under car2
            glm::vec3 normal2 = getTerrainNormal(carPosition2.x, carPosition2.z, terrainVertices, width, height);

            glm::vec3 right2 = glm::normalize(glm::cross(forwardDir2, normal2));
            glm::vec3 adjustedForward2 = glm::normalize(glm::cross(normal2, right2));

            glm::mat4 rotationMatrix2 = glm::mat4(1.0f);
            rotationMatrix2[0] = glm::vec4(right2, 0.0f);
            rotationMatrix2[1] = glm::vec4(normal2, 0.0f);
            rotationMatrix2[2] = glm::vec4(adjustedForward2, 0.0f);

            //std::cout << "Car2 Position: " << carPosition2.x << ", " << carPosition2.y << ", " << carPosition2.z << std::endl;
            //std::cout << "Car2 Yaw: " << car2Yaw << std::endl;
            
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

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000.0f);
        //glm::mat4 view = glm::lookAt(camera_position, camera_target, camera_up);

        // Camera settings
        float cameraDistance = 0.1f;  // How far behind the car
        float cameraHeight = 1.0f;    // How high above the car

        // Offset the camera behind the car
        glm::vec3 cameraOffset = -adjustedForward * cameraDistance + glm::vec3(0.0f, cameraHeight, 0.0f);
        glm::vec3 dynamicCameraPos = carPosition + cameraOffset;
        glm::vec3 dynamicCameraTarget = carPosition + adjustedForward * 5.0f; // Look slightly ahead

        glm::mat4 view = glm::lookAt(dynamicCameraPos, dynamicCameraTarget, camera_up);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Car Stats");
        ImGui::Text("Speed: %.2f", carSpeed);
        ImGui::Text("Current Waypoint: %d", currentWaypointIndex);
        
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", carPosition.x, carPosition.y, carPosition.z);
        ImGui::SliderFloat("Terrain Height", &heightScale, 0.0f, 90.0f);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (heightScale != previousHeightScale) {
            previousHeightScale = heightScale;
        
            // Rebuild terrain vertex heights
            for (int z = 0; z < height; ++z) {
                for (int x = 0; x < width; ++x) {
                    float y = heightData[z * width + x] * heightScale / 255.0f;
                    terrainVertices[z * width + x].y = y;
                }
            }
        
            // Update terrain VBO on GPU
            glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, terrainVertices.size() * sizeof(glm::vec3), &terrainVertices[0]);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        


        // Render the terrain and car
        renderScene(ourShader, trackShader, programID, terrainVAO, terrainTexture, trackTexture,
            terrainIndices, view, projection, trackModel);


            
            DrawCar(ourShader, carModel, carPosition, rotationMatrix, view, projection);
            DrawCar(ourShader, carModel2, carPosition2, rotationMatrix2, view, projection);


            StayOnTrack(carPosition, trackIndices, trackVertices, trackModelMatrix, terrainVertices, yVelocity1, isJumping1);
            StayOnTrack(carPosition2, trackIndices, trackVertices, trackModelMatrix, terrainVertices, yVelocity2, isJumping2);

          
             

        // 2. Render UI
        renderUI(textShader, carSpeed, SCR_WIDTH, SCR_HEIGHT);
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

 
    glDeleteProgram(programID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

   

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

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
void processInput(GLFWwindow* window, bool& isJumping1, float& yVelocity1)
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
            
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping1) {
                isJumping1 = true;
                yVelocity1 = jumpStrength;
                std::cout << "Jumping!" << std::endl;
            }

}


bool checkAABBCollision(const BoundingBox& box1, const BoundingBox& box2)
{
    // Check for separation in each axis
    if (box1.max.x < box2.min.x || box1.min.x > box2.max.x) return false;
    if (box1.max.y < box2.min.y || box1.min.y > box2.max.y) return false;
    if (box1.max.z < box2.min.z || box1.min.z > box2.max.z) return false;
    return true; // Overlapping on all axes = collision
}



void renderScene(Shader& ourShader, Shader& trackShader, GLuint programID, GLuint terrainVAO,
    GLuint terrainTexture, GLuint trackTexture, std::vector<unsigned int>& terrainIndices,
    glm::mat4& view, glm::mat4& projection, Model& trackModel)
    {
        // Draw Terrain******************************************************************************************
            glUseProgram(programID);

            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 MVP = projection * view * model;
            glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &model[0][0]);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, terrainTexture);
            glUniform1i(glGetUniformLocation(programID, "myTextureSampler"), 0);

            glBindVertexArray(terrainVAO);
            glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        //********************************************************************************************************* */

        // Draw Track**********************************************************************************************
            trackShader.use();

            trackShader.setMat4("projection", projection);
            trackShader.setMat4("view", view);

            glm::mat4 trackModelMatrix = glm::mat4(1.0f);
            trackModelMatrix = glm::translate(trackModelMatrix, glm::vec3(180.0f, 3.0f, 180.0f));
            trackModelMatrix = glm::scale(trackModelMatrix, glm::vec3(1.0f)); // Scale down the track
            trackModelMatrix = glm::rotate(trackModelMatrix, glm::radians(100.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate the track to align with the car's forward direction
            // Apply any scaling/positioning
            trackShader.setMat4("model", trackModelMatrix);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, trackTexture);
            trackShader.setInt("texture_diffuse1", 0);

            //trackShader.setVec3("objectColor", glm::vec3(0.2f, 0.2f, 0.2f)); // RGB (dark grey)
            trackModel.Draw(trackShader);

        //************************************************************************************************************ */



}

void StayOnTrack(glm::vec3& carPosition, std::vector<unsigned int>& trackIndices, std::vector<glm::vec3>& trackVertices,
    glm::mat4& trackModelMatrix, std::vector<glm::vec3>& terrainVertices, float& yVelocity, bool& isJumping)
{
    glm::vec3 rayOrigin = carPosition + glm::vec3(0.0f, 10.0f, 0.0f); // shoot ray from above
    glm::vec3 rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);

    float closestT = 1e9;
    bool hit = false;
    glm::vec3 hitV0, hitV1, hitV2;

    for (size_t i = 0; i < trackIndices.size(); i += 3) {
        // Transform track vertices to match visual model
        glm::vec3 v0 = glm::vec3(trackModelMatrix * glm::vec4(trackVertices[trackIndices[i]], 1.0));
        glm::vec3 v1 = glm::vec3(trackModelMatrix * glm::vec4(trackVertices[trackIndices[i+1]], 1.0));
        glm::vec3 v2 = glm::vec3(trackModelMatrix * glm::vec4(trackVertices[trackIndices[i+2]], 1.0));

        float t;
        if (rayIntersectsTriangle(rayOrigin, rayDirection, v0, v1, v2, t)) {
            if (t < closestT) {
                closestT = t;
                hit = true;
                hitV0 = v0;
                hitV1 = v1;
                hitV2 = v2;
            }
        }
    }

    float groundY = 0.0f;

    if (hit) {
        groundY = barycentricInterpolation(hitV0, hitV1, hitV2, carPosition.x, carPosition.z);
        //lastValidTrackY_car2 = groundY;
    } else {
        groundY = getTerrainHeightAt(carPosition.x, carPosition.z, terrainVertices, width, height);
        //groundY = lastValidTrackY_car2; 
    }

    // Apply gravity + lift
    if (carPosition.y > groundY + 0.1f || isJumping) {
        yVelocity += gravity * deltaTime;
        carPosition.y += yVelocity * deltaTime;

        if (carPosition.y <= groundY) {
            carPosition.y = groundY;
            yVelocity = 0.0f;
            isJumping = false;
        } else {
            isJumping = true;
        }
    } else {
        carPosition.y = groundY;
        yVelocity = 0.0f;
        isJumping = false;
    }

}

 void DrawCar(Shader& shader, Model& carModel, glm::vec3& carPosition, glm::mat4& rotationMatrix,
                glm::mat4& view, glm::mat4& projection) {

        shader.use();
         shader.setMat4("projection", projection);
            shader.setMat4("view", view);

           glm::mat4 model = glm::translate(glm::mat4(1.0f), carPosition);

            // Fake wheel bouncing based on speed
            float bounce = sin(glfwGetTime() * carSpeed * 0.1f) * 0.005f; // amplitude 0.3 units (tweak if needed)
            model = glm::translate(model, glm::vec3(0.0f, bounce, 0.0f));
            
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
           

            model *= rotationMatrix;
            model = glm::scale(model, glm::vec3(0.01f));
        shader.setMat4("model", model);
        carModel.Draw(shader);
}

void renderUI(Shader& textShader, float carSpeed, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT) {
    textShader.use();
    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(textShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    RenderText(textShader, "Speedometer", 25.0f, SCR_HEIGHT - 50.0f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));

    std::string speedText = "Speed: " + std::to_string((int)carSpeed) + " km/h";
    RenderText(textShader, speedText, 25.0f, SCR_HEIGHT - 100.0f, 1.0f, glm::vec3(0.5f, 0.8f, 0.2f));
}


void textInit()
{
    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

	// find path to font
    std::string font_name = "Antonio-Bold.ttf";
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return;
    }
	
	// load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state	
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool rayIntersectsTriangle(
    const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
    float& outT)
{
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(rayDirection, edge2);
    float a = glm::dot(edge1, h);

    if (fabs(a) < EPSILON)
        return false; // Ray is parallel to triangle

    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(rayDirection, q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    // Compute t to find out where the intersection point is on the line
    float t = f * glm::dot(edge2, q);

    if (t > EPSILON) { // Intersection
        outT = t;
        return true;
    }

    return false;
}

float SphereTrackCollision(glm::vec3 wheelWorldPos, float radius,
                            const std::vector<glm::vec3>& trackVertices,
                            const std::vector<unsigned int>& trackIndices)
{
    glm::vec3 rayOrigin = wheelWorldPos + glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3 rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);

    float closestT = 1e9;
    bool hit = false;

    for (size_t i = 0; i < trackIndices.size(); i += 3) {
        glm::vec3 v0 = trackVertices[trackIndices[i]];
        glm::vec3 v1 = trackVertices[trackIndices[i+1]];
        glm::vec3 v2 = trackVertices[trackIndices[i+2]];

        float t;
        if (rayIntersectsTriangle(rayOrigin, rayDirection, v0, v1, v2, t)) {
            if (t < closestT) {
                closestT = t;
                hit = true;
            }
        }
    }

    if (hit) {
        glm::vec3 hitPoint = rayOrigin + rayDirection * closestT;
        return hitPoint.y + radius; // Wheel rests on top of track
    } else {
        return -1.0f; // no collision
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

