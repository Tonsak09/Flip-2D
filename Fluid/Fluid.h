#pragma once
#include "glm/glm.hpp"
#include "Entity.h"
#include "Renderer.h"


struct Particle
{

private:
	float halfSize;

public:
	unsigned int index;
	glm::vec3* pos; // Address to position which is held in another vector 
	glm::vec3 vel;

	// The matrix represents the actual verticie positions 
	float positions[4 * 4];
	glm::mat4 modelMatrix;


	Particle()
		:index(-1), pos(NULL), vel(glm::vec3(0)), halfSize(-1.0f) 
	{
		// This is used for positions of corners and uv
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

	Particle(unsigned int _index, glm::vec3* _pos, glm::vec3 _vel, float _halfSize)
		:index(_index), pos(_pos), vel(_vel), halfSize(_halfSize)
	{
		// This is used for positions of corners and uv
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
};

struct Cell
{
	float* q1, q2, q3, q4;
	bool notSolid; 
};

class Fluid
{
private:
	std::vector<Particle> particles;
	std::vector<Entity> particleEntity;

	std::vector<std::vector<Cell>> velField;
	
	float gravity;
	float cellSize;
	int sideLength;


public:
	/// <summary>
	/// Set the variables used throughout the simulation 
	/// </summary>
	/// <param name="_gravity">How much are fluid particles accelerated downards</param>
	/// <param name="_cellSize">What is the size in pixels of each cell</param>
	/// <param name="_sideLength">How many cells make up one size of the square simulation area</param>
	Fluid(float _gravity, float _cellSize, int _sideLength, int particleCount, float particleSize);
	~Fluid();

	Particle GetParticle(int index);
	Entity GetEntity(int index);

	/// <summary>
	/// Used to store each particle's position 
	/// </summary>
	std::vector<glm::vec3> positions;

private:
	void SimulateParticles();
	void TransferToVelField();
	void MakeIncompressible();
	void AddChangeToParticles();

	glm::vec3 GetCellPos(int xIndex, int yIndex);

private:
	void RenderGrid(Renderer renderer);
	void RenderParticles(Renderer renderer);
};