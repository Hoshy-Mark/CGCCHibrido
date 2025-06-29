
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <unordered_map>

#include "json.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

using namespace std;
using namespace glm;
using json = nlohmann::json;

bool mouseEnabled = true;
// --- Configurações ---
const GLuint WIDTH = 800, HEIGHT = 600;

GLFWwindow *window;

// Dados para o cubo
vector<vec3> positions;
vector<vec2> texCoords;
vector<vec3> normals;

// Shader
GLuint shaderProgram;

// VAO e VBO
GLuint VAO, VBO;

// --- Estrutura do Cubo ---
struct Cube
{
	vec3 position;
	vec3 rotation; // rotações em graus
	vec3 scale;
	GLuint textureID;
};

vector<Cube> cubes;
int selectedCube = 0; // cubo selecionado

// --- Câmera FPS ---
class Camera
{
public:
	vec3 position;
	float yaw;
	float pitch;

	Camera(vec3 pos = vec3(0.0f, 0.0f, 3.0f), float y = -90.0f, float p = 0.0f)
		: position(pos), yaw(y), pitch(p) {}

	mat4 getViewMatrix()
	{
		vec3 front;
		front.x = cos(radians(yaw)) * cos(radians(pitch));
		front.y = sin(radians(pitch));
		front.z = sin(radians(yaw)) * cos(radians(pitch));
		front = normalize(front);

		vec3 right = normalize(cross(front, vec3(0.0f, 1.0f, 0.0f)));
		vec3 up = normalize(cross(right, front));

		return lookAt(position, position + front, up);
	}

	void moveForward(float delta)
	{
		vec3 front = getFrontVector();
		position += delta * front;
	}

	void moveRight(float delta)
	{
		vec3 right = getRightVector();
		position += delta * right;
	}

	void moveUp(float delta)
	{
		position.y += delta;
	}

	void rotate(float deltaYaw, float deltaPitch)
	{
		yaw += deltaYaw;
		pitch += deltaPitch;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

private:
	vec3 getFrontVector()
	{
		vec3 front;
		front.x = cos(radians(yaw)) * cos(radians(pitch));
		front.y = sin(radians(pitch));
		front.z = sin(radians(yaw)) * cos(radians(pitch));
		return normalize(front);
	}
	vec3 getRightVector()
	{
		vec3 front = getFrontVector();
		return normalize(cross(front, vec3(0.0f, 1.0f, 0.0f)));
	}
};

Camera camera;

// Estado mouse
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;

// Controle delta tempo
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Funções
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window);
GLuint loadTexture(const string &path);
bool loadOBJ(const string &objPath);
GLuint setupShader();
void setupGeometry();
void drawCube(const Cube &cube);
bool loadCubesFromJSON(const string &jsonPath);

const char *vertexShaderSource = R"(
#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    FragPos = vec3(model * vec4(aPos,1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
}
)";

const char *fragmentShaderSource = R"(
#version 400 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
    vec3 ambient = 0.2 * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float shininess = 32.0;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = spec * lightColor;

    vec3 phong = (ambient + diffuse + specular);

    vec4 texColor = texture(texture1, TexCoord);
    FragColor = vec4(phong, 1.0) * texColor;
}
)";

