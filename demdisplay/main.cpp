#include <iostream>
#define GLEW_STATIC
#define GLM_ENABLE_EXPERIMENTAL

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "IL/il.h"
#include "IL/ilu.h"
#include "IL/ilut.h"
#include "loadshader.h"
#include "gisreader.h"
#include "camera.h"

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
using namespace std;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void Do_Movement();
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool keys[1024];
GLfloat lastX = 960, lastY = 540;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

int main(int argc, char* argv[])
{
	glfwInit();
	GLFWwindow* glfwwindow1 = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "demdisplay", nullptr, nullptr);
	if (glfwwindow1 == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(glfwwindow1);
	glfwSetKeyCallback(glfwwindow1, key_callback);
	glfwSetCursorPosCallback(glfwwindow1, mouse_callback);
    glfwSetScrollCallback(glfwwindow1, scroll_callback);
    glfwSetInputMode(glfwwindow1, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}
	int width, height;
	glfwGetFramebufferSize(glfwwindow1, &width, &height);
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	ShaderInfo shaders[] = {
	{GL_VERTEX_SHADER, "../demdisplay/terrainpos.vert"},
	{GL_FRAGMENT_SHADER, "../demdisplay/terrainpos.frag"},
	{GL_NONE, NULL}
	};
	GLuint program = LoadShaders(shaders);


	GLuint vao, vbo, ebo;
	string fname,strteximg, projection;
	double ox, oy;
	float min, max, xres, yres;
	vector<vector<float>> vecs;
	vector<Vertex> vertexes;
	vector<int> indices;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	fname = R"(E:\2017202042\openglproject\demdisplay\Debug\data\drycreek.tif)";
	getRawValuesFromFile(fname, vecs, min, max, xres, yres, projection,ox,oy,width,height);
	createMesh(vecs, xres, yres, max, indices, vertexes);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertexes.size(), &vertexes[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*indices.size(), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

	GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	ILboolean success;
	unsigned int imageID;
	ilInit();
	ilGenImages(1, &imageID);
	ilBindImage(imageID);
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
	strteximg = R"(E:\2017202042\openglproject\demdisplay\Release\data\drycreeksat.tif)";
	success = ilLoadImage((ILstring)strteximg.c_str());
	if (!success) {
		ilDeleteImages(1, &imageID);
		return 0;
	}

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGB, GL_UNSIGNED_BYTE, ilGetData());
    glGenerateMipmap(GL_TEXTURE_2D);
    ilBindImage(0);
	ilDeleteImage(imageID);
    glBindTexture(GL_TEXTURE_2D, 0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glPolygonMode(GL_FRONT, GL_FILL); 
	//glPolygonMode(GL_BACK, GL_LINE);
	while (!glfwWindowShouldClose(glfwwindow1))
	{
		GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
		glfwPollEvents();

		Do_Movement();
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUseProgram(program);
		glm::mat4 persprojection = glm::perspective(glm::radians<float>(camera.Zoom), (float)width / (float)height, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		GLint viewLoc = glGetUniformLocation(program, "view");
        GLint projLoc = glGetUniformLocation(program, "projection");
		//glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(persprojection));
		glm::mat4 model = glm::mat4(1.0f);//µ¥Î»¾ØÕó
		//model = glm::translate(model, {-0.5, 0.0, -0.6});
		GLint modelLoc = glGetUniformLocation(program, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glfwSwapBuffers(glfwwindow1);
	}
	glDeleteProgram(program);
	glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
	glfwDestroyWindow(glfwwindow1);
	glfwTerminate();

	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
    {
        if(action == GLFW_PRESS)
            keys[key] = true;
        else if(action == GLFW_RELEASE)
            keys[key] = false;	
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left
    
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}	

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

void Do_Movement()
{
    if(keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if(keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if(keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}