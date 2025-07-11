/*   
    Universidade Federal do Rio Grande do Sul
             Instituto de Informática
       Departamento de Informática Aplicada

    INF01047 Fundamentos de Computação Gráfica
               Prof. Eduardo Gastal

                   TRABALHO FINAL

                Gisele Cervo Rotta
                Isadora Brigo Vidor
*/

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>
#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "object.h"
#include "collisions.h"

#define M_PI 3.14159265358979323846

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
void DrawVirtualObjectWithMaterial(const char* object_name, const tinyobj::material_t* material); // Desenha um objeto com material específico
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);
void TextRendering_ScoreandGameOVer(GLFWwindow* window);
void TextRendering_RecoverArrow(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

void ApplyMaterial(const tinyobj::material_t& material);
glm::vec3 CalculateBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
glm::vec3 ScreenToWorldCoordinates(double xpos, double ypos, GLFWwindow* window, glm::mat4 view, glm::mat4 projection);
void FireArrow(GLFWwindow* window, glm::mat4 view, glm::mat4 projection);
void UpdateArrow(float deltaTime);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    int          material_id; // ID do material associado ao objeto (-1 se não tiver material)
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Mapa para armazenar os modelos carregados e acessar seus materiais
std::map<std::string, ObjModel*> g_LoadedModels;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

float pos_x = 0.0f;
float pos_y = -10.0f;
float pos_z = 0.0f;

bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; 
bool g_MiddleMouseButtonPressed = false;
bool D_pressed = false;
bool W_pressed = false;
bool A_pressed = false;
bool S_pressed = false;
bool look_at = false;

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis para controle da flecha
bool g_ArrowFired = false;
bool g_ArrowCollided = false; 
float g_ArrowTime = 0.0f;
float g_ArrowDuration = 1.0f; 
glm::vec3 g_ArrowStartPos;
glm::vec3 g_ArrowTargetPos;
glm::vec3 g_ArrowControlPoint1;
glm::vec3 g_ArrowControlPoint2;
glm::vec3 g_ArrowCurrentPos;
glm::vec3 g_ArrowCurrentRotation;

// Variáveis globais para matrizes (para acesso no callback)
glm::mat4 g_CurrentView, g_CurrentProjection;

// Delta para variação do tempo
float g_DeltaTime = 0.0f;

// Transformações geométricas dos alvos
float T1_posx = 0.0f;
float T2_scale = 0.0f;
float T3_rotatez = 0.0f;

bool T1_flag = true;
bool T2_flag = true;

// Score e Game Over
int g_Score = 0; 
int tries = 0;
bool game_over = false;

// Variáveis que definem um programa de GPU (shaders).
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_material_id_uniform;
GLint g_lighting_model_uniform;
GLint g_Kd_uniform;
GLint g_Ka_uniform;
GLint g_Ks_uniform;
GLint g_q_uniform;

GLuint g_NumLoadedTextures = 0;

int main(int argc, char* argv[])
{
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "Final Project FCG", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }
    
    // Funções de callback para comunicação com o sistema operacional e interação do usuário.
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600);

    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    LoadShadersFromFiles();

    // Carregamento das texturas

    // Imagens da SkyBox
    LoadTextureImage("data/textures/left.png");       // TextureImage0
    LoadTextureImage("data/textures/right.png");      // TextureImage1
    LoadTextureImage("data/textures/bottom.png");     // TextureImage2
    LoadTextureImage("data/textures/top.png");        // TextureImage3
    LoadTextureImage("data/textures/front.png");      // TextureImage4
    LoadTextureImage("data/textures/back.png");       // TextureImage5

    // Texturas do modelo target
    LoadTextureImage("data/target/RGB_ca679fbef29d47908e43abddd6b40c6c_Styrofoam_diffuse.jpeg");        // TextureImage6
    LoadTextureImage("data/target/RGB_47bd90a446e546bca69fa88b48a09312_target-paper_diffuse.jpeg");     // TextureImage7
    LoadTextureImage("data/target/RGB_7011de0aa4ab44cb927a6767fa8aa3ef_wood_hinge_diffuse.jpeg");       // TextureImage8
    LoadTextureImage("data/target/RGB_da371e9e3c3d460c986fe6316c40bc6c_Wood_stand_Diffuse_final.jpeg"); // TextureImage9

    // Texturas do Character
    LoadTextureImage("data/character/RGB_1b6e32c5408a4a13ad1d8f411749c0e4_Eye_diff_001.png");                                // TextureImage10
    LoadTextureImage("data/character/RGB_6f1df117890d4d80893d2170dc81c4b3_ARCHER_FOR_SUBS_TSHIRT_2_BaseColor.1001.jpeg");    // TextureImage11
    LoadTextureImage("data/character/RGB_6f3d0106dc4540efb1e0628732bba968_ARCHER_FOR_SUBS_BELT_4_BaseColor.1001.jpeg");      // TextureImage12
    LoadTextureImage("data/character/RGB_694fa89b46224c7ab2192f701793093c_ARCHER_FOR_SUBS_ARCHER_012_BaseColor.1001.jpeg");  // TextureImage13
    LoadTextureImage("data/character/RGB_4886c183ab0b499793b6a5b448ef285d_WARRIOR_Body_new_low_001_defaultMat_BaseCo.jpeg"); // TextureImage14
    LoadTextureImage("data/character/RGB_b4890e0bef3e4568a4bd860662769c4e_ARCHER_FOR_SUBS_Material.001_BaseColor.100.jpeg"); // TextureImage15
    LoadTextureImage("data/character/RGB_e40db1c3e31f4d2a92b06d9a0bae4a48_Hair_DIff_01.jpeg");                               // TextureImage16

    // Textura do Arrow
    LoadTextureImage("data/arrow/WoodenArrowAlbedo.png"); //TextureImage17

    // Carregamento dos objetos dos modelos 3D
    ObjModel charactermodel("data/male_mesh.obj");
    ComputeNormals(&charactermodel);
    BuildTrianglesAndAddToVirtualScene(&charactermodel);

    ObjModel targetmodel("data/target/model.obj");
    ComputeNormals(&targetmodel);
    BuildTrianglesAndAddToVirtualScene(&targetmodel);

    ObjModel planemodel("data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel archermodel("data/character/model.obj");
    ComputeNormals(&archermodel);
    BuildTrianglesAndAddToVirtualScene(&archermodel);

    ObjModel arrowmodel("data/arrow/model.obj");
    ComputeNormals(&arrowmodel);
    BuildTrianglesAndAddToVirtualScene(&arrowmodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    TextRendering_Init();

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    float r = g_CameraDistance;
    float y = r*sin(g_CameraPhi);
    float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
    float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

    float speed = 4.0f;     
    float prev_time = (float)glfwGetTime();
    float delta_t;

    glm::vec4 camera_position_c;
    glm::vec4 camera_lookat_l;
    glm::vec4 camera_view_vector;
    glm::vec4 w = glm::vec4(0.0f,0.0f,1.0f,0.0f);
    glm::vec4 camera_up_vector; 

    if(look_at){
        g_CameraTheta = 0.0f; 
        g_CameraPhi = 0.0f;   
        g_CameraDistance = 3.5f;
    }
    else {
        g_CameraTheta = 3.141592f / 4;
        g_CameraPhi = 3.141592f / 6;  
        g_CameraDistance = 2.5f; 
    }

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(g_GpuProgramID);

        float current_time = (float)glfwGetTime();
        g_DeltaTime = current_time - prev_time;
        prev_time = current_time;

        // As variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. 
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        if(look_at){
            camera_position_c  = glm::vec4(pos_x + x, pos_y + 14.f + y, pos_z + 3.0f + z,1.0f); 
            camera_lookat_l    = glm::vec4(pos_x, pos_y + 14.f, pos_z + 3.0f,1.0f);
            camera_view_vector = camera_lookat_l - camera_position_c;
            camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);
        }
        else {
            camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);
            camera_view_vector = glm::vec4(-x, -y, -z, 0.0f);
            w = -camera_view_vector;
            glm::vec4 u = crossproduct(camera_up_vector, w);
        

            camera_position_c = glm::vec4(pos_x, pos_y+10.0f, pos_z, 1.0f);
        }

        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
        glm::mat4 projection;

        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -100.0f; // Posição do "far plane"

        // Projeção Perspectiva.
        float field_of_view = 3.141592 / 3.0f;
        projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

        glm::mat4 model = Matrix_Identity();
        glm::mat4 archer_model = Matrix_Identity();
        glm::mat4 arrow_model = Matrix_Identity();
        glm::mat4 planes[4]; 
        glm::mat4 targets[4]; 
        
        // Inicializa todos os elementos do array
        for(int i = 0; i < 4; i++) {
            planes[i] = Matrix_Identity();
            targets[i] = Matrix_Identity();
        }

        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // Armazena matrizes para acesso global
        g_CurrentView = view;
        g_CurrentProjection = projection;

        UpdateArrow(g_DeltaTime);

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

        // -------------  MOVIMENTAÇÃO -----------------
        float forward_x = -sin(g_CameraTheta);
        float forward_z = -cos(g_CameraTheta);

        float right_x = cos(g_CameraTheta);
        float right_z = -sin(g_CameraTheta);  
        float speed = 5.0f;

        if (W_pressed) {
            pos_x += speed * g_DeltaTime * forward_x;
            pos_z += speed * g_DeltaTime * forward_z;
        }
        if (S_pressed) {
            pos_x -= speed * g_DeltaTime * forward_x;
            pos_z -= speed * g_DeltaTime * forward_z;
        }
        if (D_pressed) {
            pos_x += speed * g_DeltaTime * right_x;
            pos_z += speed * g_DeltaTime * right_z;
        }
        if (A_pressed) {
            pos_x -= speed * g_DeltaTime * right_x;
            pos_z -= speed * g_DeltaTime * right_z;
        }

        // ARCHER
        archer_model = Matrix_Translate(pos_x, pos_y-13.0f, pos_z) 
        * Matrix_Rotate_Y(g_CameraTheta + M_PI)
        * Matrix_Scale(0.08f, 0.08f, 0.08f);
        model = archer_model;

        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, ARCHER);
        glUniform1i(g_lighting_model_uniform, 1); // Gouraud para ARCHER

        if(look_at){
            DrawVirtualObjectWithMaterial("object_0", &archermodel.materials[0]);
            DrawVirtualObjectWithMaterial("object_1", &archermodel.materials[1]);
            DrawVirtualObjectWithMaterial("object_2", &archermodel.materials[2]);
            DrawVirtualObjectWithMaterial("object_3", &archermodel.materials[3]);
            DrawVirtualObjectWithMaterial("object_4", &archermodel.materials[4]);
            DrawVirtualObjectWithMaterial("object_5", &archermodel.materials[5]);
            DrawVirtualObjectWithMaterial("object_6", &archermodel.materials[6]);
            DrawVirtualObjectWithMaterial("object_7", &archermodel.materials[7]);
            DrawVirtualObjectWithMaterial("object_8", &archermodel.materials[8]);
        }
    
        // TARGET 1
        model = Matrix_Translate(T1_posx, -23.0f, -17.0f)
        * Matrix_Rotate_X(3*M_PI/2)
        * Matrix_Scale(0.01f, 0.01f, 0.01f);
        targets[0] = model;
        
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TARGET);
        glUniform1i(g_lighting_model_uniform, 0); // Phong para TARGET
        
        DrawVirtualObjectWithMaterial("object_0_target", &targetmodel.materials[0]);
        DrawVirtualObjectWithMaterial("object_1_target", &targetmodel.materials[1]);
        DrawVirtualObjectWithMaterial("object_2_target", &targetmodel.materials[2]);
        DrawVirtualObjectWithMaterial("object_3_target", &targetmodel.materials[3]);
        DrawVirtualObjectWithMaterial("object_4_target", &targetmodel.materials[4]);
        DrawVirtualObjectWithMaterial("object_5_target", &targetmodel.materials[5]);

        // TARGET 2
        model = Matrix_Translate(15, -23.0f, 13)
        * Matrix_Rotate_X(3*M_PI/2)
        * Matrix_Rotate_Z(4.12)
        * Matrix_Scale(0.01f+T2_scale, 0.01f+T2_scale, 0.01f+T2_scale);
        targets[1] = model;
        
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TARGET);
        glUniform1i(g_lighting_model_uniform, 0); // Phong para TARGET

        DrawVirtualObjectWithMaterial("object_0_target", &targetmodel.materials[0]);
        DrawVirtualObjectWithMaterial("object_1_target", &targetmodel.materials[1]);
        DrawVirtualObjectWithMaterial("object_2_target", &targetmodel.materials[2]);
        DrawVirtualObjectWithMaterial("object_3_target", &targetmodel.materials[3]);
        DrawVirtualObjectWithMaterial("object_4_target", &targetmodel.materials[4]);
        DrawVirtualObjectWithMaterial("object_5_target", &targetmodel.materials[5]);

        // TARGET 3
        model = Matrix_Translate(-15, -23.0f, 13)
        * Matrix_Rotate_X(3*M_PI/2)
        * Matrix_Rotate_Z(-4.12+T3_rotatez)
        * Matrix_Scale(0.01f, 0.01f, 0.01f);
        targets[2] = model;
        
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TARGET);
        glUniform1i(g_lighting_model_uniform, 0); // Phong para TARGET

        DrawVirtualObjectWithMaterial("object_0_target", &targetmodel.materials[0]);
        DrawVirtualObjectWithMaterial("object_1_target", &targetmodel.materials[1]);
        DrawVirtualObjectWithMaterial("object_2_target", &targetmodel.materials[2]);
        DrawVirtualObjectWithMaterial("object_3_target", &targetmodel.materials[3]);
        DrawVirtualObjectWithMaterial("object_4_target", &targetmodel.materials[4]);
        DrawVirtualObjectWithMaterial("object_5_target", &targetmodel.materials[5]);

        float arrow_offset = -7.5f;
        // ARROW
        if (g_ArrowFired && !g_ArrowCollided) {
            // Usa a posição calculada pela curva de Bézier com rotação simplificada
            arrow_model = Matrix_Translate(g_ArrowCurrentPos.x, g_ArrowCurrentPos.y, g_ArrowCurrentPos.z)
            * Matrix_Rotate_Y(g_ArrowCurrentRotation.y + M_PI/2)
            * Matrix_Rotate_X(g_ArrowCurrentRotation.x + 5*M_PI/4.0f) // Ajusta a rotação X para apontar para frente
            * Matrix_Scale(0.3f, 0.3f, 0.3f)
            * Matrix_Translate(0.0f, 0.0f, arrow_offset);
        } else if (!g_ArrowCollided) {
            // Posição da flecha anexada ao archer, considerando sua rotação
            float archer_offset_x = -1.5f * cos(g_CameraTheta);
            float archer_offset_z = sin(g_CameraTheta);
            
            arrow_model =  Matrix_Translate(pos_x + archer_offset_x, pos_y, pos_z + archer_offset_z)
            *  Matrix_Rotate_Y(g_CameraTheta + M_PI)
            *  Matrix_Translate(-2.5f, 0, 3.5f)
            *  Matrix_Rotate_X(10.41)
            *  Matrix_Rotate_Y(9.82) 
            *  Matrix_Rotate_Z(11.58)
            *  Matrix_Scale(0.3f, 0.3f, 0.3f);
        }
        else if (g_ArrowCollided) {
            // Se a flecha colidiu, posiciona ela no local da colisão
            arrow_model = Matrix_Translate(g_ArrowCurrentPos.x, g_ArrowCurrentPos.y, g_ArrowCurrentPos.z)
            * Matrix_Rotate_Y(g_ArrowCurrentRotation.y + M_PI/2)
            * Matrix_Rotate_X(g_ArrowCurrentRotation.x + 5*M_PI/4.0f) // Ajusta a rotação X para apontar para frente
            * Matrix_Scale(0.3f, 0.3f, 0.3f)
            * Matrix_Translate(0.0f, 0.0f, arrow_offset);
        }
        model = arrow_model;
        
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, ARROW);
        glUniform1i(g_lighting_model_uniform, 0); // Phong para TARGET

        if((look_at || g_ArrowFired || g_ArrowCollided) && !game_over) {
            DrawVirtualObjectWithMaterial("WoodenArrow", &arrowmodel.materials[0]);
        }

        // PLANES
        glDisable(GL_CULL_FACE);

        // PLANE LEFT
        float size = 25.0;
        model = Matrix_Translate(-size, 0.0f, 0.0f)
        * Matrix_Rotate_Z(M_PI/2.0f)         // Inclina para o plano YZ, mas para o outro lado
        * Matrix_Scale(size, size, size);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE_LEFT);
        glUniform1i(g_lighting_model_uniform, 0); 
        DrawVirtualObject("the_plane");
        planes[0] = model; 

        // PLANE RIGHT
        model = Matrix_Translate(size, 0.0f, 0.0f)
        * Matrix_Rotate_Z(-M_PI/2.0)        // Inclina para o plano YZ
        * Matrix_Scale(size, size, size);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE_RIGHT);
        glUniform1i(g_lighting_model_uniform, 0); 
        DrawVirtualObject("the_plane");
        planes[1] = model;

        // PLANE BOTTOM
        model = Matrix_Translate(0.0f, -size, 0.0f) * Matrix_Scale(size,size,size);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE_BOTTOM);
        glUniform1i(g_lighting_model_uniform, 0); 
        DrawVirtualObject("the_plane");

        // PLANE TOP
        model = Matrix_Translate(0.0f, size, 0.0f) * Matrix_Scale(size,size,size);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE_TOP);
        glUniform1i(g_lighting_model_uniform, 0); 
        DrawVirtualObject("the_plane");

        // PLANE FRONT
        model = Matrix_Translate(0.0f, 0.0f, size)
        * Matrix_Rotate_X(M_PI/2.0f) // Flip plano para frente
        * Matrix_Scale(size, size, size);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE_FRONT);
        DrawVirtualObject("the_plane");
        planes[2] = model;

        // PLANE BACK
        model = Matrix_Translate(0.0f, 0.0f, -size)
        * Matrix_Rotate_X(-M_PI/2.0f) // Flip plano para frente
        * Matrix_Scale(size, size, size);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE_BACK);
        DrawVirtualObject("the_plane");
        planes[3] = model;

        // Teste de Intersecções
        BoundingBox archer_local_box = ComputeLocalBoundingBox(archermodel.attrib);
        BoundingBox arrow_local_box  = ComputeLocalBoundingBox(arrowmodel.attrib);
        BoundingBox target_local_box = ComputeLocalBoundingBox(targetmodel.attrib);
        BoundingBox plane_local_box = ComputeLocalBoundingBox(planemodel.attrib);

        BoundingBox archer_world_box = TransformBoundingBox(archer_local_box, archer_model);
        BoundingBox arrow_world_box  = TransformBoundingBox(arrow_local_box, arrow_model);
        BoundingBox target1_world_box = TransformBoundingBox(target_local_box, targets[0]);
        BoundingBox target2_world_box = TransformBoundingBox(target_local_box, targets[1]);
        BoundingBox target3_world_box = TransformBoundingBox(target_local_box, targets[2]);
        BoundingBox plane0_world_box = TransformBoundingBox(plane_local_box, planes[0]);
        BoundingBox plane1_world_box = TransformBoundingBox(plane_local_box, planes[1]);
        BoundingBox plane2_world_box = TransformBoundingBox(plane_local_box, planes[2]);
        BoundingBox plane3_world_box = TransformBoundingBox(plane_local_box, planes[3]);

        // Intersecção Archer
        if (IntersectAABB(archer_world_box, target1_world_box) ||  // Colisão cubo-cubo
            IntersectAABB(archer_world_box, target2_world_box) ||
            IntersectAABB(archer_world_box, target3_world_box) ||
            IntersectAABB(archer_world_box, plane0_world_box) ||  // Colisão cubo-plano
            IntersectAABB(archer_world_box, plane1_world_box) ||
            IntersectAABB(archer_world_box, plane2_world_box) ||
            IntersectAABB(archer_world_box, plane3_world_box)) {
            if (W_pressed) {
                pos_x -= speed * g_DeltaTime * forward_x;
                pos_z -= speed * g_DeltaTime * forward_z;
            }
            if (S_pressed) {
                pos_x += speed * g_DeltaTime * forward_x;
                pos_z += speed * g_DeltaTime * forward_z;
            }
            if (D_pressed) {
                pos_x -= speed * g_DeltaTime * right_x;
                pos_z -= speed * g_DeltaTime * right_z;
            }
            if (A_pressed) {
                pos_x += speed * g_DeltaTime * right_x;
                pos_z += speed * g_DeltaTime * right_z;
            }
        }

        if (PointInsideAABB(g_ArrowCurrentPos, target1_world_box) || // Colisão ponto-cubo
            PointInsideAABB(g_ArrowCurrentPos, target2_world_box) ||
            PointInsideAABB(g_ArrowCurrentPos, target3_world_box)){
            if(!g_ArrowCollided) {
                // Se a flecha colidiu, vai ficar fixa na posição da colisão
                g_ArrowFired = false;
                g_ArrowCollided = true;
                tries += 1;
                // Se a flecha colidiu com algum alvo, o jogador ganha 50 pontos
                g_Score += 50;
            }                
        }
        else if (IntersectAABB(arrow_world_box, plane0_world_box) || // Colisão cubo-plano
            IntersectAABB(arrow_world_box, plane1_world_box) ||
            IntersectAABB(arrow_world_box, plane2_world_box) ||
            IntersectAABB(arrow_world_box, plane3_world_box)){
            if(!g_ArrowCollided) {
                // Se a flecha colidiu, vai ficar fixa na posição da colisão
                g_ArrowFired = false;
                g_ArrowCollided = true;
                tries += 1;
            }
        }

        if (tries == 5){
            game_over = true;
        }

        TextRendering_ShowFramesPerSecond(window);
        TextRendering_ScoreandGameOVer(window);
        TextRendering_RecoverArrow(window);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{

    // Leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    // Criação de objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Envia imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene.
void DrawVirtualObject(const char* object_name)
{
    // Verifica se o objeto tem material associado e o aplica
    const SceneObject& obj = g_VirtualScene[object_name];
    if (obj.material_id >= 0 && g_LoadedModels.count(object_name) > 0) {
        const ObjModel* model = g_LoadedModels[object_name];
        if (obj.material_id < (int)model->materials.size()) {
            ApplyMaterial(model->materials[obj.material_id]);
            // Define o ID do material no shader
            glUniform1i(g_material_id_uniform, obj.material_id);
        }
    } else {
        // Define material_id como -1 se não houver material
        glUniform1i(g_material_id_uniform, -1);
    }

    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão utilizados para renderização.

void LoadShadersFromFiles()
{
    GLuint vertex_shader_id = LoadShader_Vertex("src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("src/shader_fragment.glsl");

    // Deleta o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Busca o endereço das variáveis definidas dentro do Vertex Shader para enviar dados para a placa de vídeo.
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_material_id_uniform = glGetUniformLocation(g_GpuProgramID, "material_id"); // Variável "material_id" em shader_fragment.glsl
    g_lighting_model_uniform = glGetUniformLocation(g_GpuProgramID, "lighting_model"); // Modelo de iluminação
    g_Kd_uniform         = glGetUniformLocation(g_GpuProgramID, "Kd_uniform"); // Propriedades do material
    g_Ka_uniform         = glGetUniformLocation(g_GpuProgramID, "Ka_uniform");
    g_Ks_uniform         = glGetUniformLocation(g_GpuProgramID, "Ks_uniform");
    g_q_uniform          = glGetUniformLocation(g_GpuProgramID, "q_uniform");

    // Vincula samplers de textura às unidades de textura corretas
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage9"), 9);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage9"), 9);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage10"), 10);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage11"), 11);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage12"), 12);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage13"), 13);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage14"), 14);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage15"), 15);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage16"), 16);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage17"), 17);
    glUseProgram(0);
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  u = b - a;
            const glm::vec4  v = c - a;
            const glm::vec4  n = crossproduct(u,v);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        // Verifica se há material associado à primeira face para usar como padrão do shape
        int shape_material_id = -1;
        if (!model->shapes[shape].mesh.material_ids.empty()) {
            shape_material_id = model->shapes[shape].mesh.material_ids[0];
        }

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; 
        theobject.num_indices    = last_index - first_index + 1; 
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;
        theobject.material_id    = shape_material_id; // ID do material associado

        g_VirtualScene[model->shapes[shape].name] = theobject;
        
        // Armazena o modelo para acessar materiais posteriormente
        g_LoadedModels[model->shapes[shape].name] = model;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 2)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());

    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. 
