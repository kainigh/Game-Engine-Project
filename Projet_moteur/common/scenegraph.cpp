#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include "scenegraph.hpp"
#include "objloader.hpp"

using namespace glm;

//////////////////////////////////////////////////////////////////////////////////////////////////

// Transform

// constructor
Transform::Transform(glm::mat4 s, glm::mat4 r, glm::mat4 t) // with matrix
{
    scale = s;
    rotation = r;
    translation = t;
}
Transform::Transform(float s, glm::mat4 r, glm::vec3 t) // float for scale and vec3 for translation
{
    scale = glm::mat4(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1);
    rotation = r;
    translation = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t[0], t[1], t[2], 1);
}
Transform::Transform(glm::vec3 t) // vec3 for translation
{
    scale = glm::mat4(1);
    rotation = glm::mat4(1);
    translation = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t[0], t[1], t[2], 1);
}
Transform::Transform(glm::mat4 r) // mat4 for rotation
{
    scale = glm::mat4(1);
    rotation = r;
    translation = glm::mat4(1);
    matrix = translation*rotation*scale;
}

// accessor
glm::mat4 Transform::get_mat()
{
    if (!computed) {matrix = translation*rotation*scale; computed = true;}
    return matrix;
}
void Transform::set_scale(glm::mat4 s)
{
    scale = s;
    computed = false;
}
void Transform::set_scale(float s)
{
    scale = glm::mat4(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1);
    computed = false;
}
void Transform::set_rotation(glm::mat4 r)
{
    rotation = r;
    computed = false;
}
void Transform::set_translation(glm::mat4 t)
{
    translation = t;
    computed = false;
}
void Transform::set_translation(glm::vec3 t)
{
    translation = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t[0], t[1], t[2], 1);
    computed = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Camera

// constructor
Camera::Camera(ScreenData & sd) : screen_data(sd)
{
    position = glm::vec3(0, 0, 0);
    forward = glm::vec3(0, 0, 1);
    up = glm::vec3(0, 1, 0);
}
Camera::Camera(glm::vec3 _position, glm::vec3 _forward, glm::vec3 _up, ScreenData & sd) : screen_data(sd)
{
    position = _position;
    forward = _forward;
    up = _up;
}

