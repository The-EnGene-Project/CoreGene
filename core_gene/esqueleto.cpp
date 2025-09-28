#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include "gl_includes.h"
#include <iostream>

#include "error.h"
#include "input_handlers.h"
// #include "shader.h"
#include "scene.h"

// ShaderPtr shd;


double savedTime = 0;
double updateTimer = 0;
const double updateInterval = 1.0/60.0; // 60 fps

static void initialize()
{
  // config do OpenGL
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  // glFrontFace(GL_CCW);
  // glCullFace(GL_BACK);
  // glEnable(GL_CULL_FACE);
  // glPolygonMode(GL_FRONT, GL_FILL); // ERRADO

  // inicia Shader Program e SceneGraph
  scene::graph()->initializeBaseShader("../shaders/vertex.glsl","../shaders/fragment.glsl");
  // scene::graph()->setView(0,1,0,1,0,1);

  // CENTERPIECE
  // inicia geometria estática

}

static void display(GLFWwindow * win)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // CENTERPIECE
  // atualiza geometria dinâmica

  // desenha geometria
  scene::graph()->draw();
  
  // errorcheck
  Error::Check("display");
}

static GLFWwindow* WindowSetup(int width, int height);

int main(void) {

    GLFWwindow* win = WindowSetup(800, 800);

    setInputCallbacks(win);

    initialize();

    while (!glfwWindowShouldClose(win)) {
      double currentTime =  glfwGetTime();
      double elapseTime = currentTime - savedTime;
      savedTime = currentTime;
        
      updateTimer += elapseTime;

      if(updateTimer >= updateInterval){
        display(win);
        glfwSwapBuffers(win);
        glfwPollEvents();
        updateTimer = 0;
      }
    }

    glfwTerminate();
    return 0;
}





static void error (int code, const char* msg)
{
  printf("GLFW error %d: %s\n", code, msg);
  glfwTerminate();
  exit(0);
}

static GLFWwindow* WindowSetup(int width, int height) {
  glfwSetErrorCallback(error);
  if (glfwInit() != GLFW_TRUE) {
    std::cerr << "Could not initialize GLFW" << std::endl;
    return 0;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

  // DIMENSOES
  GLFWwindow* win = glfwCreateWindow(width, height, "Window title", nullptr, nullptr);
  if (!win) {
      std::cerr << "Could not create GLFW window" << std::endl;
      return 0;
    }
  glfwMakeContextCurrent(win);

  if (!gladLoadGL(glfwGetProcAddress)) {
    printf("Failed to initialize GLAD OpenGL context\n");
    exit(1);
  }
  return win;
}