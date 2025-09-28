#ifndef ERROR_H
#define ERROR_H
#pragma once
#include <string>

#include <iostream>
#include <cstdlib>

#include "gl_includes.h"

class Error {
  public:
    static void Check (const std::string& msg) {
      GLenum err = glGetError();
      if (err == GL_NO_ERROR) 
        return;
      switch(err) {
        case GL_INVALID_ENUM: std::cerr << "GL error: GL_INVALID_ENUM (" << msg << ")\n"; break;
        case GL_INVALID_VALUE: std::cerr << "GL error: GL_INVALID_VALUE (" << msg << ")\n"; break;
        case GL_INVALID_OPERATION: std::cerr << "GL error: GL_INVALID_OPERATION (" << msg << ")\n"; break;
        case GL_OUT_OF_MEMORY: std::cerr << "GL error: GL_OUT_OF_MEMORY (" << msg << ")\n"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: std::cerr << "GL error: GL_INVALID_FRAMEBUFFER_OPERATION (" << msg << ")\n"; break;
        default: std::cerr << "GL error: unknown error (" << msg << ")\n"; break;
      }
      exit(1);
    }
};
#endif