// transform
void Camera::rotate(glm::mat3 r)
{
    position = r*position;
    forward = r*forward;
    up = r*up;
}
void Camera::translate(glm::vec3 t)
{
    position = position+t;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Mesh

// constructor
Mesh::Mesh() {}
Mesh::Mesh(std::vector<unsigned short> _indices, std::vector<glm::vec3> _vertices, std::vector<std::vector<unsigned short>> _triangles)
{
    indices = _indices;
    vertices = _vertices;
    triangles = _triangles;
}
Mesh::Mesh(std::string file, float s) // s for scale
{
    loadOFF(file, vertices, indices, triangles);
    if (s != 1) scale(s);
}

void Mesh::scale(float s)
{
    for (int i=0 ; i<vertices.size() ; i++)
        for (int j=0 ; j<3 ; j++) vertices[i][j] *= s;
}

// openGL
void Mesh::buffering()
{
    int size = vertices.size();
    glGenVertexArrays(1, &VertexArrayID);

    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &elementbuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0] , GL_STATIC_DRAW);
}
void Mesh::draw(GLuint matMVPid, glm::mat4 & MVP)
{
    glBindVertexArray(VertexArrayID);
    glUniformMatrix4fv(matMVPid, 1, GL_FALSE, &MVP[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, (void*)0 );
}
void Mesh::deleteBuff()
{
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &elementbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Graph

// constructor
Graph::Graph(Transform t, Graph* p)
{
    transform = t;
    if (p != NULL) p->add_child(this);
}

// graph tree
void Graph::set_parent(Graph* p)
{
    parent = p;
    if (p != NULL) p->add_child(this);
}
bool Graph::has_child(Graph* c)
{
    for (int i=0 ; i<childs.size() ; i++)
        if (c == childs[i])
            return true;
    return false;
}
void Graph::add_child(Graph* c)
{
    if (!has_child(c)) childs.push_back(c);
    c->parent = this;
}

// objects
bool Graph::has_object(Object* o)
{
    for (int i=0 ; i<childs.size() ; i++)
        if (o == objects[i])
            return true;
    return false;
}
void Graph::add_object(Object* o)
{
    if (!has_object(o)) objects.push_back(o);
    o->graph = this;
}
void Graph::set_transform(Transform t) {transform = t;}

// openGL
void Graph::buffering()
{
    for (int i=0 ; i<childs.size() ; i++)
        if (childs[i] != NULL) childs[i]->buffering();
    for (int i=0 ; i<objects.size() ; i++)
        if (objects[i] != NULL) objects[i]->buffering();
}
void Graph::draw(GLuint matMVPid, glm::mat4 model, glm::mat4 & VP, glm::mat4 & MVP)
{
    model = model*transform.get_mat();
    for (int i=0 ; i<childs.size() ; i++)
        if (childs[i] != NULL) childs[i]->draw(matMVPid, model, VP, MVP);
    for (int i=0 ; i<objects.size() ; i++)
        if (objects[i] != NULL) objects[i]->draw(matMVPid, model, VP, MVP);
}
void Graph::deleteBuff()
{
    for (int i=0 ; i<childs.size() ; i++)
        if (childs[i] != NULL) childs[i]->deleteBuff();
    for (int i=0 ; i<objects.size() ; i++)
        if (objects[i] != NULL) objects[i]->deleteBuff();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Object

// constructor
Object::Object(Mesh* m, Graph* g)
{
    type = MESH;
    mesh = m;
    if (g != NULL) g->add_object(this);
}
Object::Object(Camera* c, Graph* g)
{
    type = CAMERA;
    camera = c;
    if (g != NULL) g->add_object(this);
}
void Object::set_graph(Graph* g) {if (g != NULL) g->add_object(this);}

// camera geter
glm::vec3 Object::get_pos() {if (type == CAMERA) return camera->position;}
glm::vec3 Object::get_for() {if (type == CAMERA) return camera->forward;}
glm::vec3 Object::get_up() {if (type == CAMERA) return camera->up;}
// camera seter
void Object::set_pos(glm::vec3 v) {if (type == CAMERA) camera->position = v;}
void Object::set_for(glm::vec3 v) {if (type == CAMERA) camera->forward = v;}
void Object::set_up(glm::vec3 v) {if (type == CAMERA) camera->up = v;}
// screen geter
float Object::get_fov() {if (type == CAMERA) return camera->screen_data.fov;}
float Object::get_pro() {if (type == CAMERA) return camera->screen_data.proportion;}
float Object::get_min() {if (type == CAMERA) return camera->screen_data.min;}
float Object::get_max() {if (type == CAMERA) return camera->screen_data.max;}
// mesh geter
std::vector<unsigned short> & Object::get_indices() {if (type == MESH) return mesh->indices;}
std::vector<glm::vec3> & Object::get_vertices() {if (type == MESH) return mesh->vertices;}
std::vector<std::vector<unsigned short>> & Object::get_triangles() {if (type == MESH) return mesh->triangles;}
// transform
void Object::scale(float s) {if (type == MESH) mesh->scale(s);}
void Object::rotate(glm::mat3 r) {if (type == CAMERA) camera->rotate(r);}
void Object::translate(glm::vec3 t) {if (type == CAMERA) camera->translate(t);}


void Object::computeView(glm::mat4 & view)
{
    if (type == CAMERA)
    {
        glm::vec4 pos = glm::vec4(camera->position, 1);
        glm::vec4 forw = glm::vec4(camera->forward, 1);
        glm::vec4 up = glm::vec4(camera->up, 1);
        Graph* g = graph;
        while (g)
        {
            pos = (g->transform.get_mat())*pos;
            forw = (g->transform.get_mat())*forw;
            up = (g->transform.get_mat())*up;
            g = g->parent;
        }
        view = glm::lookAt(glm::vec3(pos), glm::vec3(pos) + glm::vec3(forw), glm::vec3(up));
    }
}
void Object::computeProj(glm::mat4 & proj)
{
    if (type == CAMERA)
        proj = glm::perspective(camera->screen_data.fov, camera->screen_data.proportion, camera->screen_data.min, camera->screen_data.max);
}

// Gl
void Object::buffering() {if (type == MESH) mesh->buffering();}
void Object::draw(GLuint matMVPid, glm::mat4 model, glm::mat4 & VP, glm::mat4 & MVP)
{
    if (type == MESH)
    {
        MVP = VP*model;
        mesh->draw(matMVPid, MVP);
    }
}
void Object::deleteBuff() {if (type == MESH) mesh->deleteBuff();}