int main()
{
	string cubeJsonPath = "cubes.json";												   // ajuste para seu arquivo JSON
	string objPath = "C:/Users/Kamar/Downloads/CGCCHibrido/assets/Modelos3D/Cube.obj"; // seu arquivo OBJ do cubo

	// Inicializa GLFW
	if (!glfwInit())
	{
		cout << "Failed to initialize GLFW\n";
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Cubes Movable with FPS Camera", nullptr, nullptr);
	if (!window)
	{
		cout << "Failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Configura callback mouse
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD\n";
		return -1;
	}

	glViewport(0, 0, WIDTH, HEIGHT);
	glEnable(GL_DEPTH_TEST);

	// Carrega cubos do JSON
	if (!loadCubesFromJSON(cubeJsonPath))
	{
		cout << "Failed to load cubes JSON\n";
		return -1;
	}

	// Carrega OBJ
	if (!loadOBJ(objPath))
	{
		cout << "Failed to load OBJ\n";
		return -1;
	}

	setupGeometry();
	shaderProgram = setupShader();

	// Carregar texturas dos cubos
	for (auto &cube : cubes)
	{
	}


	// Luz e câmera
	vec3 lightPos(3.0f, 3.0f, 3.0f);
	vec3 lightColor(1.0f, 1.0f, 1.0f);
	vec3 objectColor(1.0f, 1.0f, 1.0f);

	mat4 projection = perspective(radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

	while (!glfwWindowShouldClose(window))
	{
		// Tempo
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Input
		processInput(window);

		// Render
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
		mat4 view = camera.getViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));

		glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, value_ptr(lightPos));
		glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, value_ptr(camera.position));
		glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, value_ptr(lightColor));
		glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, value_ptr(objectColor));

		for (const Cube &cube : cubes)
		{
			drawCube(cube);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

// Função que carrega o OBJ (simples, sem indices, triangulado)
// Atribui valores aos vetores globais: positions, texCoords, normals
bool loadOBJ(const string &objPath)
{
	ifstream file(objPath);
	if (!file.is_open())
	{
		cout << "Failed to open OBJ file: " << objPath << endl;
		return false;
	}

	vector<vec3> temp_positions;
	vector<vec2> temp_texCoords;
	vector<vec3> temp_normals;

	vector<unsigned int> vertexIndices, texCoordIndices, normalIndices;

	string line;
	while (getline(file, line))
	{
		if (line.substr(0, 2) == "v ")
		{
			istringstream s(line.substr(2));
			vec3 v;
			s >> v.x >> v.y >> v.z;
			temp_positions.push_back(v);
		}
		else if (line.substr(0, 3) == "vt ")
		{
			istringstream s(line.substr(3));
			vec2 vt;
			s >> vt.x >> vt.y;
			temp_texCoords.push_back(vt);
		}
		else if (line.substr(0, 3) == "vn ")
		{
			istringstream s(line.substr(3));
			vec3 vn;
			s >> vn.x >> vn.y >> vn.z;
			temp_normals.push_back(vn);
		}
		else if (line.substr(0, 2) == "f ")
		{
			istringstream s(line.substr(2));
			string vertex1, vertex2, vertex3;
			s >> vertex1 >> vertex2 >> vertex3;

			unsigned int vi[3], ti[3], ni[3];
			for (int i = 0; i < 3; ++i)
			{
				string vertexStr = (i == 0) ? vertex1 : (i == 1) ? vertex2
																 : vertex3;
				size_t pos1 = vertexStr.find('/');
				size_t pos2 = vertexStr.find('/', pos1 + 1);

				vi[i] = stoi(vertexStr.substr(0, pos1)) - 1;

				ti[i] = stoi(vertexStr.substr(pos1 + 1, pos2 - pos1 - 1)) - 1;

				ni[i] = stoi(vertexStr.substr(pos2 + 1)) - 1;
			}

			for (int i = 0; i < 3; ++i)
			{
				positions.push_back(temp_positions[vi[i]]);
				texCoords.push_back(temp_texCoords[ti[i]]);
				normals.push_back(temp_normals[ni[i]]);
			}
		}
	}
	file.close();
	return true;
}

// Setup VAO, VBO
void setupGeometry()
{
	vector<float> data;
	for (size_t i = 0; i < positions.size(); ++i)
	{
		// pos x,y,z
		data.push_back(positions[i].x);
		data.push_back(positions[i].y);
		data.push_back(positions[i].z);

		// tex coords u,v
		data.push_back(texCoords[i].x);
		data.push_back(texCoords[i].y);

		// normals x,y,z
		data.push_back(normals[i].x);
		data.push_back(normals[i].y);
		data.push_back(normals[i].z);
	}

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

	// posição
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	// textura
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// normal
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

// Compila e cria shader program
GLuint setupShader()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);

	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		cout << "Error compiling vertex shader:\n"
			 << infoLog << endl;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		cout << "Error compiling fragment shader:\n"
			 << infoLog << endl;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		cout << "Error linking shader program:\n"
			 << infoLog << endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

// Carrega textura e retorna ID
GLuint loadTexture(const string &path)
{
	if (path.empty())
	{
		cout << "loadTexture: caminho vazio\n";
		return 0;
	}

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
	if (!data)
	{
		cout << "Failed to load texture: " << path << endl;
		return 0;
	}
	cout << "Loaded texture: " << path << " (" << width << "x" << height << "), channels: " << nrChannels << endl;

	GLuint textureID;
	glGenTextures(1, &textureID);

	GLenum format;
	if (nrChannels == 1)
		format = GL_RED;
	else if (nrChannels == 3)
		format = GL_RGB;
	else if (nrChannels == 4)
		format = GL_RGBA;
	else
	{
		cout << "Formato de textura não suportado: " << nrChannels << " canais\n";
		stbi_image_free(data);
		return 0;
	}

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);
	return textureID;
}

