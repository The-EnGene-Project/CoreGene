#ifndef ERROR_H
#define ERROR_H
#pragma once

#include <string>
#include <iostream>
#include <cstdlib>
#include <iomanip> // Required for std::hex
#include <sstream>

#include <stacktrace>

#include "gl_includes.h"

class Error {
public:
    /**
     * @brief Enables the modern OpenGL debug message callback.
     * This is the recommended way to handle errors.
     * Call this ONCE after your OpenGL context is created and made current.
     */
    static void EnableDebugCallback() {
        GLint flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        
        // Check if the debug context flag is set
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            // Makes the callback synchronous (call from the same thread)
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
            glDebugMessageCallback(OpenGLDebugCallback, nullptr);
            
            // You can optionally control which messages you want to see
            // glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            
            std::cout << "OpenGL Debug Callback enabled." << std::endl;
        } else {
            std::cerr << "Warning: Could not enable OpenGL Debug Callback. "
                      << "Did you request a debug context?" << std::endl;
        }
    }

    /**
     * @brief Legacy error check using glGetError().
     * Still useful for forcing a check and crash at a specific point.
     * Use the GL_CHECK(msg) macro instead of calling this directly.
     */
    static void CheckInternal(const std::string& msg, const char* file, int line) {
        GLenum err;
        bool hasError = false;
        
        // Loop until all errors are cleared
        while ((err = glGetError()) != GL_NO_ERROR) {
            hasError = true;
            std::string errorString;
            switch (err) {
                case GL_INVALID_ENUM:    errorString = "GL_INVALID_ENUM";    break;
                case GL_INVALID_VALUE:   errorString = "GL_INVALID_VALUE";   break;
                case GL_INVALID_OPERATION: errorString = "GL_INVALID_OPERATION"; break;
                case GL_OUT_OF_MEMORY:   errorString = "GL_OUT_OF_MEMORY";   break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: 
                                         errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; 
                                         break;
                default:                 errorString = "UNKNOWN_ERROR";      break;
            }
            std::cerr << "--- OpenGL Error (glGetError) ---" << "\n"
                      << "Message:  " << msg << "\n"
                      << "Error:    " << errorString << " (0x" << std::hex << err << std::dec << ")" << "\n"
                      << "Location: " << file << ":" << line << "\n"
                      << "---------------------------------" << std::endl;
        }
        
        if (hasError) {
            std::cerr << "Stack Trace:\n" << stacktrace::to_string(stacktrace::stacktrace()) << std::endl;
            exit(1); // Abort
        }
    }

    /**
     * @brief Inserts a custom marker into the OpenGL debug message stream.
     * This is called by the GL_CHECK() macro.
     */
    static void InsertMarker(const std::string& msg, const char* file, int line) {
        // Format a detailed message string
        std::string full_message = "CHECKPOINT: \"" + msg + "\" at " + 
                                   std::string(file) + ":" + std::to_string(line);

        // Push the message into the debug stream
        // It will be picked up by our OpenGLDebugCallback
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION,
                             GL_DEBUG_TYPE_MARKER,
                             0, // You can assign custom IDs if you want
                             GL_DEBUG_SEVERITY_NOTIFICATION,
                             full_message.length(),
                             full_message.c_str());
    }

private:
    /**
     * @brief The actual callback function that OpenGL will call with detailed messages.
     * Note: The APIENTRY macro ensures the correct calling convention.
     */
    static void APIENTRY OpenGLDebugCallback(GLenum source, GLenum type, GLuint id,
                                             GLenum severity, GLsizei length,
                                             const GLchar* message, const void* userParam) {
        
        // Ignore some non-significant error codes
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

        std::stringstream ss;

        ss << "--- OpenGL Debug Message ---" << "\n";
        
        ss << "Severity: ";
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:   ss << "HIGH";   break;
            case GL_DEBUG_SEVERITY_MEDIUM: ss << "MEDIUM"; break;
            case GL_DEBUG_SEVERITY_LOW:    ss << "LOW";    break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: ss << "NOTIFICATION"; return; // Often too noisy
            default: ss << "UNKNOWN"; break;
        }
        ss << "\n";

        ss << "Type: ";
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               ss << "Error";               break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ss << "Deprecated Behavior"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ss << "Undefined Behavior";  break;
            case GL_DEBUG_TYPE_PORTABILITY:         ss << "Portability";         break;
            case GL_DEBUG_TYPE_PERFORMANCE:         ss << "Performance";         break;
            case GL_DEBUG_TYPE_MARKER:              ss << "Marker";              break;
            case GL_DEBUG_TYPE_OTHER:               ss << "Other";               break;
            default: ss << "UNKNOWN"; break;
        }
        ss << "\n";
        
        ss << "Source: ";
        switch (source) {
            case GL_DEBUG_SOURCE_API:             ss << "API";             break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   ss << "Window System";   break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: ss << "Shader Compiler"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     ss << "Third Party";     break;
            case GL_DEBUG_SOURCE_APPLICATION:     ss << "Application";     break;
            case GL_DEBUG_SOURCE_OTHER:           ss << "Other";           break;
            default: ss << "UNKNOWN"; break;
        }
        ss << "\n";

        ss << "Message: " << message << "\n";
        ss << "----------------------------" << std::endl;

        std::cerr << ss.str() << std::endl;

        // If the severity is high, treat it as a fatal error
        if (severity == GL_DEBUG_SEVERITY_HIGH) {
            std::cerr << "ABORTING due to high-severity OpenGL error." << std::endl;
            // exit(1);
        }
    }
};

/**
 * @brief Public macro for legacy checking with glGetError().
 * This will automatically capture the file and line number.
 */
#define GL_CHECK(msg) Error::CheckInternal(msg, __FILE__, __LINE__)

#endif // ERROR_H