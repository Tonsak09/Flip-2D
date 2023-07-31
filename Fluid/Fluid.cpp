#include "Fluid.h"
#include <iostream>
Fluid::Fluid(float _gravity, glm::vec3 startVel, float _cellSize, int _sideLength, int particleCount, float particleSize)
	:gravity(_gravity), cellSize(_cellSize), sideLength(_sideLength)
{
	// Set up vectors 
	particles = std::vector<Particle>(particleCount);
	particleEntity = std::vector<Entity>();
	positions = std::vector<glm::vec3>(particleCount);

	cells = std::vector<Cell>();

	velField = std::vector<std::vector<Cell>>();

	if(cellSize < 5)
		cellSize = 5;


	// Setup all particles & entities 
	for (unsigned int i = 0; i < particleCount; i++)
	{
		particles[i] = Particle(i, &positions[i], glm::vec3(0), particleSize / 2.0f);
		particleEntity.push_back(Entity(positions[i], particleSize));

		particles[i].SetVel(startVel);
	}

	// These represent the velocity field that surrond the 
	// current cell 
	float* xQLess = nullptr;
	float* xQMore = nullptr;
	float* yQLess = nullptr;
	float* yQMore = nullptr;

	float* xRLess = nullptr;
	float* xRMore = nullptr;
	float* yRLess = nullptr;
	float* yRMore = nullptr;

	float* xPLess = nullptr;
	float* xPMore = nullptr;
	float* yPLess = nullptr;
	float* yPMore = nullptr;

	// Setup cells
	for (unsigned int x = 0; x < sideLength; x++)
	{
		bool isXEdge = (x == 0 || x == sideLength - 1);

		// Previous edge is set
		xQLess = xQMore;
		xRLess = xRMore;
		xPLess = xPMore;

		// Check if next is out of bounds or curent is edge 
		if (x + 1 >= sideLength - 1)
		{
			// next cell over is null 
			xQMore = nullptr;
			xRMore = nullptr;
			xPMore = nullptr;
		}
		else
		{
			xQMore = new float(0.0f);
			xRMore = new float(0.0f);
			xPMore = new float(0.0f);
		}




		// Reset y 
		yQLess = nullptr;
		yQMore = nullptr;

		yRLess = nullptr;
		yRMore = nullptr;

		yPLess = nullptr;
		yPMore = nullptr;

		for (unsigned int y = 0; y < sideLength; y++)
		{
			bool isYEdge = (y == 0 || y == sideLength - 1);
			//std::cout << isXEdge << std::endl;

			// Previous edge is set
			yQLess = yQMore;
			yRLess = yRMore;
			yPLess = yPMore;


			// Check if next is out of bounds or curent is edge 
			if (y + 1 >= sideLength - 1)
			{
				// next cell over is null 
				yQMore = nullptr;
				yRMore = nullptr;
				yPMore = nullptr;
			}
			else
			{
				yQMore = new float(0.0f);
				yRMore = new float(0.0f);
				yPMore = new float(0.0f);
			}
			

			// Check if on an edge 
			bool isSolid = isXEdge || isYEdge;

			Cell::PushDirections pushDir = Cell::None;
			if (isSolid)
			{
				// Whether to set to push horizontally
				// or vertically 
				pushDir = isXEdge ? Cell::XAxis : Cell::YAxis;
			}

			cells.push_back(Cell(x, y, _cellSize, isSolid, 
				xQLess, xQMore, yQLess, yQMore, 
				xRLess, xRMore, yRLess, yRMore, 
				xPLess, xPMore, yPLess, yPMore, 
				pushDir));

		}
	}
}

Fluid::~Fluid()
{
	//delete particles;
	//delete particleEntity;
	//delete positions;

	//delete velField;
}

void Fluid::SetParticlePosition(unsigned int index, glm::vec3 pos)
{
	positions[index] = pos;
}

/// <summary>
/// Get the model matrix that represents the particle of the given index 
/// </summary>
/// <returns></returns>
glm::mat4 Fluid::GetModel(int index)
{
	// Create a model matrix 
	return glm::translate(glm::mat4(1.0f), *particles[index].pos);
}

