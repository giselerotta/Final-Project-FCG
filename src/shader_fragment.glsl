#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem)
in vec2 texcoords;

// Cor calculada no vertex shader para Gouraud
in vec3 gouraud_color;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define CHARACTER 0
#define PLANE_LEFT 11
#define PLANE_RIGHT 12
#define PLANE_BOTTOM 13
#define PLANE_TOP 14
#define PLANE_FRONT 15
#define PLANE_BACK 16
#define TARGET 2
#define ARCHER 3
#define ARROW 4

uniform int object_id;

// ID do material do objeto atual
uniform int material_id;

// Modelo de iluminação: 0 = Phong, 1 = Gouraud
uniform int lighting_model;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;
uniform sampler2D TextureImage6;
uniform sampler2D TextureImage7;
uniform sampler2D TextureImage8;
uniform sampler2D TextureImage9;
uniform sampler2D TextureImage10;
uniform sampler2D TextureImage11;
uniform sampler2D TextureImage12;
uniform sampler2D TextureImage13;
uniform sampler2D TextureImage14;
uniform sampler2D TextureImage15;
uniform sampler2D TextureImage16;
uniform sampler2D TextureImage17;

// Variáveis para acesso às propriedades espectrais do objeto
uniform vec3 Kd_uniform;
uniform vec3 Ka_uniform;
uniform vec3 Ks_uniform;
uniform float q_uniform;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    // Luz mais frontal e alta para reduzir sombras
    vec4 l = normalize(vec4(0.2,1.0,1.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = -l + 2*n*(dot(n,l)); 

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    float q; // Expoente especular para o modelo de iluminação de Phong

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    vec4 texcolor = vec4(1.0, 1.0, 1.0, 1.0); // Cor padrão branca
    if ( object_id == CHARACTER )
    {
        Kd = vec3(0.08, 0.4, 0.8);
        Ks = vec3(0.8, 0.8, 0.8);
        Ka = vec3(0.04,0.2,0.4);
        q = 32.0;
    }
    else if (object_id == TARGET)
    {
        // Usa as propriedades do material MTL via uniformes
        Kd = Kd_uniform;
        Ka = Ka_uniform;
        Ks = Ks_uniform;
        q = q_uniform;
        
        // Aplica textura baseada no material ID
        if (material_id == 0) {
            texcolor = texture(TextureImage6, texcoords); // target-paper
        } else if (material_id == 1) {
            texcolor = texture(TextureImage7, texcoords); // wood_hinge
        } else if (material_id == 2) {
            texcolor = texture(TextureImage8, texcoords); // Styrofoam
        } else if (material_id == 3) {
            texcolor = texture(TextureImage9, texcoords); // Wood_stand
        } else if (material_id == 4) {
            texcolor = texture(TextureImage6, texcoords); // target-paper (repetido)
        } else if (material_id == 5) {
            texcolor = texture(TextureImage8, texcoords); // Styrofoam (repetido)
        }
        
        // Mistura a textura com as propriedades do material
        Kd = Kd * texcolor.rgb;
    }
    else if (object_id == ARCHER)
    {
        // Usa as propriedades do material MTL via uniformes
        Kd = Kd_uniform;
        Ka = Ka_uniform;
        Ks = Ks_uniform;
        q = q_uniform;
        
        // Aplica textura baseada no material ID
        if (material_id == 0) { // CARA - OK
            texcolor = texture(TextureImage13, texcoords);  
        } else if (material_id == 1) { // Arco
         texcolor = texture(TextureImage12, texcoords); 
        } else if (material_id == 2) { // Cabelo do archer
            texcolor = texture(TextureImage16, texcoords);  
        } else if (material_id == 3) { // Membros inferiores do archer
            texcolor = texture(TextureImage15, texcoords);
        } else if (material_id == 4) { // Roupa do archer
            texcolor = texture(TextureImage11, texcoords);  
        } else if (material_id == 5) { // Corpo do archer
          texcolor = texture(TextureImage14, texcoords); 
        } else if (material_id == 6) { 
          texcolor = texture(TextureImage10, texcoords); 
        } else if (material_id == 7) { // Olhos do archer 
            texcolor = texture(TextureImage10, texcoords); 
        } else if (material_id == 8) { 
            texcolor = texture(TextureImage11, texcoords); 
        }
        
        // Mistura a textura com as propriedades do material
        Kd = Kd * texcolor.rgb;
    }
    else if (object_id == ARROW)
    {
        // Usa as propriedades do material MTL via uniformes
        Kd = Kd_uniform;
        Ka = Ka_uniform;
        Ks = Ks_uniform;
        q = q_uniform;
        
        // Aplica textura baseada no material ID
        if (material_id == 0) {
            texcolor = texture(TextureImage17, texcoords); 
        } 
        
        // Mistura a textura com as propriedades do material
        Kd = Kd * texcolor.rgb;
    }
    else if (
        object_id == PLANE_LEFT  || object_id == PLANE_RIGHT ||
        object_id == PLANE_TOP   || object_id == PLANE_BOTTOM ||
        object_id == PLANE_FRONT || object_id == PLANE_BACK )
    {
        vec2 uv;
        if (object_id == PLANE_LEFT) {
            uv = vec2(1.0 - texcoords.y, texcoords.x);
            texcolor = texture(TextureImage0, uv);
        }
        else if (object_id == PLANE_RIGHT) {
            uv = vec2(texcoords.y, 1.0 - texcoords.x);
            texcolor = texture(TextureImage1, uv);
        }
        else if (object_id == PLANE_BOTTOM) {
            uv = vec2(texcoords.x, 1.0 - texcoords.y);
            texcolor = texture(TextureImage2, uv);
        }
        else if (object_id == PLANE_TOP) {
            uv = vec2(texcoords.x, texcoords.y);
            texcolor = texture(TextureImage3, uv);
        }
        else if (object_id == PLANE_FRONT) {
            uv = vec2(texcoords.x, texcoords.y);
            texcolor = texture(TextureImage4, uv);
        }
        else if (object_id == PLANE_BACK) {
            uv = vec2(1.0 - texcoords.x, 1.0 - texcoords.y);
            texcolor = texture(TextureImage5, uv);
        }

        color.rgb = texcolor.rgb;
        color.a = 1.0;

        // Correção gamma
        color.rgb = pow(color.rgb, vec3(1.0/2.2));
        return;
    }
    else // Objeto desconhecido = preto
    {
        Kd = vec3(0.0,0.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.0,0.0,0.0);
        q = 1.0;
    }

    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,1.0); 

    // Espectro da luz ambiente 
    vec3 Ia = vec3(0.4,0.4,0.4); 

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd*I*max(0.3,dot(n,l)); 

    // Termo ambiente
    vec3 ambient_term = Ka*Ia; 

    // Termo especular utilizando o modelo de iluminação de Phong
    vec3 phong_specular_term  = Ks*I*(pow(max(0,dot(r,v)),q)); 

    color.a = 1;

    if (lighting_model == 1) {
        // Modelo Gouraud: usa a cor interpolada do vertex shader
        // Aplica texturas se disponível
        
        color.rgb = gouraud_color * texcolor.rgb;
    } else {
        // Modelo Phong: calcula iluminação no fragment shader

        color.rgb = lambert_diffuse_term;
    }

    // Cor final com correção gamma, considerando monitor sRGB.
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}

