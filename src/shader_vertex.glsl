#version 330 core

// Atributos de vértice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a função BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Modelo de iluminação: 0 = Phong, 1 = Gouraud
uniform int lighting_model;

// Propriedades do material para Gouraud
uniform vec3 Kd_uniform;
uniform vec3 Ka_uniform;
uniform vec3 Ks_uniform;
uniform float q_uniform;

// Atributos de vértice que serão gerados como saída ("out") pelo Vertex Shader.
// ** Estes serão interpolados pelo rasterizador! ** gerando, assim, valores
// para cada fragmento, os quais serão recebidos como entrada pelo Fragment
// Shader. Veja o arquivo "shader_fragment.glsl".
out vec4 position_world;
out vec4 normal;

out vec2 texcoords;

// Cor calculada no vertex shader para Gouraud
out vec3 gouraud_color;

void main()
{
    // A variável gl_Position define a posição final de cada vértice
    // OBRIGATORIAMENTE em "normalized device coordinates" (NDC)

    // O código em "main.cpp" define os vértices dos modelos em coordenadas
    // locais de cada modelo (array model_coefficients). Abaixo, utilizamos
    // operações de modelagem, definição da câmera, e projeção, para computar
    // as coordenadas finais em NDC (variável gl_Position). Após a execução
    // deste Vertex Shader, a placa de vídeo (GPU) fará a divisão por W.

    gl_Position = projection * view * model * model_coefficients;

    // Posição do vértice atual no sistema de coordenadas global.
    position_world = model * model_coefficients;

    // Normal do vértice atual no sistema de coordenadas global.
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    texcoords = texture_coefficients;

    // Calcula iluminação Gouraud no vertex shader se necessário
    if (lighting_model == 1) {
        vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
        vec4 camera_position = inverse(view) * origin;

        // Vetores para iluminação
        vec4 p = position_world;
        vec4 n = normalize(normal);
        vec4 l = normalize(vec4(0.2, 1.0, 1.0, 0.0));
        vec4 v = normalize(camera_position - p);
        vec4 r = -l + 2*n*(dot(n,l));

        // Propriedades do material
        vec3 Kd = Kd_uniform;
        vec3 Ka = Ka_uniform;
        vec3 Ks = Ks_uniform;
        float q = q_uniform;

        // Espectros de luz
        vec3 I = vec3(1.0, 1.0, 1.0);
        vec3 Ia = vec3(0.4, 0.4, 0.4);

        // Termos de iluminação
        vec3 lambert_diffuse_term = Kd * I * max(0.3, dot(n, l));
        vec3 ambient_term = Ka * Ia;
        vec3 phong_specular_term = Ks * I * pow(max(0, dot(r, v)), q);

        gouraud_color = lambert_diffuse_term + ambient_term + phong_specular_term;
    } 
    else {
        gouraud_color = vec3(1.0, 1.0, 1.0); // Cor padrão para Phong
    }
}