/// <summary>
/// Get a particle based on the given index 
/// </summary>
/// <returns></returns>
Particle* Fluid::GetParticle(int index)
{
	return &particles[index];
}

/// <summary>
/// Get a entity based on the given index 
/// </summary>
/// <returns></returns>
Entity Fluid::GetEntity(int index)
{
	return particleEntity[index];
}

/// <summary>
/// Get what cell this world position is in if possible
/// </summary>
/// <returns>Address to cell or nullptr</returns>
Cell* Fluid::PosToCell(glm::vec2 pos, float trueCellSize)
{
	// Get the index of cell this particle is in 
	int xCell = pos.y / trueCellSize;
	int yCell = pos.x / trueCellSize;


	int cellIndex = xCell + sideLength * yCell;

	if (cellIndex >= cells.size())
		return nullptr;

	return &cells[cellIndex];
}

/// <summary>
/// Get the cells position pixel coordinates based on the given indicies
/// </summary>
/// <returns></returns>
glm::vec3 Fluid::GetCellPos(int xIndex, int yIndex)
{
	float halfLength = cellSize / 2.0f;
	return glm::vec3(
		xIndex * cellSize + halfLength,
		yIndex * cellSize + halfLength,
		0
	);
}

glm::vec2 Fluid::GetCellVel(Cell cell)
{
	float x = 0.0f; 
	if (cell.q1 != nullptr)
	{
		x += *cell.q1;
	}
	if (cell.q2 != nullptr)
	{
		x += *cell.q2;
	}

	float y = 0.0f;
	if (cell.q3 != nullptr)
	{
		y += *cell.q3;
	}

	if (cell.q4 != nullptr)
	{
		y += *cell.q4;
	}


	return glm::vec2(x, y) / 2.0f;
}

/// <summary>
/// Get the cells that make up the grid 
/// </summary>
/// <returns></returns>
std::vector<Cell> Fluid::GetCells()
{
	return cells;
}

/// <summary>
/// Used to make sure that the particle is within the bounds 
/// of the grid and that there is no overlap for the particles 
/// </summary>
/// <param name="particle"></param>
/// <param name="trueCellSize"></param>
/// <param name="cellWallThickness"></param>
void Fluid::CorrectParticlePos(Particle* particle, float trueCellSize, int cellWallThickness)
{
	// Keep particle in bounds  

	// Get the size of the grid. Don't correct based on collision cells but just if it goes out of range
	// on each axis

	float axisLimt = (sideLength - cellWallThickness) * trueCellSize;
	float axisMin = cellWallThickness * trueCellSize;

	// X Check
	if (particle->pos->x <= axisMin)
	{
		*particle->pos = glm::vec3(axisMin + particle->GetHalfSize(), particle->pos->y, 0.0f);
		particle->SetVel(glm::vec3(-particle->vel.x / 2.0f, particle->vel.y, 0.0f));
	}
	else if (particle->pos->x >= axisLimt)
	{
		*particle->pos = glm::vec3(axisLimt - particle->GetHalfSize(), particle->pos->y, 0.0f);
		particle->SetVel(glm::vec3(-particle->vel.x / 2.0f, particle->vel.y, 0.0f));
	}

	// Y Check
	if (particle->pos->y <= axisMin)
	{
		*particle->pos = glm::vec3(particle->pos->x, axisMin + particle->GetHalfSize(), 0.0f);
		particle->SetVel(glm::vec3(particle->vel.x, -particle->vel.y / 2.0f, 0.0f));
	}
	else if (particle->pos->y >= axisLimt)
	{
		*particle->pos = glm::vec3(particle->pos->x, axisLimt - particle->GetHalfSize(), 0.0f);
		particle->SetVel(glm::vec3(particle->vel.x, -particle->vel.y / 2.0f, 0.0f));
	}




	// Particle update parent 

	Cell* cell = PosToCell(*particle->pos, trueCellSize);
	if (!cell->ContainsParticle(particle))
	{
		// FIND OLD CELL BASED ON X AND Y INDEX 
		Cell* oldCell = PosToCell((cellSize + 0.5f) * particle->parentIndex, cellSize);

		// ADD CURRENT PARTICLE TO NEW CELL PARENT 
		cell->AddParticle(particle);

		if (oldCell != nullptr)
		{
			// REMOVE CURRENT PARTICLE FROM THAT CELLS LIST 
			oldCell->RemoveParticle(particle);
		}
	}
}

