#pragma once

#include "glm/glm.hpp"

class Entity
{
private:
	glm::mat4 modelMatrix;
	float halfSize;

public:
	Entity();
	Entity(glm::vec3 _position, float _size);
	~Entity();

	// Positional functions
	void Move(glm::vec3 translation);
	void SetPos(glm::vec3 pos);
	glm::mat4 GetModelMatrix();

	float positions[4 * 4];
	// How to make the static version work???
	const unsigned int INDICIES[6] =
	{
		0, 1, 2,
		2, 3, 0,
	};
};