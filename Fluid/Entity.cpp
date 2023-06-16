#include "Entity.h"
#include "glm/gtc/matrix_transform.hpp"

Entity::Entity()
    :   Entity(glm::vec3(0), 50.0f)
{
    
}

Entity::Entity(glm::vec3 _position, float _size)
    :halfSize(_size / 2)
{

    // This is used for positions and uv
    float posTemp[] =
    {
            -halfSize, -halfSize, 0.0f, 0.0f,   // 0
             halfSize, -halfSize, 1.0f, 0.0f,   // 1
             halfSize,  halfSize, 1.0f, 1.0f,   // 2
            -halfSize,  halfSize, 0.0f, 1.0f    // 3
    };

    modelMatrix = glm::mat4(1.0f);

    // Assign variables to 
    for (unsigned int i = 0; i < 4 * 4; i++)
    {
        positions[i] = posTemp[i];
    }
}

Entity::~Entity()
{
    //delete[] positions;
}

/// <summary>
/// Move the entity from its current position
/// a specified amounts 
/// </summary>
/// <param name="translation">How much the entity should be moved and in what direction</param>
void Entity::Move(glm::vec3 translation)
{
    modelMatrix = glm::translate(modelMatrix, translation);
}

/// <summary>
/// Get the current matrix of this entity
/// </summary>
/// <returns></returns>
glm::mat4 Entity::GetModelMatrix()
{
    return modelMatrix;
}