GLuint LoadShader_Vertex(const char* filename)
{
    // Cria um identificador (ID) para este shader
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carrega e compila o shader
    LoadShader(filename, vertex_shader_id);

    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL.
GLuint LoadShader_Fragment(const char* filename)
{
    // Cria um identificador (ID) para este shader.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carrega e compilam o shader
    LoadShader(filename, fragment_shader_id);

    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{

    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    glCompileShader(shader_id);

    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    delete [] log;
}

// Cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Cria um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    return program_id;
}

// Redimensiona janela do sistema operacional, alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse
double g_LastCursorPosX, g_LastCursorPosY;

// Callback chamado quando o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        g_MiddleMouseButtonPressed = false;
    }
}

// Callback chamado quando o usuário movimentar o cursor do mouse em cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualiza parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f/2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Callback chamado quando o usuário movimenta o scroll do mouse
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualiza a distância da câmera para a origem simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Callback chamado quando o usuário pressionar alguma tecla do teclado. 
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        look_at = !look_at;
    }
    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Se o usuário apertar a tecla espaço, dispara a flecha ou reseta os ângulos.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        // Dispara a flecha se ainda não foi disparada
        if (!g_ArrowFired && !g_ArrowCollided) {
            FireArrow(window, g_CurrentView, g_CurrentProjection);
        } 
    }

    // Se o usuário apertar a tecla C, reseta a posição da flecha.
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        g_ArrowCollided = false;
        g_ArrowCurrentPos = g_ArrowStartPos;
    }

    // Transformação Target 1 de Translação
    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        if (T1_flag) { // Ir para diretia
            T1_posx = T1_posx + 1.0f;   
            if (T1_posx >= 17.0f)
                T1_flag = false; // Reseta a posição se ultrapassar o limite
        } else {
            T1_posx = T1_posx - 1.0f;              
            if (T1_posx <= -17.0f)
                T1_flag = true; // Reseta a posição se ultrapassar o limite
        }
    }

    // Transformação Target 2 de Escala
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        if (T2_flag) { // Ir para diretia
            T2_scale = T2_scale + 0.002f;   
            if (T2_scale >= 0.01f)
                T2_flag = false; // Reseta a posição se ultrapassar o limite
        } else {
            T2_scale = T2_scale - 0.002f;              
            if (T2_scale <= 0.0f)
                T2_flag = true; // Reseta a posição se ultrapassar o limite
        }
    }

    // Transformação Target 3 de Rotação
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        T3_rotatez = T3_rotatez + M_PI/4; 
    }

    // Teste de teclas de movimentação do archer (W, A, S, D) 
    if (key == GLFW_KEY_D)
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            D_pressed = true;
        }
        else if (action == GLFW_RELEASE)
            D_pressed = false;
    }

    if (key == GLFW_KEY_A)
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            A_pressed = true;
        }
        else if (action == GLFW_RELEASE)
            A_pressed = false;
    }

    if (key == GLFW_KEY_S)
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            S_pressed = true;
        }
        else if (action == GLFW_RELEASE)
            S_pressed = false;
    }

    if (key == GLFW_KEY_W)
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            W_pressed = true;
        }
        else if (action == GLFW_RELEASE)
            W_pressed = false;
    }

}

