#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

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

		// Assign variables to 
		for (unsigned int i = 0; i < 4 * 4; i++)
		{
			positions[i] = posTemp[i];
		}
	}

	/// <summary>
	/// Updates the particles position based on its velocity 
	/// </summary>
	/// <param name="timeStep"></param>
	void MoveByVel(float timeStep)
	{
		*pos += vel * timeStep;
	}

	/// <summary>
	/// Set the velocity of the particle 
	/// </summary>
	void SetVel(glm::vec3 nextVel)
	{
		vel = nextVel;
	}

	float GetHalfSize()
	{
		return halfSize;
	}
};

struct Cell
{
	float* q1, q2, q3, q4; // Shared corners between cells 
	float halfSize;
	bool isSolid; 

	int xIndex;
	int yIndex;

	/// <summary>
	/// In what direction should particles be moved
	/// </summary>
	enum PushDirections
	{
		None  = 0,
		XAxis = 1,
		YAxis = 1
	};
	PushDirections pushDir;

	Cell(int _xIndex, int _yIndex, float _halfSize, bool _isSolid, 
		Cell::PushDirections _pushDirection)
		: xIndex(_xIndex), yIndex(_yIndex), halfSize(_halfSize), isSolid(_isSolid)
	{
		pushDir = _pushDirection;
	}
	
};

class Fluid
{
private:
	std::vector<Particle> particles;
	std::vector<Entity> particleEntity;
	/// <summary>
	/// Used to store each particle's position 
	/// </summary>
	std::vector<glm::vec3> positions;

	std::vector<std::vector<Cell>> velField;
	std::vector<Cell> cells;
	
	float gravity;
	float cellSize; // NOT HALFSIZE!!!
	int sideLength;


public:
	/// <summary>
	/// Set the variables used throughout the simulation 
	/// </summary>
	/// <param name="_gravity">How much are fluid particles accelerated downards</param>
	/// <param name="_cellSize">What is the size in pixels of each cell</param>
	/// <param name="_sideLength">How many cells make up one size of the square simulation area</param>
	Fluid(float _gravity, glm::vec3 startVel, float _cellSize, int _sideLength, int particleCount, float particleSize);
	~Fluid();

	void SetParticlePosition(unsigned int index, glm::vec3 pos);

	Particle* GetParticle(int index);
	Entity GetEntity(int index);
	glm::mat4 GetModel(int index);

	void SimulateParticles(float timeStep);
	void TransferToVelField();
	void MakeIncompressible();
	void AddChangeToParticles();

	glm::vec3 GetCellPos(int xIndex, int yIndex);
	std::vector<Cell> GetCells();
};