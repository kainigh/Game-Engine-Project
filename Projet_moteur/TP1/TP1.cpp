// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <bits/stdc++.h>
#include <math.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <common/scenegraph.hpp>

// Matrice
glm::mat4 m_view;
glm::mat4 m_project;
glm::mat4 VP;
glm::mat4 MVP;

// Handle
GLuint programID;
GLuint matMVPid;

void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// graph
Graph main_graph(Transform(1));

// screen
ScreenData screen_data{45, 4.0/3.0, 0.1, 100};

// camera
int camera_num = 1;
Camera cam1(glm::vec3(0.0f, -4.0f,  2.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f,  1.0f), screen_data);
Camera cam2(glm::vec3(0.0f, 0.0f,  6.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f,  0.0f), screen_data);
Object camera1(&cam1, &main_graph);
Object camera2(&cam2, &main_graph);
Object* camera = &camera1;

// mesh
Mesh sol_mesh;
Object sol(&sol_mesh, &main_graph);

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

/*******************************************************************************/

// setup
void loadPlan(glm::vec3 init, glm::vec3 cote1, glm::vec3 cote2, int n1, int n2, std::vector<unsigned short> & indices, std::vector<glm::vec3> & indexed_vertices)
{
    // init vertices
    indexed_vertices.resize(n1*n2);
    for (int y=0 ; y<n2 ; y++) for (int x=0 ; x<n1 ; x++) indexed_vertices[y*n1+x] = init + float(x)/(n1-1)*cote1 + float(y)/(n2-1)*cote2;
    // init triangles
    indices.resize((n1-1)*(n2-1)*6);
    for (int y=0 ; y<n2-1 ; y++)
        for (int x=0 ; x<n1-1 ; x++)
        {
            int pos = (y*(n1-1)+x)*6;
            indices[pos] = y*n1+x;
            indices[pos+1] = y*n1+x+1;
            indices[pos+2] = (y+1)*n1+x+1;
            indices[pos+3] = y*n1+x;
            indices[pos+4] = (y+1)*n1+x+1;
            indices[pos+5] = (y+1)*n1+x;
        }
}

void modifyCoordonnee(glm::vec3 direction, float amplitude, std::vector<glm::vec3> & indexed_vertices)
{
    direction = amplitude*glm::normalize(direction);
    for (int i=0 ; i<indexed_vertices.size() ; i++)
    {
        float random = (float)(rand()) / (float)(RAND_MAX) * 2.0 - 1.0;
        indexed_vertices[i] += random*direction;
    }
}

void setSol()
{
    loadPlan(glm::vec3(-1, -1, 0), glm::vec3(2, 0, 0), glm::vec3(0, 2, 0), 16, 16, sol.get_indices(), sol.get_vertices());
    modifyCoordonnee(glm::vec3(0, 0, 1), 0.5, sol.get_vertices());
}

void setMeshs()
{
    setSol();
}

void setMainGraph()
{
}


void setHandle()
{
    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "vertex_shader.glsl", "fragment_shader.glsl" );

    // Get a handle for our "Model View Projection" matrices uniforms
    matMVPid = glGetUniformLocation(programID, "MVP");
}

void setScene()
{
    setMeshs();
    setMainGraph();
    camera->computeProj(m_project);
    setHandle();
}


void changeMode()
{
    switch (camera_num)
    {
        case 0 : {camera = &camera1; break;}
        case 1 : {camera = &camera2; break;}
    }
    camera_num = (camera_num+1)%2;
    camera->computeProj(m_project);
}

int setGL()
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
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    //glEnable(GL_CULL_FACE);
    return 0;
}


void computeDataFrame()
{
}

void draw()
{
    glUseProgram(programID);
    camera->computeView(m_view);
    VP = m_project*m_view;
    main_graph.draw(matMVPid, glm::mat4(1), VP, MVP);  
}

int main( void )
{
    int set_GL = setGL();
    if (set_GL != 0) return set_GL;

    setScene();
    // Load it into a VBO
    // Generate a buffer for the indices as well
    main_graph.buffering();

    // Get a handle for our "LightPosition" uniform
    glUseProgram(programID);
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

    // TEXTURES **

    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    float time;

    do{
        // Measure speed
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        time += deltaTime;
        nbFrames++;
        if (time > 1)
        {
            std::cout<<nbFrames<<std::endl;
            time = 0;
            nbFrames = 0;
        }

        // input
        processInput(window);

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        computeDataFrame(); // modifications between input and drawing

        draw();

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

    // Cleanup VBO and shader
    glDeleteProgram(programID);
    main_graph.deleteBuff();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------

bool C_pressd = false;

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    //Camera zoom in and out
    float cameraSpeed = 2.5 * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->set_pos(camera->get_pos() + cameraSpeed * camera->get_for());
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->set_pos(camera->get_pos() - cameraSpeed * camera->get_for());
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->set_pos(camera->get_pos() + cameraSpeed * glm::normalize(glm::cross(camera->get_up(), camera->get_for())));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->set_pos(camera->get_pos() + cameraSpeed * glm::normalize(glm::cross(camera->get_for(), camera->get_up())));
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !C_pressd)
    {
        changeMode();
        C_pressd = true;
    }
    if (C_pressd && glfwGetKey(window, GLFW_KEY_C) != GLFW_PRESS)
        C_pressd = false;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}