// Função para aplicar as propriedades do material
void ApplyMaterial(const tinyobj::material_t& material)
{
    // Aplica as propriedades do material via uniformes, utilizados nos shaders posteriormente
    glUniform3f(g_Kd_uniform, 
        material.diffuse[0], 
        material.diffuse[1], 
        material.diffuse[2]);
    glUniform3f(g_Ka_uniform, 
        material.ambient[0], 
        material.ambient[1], 
        material.ambient[2]);
    glUniform3f(g_Ks_uniform, 
        material.specular[0], 
        material.specular[1], 
        material.specular[2]);
    glUniform1f(g_q_uniform, material.shininess);
}

// Desenha um objeto armazenado em g_VirtualScene com material específico
void DrawVirtualObjectWithMaterial(const char* object_name, const tinyobj::material_t* material)
{
    // Aplica o material se fornecido
    if (material != nullptr) {
        ApplyMaterial(*material);
    }
    
    DrawVirtualObject(object_name);
}

// Calcula ponto na curva cúbica de Bézier
glm::vec3 CalculateBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
    float u = 1.0f - t;
    
    glm::vec3 point = (u*u*u) * p0; // (1-t)^3 * P0
    point += 3 * (u*u) * t * p1;   // 3 * (1-t)^2 * t * P1
    point += 3 * u * (t*t) * p2;   // 3 * (1-t) * t^2 * P2
    point += (t*t*t) * p3;          // t^3 * P3
    
    return point;
}

