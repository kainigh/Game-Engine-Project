#version 330 core

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



/*
#version 330 core


in vec3 Normal_cameraspace;
in vec3 LightDirection_cameraspace;
in vec3 Position_worldspace;

uniform sampler2D myTextureSampler;

in vec2 UV;
in vec3 FragPos;
in vec3 Normal;

out vec3 color;

uniform sampler2D grassTexture;
uniform sampler2D rockTexture;
uniform sampler2D snowTexture;
uniform vec3 LightPosition_worldspace;

// Light colors
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0); // White
uniform vec3 objectColor = vec3(1.0, 1.0, 1.0); // Orange-ish

void main(){
    // Simple diffuse lighting
    vec3 norm = normalize(Normal_cameraspace);
    vec3 lightDir = normalize(LightPosition_worldspace - FragPos);
    float diff = max(dot(Normal, lightDir), 0.0);

    // Distance to light
    float distance = length(LightDirection_cameraspace);

    // Attenuation
    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    vec3 diffuse = diff * lightColor * attenuation;
    vec3 ambient = 0.1 * lightColor;

    // Sample textures
    vec3 grassColor = texture(grassTexture, UV * 4.0).rgb;
    vec3 rockColor  = texture(rockTexture,  UV * 4.0).rgb;
    vec3 snowColor  = texture(snowTexture,  UV * 4.0).rgb;

    float height = FragPos.y;

    // Blending by height
    vec3 finalColor;
    if (height < 10.0)
        finalColor = grassColor;
    else if (height < 30.0)
        finalColor = mix(grassColor, rockColor, smoothstep(10.0, 30.0, height));
    else
        finalColor = mix(rockColor, snowColor, smoothstep(30.0, 50.0, height));

    finalColor.rgb *= diff; // Apply lighting
    color = finalColor * (ambient + diffuse) * objectColor * texture(myTextureSampler, UV).rgb * 4.5; // Apply texture and color
}

*/



/*
#version 330 core

in vec3 Normal_cameraspace;
in vec3 LightDirection_cameraspace;
in vec3 Position_worldspace;

uniform sampler2D myTextureSampler;
in vec2 UV;
out vec3 color;

uniform vec3 LightPosition_worldspace;

// Light colors
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0); // White
uniform vec3 objectColor = vec3(1.0, 1.0, 1.0); // Orange-ish

void main() {
    vec3 norm = normalize(Normal_cameraspace);
    vec3 lightDir = normalize(LightDirection_cameraspace);
    //vec3 texColor = texture(earthTexture, UV).rgb;

    // Diffuse component
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Distance to light
    float distance = length(LightDirection_cameraspace);
    
    // Attenuation
    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    vec3 diffuse = diff * lightColor * attenuation;
    vec3 ambient = 0.1 * lightColor;

    color = (ambient + diffuse) * objectColor * texture(myTextureSampler, UV).rgb * 3.5;
}
*/



/*
#version 330 core

in vec3 Normal_cameraspace;
in vec3 LightDirection_cameraspace;

out vec3 color;

void main() {
    // Normalize vectors
    vec3 n = normalize(Normal_cameraspace);
    vec3 l = normalize(LightDirection_cameraspace);

    float cosTheta = clamp(dot(n, l), 0, 1);
    vec3 diffuseColor = vec3(1.0, 0.7, 0.3); // orange-ish
    vec3 ambientColor = vec3(0.2, 0.2, 0.2);

    color = ambientColor + diffuseColor * cosTheta;
}
*/


/*
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{    
    FragColor = texture(texture_diffuse1, TexCoords);
}
*/
