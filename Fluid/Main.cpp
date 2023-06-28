#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Renderer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

#include <vector>
#include "Entity.h"
#include "fastnoiselite/FastNoiseLite.h"

#include <Windows.h>
#include <string.h>

#include "Fluid.h"
#include "Main.h"

const float GetRand()
{
    return ((double)rand() / (RAND_MAX));
}

/// <summary>
/// Renderers each particle
/// </summary>
void RenderParticles(const int& PARTICLECOUNT, glm::mat4& proj, glm::mat4& view, Fluid& fluid, Shader& shader, Renderer& renderer, VertexArray& va, IndexBuffer& ib)
{
    for (unsigned int i = 0; i < PARTICLECOUNT; i++)
    {
        glm::mat4 model = fluid.GetModel(i);

        glm::mat4 mvp = proj * view * model;
        shader.Bind();

        shader.SetUniformMat4f("u_MVP", mvp);
        renderer.Draw(va, ib, shader);
    }
}

/// <summary>
/// Renders the grid the particles are organized on 
/// </summary>
void RenderGrid(const float& CELLSIZE, const float& CELLSPACINGSIZE, const int& GRIDSIZECOUNT, const float& CELLVISUALSCALAR, glm::mat4& proj, glm::mat4& view, Shader& shader, Renderer& renderer, VertexArray& va, IndexBuffer& ib)
{
    float trueCellSize = CELLSIZE + CELLSPACINGSIZE;
    for (unsigned int x = 0; x < GRIDSIZECOUNT; x++)
    {
        for (unsigned int y = 0; y < GRIDSIZECOUNT; y++)
        {
            // Get the center of a cell by taking into account
            // size and borders 
            glm::vec3 center = glm::vec3
            (
                x * trueCellSize + (trueCellSize / 2.0f),
                y * trueCellSize + (trueCellSize / 2.0f),
                0.0f
            );

            // Move 
            glm::mat4 model = glm::translate(glm::mat4(1.0f), center);
            // Scale 
            model = glm::scale(model, glm::vec3(CELLVISUALSCALAR, CELLVISUALSCALAR, CELLVISUALSCALAR));
            

            glm::mat4 mvp = proj * view * model;
            shader.Bind();

            shader.SetUniformMat4f("u_MVP", mvp);
            renderer.Draw(va, ib, shader);
        }
    }
}

void SetColor(Shader& shader, glm::vec4& color)
{
    shader.Bind();
    shader.SetUniform4f("u_Color", color.r, color.g, color.b, color.a);
    shader.Unbind();
}

int main(void)
{
    // Variables 
    const int WIDTH = 960;
    const int HEIGHT = 540;

    const int PARTICLECOUNT = 20;
    const float STANDARDSIZE = 10.0f;

    const int GRIDSIZECOUNT = 10;
    const float CELLSIZE = 100.0f;
    const float CELLSPACINGSIZE = -1.0f;
    const float CELLVISUALSCALAR = 10.0f;

    // Useful reference 
    const unsigned int INDICIES[6] =
    {
        0, 1, 2,
        2, 3, 0,
    };

    glm::vec4 CELLCOLOR = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    glm::vec4 PARTICLECOLOR = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

    // Counts so easier to call 
    int indiciesCount = 6 * PARTICLECOUNT;    // One Entity has 6 indicies 
    int positionCount = 16 * PARTICLECOUNT;   // One Entity has 16 positions (Includes corners and UV)

    // Set random seed
    srand(time(NULL));

    #pragma region glfwWindow
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    // Vertsion setup
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WIDTH, HEIGHT, "Flip", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(3);

    glewInit();
    #pragma endregion

    #pragma region Rendering&Entity
    
    Fluid fluid(9.81f, CELLSIZE, GRIDSIZECOUNT, PARTICLECOUNT, STANDARDSIZE);
    Entity entity(glm::vec3(0), 100.0f); // Used for indicy mat TEMP

    // Set starting positions 
    for (unsigned int i = 0; i < PARTICLECOUNT; i++)
    {
        glm::vec3 rand = glm::vec3(GetRand() * WIDTH, GetRand() * HEIGHT, 0);
        fluid.SetParticlePosition(i, rand);
    }


    // Vectors that contain the positions and indicies of all entities 
    std::vector<float> flattenedPositions = std::vector <float>();
    std::vector<unsigned int> indicies = std::vector <unsigned int>();
    

    // Go through the vector and combine all the positions and indicies 

    // Particle Positions 
    for (unsigned int i = 0; i < PARTICLECOUNT; i++)
    {
        // Apply individual entity positions to main vector 
        for (unsigned int p = 0; p < positionCount; p++)
        {
            flattenedPositions.push_back(fluid.GetParticle(i).positions[p]);
        }

        // Apply individual entity indicies to main vector 
        for (unsigned int j = 0; j < indiciesCount; j++)
        {
            indicies.push_back(INDICIES[j] + (i * 4));
        }
    }

    

    // Get the beginning of each main vector 
    float* posPointer = &flattenedPositions[0];
    unsigned int* indexPointer = &indicies[0];


    // Blending 
    GLCall(glEnable(GL_BLEND));
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    #pragma endregion

    {
        #pragma region Setup
        unsigned int vao;
        GLCall(glGenVertexArrays(1, &vao));
        GLCall(glBindVertexArray(vao));

        VertexArray va;
        VertexBuffer vb(posPointer, (positionCount) * sizeof(float));
        VertexBufferLayout layout;

        layout.Push<float>(2);
        layout.Push<float>(2);

        va.AddBuffer(vb, layout);

        IndexBuffer ib(indexPointer, 6);

        // Setup matricies 
        glm::mat4 proj = glm::ortho(0.0f, (float)WIDTH, 0.0f, (float)HEIGHT, -1.0f, 1.0f);
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));

        // Setup shader 
        Shader shader("res/shaders/Basic.shader");
        shader.Bind();

        // Bind the texture 
        Texture texture("res/textures/BevelSquare.png");
        texture.Bind();
        shader.SetUniform1i("u_Texture", 0);

        va.Unbind();
        vb.Unbind();
        ib.Unbind();
        shader.Unbind();

        Renderer renderer;

        // Imgui setup
        ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(window, true);
        ImGui::StyleColorsDark();

        // Used for manual control of each particle 
        std::vector<glm::vec3> translations = std::vector<glm::vec3>(PARTICLECOUNT);
        #pragma endregion

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            /* Render here */
            renderer.Clear();

            // Rendering the grid
            SetColor(shader, CELLCOLOR);
            RenderGrid(CELLSIZE, CELLSPACINGSIZE, GRIDSIZECOUNT, CELLVISUALSCALAR, proj, view, shader, renderer, va, ib);

            // Rendering the particle
            SetColor(shader, PARTICLECOLOR);
            RenderParticles(PARTICLECOUNT, proj, view, fluid, shader, renderer, va, ib);
            
            #pragma region GUI
            // gui
            glfwPollEvents();
            ImGui_ImplGlfwGL3_NewFrame();


            { // Actual gui values and interface 
                static float f = 0.0f;
                for (unsigned int i = 0; i < translations.size(); i++)
                {
                    // Apply a slider for each entity 
                    std::string title = "Translation ";
                    std::string index = std::to_string(i);
                    std::string combine = title + index;

                    ImGui::SliderFloat3(combine.c_str(), &translations[i].x, 0.0f, 960.0f);
                }

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            } 

            ImGui::Render();
            ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
            #pragma endregion

            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();

            // Update at constant (enough) time
            Sleep(0.1f);
        }
    }
    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}



