#version 330 core

in vec2 UV;
uniform sampler2D myTextureSampler;
out vec4 color;

void main() {
    color = texture(myTextureSampler, UV);
}


/*
in vec2 UV;
in vec3 FragPos;
in vec3 Normal;
in vec3 LightDirection_cameraspace;

out vec4 color;

uniform sampler2D grassTexture;
uniform sampler2D rockTexture;
uniform sampler2D snowTexture;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(LightDirection_cameraspace);

    float diff = max(dot(norm, lightDir), 0.0);
    float ambientStrength = 0.4;
    float lighting = diff + ambientStrength;

    vec4 grass = texture(grassTexture, UV * 10.0);
    vec4 rock  = texture(rockTexture,  UV * 10.0);
    vec4 snow  = texture(snowTexture,  UV * 10.0);

    float h = FragPos.y;
    vec4 blended;

    if (h < 20.0)
        blended = grass;
    else if (h < 50.0)
        blended = mix(grass, rock, smoothstep(20.0, 50.0, h));
    else
        blended = mix(rock, snow, smoothstep(50.0, 80.0, h));

    blended.rgb *= lighting;
    color = blended;
}
*/
