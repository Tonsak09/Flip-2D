#pragma once
#include "glm/glm.hpp"
#include "Entity.h"
#include "Renderer.h"


struct Particle
{
	int index;
	glm::vec3 pos;
	glm::vec3 vel;

	Particle(int _index, glm::vec3 _pos, glm::vec3 _vel)
		:index(_index), pos(_pos), vel(_vel) {}
	
};

struct Cell
{
	float* q1, q2, q3, q4;
	bool notSolid; 
};

class Fluid
{
private:
	std::vector<Particle>* particles;
	std::vector<Entity>* particleEntity;

	std::vector<std::vector<Cell>>* velField;
	
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
	Fluid(float _gravity, float _cellSize, int _sideLength);

	~Fluid();

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