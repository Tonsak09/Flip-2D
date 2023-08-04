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
#include "Collision.h"
#include "Main.h" // Auto generated?? 

#include<ctime>

const float GetRand()
{
    return ((double)rand() / (RAND_MAX));
}

void SetColor(Shader& shader, glm::vec4& color)
{
    shader.Bind();
    shader.SetUniform4f("u_Color", color.r, color.g, color.b, color.a);
    shader.Unbind();
}

void PrintVec3(glm::vec3 vector)
{
    std::cout << vector.x << ", " << vector.y << ", " << vector.z << std::endl;
}

void PrintVec2(glm::vec2 vector)
{
    std::cout << vector.x << ", " << vector.y << std::endl;
}

void CursorPositionCallback(GLFWwindow* window, double xPos, double yPos)
{
    //PrintVec2(glm::vec2(xPos, yPos));
}
/// <summary>
/// Logic that applies to each particle 
/// </summary>
void ParticleLogic(const int& PARTICLECOUNT, glm::mat4& proj, glm::mat4& view, Fluid& fluid, Shader& shader, Renderer& renderer, VertexArray& va, IndexBuffer& ib, bool showParticles)
{
    for (unsigned int i = 0; i < PARTICLECOUNT; i++)
    {
        glm::mat4 model = fluid.GetModel(i);

        glm::mat4 mvp = proj * view * model;
        shader.Bind();

        shader.SetUniformMat4f("u_MVP", mvp);

        if(showParticles)
            renderer.Draw(va, ib, shader);
    }
}

/// <summary>
/// Logic that applies to each cell in the grid 
/// </summary>
void GridLogic(const float& CELLSIZE, const float& CELLSPACINGSIZE, const int& GRIDSIZECOUNT, const float& CELLVISUALSCALAR, const int& PARTICLECOUNT, const float& MOUSERADIUS, glm::vec4& commonCellColor, glm::vec4& SOLIDCELLCOLOR, glm::vec4& barrierColor, glm::mat4& proj, glm::mat4& view, Shader& shader, Renderer& renderer, VertexArray& va, IndexBuffer& ib, Fluid& fluid, glm::vec2 mousePos, bool showCellHasParticles, float cellWallThickness)
{
    std::vector<Cell> cells = fluid.GetCells();

    int cellCount = cells.size();
    float trueCellSize = CELLSIZE + CELLSPACINGSIZE;

    //Cell* mouseCell = fluid.PosToCell(mousePos, trueCellSize);
    //std::cout << mouseCell << std::endl;

    for (unsigned int i = 0; i < cellCount; i++)
    {
        Cell* current = &cells[i];
        int x = (*current).xIndex;
        int y = (*current).yIndex;

       

        if (showCellHasParticles)
        {
            if (current->GetParticleCount() > 0)
            {
                SetColor(shader, SOLIDCELLCOLOR);
            }
            else
            {

                if ((current->xIndex < cellWallThickness) || (current->xIndex + cellWallThickness >= GRIDSIZECOUNT) || 
                    (current->yIndex < cellWallThickness) || (current->yIndex + cellWallThickness >= GRIDSIZECOUNT))
                {
                    // Cell wall
                    SetColor(shader, barrierColor);
                }
                else
                {
                    SetColor(shader, commonCellColor);
                }
            }
        }
        else
        {

            if ((current->xIndex < cellWallThickness) || (current->xIndex + cellWallThickness >= GRIDSIZECOUNT) ||
                (current->yIndex < cellWallThickness) || (current->yIndex + cellWallThickness >= GRIDSIZECOUNT))
            {
                // Cell wall
                SetColor(shader, barrierColor);
            }
            else
            {
                SetColor(shader, commonCellColor);
            }
        }

        // Show mouse radius 
        if (glm::distance(fluid.GetCellPos(current->xIndex, current->yIndex), glm::vec3(mousePos, 0.0f)) <= MOUSERADIUS)
        {
            SetColor(shader, barrierColor);
        }
        

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

        // Render Grid 
        glm::mat4 mvp = proj * view * model;
        shader.Bind();

        shader.SetUniformMat4f("u_MVP", mvp);
        renderer.Draw(va, ib, shader);
    }
}

