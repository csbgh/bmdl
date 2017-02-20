#include <vector>

#include <GL/glew.h>
#include <GL/wglew.h>
#include <GLFW/glfw3.h>
#include "bmdl.h"
#include "Shader.h"

#define XRES    1280
#define YRES    720
#define DEG2RAD 0.0174533f

struct Vert { BmVec3 pos; BmVec3 normal; };
struct SubMesh { uint32_t offset; uint32_t count; };

struct Mesh
{
	GLuint vertexArrayID;
	GLuint vertexBuffer;
	GLuint indexBuffer;

	uint32_t vertexCount;
	uint32_t indiceCount;

	std::vector<SubMesh> subMeshes;
};

struct UniformBufferData { BmMat4 modelViewProjection; };

bool	InitializeGraphics();
void	Draw(const Mesh& mesh);
void	CreateMesh(Mesh* mesh, const void* vertexData, uint32_t vertexCount, const void* indiceData, uint32_t indiceCount, GLenum usage);
GLuint	CreateShaderObject(const char* source, GLenum shaderType);
GLuint	CreateBuffer(const void* data, uint32_t size, GLenum target, GLenum usage);
void	UpdateBuffer(GLuint buffer, const void* data, uint32_t size, GLenum target, GLenum usage);

static void ErrorCallback(int error, const char* description);
static BmMat4 GetMVPMat(float fovy, float aspect, float zNear, float zFar, BmVec4 translation, float rotation);
static BmMat4 GetRotationMat(float angle, BmVec3 axis);

GLFWwindow* window;
GLuint shaderID;
GLuint programID;

GLuint uniformBuffer;
UniformBufferData uniformData;

std::vector<Mesh> meshList;

int main()
{
	if (!InitializeGraphics())
		return 0;

	// Load our model
	BmModel<Vert>* model = bmdl::LoadModel<Vert>("resources/Lara.bmf"); // TODO : Force setting layout argument, and maybe vert also...?

	// create meshes from model
	for (uint32_t m = 0; m < model->meshList.count; m++)
	{
		BmMesh<Vert, uint16_t>* curMesh = &model->meshList[m];

		Mesh newMesh;
		CreateMesh(&newMesh, curMesh->vertices.data, curMesh->vertices.count, curMesh->indices.data, curMesh->indices.count, GL_DYNAMIC_DRAW);

		meshList.push_back(newMesh);
	}

	BmVec4 camPos = BmVec4(0.0f, -2.15f, -4.0f, 1.0f);
	float camRotation = 0.0f;

	// enter main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// update camera view
		camRotation += 0.2f;
		uniformData.modelViewProjection = GetMVPMat(70.0f, static_cast<float>(XRES) / static_cast<float>(YRES), 0.001f, 1000.0f, camPos, camRotation);
		UpdateBuffer(uniformBuffer, &uniformData, sizeof(UniformBufferData), GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);

		// draw meshes of model
		glUseProgram(programID);
		for (auto const& mesh : meshList)
			Draw(mesh);

		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}

void Draw(const Mesh& mesh)
{
	glBindVertexArray(mesh.vertexArrayID);
	glDrawElementsBaseVertex(GL_TRIANGLES, mesh.indiceCount, GL_UNSIGNED_SHORT, 0, 0);
	glBindVertexArray(0);
}

bool InitializeGraphics()
{
	// initialize set error callback for glfw
	glfwSetErrorCallback(ErrorCallback);
	if (!glfwInit())
		return false;

	// ask for opengl 3.3 core profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// create new GLFW window and set the current OpenGL context
	window = glfwCreateWindow(XRES, YRES, "BMDL - OpenGL3 Example", NULL, NULL);
	glfwMakeContextCurrent(window);

	// initialize glew exit if error
	GLenum err = glewInit();
	if (GLEW_OK != err)
		return false;

	glViewport(0, 0, XRES, YRES);

	// enable depth testing
	glEnable(GL_DEPTH_TEST);

	// create Shader
	GLuint vertexShader = CreateShaderObject(vertSource, GL_VERTEX_SHADER);
	GLuint fragmentShader = CreateShaderObject(fragSource, GL_FRAGMENT_SHADER);
	programID = glCreateProgram();

	// attach vertex and fragment shaders
	glAttachShader(programID, vertexShader);
	glAttachShader(programID, fragmentShader);

	// link and set shader program
	glLinkProgram(programID);
	glUseProgram(programID);

	// Create uniform buffer
	uniformBuffer = CreateBuffer(&uniformData, sizeof(UniformBufferData), GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformBuffer, 0, sizeof(UniformBufferData));

	return true;
}

