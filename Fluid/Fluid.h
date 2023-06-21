#pragma once
#include "glm/glm.hpp"
#include <vector>

struct Particle
{
	glm::vec2 pos;
	glm::vec2 vel;
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
	std::vector<std::vector<Cell>> VelField;
	
	float gravity;
	float cellSize;
	int sideLength;

private:
	void SimulateParticles();
	void TransferToVelField();
	void MakeIncompressible();
	void AddChangeToParticles();
};