/// <summary>
/// Move the particles based on their velocity
/// Also makes sure that particles stay within bounds 
/// </summary>
void Fluid::SimulateParticles(float timeStep, int maxParticleChecks, int cellWallThickness)
{
	for (unsigned int i = 0; i < particles.size(); i++)
	{
		Particle *current = &particles[i];
		
		(*current).vel += glm::vec3(0.0f, 1.0f, 0.0f) * gravity * timeStep;

		// Set up start and end of move 
		glm::vec2 start = glm::vec2(current->pos->x, current->pos->y);
		glm::vec2 next = *current->pos + current->vel * timeStep;

		// Change pos and make correction if necessary 
		*current->pos = glm::vec3(next, 0.0f);
		CorrectParticlePos(current, cellSize, cellWallThickness);
	}

	// I know the below code is horrifying to look at but having it split with
	// multiple cells helps optimize it like a quad tree 

	// GO THROUGH EACH CELL AND SEPERATE PARTICLES 
	for (unsigned c = 0; c < maxParticleChecks; c++)
	{
		for (unsigned int i = 0; i < cells.size(); i++)
		{
			Cell* cell = &cells[i];

			if (cell == nullptr)
			{
				std::cout << "Cell does not exist" << std::endl;
				continue;
			}

			int particleCount = cell->GetParticleCount();
			for (unsigned int a = 0; a < particleCount; a++)
			{
				Particle* childA = cell->GetParticle(a);

				// Safety 
				if (childA == nullptr)
					continue;

				for (unsigned int b = 0; b < particleCount; b++)
				{
					Particle* childB = cell->GetParticle(b);

					// Safety 
					if (childB == nullptr)
						continue;

					// Find center and split both particles
					// an equal distance away from it 

					glm::vec3 center = (*childA->pos + *childB->pos) / 2.0f;
					glm::vec3 dir = (*childA->pos - center);
					float length = glm::length(dir);
					
					if (length > 2.5f)
					{
						continue;
					}

					if (length == 0)
					{
						dir = glm::vec3(((double)rand() / (RAND_MAX)), ((double)rand() / (RAND_MAX)), 0.0f);
						length = glm::length(dir);
						//std::cout << "Rand direction" << std::endl;
					}
					dir /= length;
					//std::cout << (center - (dir * childA->GetHalfSize())).x << std::endl;

					/*if (childA->GetHalfSize() == 0.0f)
					{
						std::cout << "Pos" << std::endl;
					}*/

					// Apply change 
					if (childA->pos->y > childB->pos->y)
					{
						*childA->pos += glm::vec3(0, 2.5f, 0);
						*childB->pos -= glm::vec3(0, 2.5f, 0);
					}
					else
					{
						*childA->pos -= glm::vec3(0, 2.5f, 0);
						*childB->pos += glm::vec3(0, 2.5f, 0);
					}
					
					//*childB->pos = center - dir * (1.0f);
					break;
				}
			}
		}
	}
}