// Desenha cubo dado (posição, rotação, escala, textura)
void drawCube(const Cube &cube)
{
	mat4 model = mat4(1.0f);
	model = translate(model, cube.position);
	model = rotate(model, radians(cube.rotation.x), vec3(1, 0, 0));
	model = rotate(model, radians(cube.rotation.y), vec3(0, 1, 0));
	model = rotate(model, radians(cube.rotation.z), vec3(0, 0, 1));
	model = scale(model, cube.scale);

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(model));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cube.textureID);
	glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)positions.size());
	glBindVertexArray(0);
}
static bool mKeyPressedLastFrame = false;

// Processa input de teclado (movimenta câmera e cubo selecionado)
void processInput(GLFWwindow *window)
{

	// Detecta tecla M para ligar/desligar mouse
    bool mKeyPressed = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
    if (mKeyPressed && !mKeyPressedLastFrame)
    {
        mouseEnabled = !mouseEnabled;
        if (mouseEnabled)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true; // resetar para evitar salto no movimento
            cout << "Mouse control ENABLED\n";
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cout << "Mouse control DISABLED\n";
        }
    }

    mKeyPressedLastFrame = mKeyPressed;
	float cameraSpeed = 2.5f * deltaTime;
	float cubeMoveSpeed = 1.0f * deltaTime;
	float cubeRotateSpeed = 45.0f * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// Movimenta câmera
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.moveForward(cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.moveForward(-cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.moveRight(-cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.moveRight(cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.moveUp(cameraSpeed);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.moveUp(-cameraSpeed);

	// Seleção de cubo 1-9
	for (int i = GLFW_KEY_1; i <= GLFW_KEY_9; i++)
	{
		if (glfwGetKey(window, i) == GLFW_PRESS)
		{
			int idx = i - GLFW_KEY_1;
			if (idx < (int)cubes.size())
			{
				selectedCube = idx;
			}
		}
	}

	Cube &c = cubes[selectedCube];

	// Movimento cubo selecionado (no plano XY e eixo Z)
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		c.position.y += cubeMoveSpeed;
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		c.position.y -= cubeMoveSpeed;
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		c.position.x -= cubeMoveSpeed;
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		c.position.x += cubeMoveSpeed;
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		c.position.z += cubeMoveSpeed;
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		c.position.z -= cubeMoveSpeed;

	// Rotação cubo selecionado (IJKL/U/O)
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		c.rotation.x += cubeRotateSpeed;
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		c.rotation.x -= cubeRotateSpeed;
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		c.rotation.y += cubeRotateSpeed;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		c.rotation.y -= cubeRotateSpeed;
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
		c.rotation.z += cubeRotateSpeed;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		c.rotation.z -= cubeRotateSpeed;
}
// Callback para controlar o olhar da câmera pelo mouse
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (!mouseEnabled)
        return;

    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)(xpos - lastX);
    float yoffset = (float)(lastY - ypos); // invertido porque Y-coord do mouse é do topo
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.rotate(xoffset, yoffset);
}

bool loadCubesFromJSON(const string &jsonPath)
{
	cout << "Abrindo arquivo JSON: " << jsonPath << endl;
	ifstream file("C:/Users/Kamar/Downloads/CGCCHibrido/assets/cube.json"); // substitua aqui pelo caminho absoluto
	if (!file.is_open())
	{
		cout << "Erro ao abrir arquivo JSON: " << jsonPath << endl;
		return false;
	}

	json j;
	file >> j;

	unordered_map<int, unsigned int> idToTextureID;

	for (const auto &c : j)
	{
		int id = c["id"];
		string texPath = c["texture"];
		cout << "Lendo cubo com id: " << id << ", textura: '" << texPath << "'" << endl;

		if (texPath.empty())
		{
			cout << "loadTexture: caminho vazio para id " << id << endl;
			continue; // evita tentar carregar textura vazia
		}

		if (idToTextureID.find(id) == idToTextureID.end())
		{
			unsigned int textureID = loadTexture(texPath);
			if (textureID == 0)
			{
				cout << "Falha ao carregar textura: " << texPath << endl;
				return false;
			}
			idToTextureID[id] = textureID;
		}

		Cube cube;
		cube.position = vec3(c["initial_position"][0], c["initial_position"][1], c["initial_position"][2]);
		cube.rotation = vec3(0.0f);
		cube.scale = vec3(1.0f);
		cube.textureID = idToTextureID[id];

		cubes.push_back(cube);
	}
	return true;
}