int main(void)
{
    #pragma region Intialize 

    // Start screen size  
    const int WIDTH = 1200;
    const int HEIGHT = 800;

    // Particle
    const int PARTICLECOUNT = 500;
    const float STANDARDSIZE = 18.0f; // Typically 10 

    // Cell Details 
    const int CELLWALLTHICKNESS = 2;
    const int GRIDSIZECOUNT = 30;
    const float CELLSIZE = 20.0f;
    const float CELLSPACINGSIZE = 0.0f;
    const float CELLVISUALSCALAR = 1.0f;

    // Physics
    const float TIMESTEP = 0.03f;
    const float STARTGRAVITY = -50.0f;
    const int MAXPARTICLECHECKS = 1;

    // Spawning 
    const glm::vec3 STARTOFFSET = glm::vec3(190.0f, 100.0f, 0.0f);
    const float STARTRADIUS = 200.0f;


    // Useful reference 
    const unsigned int INDICIES[6] =
    {
        0, 1, 2,
        2, 3, 0,
    };



    // Counts so easier to call 
    int indiciesCount = 6 * PARTICLECOUNT;    // One Entity has 6 indicies 
    int positionCount = 16 * PARTICLECOUNT;   // One Entity has 16 positions (Includes corners and UV)

    // Set random seed
    srand(time(NULL));

    #pragma endregion

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


    // Set up mouse callback 
    glfwSetCursorPosCallback(window, CursorPositionCallback);

    #pragma endregion

    #pragma region Rendering&Entity
    
    Fluid fluid(STARTGRAVITY, glm::vec3(0.0f, 20.0f, 0.0f), CELLSIZE, GRIDSIZECOUNT, PARTICLECOUNT, STANDARDSIZE);

    // Set starting positions 
    for (unsigned int i = 0; i < PARTICLECOUNT; i++)
    {
        glm::vec3 rand = glm::vec3(GetRand() * STARTRADIUS, GetRand() * STARTRADIUS, 0);
        fluid.SetParticlePosition(i, STARTOFFSET + rand);
    }

    
    // Vectors that contain the positions and indicies of all entities 
    std::vector<float> flattenedPositions = std::vector <float>();
    std::vector<unsigned int> indicies = std::vector <unsigned int>();
    

    // Go through the vector and combine all the positions and indicies 

    // Particle Positions 
    for (unsigned int i = 0; i < PARTICLECOUNT; i++)
    {
        Particle* current = fluid.GetParticle(i);
        // Apply individual entity positions to main vector 
        for (unsigned int p = 0; p < positionCount; p++)
        {
            flattenedPositions.push_back((*current).positions[p]);
        }

        // Apply individual entity indicies to main vector 
        for (unsigned int j = 0; j < indiciesCount / PARTICLECOUNT; j++)
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


    #pragma region Runtime Variables

    glm::vec2 mousePosHold;

     // Visual 
    bool showParticles = true;
    bool showCellHasParticles = true;
    

    glm::vec4 commonCellColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    glm::vec4 occupiedCellColor = glm::vec4(0.5f, 0.0f, 0.0f, 1.0f);
    glm::vec4 barrierColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    glm::vec4 particleColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

    float commonCellColorArr[4] = {0.2f, 0.2f, 0.2f, 1};
    float occupiedCellColorArr[4] = {0.5f, 0.0f, 0.0f, 1};
    float barrierColorArr[4] = {0, 0, 1, 1};
    float particleColorArr[4] = {1.0f, 0, 0, 1};


    // Interaction 
    float mouseRadius = 40.0f;
    float gravity = STARTGRAVITY;
    int cellWallThickness = CELLWALLTHICKNESS;

    bool isPaintbrush = false;


    // Physics 
    float overrelazation = 1.5f;
    float densityMultiplier = 1.0f;


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

        clock_t mainClock = clock();

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            clock_t time_req;
            time_req = clock();
            /* Render here */
            renderer.Clear();

            #pragma region Main Logic

            // Mouse Variables 
            double x;
            double y;
            int h;

            glfwGetCursorPos(window, &x, &y);
            glfwGetWindowSize(window, nullptr, &h);

            // Left clicking 
            int leftButtonState = glfwGetMouseButton(window, 0);
            if (leftButtonState == 1)
            {
               /* glm::vec2 nextMouse = glm::vec2(x, h - y);
                mousePosHold = nextMouse;*/
            }

            // Right clicking 
            int rightButtonState = glfwGetMouseButton(window, 1);
           /* if (rightButtonState == 1)
            {
                glm::vec2 nextMouse = glm::vec2(x, h - y);
                mousePosHold = nextMouse;
            }*/

            glm::vec2 nextMouse = glm::vec2(x, h - y);
            mousePosHold = nextMouse;

            // Priority of left button (draw) 
            int buttonState = (leftButtonState == 1) ?
                1 : // Will spawn Particles 
                (rightButtonState == 1) ? 
                2 : // Will destroy Particles 
                0;  // Will do Nothing 


            // Rendering the grid and its logic 
            GridLogic(CELLSIZE, CELLSPACINGSIZE, GRIDSIZECOUNT, CELLVISUALSCALAR, PARTICLECOUNT, mouseRadius, commonCellColor, occupiedCellColor, barrierColor, proj, view, shader, renderer, va, ib, fluid, mousePosHold, showCellHasParticles, cellWallThickness);

            // Rendering the particle
            SetColor(shader, particleColor);

            float trueCellSize = CELLSIZE + CELLSPACINGSIZE;
            ParticleLogic(PARTICLECOUNT, proj, view, fluid, shader, renderer, va, ib, showParticles);

            #pragma endregion

            #pragma region GUI
            // gui
            glfwPollEvents();
            ImGui_ImplGlfwGL3_NewFrame();

            { // Actual gui values and interface 

                ImGui::Text("What to Render");

                ImGui::Checkbox("Visualize Particles", &showParticles);
                ImGui::Checkbox("Visualize Cells with particles", &showCellHasParticles);


                ImGui::Text("Color");

                ImGui::ColorEdit4("Mouse Color", barrierColorArr);
                barrierColor = glm::vec4(barrierColorArr[0], barrierColorArr[1], barrierColorArr[2], barrierColorArr[3]);

                ImGui::ColorEdit4("Cell Default Color", commonCellColorArr);
                commonCellColor = glm::vec4(commonCellColorArr[0], commonCellColorArr[1], commonCellColorArr[2], commonCellColorArr[3]);

                ImGui::ColorEdit4("Cell Occupied Color", occupiedCellColorArr);
                occupiedCellColor = glm::vec4(occupiedCellColorArr[0], occupiedCellColorArr[1], occupiedCellColorArr[2], occupiedCellColorArr[3]);

                ImGui::ColorEdit4("Particle Color", particleColorArr);
                particleColor = glm::vec4(particleColorArr[0], particleColorArr[1], particleColorArr[2], particleColorArr[3]);

                ImGui::Text("Interaction");
                ImGui::SliderFloat("Mouse Radius", &mouseRadius, 1.0f, 90.0f);
                ImGui::SliderInt("Cell Wall Thickness", &cellWallThickness, 1.0f, 10.0f);
                //ImGui::Checkbox("Is Paintbrush", &isPaintbrush);

                ImGui::Text("Physics");
                //ImGui::SliderFloat("Overrelaxation", &overrelazation, 1.0f, 2.0f);
                //ImGui::SliderFloat("Density Multipliers", &densityMultiplier, 1.0f, 2.0f);
                ImGui::SliderFloat("Gravity", &gravity, -200.0f, 200.0f);
                fluid.SetGravity(gravity);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            } 

            ImGui::Render();
            ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
            #pragma endregion

            #pragma region FLIP Sim
            fluid.SimulateFlip(TIMESTEP, 7, overrelazation, densityMultiplier);


            fluid.SimulateParticles(TIMESTEP, MAXPARTICLECHECKS, cellWallThickness, glm::vec3(mousePosHold, 0.0f), mouseRadius,
                isPaintbrush ? buttonState : -1,
                STANDARDSIZE);

            #pragma endregion

            #pragma region Final GLFW
            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();
            #pragma endregion

            // Update at constant (enough) time
            time_req = clock() - time_req;
            Sleep(TIMESTEP - (float)time_req / CLOCKS_PER_SEC);
        }
    }
    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}