/// <summary>
/// Apply the particle velocities to the grid 
/// </summary>
void Fluid::TransferToVelField(std::vector<Cell> *nextValues)
{
	// Reset all cells 
	for (unsigned int i = 0; i < nextValues->size(); i++)
	{
		Cell cell = (*nextValues)[i];

		if (cell.q1 != nullptr)
		{
			*cell.q1 = 0;
		}
		if (cell.q2 != nullptr)
		{
			*cell.q2 = 0;
		}
		if (cell.q3 != nullptr)
		{
			*cell.q3 = 0;
		}
		if (cell.q4 != nullptr)
		{
			*cell.q4 = 0;
		}

		if (cell.r1 != nullptr)
		{
			*cell.r1 = 0;
		}
		if (cell.r2 != nullptr)
		{
			*cell.r2 = 0;
		}
		if (cell.r3 != nullptr)
		{
			*cell.r3 = 0;
		}
		if (cell.r4 != nullptr)
		{
			*cell.r4 = 0;
		}
	}

	// Calculate new q values of each cell
	for (unsigned int i = 0; i < particles.size(); i++)
	{
		Particle* current = &particles[i];
		glm::vec2 particlePos = glm::vec2((*(current->pos)).x, (*(current->pos)).y - (cellSize / 2.0f));

		// Get the index of cell this particle is in 
		int xCell = particlePos.x / cellSize;
		int yCell = particlePos.y / cellSize;


		int cellIndex = xCell + sideLength * yCell;
		if (cellIndex >= nextValues->size())
		{
			continue;
		}
		Cell cell = (*nextValues)[cellIndex];
		float deltaX = glm::abs(xCell * cellSize - particlePos.x);
		float deltaY = glm::abs(yCell * cellSize - particlePos.y);


		float w1 = (1.0f - deltaX / cellSize) * (1.0f - deltaY / cellSize);
		float w2 = deltaX / cellSize * (1 - deltaY / cellSize);
		float w3 = deltaX / cellSize * deltaY / cellSize;
		float w4 = (1 - deltaX / cellSize) * deltaY / cellSize;

		float pSum = 0.0f;
		int pCount = 0;
		// Apply velocities to the next grid 
		if (cell.q1 != nullptr)
		{
			*cell.q1 += w1 * current->vel.x;
			*cell.r1 += w1;
			*cell.p1 += w1;

			pSum += w1;
			pCount++;
		}
		if (cell.q2 != nullptr)
		{
			*cell.q2 += w2 * current->vel.x;
			*cell.r2 += w2;
			*cell.p2 += w2;

			pSum += w2;
			pCount++;
		}
		if (cell.q3 != nullptr)
		{
			*cell.q3 += w3 * current->vel.y;
			*cell.r3 += w3;
			*cell.p3 += w3;

			pSum += w3;
			pCount++;
		}
		if (cell.q4 != nullptr)
		{
			*cell.q4 += w4 * current->vel.y;
			*cell.r4 += w4;
			*cell.p4 += w4;

			pSum += w4;
			pCount++;
		}

		cell.averageP = pSum / pCount;
	}

	for (unsigned int i = 0; i < nextValues->size(); i++)
	{
		Cell cell = (*nextValues)[i];
		
		if (cell.q1 != nullptr)
		{
			if (glm::abs(*cell.r1) > 0.0f)
				*cell.q1 /= *cell.r1;
		}
		if (cell.q3 != nullptr)
		{
			if(glm::abs(*cell.r3) > 0.0f)
				*cell.q3 /= *cell.r3;
		}
	}
}

/// <summary>
/// Make the grid have an equal amout of fluid inflow and outflow 
/// </summary>
void Fluid::MakeIncompressible(std::vector<Cell>* nextValues, int iterations, float overrelaxation)
{
	overrelaxation = glm::clamp(overrelaxation, 1.0f, 2.0f);

	for (unsigned int i = 0; i < iterations; i++)
	{
		for (unsigned int c = 0; c < nextValues->size(); c++)
		{
			Cell cell = (*nextValues)[c];
			float divergence = 0.0f; // Used to make incompressible by "spreading" out values 
			float s = 0.0f; // Accomodate for solid cells 

			float p = 0.0f; // Demsityl 

			// Divergence and s value calculations 
			if (cell.q1 != nullptr)
			{
				divergence += *cell.q1;
				s += 1;

				p += *cell.p1;
			}

			if (cell.q2 != nullptr)
			{
				divergence -= *cell.q2;
				s += 1;

				p -= *cell.p2;
			}

			if (cell.q3 != nullptr)
			{
				divergence += *cell.q3;
				s += 1;

				p += *cell.p3;
			}

			if (cell.q4 != nullptr)
			{
				divergence -= *cell.q4;
				s += 1;

				p -= *cell.p4;
			}

			divergence *= overrelaxation;
			divergence -= 1.0f * (p - cell.averageP);

			// Apply incompressibility 
			if (cell.q1 != nullptr)
			{
				*cell.q1 += (divergence * !cell.isSolid) / s;
			}

			if (cell.q2 != nullptr)
			{
				*cell.q2 += (divergence * !cell.isSolid) / s;
			}

			if (cell.q3 != nullptr)
			{
				*cell.q3 += (divergence * !cell.isSolid) / s;
			}

			if (cell.q4 != nullptr)
			{
				*cell.q4 += (divergence * !cell.isSolid) / s;
			}
		}
	}
}

