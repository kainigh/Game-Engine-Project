#include <iostream>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "scenegraph.hpp"
#include "objloader.hpp"

using namespace glm;
using namespace std;

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
Mesh::Mesh() {std::cout<<"mesh"<<std::endl;}
Mesh::Mesh(std::vector<unsigned int> _indices, std::vector<Vertex> _vertices)
{
    indices = _indices;
    vertices = _vertices;
}
Mesh::Mesh(std::string file, float s) // s for scale
{
    std::vector<glm::vec3 *> v;
    loadOFF(file, v, indices);
    int vs = v.size();
    vertices.resize(vs);
    for (int i=0 ; i<vs ; i++) vertices[i] = Vertex{*v[i]};
    if (s != 1) scale(s);
}

void Mesh::scale(float s)
{
    for (int i=0 ; i<vertices.size() ; i++)
        for (int j=0 ; j<3 ; j++) vertices[i].Position[j] *= s;
}

// openGL
void Mesh::buffering()
{
    glGenVertexArrays(1, &VertexArrayID);
    glGenBuffers(1, &vertexbuffer);
    glGenBuffers(1, &elementbuffer);

    glBindVertexArray(VertexArrayID);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0] , GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);	
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);	
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    // vertex bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
    // ids
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
    // weights
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
    glBindVertexArray(0);
}
void Mesh::draw(GLuint matMVPid, glm::mat4 & MVP)
{
    glBindVertexArray(VertexArrayID);
    glUniformMatrix4fv(matMVPid, 1, GL_FALSE, &MVP[0][0]);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0 );
}
void Mesh::deleteBuff()
{
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &elementbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// VMesh

// function
void VMesh::processNode(aiNode *node, const aiScene *scene)
{
    // process each mesh located at the current node
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        // data to fill
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        //std::vector<Texture> textures;

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        /*
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        */
        meshs.push_back(Mesh(indices, vertices));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }

}

// constructor
VMesh::VMesh(std::string file)
{
    std::cout<<"Vmesh"<<std::endl;
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(file, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    // retrieve the directory path of the filepath
    directory = file.substr(0, file.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
}

void VMesh::scale(float s) {for (int i=0 ; i<meshs.size() ; i++) meshs[i].scale(s);}

// openGL
void VMesh::buffering() {for (int i=0 ; i<meshs.size() ; i++) meshs[i].buffering();}
void VMesh::draw(GLuint matMVPid, glm::mat4 & MVP) {for (int i=0 ; i<meshs.size() ; i++) meshs[i].draw(matMVPid, MVP);}
void VMesh::deleteBuff() {for (int i=0 ; i<meshs.size() ; i++) meshs[i].deleteBuff();}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Graph

// constructor
Graph::Graph(Transform t, Graph* p)
{
    transform = t;
    if (p != NULL) p->add_child(this);
    childs.resize(0);
    objects.resize(0);
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
    for (int i=0 ; i<objects.size() ; i++)
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
Object::Object(VMesh* m, Graph* g)
{
    type = VMESH;
    vmesh = m;
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
std::vector<unsigned int> & Object::get_indices() {if (type == MESH) return mesh->indices;}
std::vector<Vertex> & Object::get_vertices() {if (type == MESH) return mesh->vertices;}
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
void Object::buffering()
{
    if (type == MESH) mesh->buffering();
    if (type == VMESH) vmesh->buffering();
}
void Object::draw(GLuint matMVPid, glm::mat4 model, glm::mat4 & VP, glm::mat4 & MVP)
{
    if (type == MESH)
    {
        MVP = VP*model;
        mesh->draw(matMVPid, MVP);
    }
    if (type == VMESH)
    {
        MVP = VP*model;
        vmesh->draw(matMVPid, MVP);
    }
}
void Object::deleteBuff()
{
    if (type == MESH) mesh->deleteBuff();
    if (type == VMESH) vmesh->deleteBuff();
}