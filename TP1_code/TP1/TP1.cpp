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

//#include "model.h"

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

bool isOrbitalMode = false;  // Toggle between free and orbital camera
float rotationSpeed = 0.5f;  // Initial rotation speed
float orbitAngle = 0.0f;     // Angle for rotation
float orbitRadius = 150.0f;  // Distance of the camera from the terrain

bool isOrbitingTerrain = false;  // Toggle orbiting mode
float terrainRotationSpeed = 10.0f; // Default rotation speed
float terrainAngle = 0.0f; // Keeps track of rotation angle

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
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
    //GLuint programID = LoadShaders( "vertex_shader.glsl", "fragment_shader.glsl" );
    GLuint programID = LoadShaders("vertex_shader.glsl", "fragment_shader.glsl");

    Texture earthTex("earth.png");
    Texture sunTex("2k_sun.jpg");
    Texture moonTex("s3.ppm");
    Texture grassTex("../texture/grass.png");
    Texture rockTex("../texture/rock.png");
    Texture snowTex("../texture/snowrocks.png");


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
    SphereMesh sphere("planet.obj");

    
    const float scale = 1.75f;
    Entity sun(&sphere, &sunTex);
    //sun.m_transform.setLocalPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    //std::cout << "Sun position: " << sun.m_transform.getLocalPosition().x << " " << sun.m_transform.getLocalPosition().y << " " << sun.m_transform.getLocalPosition().z << std::endl;
    sun.m_transform.setLocalScale(glm::vec3(scale));

    float startX = 100.0f;
    float startZ = 100.0f;
    float startY = getTerrainHeightAt(startX, startZ, terrainVertices, width, height) + jumpOffset; // Adjust Y position based on terrain height

    sun.m_transform.setLocalPosition(glm::vec3(startX, startY, startZ));
    /*
    // Add planet as child of sun
    auto planetEntity = std::make_unique<Entity>(&sphere, &earthTex);
    planetEntity->m_transform.setLocalPosition(glm::vec3(15, 0, 0));
    planetEntity->m_transform.setLocalScale(glm::vec3(0.5f));
    Entity* planetPtr = planetEntity.get(); // Save pointer
    sun.addChild(std::move(planetEntity));

    // Add moon as child of planet
    auto moonEntity = std::make_unique<Entity>(&sphere, &moonTex);
    moonEntity->m_transform.setLocalPosition(glm::vec3(10, 0, 0));
    moonEntity->m_transform.setLocalScale(glm::vec3(0.25f));
    Entity* moonPtr = moonEntity.get(); // Save pointer
    planetPtr->addChild(std::move(moonEntity));
    */
    
    
    
    // Get a handle for our "LightPosition" uniform
    glUseProgram(programID);
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

    
    
    std::function<void(Entity&)> drawEntity = [&](Entity& e) {
        glm::mat4 model = e.m_transform.getModelMatrix();
        glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &model[0][0]);
    
        //e.texture->bind();
        //glUniform1i(glGetUniformLocation(programID, "myTextureSampler"), 0);

        //e.texture->bind(GL_TEXTURE3);  // Use a different texture unit for objects (e.g., sun)
        sunTex.bind(GL_TEXTURE3);
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


        glm::vec3 spherePos = sun.m_transform.getLocalPosition();  // sphere's current position
        glm::vec3 inputDir(0.0f);
        float moveSpeed = 50.0f * deltaTime;

        // Arrow key input for X/Z
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            inputDir.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            inputDir.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            inputDir.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            inputDir.x += 1.0f;

        // Movement along slope
        if (glm::length(inputDir) > 0.0f) {
            inputDir = glm::normalize(inputDir);
            glm::vec3 normal = getTerrainNormal(spherePos.x, spherePos.z, terrainVertices, width, height);

            glm::vec3 right = glm::normalize(glm::cross(normal, glm::vec3(0, 0, 1)));
            glm::vec3 forward = glm::normalize(glm::cross(right, normal));
            glm::vec3 moveDir = glm::normalize(inputDir.x * right + inputDir.z * forward);

            spherePos += moveDir * moveSpeed;
        }

        // Handle jumping
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping) {
            isJumping = true;
            yVelocity = jumpStrength;
            std::cout << "Jumping!" << std::endl;
        }
        
        // Apply gravity
        yVelocity += gravity * deltaTime;
        spherePos.y += yVelocity * deltaTime;

        spherePos.x = glm::clamp(spherePos.x, 1.0f, (width - 2) * 2.0f);
        spherePos.z = glm::clamp(spherePos.z, 1.0f, (height - 2) * 2.0f);


        // Terrain height
        float terrainY = getTerrainHeightAt(spherePos.x, spherePos.z, terrainVertices, width, height);
        
        float desiredY = terrainY + jumpOffset;

        if (spherePos.y <= desiredY) {
            // Sphere is landing
            spherePos.y = desiredY;
            yVelocity = 0.0f;
            isJumping = false;
        } else {
            // Terrain has risen under the sphere — force it up
            float terrainBelow = desiredY;
            if (!isJumping && spherePos.y < terrainBelow) {
                spherePos.y = terrainBelow;
            }
        }
        

        sun.m_transform.setLocalPosition(spherePos);

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
    

        
        // Rotate the terrain around the Y-axis
        glm::mat4 model = glm::mat4(1.0f);
        //model = glm::translate(model, glm::vec3(0.0f, -1.0f, -10.0f));  // Keep terrain at correct position
        //model = glm::rotate(model, glm::radians(terrainAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Apply rotation

        // View & Projection remain unchanged
        //glm::mat4 view = glm::lookAt(camera_position, camera_target, camera_up);

        glm::vec3 targetOffset(0.0f, 10.0f, 125.0f);  // camera offset above & behind sphere
        glm::vec3 desiredCamPos = sun.m_transform.getLocalPosition() + targetOffset;

        // Smooth follow using linear interpolation (LERP)
        float cameraSmoothness = 5.0f;
        camera_position = glm::mix(camera_position, desiredCamPos, cameraSmoothness * deltaTime);
        camera_target = glm::mix(camera_target, sun.m_transform.getLocalPosition(), cameraSmoothness * deltaTime);
        
        glm::mat4 view = glm::lookAt(camera_position, camera_target, camera_up);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000.0f);

        
        root->drawLOD(camera_position);     // Display tiles with LOD
        root->drawBounds();                 // Optional debug lines around quadtree cells
        
        glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(programID, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(programID, "projection"), 1, GL_FALSE, &projection[0][0]);

        /*
        planetPtr->m_transform.setLocalRotation({0.0f, glfwGetTime() * 100.0f, 0.0f});
        moonPtr->m_transform.setLocalRotation({0.0f, glfwGetTime() * 50.0f, 0.0f});
       
        planetOrbitAngle += 20.0f * deltaTime;  // degrees per second
        moonOrbitAngle += 80.0f * deltaTime;

        planetPtr->m_transform.setLocalPosition(
            rotateAroundParent(glm::vec3(15.0f, 0.0f, 0.0f), planetOrbitAngle)
        );
        
        moonPtr->m_transform.setLocalPosition(
            rotateAroundParent(glm::vec3(10.0f, 0.0f, 0.0f), moonOrbitAngle)
        );
        */
        

        glm::vec3 pos = sun.m_transform.getLocalPosition();
       

        sun.updateSelfAndChild();
        //drawEntity(sun);
        grassTex.bind(GL_TEXTURE0);
        rockTex.bind(GL_TEXTURE1);
        snowTex.bind(GL_TEXTURE2);
    
        glUniform1i(glGetUniformLocation(programID, "grassTexture"), 0);
        glUniform1i(glGetUniformLocation(programID, "rockTexture"), 1);
        glUniform1i(glGetUniformLocation(programID, "snowTexture"), 2);

        // Set terrain model matrix
        //glm::mat4 terrainModel = glm::translate(glm::mat4(1.0f), glm::vec3(-width / 2.0f, -20.0f, -height / 2.0f));
        glm::mat4 terrainModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, GL_FALSE, &terrainModel[0][0]);

        // Draw terrain
        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);


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