// Converte coordenadas de tela para coordenadas do jogo
glm::vec3 ScreenToWorldCoordinates(double xpos, double ypos, GLFWwindow* window, 
                                   glm::mat4 view, glm::mat4 projection)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    // Normaliza coordenadas de tela para NDC
    float x = (2.0f * xpos) / width - 1.0f;
    float y = 1.0f - (2.0f * ypos) / height;
    
    // Calcula ray direction
    glm::mat4 invProjection = glm::inverse(projection);
    glm::mat4 invView = glm::inverse(view);
    
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = invProjection * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec4 rayWorld = invView * rayEye;
    
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorld));
    
    glm::vec4 cameraPos = invView * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Projeta o ray para um plano no nível do archer
    float planeY = pos_y - 15.0f;
    float t = (planeY - cameraPos.y) / rayDir.y;
    glm::vec3 targetPos = glm::vec3(cameraPos) + t * rayDir;
    
    return targetPos;
}

// Dispara a flecha
void FireArrow(GLFWwindow* window, glm::mat4 view, glm::mat4 projection)
{
    if (g_ArrowFired) return; // Já disparada
    
    g_ArrowFired = true;
    g_ArrowTime = 0.0f;
    
    // Calcula a posição inicial da flecha considerando a rotação e posição do archer
    float archer_offset_x = -1.5f * cos(g_CameraTheta);
    float archer_offset_z = sin(g_CameraTheta);
    g_ArrowStartPos = glm::vec3(pos_x + archer_offset_x, pos_y, pos_z + archer_offset_z);
    
    // Posição final da flecha com base na posição do cursor do mouse
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    g_ArrowTargetPos = ScreenToWorldCoordinates(xpos, ypos, window, view, projection);
    
    // Calcula pontos de controle para a curva de Bézier
    glm::vec3 direction = glm::normalize(g_ArrowTargetPos - g_ArrowStartPos);
    float distance = glm::length(g_ArrowTargetPos - g_ArrowStartPos);

    float arrow_length = 15.0926f;
    float arrow_scale = 0.3f;
    float arrow_offset = (arrow_length / 2.0f) * arrow_scale;

    g_ArrowStartPos += direction * arrow_offset;
    g_ArrowTargetPos += direction * arrow_offset;
    
    // Pontos de controle da curva 
    g_ArrowControlPoint1 = g_ArrowStartPos + direction * (distance * 1/3) + glm::vec3(0, 4.5f, 0);
    g_ArrowControlPoint2 = g_ArrowStartPos + direction * (distance * 2/3) + glm::vec3(0, 3.0f, 0);
    
    g_ArrowCurrentPos = g_ArrowStartPos;
}