void CreateMesh(Mesh* mesh, const void* vertexData, uint32_t vertexCount, const void* indiceData, uint32_t indiceCount, GLenum usage)
{
	if (!mesh)
		return;

	mesh->vertexCount = vertexCount;
	mesh->indiceCount = indiceCount;

	// clear bound vertex array object
	glBindVertexArray(0);

	// create and bind a new VAO
	glGenVertexArrays(1, &mesh->vertexArrayID);
	glBindVertexArray(mesh->vertexArrayID);

	//uint32 stride = GetAttributeMaskSize(vertexAttributeFlags);
	uint32_t vertStride = 6 * sizeof(float);
	uint32_t indiceStride = sizeof(uint16_t);

	// Create vertex and index buffers
	mesh->vertexBuffer = CreateBuffer(vertexData, vertexCount * vertStride, GL_ARRAY_BUFFER, usage);
	mesh->indexBuffer = CreateBuffer(indiceData, indiceCount * indiceStride, GL_ELEMENT_ARRAY_BUFFER, usage);

	// set vertex attributes
	glEnableVertexAttribArray(0); // position
	glVertexAttribPointer(0, 3, GL_FLOAT, false, vertStride, (GLvoid*)0);
	glEnableVertexAttribArray(1); // normal
	glVertexAttribPointer(1, 3, GL_FLOAT, false, vertStride, (GLvoid*)(3*sizeof(float)));

	// reset bound vao so that it is not modified
	glBindVertexArray(0);

	// reset
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLuint CreateShaderObject(const char* source, GLenum shaderType)
{
	// create new shader object of the given type
	shaderID = glCreateShader(shaderType);

	// set shader object source code and compile
	const GLchar* shaderSource = (const GLchar*)source;
	glShaderSource(shaderID, 1, &shaderSource, nullptr);
	glCompileShader(shaderID);

	return shaderID;
}

// generate new buffer and fill it with data
GLuint CreateBuffer(const void* data, uint32_t size, GLenum target, GLenum usage)
{
	GLuint buffer;
	glGenBuffers(1, &buffer);
	UpdateBuffer(buffer, data, size, target, usage);

	return buffer;
}

void UpdateBuffer(GLuint buffer, const void* data, uint32_t size, GLenum target, GLenum usage)
{
	glBindBuffer(target, buffer);
	glBufferData(target, size, data, usage);
}

static void ErrorCallback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static BmMat4 GetMVPMat(float fovy, float aspect, float zNear, float zFar, BmVec4 translation, float rotation)
{
	// model
	BmMat4 model = GetRotationMat(rotation, BmVec3(0.0f, 0.0f, 1.0f));

	// view
	BmMat4 viewTranslation;
	viewTranslation[3] = translation;
	BmMat4 viewRot = GetRotationMat(-70.0f, BmVec3(1.0f, 0.0f, 0.0f));
	BmMat4 view = viewTranslation * viewRot;

	// perspective projection
	BmMat4 projection(0.0f);

	float tanHalfFovy = tan((fovy * DEG2RAD) / 2.0f);

	projection[0][0] = 1.0f / (aspect * tanHalfFovy);
	projection[1][1] = 1.0f / (tanHalfFovy);
	projection[2][2] = -(zFar + zNear) / (zFar - zNear);
	projection[2][3] = -1.0f;
	projection[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);

	return projection * view * model;
}

static BmMat4 GetRotationMat(float angle, BmVec3 axis)
{
	float r = angle * DEG2RAD;
	float c = cos(r);
	float s = sin(r);

	BmVec3 c1 = BmVec3(1.0f - c) * axis;

	BmMat4 rot;
	rot[0][0] = c + c1[0] * axis[0];
	rot[0][1] = c1[0] * axis[1] + s * axis[2];
	rot[0][2] = c1[0] * axis[2] - s * axis[1];

	rot[1][0] = c1[1] * axis[0] - s * axis[2];
	rot[1][1] = c + c1[1] * axis[1];
	rot[1][2] = c1[1] * axis[2] + s * axis[0];

	rot[2][0] = c1[2] * axis[0] + s * axis[1];
	rot[2][1] = c1[2] * axis[1] - s * axis[0];
	rot[2][2] = c + c1[2] * axis[2];

	return rot;
}