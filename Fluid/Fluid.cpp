#include "Fluid.h"

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

	// Setup cells
	for (unsigned int x = 0; x < sideLength; x++)
	{
		bool isXEdge = (x <= 0 || x >= sideLength - 1);


		// Previous edge is set
		xQLess = xQMore;
		xRLess = xRMore;

		// Check if next is out of bounds or curent is edge 
		if (x + 1 >= sideLength - 1)
		{
			// next cell over is null 
			xQMore = nullptr;
			xRMore = nullptr;
		}
		else
		{
			xQMore = new float(0.0f);
			xRMore = new float(0.0f);
		}

		if (xQMore != nullptr)
		{
			qValues.push_back(xQMore);
			rValues.push_back(xRMore);
		}


		// Reset y 
		yQLess = nullptr;
		yQMore = nullptr;

		yRLess = nullptr;
		yRMore = nullptr;

		for (unsigned int y = 0; y < sideLength; y++)
		{
			bool isYEdge = (y == 0 || y == sideLength - 1);

			// Previous edge is set
			yQLess = yQMore;
			yRLess = yRMore;


			// Check if next is out of bounds or curent is edge 
			if (y + 1 >= sideLength - 1)
			{
				// next cell over is null 
				yQMore = nullptr;
				yRMore = nullptr;
			}
			else
			{
				yQMore = new float(0.0f);
				yRMore = new float(0.0f);
			}
			
			if (yQMore != nullptr)
			{
				qValues.push_back(yQMore);
				rValues.push_back(yRMore);
			}

			// Check if on an edge 
			bool isSolid = isXEdge || isYEdge;

			Cell::PushDirections pushDir = Cell::None;
			if (isSolid)
			{
				// Whether to set to push horizontally
				// or vertically 
				pushDir = (x == 0 || x == sideLength - 1) ? Cell::XAxis : Cell::YAxis;
			}

			cells.push_back(Cell(x, y, _cellSize, isSolid, 
				xQLess, xQMore, yQLess, yQMore, 
				xRLess, xRMore, yRLess, yRMore, 
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


	return glm::vec2(x, y);
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
/// Move the particles based on their velocity 
/// </summary>
void Fluid::SimulateParticles(float timeStep)
{
	for (unsigned int i = 0; i < particles.size(); i++)
	{
		Particle *current = &particles[i];
		
		(*current).vel += glm::vec3(0.0f, 1.0f, 0.0f) * gravity * timeStep;
		(*current).MoveByVel(timeStep);
	}
}


/// <summary>
/// Apply the particle velocities to the grid 
/// </summary>
void Fluid::TransferToVelField(std::vector<Cell> *nextValues)
{
	// Calculate qp for each particle using previous corner values 
	for (unsigned int i = 0; i < particles.size(); i++)
	{
		Particle* current = &particles[i];

		// Grid offset 
		glm::vec2 particlePos = glm::vec2((*(current->pos)).x, (*(current->pos)).y - (cellSize / 2.0f));


		// Get the index of cell this particle is in 
		int xCell = particlePos.x / cellSize;
		int yCell = particlePos.y / cellSize;

		int cellIndex = xCell + sideLength * yCell;

		if (cellIndex >= nextValues->size())
			continue;

		Cell cell = (*nextValues)[cellIndex];

		// Distance from edges 
		float deltaX = particlePos.x - cellSize;
		float deltaY = particlePos.y - cellSize;

		// Weights for how much each corner is affected by particle
		float w1 = (1.0f - xCell) * (1.0f - yCell);
		float w2 = xCell * (1 - yCell);
		float w3 = xCell * yCell;
		float w4 = (1 - xCell) * yCell;

		float numerator = 0.0f;
		float denomenator = 0.0f;

		// Add corner points 
		if (cell.q1 != nullptr)
		{
			numerator += w1 * *cell.q1;
			denomenator += w1;
		}

		if (cell.q2 != nullptr)
		{
			numerator += w2 * *cell.q2;
			denomenator += w2;
		}

		if (cell.q3 != nullptr)
		{
			numerator += w3 * *cell.q3;
			denomenator += w3;
		}

		if (cell.q4 != nullptr)
		{
			numerator += w4 * *cell.q4;
			denomenator += w4;
		}
		current->qp = numerator / denomenator;
	}

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

		float w1 = (1.0f - xCell) * (1.0f - yCell);
		float w2 = xCell * (1 - yCell);
		float w3 = xCell * yCell;
		float w4 = (1 - xCell) * yCell;


		// Apply velocities to the next grid 
		if (cell.q1 != nullptr)
		{
			*cell.q1 += w1 * current->qp;
			*cell.r1 += w1;
		}
		if (cell.q2 != nullptr)
		{
			*cell.q2 += w2 * current->qp;
			*cell.r2 += w2;
		}
		if (cell.q3 != nullptr)
		{
			*cell.q3 += w3 * current->qp;
			*cell.r3 += w3;
		}
		if (cell.q4 != nullptr)
		{
			*cell.q4 += w4 * current->qp;
			*cell.r4 += w4;
		}
	}


	// Apply for final change of weights 
	/*for (unsigned int i = 0; i < qValues.size(); i++)
	{
		*qValues[i] = *qValues[i] / *rValues[i];
	}*/

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

			// Divergence and s value calculations 
			if (cell.q1 != nullptr)
			{
				divergence += *cell.q1;
				s += 1;
			}

			if (cell.q2 != nullptr)
			{
				divergence -= *cell.q2;
				s += 1;
			}

			if (cell.q3 != nullptr)
			{
				divergence += *cell.q3;
				s += 1;
			}

			if (cell.q4 != nullptr)
			{
				divergence -= *cell.q4;
				s += 1;
			}

			divergence *= overrelaxation;

			// Apply incompressibility 
			if (cell.q1 != nullptr)
			{
				*cell.q1 += divergence / s;
			}

			if (cell.q2 != nullptr)
			{
				*cell.q2 += divergence / s;
			}

			if (cell.q3 != nullptr)
			{
				*cell.q3 += divergence / s;
			}

			if (cell.q4 != nullptr)
			{
				*cell.q4 += divergence / s;
			}
		}
	}
}

void Fluid::AddChangeToParticles(std::vector<Cell>* nextValues)
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

		glm::vec2 next = GetCellVel(cellNext);
		glm::vec2 old = GetCellVel(cellOld);
		glm::vec2 changeInGridVel = next - old;

		current->vel += glm::vec3(changeInGridVel, 0.0f);
		cells = *nextValues;
	}
}

void Fluid::SimulateFlip()
{
	// Simulate the made FLIP calculations 
	std::vector<Cell> nextValues(cells);


	for (unsigned int i = 0; i < nextValues.size(); i++)
	{
		Cell* cell = &(nextValues[i]);

		float* _q1 = cell->q1;
		if (_q1 != nullptr)
		{
			cell->q1 = new float(1.0f);
			cell->r1 = new float(0.0f);
		}

		float* _q2 = cell->q1;
		if (_q1 != nullptr)
		{
			cell->q2 = new float(-1.0f);
			cell->r2 = new float(0.0f);
		}

		float* _q3 = cell->q1;
		if (_q1 != nullptr)
		{
			cell->q3 = new float(1.0f);
			cell->r3 = new float(0.0f);
		}

		float* _q4 = cell->q1;
		if (_q1 != nullptr)
		{
			cell->q4 = new float(-1.0f);
			cell->r4 = new float(0.0f);
		}
	}


	TransferToVelField(&nextValues);
	MakeIncompressible(&nextValues, 7, 1.0f);
	AddChangeToParticles(&nextValues);
}

// AT SOME POINT OLD'S CELL VALUES ALL BECOME NULL