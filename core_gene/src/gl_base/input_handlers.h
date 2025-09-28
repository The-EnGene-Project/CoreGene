#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H
#pragma once

#include "gl_includes.h"
#include <iostream>

bool modoWireframe = false;

static void keyboard(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    else if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {}// scene::graph()->clearGraph();
    // Verifica se a tecla pressionada é a 'T'
    else if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        // Inverte o estado atual
        modoWireframe = !modoWireframe;

        if (modoWireframe)
        {
            // Ativa o modo wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else
        {
            // Ativa o modo de preenchimento (fill)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
}

static void cursorpos(GLFWwindow * win, double xpos, double ypos)
{
 // convert screen pos (upside down) to framebuffer pos (e.g., retina displays)
 int wn_w, wn_h, fb_w, fb_h;
 glfwGetWindowSize(win, &wn_w, &wn_h);
 glfwGetFramebufferSize(win, &fb_w, &fb_h);
 double x = xpos * fb_w / wn_w;
 double y = (wn_h - ypos) * fb_h / wn_h;
 std::cout << "(x,y): " << x << ", " << y << std::endl;
}

static void mousebutton(GLFWwindow * win, int button, int action, int mods)
{
    if (action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(win, &xpos, &ypos);

        int fb_w, fb_h;
        glfwGetFramebufferSize(win, &fb_w, &fb_h);
        
        // Converte coordenadas de tela (pixels) para Coordenadas de Dispositivo Normalizadas (NDC) [-1, 1]
        float x_ndc = ((float)xpos / (float)fb_w) * 2.0f - 1.0f;
        float y_ndc = (1.0f - ((float)ypos / (float)fb_h)) * 2.0f - 1.0f;

        // scene::handleMouseClick(x_ndc, y_ndc, button);
    }
}

static void resize(GLFWwindow * win, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void setInputCallbacks(GLFWwindow * win) {
    glfwSetFramebufferSizeCallback(win, resize);
    glfwSetKeyCallback(win, keyboard);
    glfwSetMouseButtonCallback(win, mousebutton);
}

#endif