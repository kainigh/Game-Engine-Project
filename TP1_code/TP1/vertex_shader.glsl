#version 330 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;

out vec2 UV;
out vec3 FragPos;
out vec3 Normal;
out vec3 LightDirection_cameraspace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 LightPosition_worldspace;

void main() {
    vec4 worldPosition = model * vec4(vertexPosition_modelspace, 1.0);
    FragPos = worldPosition.xyz;

    Normal = mat3(transpose(inverse(model))) * vertexNormal_modelspace;
    UV = vertexUV;

    // Calculate light direction in camera space
    vec3 vertexPosition_cameraspace = vec3(view * worldPosition);
    vec3 lightPosition_cameraspace = vec3(view * vec4(LightPosition_worldspace, 1.0));
    LightDirection_cameraspace = normalize(lightPosition_cameraspace - vertexPosition_cameraspace);

    gl_Position = projection * view * worldPosition;
    
}






/*
#version 330 core

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 LightPosition_worldspace;

out vec3 Position_worldspace;
out vec3 Normal_cameraspace;
out vec3 LightDirection_cameraspace;

out vec2 UV;  // Pass UV to fragment shader


void main() {
    vec4 vertexPosition_worldspace = model * vec4(vertexPosition_modelspace, 1);
    //Position_worldspace = vertexPosition_worldspace.xyz;
    Position_worldspace = (model * vec4(vertexPosition_modelspace, 1.0)).xyz;

    vec3 vertexPosition_cameraspace = (view * vertexPosition_worldspace).xyz;
    vec3 lightPosition_cameraspace = (view * vec4(LightPosition_worldspace,1)).xyz;

    LightDirection_cameraspace = lightPosition_cameraspace - vertexPosition_cameraspace;

    Normal_cameraspace = (view * model * vec4(vertexNormal_modelspace,0)).xyz;

    gl_Position = projection * vec4(vertexPosition_cameraspace,1);
    UV = vertexUV;
}
*/


/*
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords;    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
*/