void Fluid::AddChangeToParticles(std::vector<Cell>* nextValues, float timeStep)
{
	for (unsigned int i = 0; i < particles.size(); i++)
	{
		Particle* current = &particles[i];
		glm::vec2 particlePos = glm::vec2((*(current->pos)).x, (*(current->pos)).y - (cellSize / 2.0f));

		int xCell = particlePos.x / cellSize;
		int yCell = particlePos.y / cellSize;

		int cellIndex = xCell + sideLength * yCell;
		if (cellIndex >= nextValues->size())
			continue;
		Cell cellNext = (*nextValues)[cellIndex];
		Cell cellOld = cells[cellIndex];

		float deltaX = glm::abs(xCell * cellSize - particlePos.x);
		float deltaY = glm::abs(yCell * cellSize - particlePos.y);

		// Weights for how much each corner is affected by particle
		float w1 = (1.0f - deltaX / cellSize) * (1.0f - deltaY / cellSize);
		float w2 = deltaX / cellSize * (1 - deltaY / cellSize);
		float w3 = deltaX / cellSize * (deltaY / cellSize);
		float w4 = (1 - deltaX / cellSize) * (deltaY / cellSize);

		float denomenator = 0.0f;

		float xComp = 0.0f;
		float yComp = 0.0f;

		// Add corner points 
		if (cellOld.q1 != nullptr)
		{
			xComp += w1 * (*cellNext.q1 - *cellOld.q1);
			denomenator += w1;
		}

		if (cellOld.q2 != nullptr)
		{
			xComp += w2 * (*cellNext.q2 - *cellOld.q2);
			denomenator += w2;
		}

		if (cellOld.q3 != nullptr)
		{
			yComp += w3 * (*cellNext.q3 - *cellOld.q3);
			denomenator += w3;
		}

		if (cellOld.q4 != nullptr)
		{
			yComp += w4 * (*cellNext.q4 - *cellOld.q4);
			denomenator += w4;
		}

		current->vel += (glm::vec3(
			isnan(xComp) ? 0.0f : xComp, 
			isnan(yComp) ? 0.0f : yComp, 
			0.0f) / denomenator) * timeStep;
	}

	cells = *nextValues;
}

void Fluid::SimulateFlip(float timeStep)
{
	// Simulate the made FLIP calculations 
	std::vector<Cell> nextValues(cells);


	for (unsigned int i = 0; i < nextValues.size(); i++)
	{
		Cell* cell = &(nextValues[i]);
		Cell oldCell = (cells[i]);

		float* _q1 = cell->q1;
		if (_q1 != nullptr)
		{
			cell->q1 = new float(*oldCell.q1);
			cell->r1 = new float(*oldCell.r1);
		}

		float* _q2 = cell->q2;
		if (_q2 != nullptr)
		{
			cell->q2 = new float(*oldCell.q2);
			cell->r2 = new float(*oldCell.r2);
		}

		float* _q3 = cell->q3;
		if (_q3 != nullptr)
		{
			cell->q3 = new float(*oldCell.q3);
			cell->r3 = new float(*oldCell.r3);
		}

		float* _q4 = cell->q4;
		if (_q4 != nullptr)
		{
			cell->q4 = new float(*oldCell.q4);
			cell->r4 = new float(*oldCell.r4);
		}
	}


	TransferToVelField(&nextValues);
	MakeIncompressible(&nextValues, 7, 1.0f);
	AddChangeToParticles(&nextValues, timeStep);
}
