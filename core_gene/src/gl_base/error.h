#ifndef ERROR_H
#define ERROR_H
#pragma once

#include <string>
#include <iostream>
#include <cstdlib>
#include <iomanip> // Required for std::hex
#include <sstream>

#include "gl_includes.h"

#include <iostream>
#include <iomanip>
#include <cstdint> // For uint64_t
#include <backtrace.h> // The libbacktrace header

// Data to pass to our callback
struct BacktraceState {
    int error;
    int frame_num;
};

// Callback for backtrace_full
// This is called for every frame
static int backtrace_callback(void* data, uintptr_t pc,
                              const char* filename, int lineno, const char* function) {
    BacktraceState* state = (BacktraceState*)data;
    
    std::cerr << std::setw(2) << state->frame_num << ": ";

    if (function) {
        std::cerr << function;
    } else {
        std::cerr << "(No Symbol)";
    }

    std::cerr << " at 0x" << std::hex << (uint64_t)pc << std::dec;
    
    if (filename) {
        std::cerr << " (" << filename << ":" << lineno << ")";
    }
    
    std::cerr << "\n";
    
    state->frame_num++;
    return 0; // 0 = continue to next frame
}

// Callback for error handling
static void my_backtrace_error_callback(void* data, const char* msg, int errnum) {
    BacktraceState* state = (BacktraceState*)data;
    std::cerr << "libbacktrace error: " << msg << " (" << errnum << ")\n";
    state->error = 1;
}

static void print_stacktrace() {
    BacktraceState state = {0, 0};
    
    // 1. Initialize the backtrace state
    //    The first argument is your executable's path. 
    //    NULL works if the executable is in the current dir or PATH.
    //    For robustness, you might want to pass argv[0] here.
    struct backtrace_state* bt_state = backtrace_create_state(
        NULL, // Pass your executable's path for best results (e.g., argv[0])
        1,    // Threaded? (1 = yes)
        my_backtrace_error_callback,
        &state  // Data for error callback
    );
    
    if (!bt_state) {
        std::cerr << "Failed to create backtrace state.\n";
        return;
    }

    std::cerr << "\n--- Stack Trace (libbacktrace) ---\n";

    // 2. Do the full backtrace
    //    0 = skip 0 frames from the top (change to 1 to skip this function)
    backtrace_full(bt_state, 0, backtrace_callback, my_backtrace_error_callback, &state);

    std::cerr << "------------------------------------" << std::endl;
    
    // backtrace_create_state returns NULL on error, no need to free
}

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
            print_stacktrace();
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
            print_stacktrace();
            exit(1);
        }
    }
};

/**
 * @brief Public macro for legacy checking with glGetError().
 * This will automatically capture the file and line number.
 */
#define GL_CHECK(msg) Error::CheckInternal(msg, __FILE__, __LINE__)

#endif // ERROR_H