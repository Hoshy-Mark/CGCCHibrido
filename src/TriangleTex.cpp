/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 13/08/2024
 *
 */

#include <iostream>
#include <string>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

#include <cmath>

// Protótipo da função de callback de teclado
// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
GLuint setupGeometry();

void drawCube(GLuint shaderID, GLuint VAO, vec3 position, vec3 scaleVec, float angle, vec3 axis = vec3(0.0, 0.0, 1.0));

vec3 cubePosition = vec3(400.0f, 300.0f, 0.0f);
vec3 cubeScale = vec3(100.0f, 100.0f, 100.0f); // agora escala 3D
float cubeRotationX = 0.0f;
float cubeRotationY = 0.0f;
float cubeRotationZ = 0.0f;

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Vertex Shader (GLSL) com atributo de cor
const GLchar *vertexShaderSource = R"(
#version 400 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out vec3 vertexColor;

uniform mat4 projection;
uniform mat4 model;

void main()
{
    gl_Position = projection * model * vec4(position, 1.0);
    vertexColor = color;
}
)";

// Fragment Shader (GLSL) recebe cor interpolada
const GLchar *fragmentShaderSource = R"(
#version 400 core
in vec3 vertexColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0);
}
)";
// Função MAIN
int main()
{
    // Inicialização da GLFW
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Cubo Colorido por Vértice", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();
    GLuint VAO = setupGeometry();

    glUseProgram(shaderID);

    // Matriz de projeção ortográfica 3D
    mat4 projection = ortho(0.0f, 800.0f, 0.0f, 600.0f, -500.0f, 500.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        drawCube(shaderID, VAO, cubePosition, cubeScale, 0.0f);

        // Exemplo: desenha outras instâncias
        drawCube(shaderID, VAO, cubePosition + vec3(200.0f, 0.0f, 0.0f), cubeScale * 0.5f, 0.0f);
        drawCube(shaderID, VAO, cubePosition + vec3(-200.0f, -100.0f, 0.0f), cubeScale * 0.75f, 0.0f);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    const float moveSpeed = 10.0f;
    const float scaleSpeed = 0.05f;
    const float rotationSpeed = 5.0f;

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // Fechar com ESC
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, GL_TRUE);

        // Movimento no eixo X e Z
        if (key == GLFW_KEY_W)
            cubePosition.y += moveSpeed;
        if (key == GLFW_KEY_S)
            cubePosition.y -= moveSpeed;
        if (key == GLFW_KEY_A)
            cubePosition.x -= moveSpeed;
        if (key == GLFW_KEY_D)
            cubePosition.x += moveSpeed;

        // Movimento no eixo Y
        if (key == GLFW_KEY_I)
            cubePosition.z += moveSpeed;
        if (key == GLFW_KEY_J)
            cubePosition.z -= moveSpeed;

        // Escala
        if (key == GLFW_KEY_LEFT_BRACKET) // [
            cubeScale *= (1.0f - scaleSpeed);
        if (key == GLFW_KEY_RIGHT_BRACKET) // ]
            cubeScale *= (1.0f + scaleSpeed);

        // Rotação
        if (key == GLFW_KEY_X)
            cubeRotationX += rotationSpeed;
        if (key == GLFW_KEY_Y)
            cubeRotationY += rotationSpeed;
        if (key == GLFW_KEY_Z)
            cubeRotationZ += rotationSpeed;
    }
}


// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::VERTEX_SHADER_COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::FRAGMENT_SHADER_COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER_PROGRAM_LINKING_FAILED\n"
             << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Carrega uma textura de arquivo e retorna o id da textura na GPU, além de retornar a largura e altura da imagem carregada
GLuint loadTexture(string filePath, int &width, int &height)
{
	int nrChannels;
	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
	if (!data)
	{
		cout << "Falha ao carregar textura: " << filePath << endl;
		return 0;
	}
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	// Configurações de filtro de textura
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S = eixo x da textura
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // T = eixo y da textura
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Determina o formato da textura de acordo com o número de canais da imagem
	GLenum format;
	if (nrChannels == 1)
		format = GL_RED;
	else if (nrChannels == 3)
		format = GL_RGB;
	else if (nrChannels == 4)
		format = GL_RGBA;
	else
		format = GL_RGB;
	// Carrega os dados da imagem na textura da GPU
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	return textureID;
}

// Esta função agora se chama drawCube, pois o que ela desenha é um cubo texturizado
// Para simplicidade, o parâmetro 'color' não está sendo usado (você pode implementá-lo para colorir o cubo)
// A função usa as rotações globais cubeRotationX, cubeRotationY e cubeRotationZ para aplicar rotação no cubo
void drawCube(GLuint shaderID, GLuint VAO, vec3 position, vec3 scaleVec, float angle, vec3 axis)
{
    glUseProgram(shaderID);

    mat4 model = mat4(1.0f);

    // Translação, escala e rotação
    model = translate(model, position);
    model = rotate(model, radians(cubeRotationX), vec3(1.0f, 0.0f, 0.0f));
    model = rotate(model, radians(cubeRotationY), vec3(0.0f, 1.0f, 0.0f));
    model = rotate(model, radians(cubeRotationZ), vec3(0.0f, 0.0f, 1.0f));
    model = scale(model, scaleVec);

    GLuint modelLoc = glGetUniformLocation(shaderID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// Setup da geometria do cubo: cria VAO e VBO com os dados de posição e coordenadas de textura
GLuint setupGeometry()
{
    GLfloat vertices[] = {
        // Positions          // Colors (RGB)
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, // vermelho
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, // verde
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, // azul
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, // amarelo
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f, // magenta
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f, // ciano
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f, // branco
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f, // cinza
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.5f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.5f, 1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  0.3f, 0.7f, 0.9f,
         0.5f,  0.5f, -0.5f,  0.6f, 0.1f, 0.3f,
         0.5f, -0.5f, -0.5f,  0.8f, 0.5f, 0.2f,
         0.5f, -0.5f, -0.5f,  0.8f, 0.5f, 0.2f,
         0.5f, -0.5f,  0.5f,  0.2f, 0.8f, 0.5f,
         0.5f,  0.5f,  0.5f,  0.3f, 0.7f, 0.9f,

        -0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.3f,
         0.5f, -0.5f, -0.5f,  0.1f, 0.9f, 0.3f,
         0.5f, -0.5f,  0.5f,  0.3f, 0.9f, 0.1f,
         0.5f, -0.5f,  0.5f,  0.3f, 0.9f, 0.1f,
        -0.5f, -0.5f,  0.5f,  0.9f, 0.3f, 0.1f,
        -0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.3f,

        -0.5f,  0.5f, -0.5f,  0.2f, 0.3f, 0.9f,
         0.5f,  0.5f, -0.5f,  0.3f, 0.2f, 0.9f,
         0.5f,  0.5f,  0.5f,  0.9f, 0.3f, 0.2f,
         0.5f,  0.5f,  0.5f,  0.9f, 0.3f, 0.2f,
        -0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.3f,
        -0.5f,  0.5f, -0.5f,  0.2f, 0.3f, 0.9f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return VAO;
}
