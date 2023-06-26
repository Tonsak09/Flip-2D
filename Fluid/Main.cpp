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

const float GetRand()
{
    return ((double)rand() / (RAND_MAX));
}

int main(void)
{
    // Variables 
    const int WIDTH = 960;
    const int HEIGHT = 540;
    const int PARTICLECOUNT = 20;

    float noiseRadius = 100.0f;
    std::vector<glm::vec2> noiseCoords;
    //std::vector<glm::vec3> startPositions;

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
    
    Fluid fluid(9.81f, 10.0f, 10, PARTICLECOUNT, 10.0f);
    Entity entity(glm::vec3(0), 100.0f); // Used for indicy mat TEMP

    // Create entities 
    //std::vector<Entity> entities;
    for (unsigned int i = 0; i < PARTICLECOUNT; i++)
    {
        //Entity entity = Entity(glm::vec3(0), 100.0f);
        //entities.push_back(entity);

        //startPositions.push_back(glm::vec3(GetRand() * WIDTH, GetRand() * HEIGHT, 0));
        //std::cout << "POS (" << startPositions[i].x << ", " << startPositions[i].y << ")" << std::endl;

        fluid.positions[i] = glm::vec3(GetRand() * WIDTH, GetRand() * HEIGHT, 0);

        noiseCoords.push_back(glm::vec2(GetRand() * 200, GetRand() * 200));
    }

    // Counts so easier to call 
    int indiciesCount = 6 * PARTICLECOUNT;    // One Entity has 6 indicies 
    int positionCount = 16 * PARTICLECOUNT;   // One Entity has 16 positions (Includes corners and UV)

    // Vectors that contain the positions and indicies of all entities 
    std::vector<float> flattenedPositions = std::vector <float>();
    std::vector<unsigned int> indicies = std::vector <unsigned int>();
    

    // Go through the vector and combine all the positions
    // and indicies 
    for (unsigned int i = 0; i < PARTICLECOUNT; i++)
    {
        // Apply individual entity positions to main vector 
        for (unsigned int p = 0; p < positionCount; p++)
        {
            flattenedPositions.push_back(fluid.GetEntity(i).positions[p]);
        }

        // Apply individual entity indicies to main vector 
        for (unsigned int j = 0; j < indiciesCount; j++)
        {
            indicies.push_back(fluid.GetEntity(i).INDICIES[j] + (i * 4));
        }
    }

    // Get the beginning of each main vector 
    float* posPointer = &flattenedPositions[0];
    unsigned int* indexPointer = &indicies[0];

    //std::vector<float> noise = GenerateNoiseDate();

    // Create and configure FastNoise object
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // Gather noise data
    std::vector<float> noiseData(500 * 500);
    int index = 0;

    for (int y = 0; y < 500; y++)
    {
        for (int x = 0; x < 500; x++)
        {
            float currentNoise = noise.GetNoise((float)x, (float)y) * noiseRadius;
            noiseData[index++] = currentNoise;
        }
    }

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
        VertexBuffer vb(posPointer, positionCount * sizeof(float));
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
        Texture texture("res/textures/Eidos_Logo.jpeg");
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

        std::vector<glm::vec3> translations = std::vector<glm::vec3>(PARTICLECOUNT);
        #pragma endregion

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            /* Render here */
            renderer.Clear();

            #pragma region EntityLogic
            for (unsigned int i = 0; i < PARTICLECOUNT; i++)
            {
                //glm::mat4 model = glm::translate(glm::mat4(1.0f), translations[i]);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), *fluid.GetParticle(i).pos + glm::vec3(noiseData[(int)noiseCoords[i].x], noiseData[(int)noiseCoords[i].y], 0));
                glm::mat4 mvp = proj * view * model;
                shader.Bind();
                    
                shader.SetUniformMat4f("u_MVP", mvp);
                renderer.Draw(va, ib, shader);

                noiseCoords[i] += glm::vec2(1);

                if (noiseCoords[i].x >= noiseData.size())
                {
                    noiseCoords[i].x = 0;
                }

                if (noiseCoords[i].y >= noiseData.size())
                {
                    noiseCoords[i].y = 0;
                }
            }

            #pragma endregion
            
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

                    ImGui::SliderFloat3(combine.c_str(), & translations[i].x, 0.0f, 960.0f);
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

