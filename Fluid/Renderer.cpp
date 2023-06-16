#include "Renderer.h"
#include <iostream>

void GLClearError()
{
    // Errors are stored through flags that only get 
    // cleared one at a time which requries continous
    // calls 
    while (glGetError() != GL_NO_ERROR)
    {
        // If there is still an error to clear 

    }
}

bool GLLogCall(const char* function, const char* file, int line)
{
    // Prints out each error on a different line 
    while (GLenum error = glGetError())
    {
        std::cout << "[OpenGL Error] (" << error << ")" << function << " " << file << ":" << line << std::endl;
        return false;
    }
    return true;
}

void Renderer::Clear() const
{
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const
{
    shader.Bind();
    va.Bind();
    ib.Bind();
    GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
}

