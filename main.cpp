/*

TODO:

1. Finish model loading functionality
2. Independent window class with UI
3. Integrate any physics library (bullet) and create some examples
4. Extend camera functionality (rotations with quaternions, camera rotating around specific point ability)
5. // ...

*/

#define GLEW_STATIC
#define EXIT_SUCCESS 0

#include <gl/glew.h>
#include <gl/glfw3.h>

#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <gl/glm/gtc/type_ptr.hpp>
#include <gl/glm/gtc/random.hpp>
#include <SOIL.h>

#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <cassert>
#include <iostream>
#include <stdexcept>

#include "shader_manager.h"
#include "camera.h"
#include "model.h"

glm::vec2 WINDOW_SIZE(1200, 800);
glm::vec2 lastMousePos(WINDOW_SIZE.x/2.0, WINDOW_SIZE.y/2.0);
glm::vec3 cameraPosition(0.0, 0.0, 25.0);

Camera camera(cameraPosition, glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 1.0, 0.0));

bool keys[1024];
void key_callback(GLFWwindow* window, int, int, int, int);
void do_movement(const GLfloat&);
void mouse_callback(GLFWwindow* window, double, double);
void scroll_callback(GLFWwindow* window, double, double);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (action == GLFW_PRESS)
    {
        keys[key] = true;
    }
    if (action == GLFW_RELEASE)
    {
        keys[key] = false;
    }
}

void do_movement(const GLdouble& dt)
{
    if (keys[GLFW_KEY_W])  //  forward
    {
        camera.handleKeyInput(FORWARD, dt);
    }
    if (keys[GLFW_KEY_S])  //  backward
    {
        camera.handleKeyInput(BACKWARD, dt);
    }
    if (keys[GLFW_KEY_A])  //  left
    {
        camera.handleKeyInput(LEFT, dt);
    }
    if (keys[GLFW_KEY_D])  //  right
    {
        camera.handleKeyInput(RIGHT, dt);
    }
}

void mouse_callback(GLFWwindow* window, double currentMouseX, double currentMouseY)
{
    GLdouble dx = currentMouseX - lastMousePos.x;
    GLdouble dy = lastMousePos.y - currentMouseY;
    lastMousePos.x = currentMouseX;
    lastMousePos.y = currentMouseY;
    camera.handleMouseInput(dx, dy);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.handleMouseScrollInput(xoffset, yoffset);
}

int main(int argc, char *argv[])
{
    //	initialise GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("glfwInit failed");
        return -1;
    }
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    GLFWwindow* window = glfwCreateWindow((int)WINDOW_SIZE.x, (int)WINDOW_SIZE.y, "Opengl demo", nullptr, nullptr);
    if (!window)
    {
        throw std::runtime_error("glfwOpenWindow failed. Can your hardware handle OpenGL 4.5?");
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowPos(window, 10, 50);
    glViewport(0, 0, (GLsizei)WINDOW_SIZE.x, (GLsizei)WINDOW_SIZE.y);
    glEnable(GL_DEPTH_TEST);

    //	initialise GLEW
    glewExperimental = GL_TRUE;                     //  stops glew crashing on OSX :-/
    if (glewInit() != GLEW_OK)
    {
        throw std::runtime_error("glewInit failed");
    }

    ShaderManager shaderManager;
    Shader vshader(GL_VERTEX_SHADER, "shaders/vshader");
    Shader fshader(GL_FRAGMENT_SHADER, "shaders/fshader");
    Shader gshader(GL_GEOMETRY_SHADER, "shaders/gshader");

    GLuint shaderProgram = shaderManager.buildProgram(vshader, fshader, gshader);
    shaderManager.use(shaderProgram);

	Model nanosuit("nanosuit/nanosuit.obj");

    glm::mat4 projection, view, model, T, Tback, R, S, mvp, pv, freeTranslate, normalMatrix;
    projection = glm::perspective(camera.getZOOM(), WINDOW_SIZE.x/WINDOW_SIZE.y, 0.1f, 10000.0f);
	
	GLuint cameraPositionLoc = glGetUniformLocation(shaderProgram, "cameraPosition"),
			modelLoc =		   glGetUniformLocation(shaderProgram, "model"), 	
			viewLoc =		   glGetUniformLocation(shaderProgram, "view"),
			mvpLoc =		   glGetUniformLocation(shaderProgram, "mvp"),
			normalMatrixLoc =  glGetUniformLocation(shaderProgram, "normalMatrix");

    GLdouble currentFrame = 0.0f, 
			 lastFrame = 0.0f,
			 dt = 0.0f;

	while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        currentFrame = glfwGetTime();
        dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        projection = glm::perspective(camera.getZOOM(), WINDOW_SIZE.x/WINDOW_SIZE.y, 0.1f, 10000.0f);
        view = camera.getViewMatrix();
        glUniform3f(cameraPositionLoc, camera.position.x, camera.position.y, camera.position.z);
		
		//	render model and send trasnform matrices to shader

		//glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(model));
		//glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(view));
		//glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
		//glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));



        glfwSwapBuffers(window);
    }


    glfwTerminate();
    return EXIT_SUCCESS;
}