// Atualiza a posição da flecha
void UpdateArrow(float deltaTime)
{
    if (!g_ArrowFired || g_ArrowCollided) return;
    
    g_ArrowTime += deltaTime;
    float t = g_ArrowTime / g_ArrowDuration;
    
    if (t >= 1.0f) {
        // Flecha chegou ao destino, reseta
        g_ArrowFired = false;
        g_ArrowTime = 0.0f;
        t = 1.0f;
    }
    
    // Calcula posição atual na curva de Bézier
    g_ArrowCurrentPos = CalculateBezierPoint(t, g_ArrowStartPos, g_ArrowControlPoint1, 
                                           g_ArrowControlPoint2, g_ArrowTargetPos);
    
    // Calcula orientação baseada na direção direta do alvo
    glm::vec3 direction = glm::normalize(g_ArrowTargetPos - g_ArrowStartPos);

    g_ArrowCurrentRotation.y = atan2(-direction.x, -direction.z);
    g_ArrowCurrentRotation.x = asin(direction.y);
    g_ArrowCurrentRotation.z = 0.0f;
}

// Callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Escreve na tela o número de quadros renderizados por segundo
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para renderizar o texto de "GAME OVER" e a pontuação na tela
void TextRendering_ScoreandGameOVer(GLFWwindow* window)
{
    char game_over_string[10] = "GAME OVER";
    char score_string[15];

    snprintf(score_string, sizeof(score_string), "SCORE: %d", g_Score);

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    int score_length = strlen(score_string);
    int game_over_length = strlen(game_over_string);

    float score_x = ((1.0f - (score_length * charwidth * 3.0f)) / 2.0f) - 0.5f;

    float game_over_x = ((1.0f - (game_over_length * charwidth * 7.0f)) / 2.0f) - 0.5f;

    TextRendering_PrintString(window, score_string, score_x, 1.0f - (lineheight * 3.0f), 3.0f);
    if (game_over) {
        TextRendering_PrintString(window, game_over_string, game_over_x, lineheight/2.0f, 7.0f); 
    }
}

// Renderizar mensagem de recuperar flecha
void TextRendering_RecoverArrow(GLFWwindow* window)
{
    if (!g_ArrowCollided) return; // Só mostra se a flecha estiver fixa
    
    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);
    
    const char* message = "Aperte a tecla C para recuperar a flecha";
    int numchars = strlen(message);
    
    float x = -1.0f + (2.0f - numchars * charwidth) / 2.0f; // Centralizado
    float y = -1.0f + lineheight * 2.0f; // Parte inferior da tela
    
    TextRendering_PrintString(window, message, x, y, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // Para cada face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // Para cada vertice na face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

