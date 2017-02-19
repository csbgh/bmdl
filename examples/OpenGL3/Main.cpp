#include <stdio.h>
#include <vector>

#include <GL/glew.h>
#include <GL/wglew.h>
#include <GLFW/glfw3.h>

#include "Shader.h"

#include "bmdl.h"

#define XRES    1280
#define YRES    720

struct Vert
{
	float x, y, z;
};

struct SubMesh
{
	uint32_t offset;
	uint32_t count;
};

struct Mesh
{
	GLuint vertexArrayID;
	GLuint vertexBuffer;
	GLuint indexBuffer;

	uint32_t vertexCount;
	uint32_t indiceCount;

	std::vector<SubMesh> subMeshes;
};

struct UniformBufferData
{
	BmMat4 modelViewProjection;
};

bool	InitializeGraphics();
void	Draw(const Mesh& mesh);
void	CreateMesh(Mesh* mesh, const void* vertexData, uint32_t vertexCount, const void* indiceData, uint32_t indiceCount, GLenum usage);
GLuint	CreateShaderObject(const char* source, GLenum shaderType);
GLuint	CreateBuffer(const void* data, uint32_t size, GLenum target, GLenum usage);

static void ErrorCallback(int error, const char* description);

GLFWwindow* window;
GLuint shaderID;
Mesh testMesh;
GLuint programID;
UniformBufferData uniformData;

std::vector<Mesh> meshList;

struct TestVert
{
	BmVec3 pos;
	BmVec3 normal;
};

int main()
{
	if (!InitializeGraphics())
		return 0;

	BmMat4 mvp;
	mvp[0][0] = 1.0f;

	// Load our model
	BmModel<TestVert, uint16_t>* model = bmdl::LoadModel<TestVert, uint16_t>("resources/Pyramid.bmf");

	// create meshes from model
	for (uint32_t m = 0; m < model->meshList.count; m++)
	{
		BmMesh<TestVert, uint16_t>* curMesh = &model->meshList[m];

		Mesh newMesh;
		CreateMesh(&newMesh,
				curMesh->vertices.data, curMesh->vertices.count,
				curMesh->indices.data, curMesh->indices.count,
				GL_DYNAMIC_DRAW);

		meshList.push_back(newMesh);

		printf("add mesh to draw %i, %i \n", curMesh->vertices.count, curMesh->indices.count);
	}

	// enter main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw model
		glUseProgram(programID);
		for (auto const& mesh : meshList)
		{
			Draw(mesh);
		}

		//Draw(testMesh);

		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;
}

void Draw(const Mesh& mesh)
{
	glBindVertexArray(mesh.vertexArrayID);

	uint32_t indexOffset = 0;
	uint32_t vertexOffset = 0;

	glDrawElementsBaseVertex(GL_TRIANGLES, mesh.indiceCount, GL_UNSIGNED_SHORT, (void*)(indexOffset * sizeof(GLushort)), vertexOffset);

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

	// create new GLFW window and set the current OpenGL context
	window = glfwCreateWindow(XRES, YRES, "BMDL OpenGL3 Example", NULL, NULL);
	glfwMakeContextCurrent(window);

	// initialize glew exit if error
	GLenum err = glewInit();
	if (GLEW_OK != err)
		return false;

	glViewport(0, 0, XRES, YRES);
	glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

	// set depth state
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_GREATER);

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

	// create our mesh
	Vert quadVerts[] = {
		{ -0.5f, -0.5f, 0.0f},
		{  0.5f, -0.5f, 0.0f },
		{  0.5f,  0.5f, 0.0f },
		{ -0.5f,  0.5f, 0.0f },
	};

	uint16_t quadIndices[] = {
		0, 1, 2,
		0, 2, 3
	};

	CreateMesh(&testMesh, quadVerts, 4, quadIndices, 6, GL_DYNAMIC_DRAW);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// Create uniform buffer
	GLuint uniformBuffer = CreateBuffer(&uniformData, sizeof(UniformBufferData), GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformBuffer, 0, sizeof(UniformBufferData));

	return true;
}

void CreateMesh(Mesh* mesh,
				const void* vertexData,
				uint32_t vertexCount,
				const void* indiceData,
				uint32_t indiceCount,
				GLenum usage)
{
	if (!mesh)
		return;

	mesh->vertexCount = vertexCount;
	mesh->indiceCount = indiceCount;

	// clear binded vertex array object
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
	uint32_t offset = 0;
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, vertStride, (GLvoid*)offset);

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

	// check the result of the shaders compilation
	GLint result = GL_FALSE;
	GLint infoLength = 0;

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLength);

	// output information from the compilation of the shader if there is any
	if (infoLength > 0)
	{
		GLchar *infoLog = new GLchar[infoLength];
		glGetShaderInfoLog(shaderID, infoLength, NULL, infoLog);

		printf("============== Shader Object Log =============\n");
		printf("Shader : \n%s", infoLog);
		printf("==============================================\n");

		delete[] infoLog;
	}

	return shaderID;
}

GLuint CreateBuffer(const void* data, uint32_t size, GLenum target, GLenum usage)
{
	// generate and bind new buffer
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(target, buffer);

	// fill buffer
	glBufferData(target, size, data, usage);

	return buffer;
}

static void ErrorCallback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}


/*

static const char *getGLErrorString(GLenum err)
{
switch (err)
{
case GL_NO_ERROR: return "GL_NO_ERROR";
case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
case GL_STACK_UNDERFLOW: return"GL_STACK_UNDERFLOW";
case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
default:
return "UNKNOWN_ERROR";
}
}

static void logGLError(const char *msg)
{
GLenum err = glGetError();

if (err == GL_NO_ERROR)
{
printf(msg);
return;
}

printf("GL3 : %s : [%s]", msg, getGLErrorString(err));
}
*/