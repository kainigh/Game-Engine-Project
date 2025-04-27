#ifndef SCENEGRAPH_HPP
#define SCENEGRAPH_HPP


// enum
enum typeObj {MESH, CAMERA};


// struct
struct ScreenData
{
    float fov;
    float proportion;
    float min;
    float max;
};


// class
class Transform
{
    private :
    glm::mat4 scale;
    glm::mat4 rotation;
    glm::mat4 translation;
    glm::mat4 matrix;
    bool computed = false;

    public :
    // constructor
    Transform(glm::mat4 s, glm::mat4 r, glm::mat4 t); // with matrix
    Transform(float s=1, glm::mat4 r=glm::mat4(1), glm::vec3 t=glm::vec3(0, 0, 0)); // float for scale and vec3 for translation
    Transform(glm::vec3 t); // vec3 for translation
    Transform(glm::mat4 r); // mat4 for rotation

    // accessor
    glm::mat4 get_mat();
    void set_scale(glm::mat4 s);
    void set_scale(float s);
    void set_rotation(glm::mat4 r);
    void set_translation(glm::mat4 t);
    void set_translation(glm::vec3 t);
};

class Camera
{
    public :
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 up;
    ScreenData & screen_data;

    // constructor
    Camera(ScreenData & sd);
    Camera(glm::vec3 _position, glm::vec3 _forward, glm::vec3 _up, ScreenData & sd);

    // transform
    void rotate(glm::mat3 r);
    void translate(glm::vec3 t);
};

class Mesh
{
    public :
    std::vector<unsigned short> indices;
    std::vector<glm::vec3> vertices;
    std::vector<std::vector<unsigned short>> triangles;

    GLuint VertexArrayID;
    GLuint vertexbuffer;
    GLuint elementbuffer;

    // constructor
    Mesh();
    Mesh(std::vector<unsigned short> _indices, std::vector<glm::vec3> _vertices, std::vector<std::vector<unsigned short>> _triangles);
    Mesh(std::string file, float s=1); // s for scale

    void scale(float s);

    // openGL
    void buffering();
    void draw(GLuint matMVPid, glm::mat4 & MVP);
    void deleteBuff();
};

class Object;

class Graph
{
    public :
    Graph* parent;
    std::vector<Graph*> childs;
    std::vector<Object*> objects;
    Transform transform;

    // constructor
    Graph(Transform t=Transform(), Graph* p=NULL);

    // graph tree
    void set_parent(Graph* p);
    bool has_child(Graph* c);
    void add_child(Graph* c);

    // objects
    bool has_object(Object* o);
    void add_object(Object* o);
    void set_transform(Transform t);

    // openGL
    void buffering();
    void draw(GLuint matMVPid, glm::mat4 model, glm::mat4 & VP, glm::mat4 & MVP);
    void deleteBuff();
};

class Object
{
    private :
    typeObj type;
    Mesh* mesh;
    Camera* camera;

    public :
    Graph* graph;

    // constructor
    Object(Mesh* m, Graph* g=NULL);
    Object(Camera* c, Graph* g=NULL);
    void set_graph(Graph* g);

    // camera geter
    glm::vec3 get_pos();
    glm::vec3 get_for();
    glm::vec3 get_up();
    // camera seter
    void set_pos(glm::vec3 v);
    void set_for(glm::vec3 v);
    void set_up(glm::vec3 v);
    // screen geter
    float get_fov();
    float get_pro();
    float get_min();
    float get_max();
    // mesh geter
    std::vector<unsigned short> & get_indices();
    std::vector<glm::vec3> & get_vertices();
    std::vector<std::vector<unsigned short>> & get_triangles();
    // transform (modify mesh or camera, not just the object)
    void scale(float s);    // mesh
    void rotate(glm::mat3 r);   // camera
    void translate(glm::vec3 t);    // camera

    void computeView(glm::mat4 & view);
    void computeProj(glm::mat4 & proj);

    // Gl
    void buffering();
    void draw(GLuint matMVPid, glm::mat4 model, glm::mat4 & VP, glm::mat4 & MVP);
    void deleteBuff();
};

#endif