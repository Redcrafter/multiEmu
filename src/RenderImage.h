#pragma once
#include <GL/glew.h>
#include <cstdint>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "md5.h"

struct Color {
	uint8_t R, G, B;
};

class RenderImage {
private:
	GLuint textureID;
	int Width, Height;
	Color* imgData;

	bool changed = false;
public:
	glm::mat4 mat;
	
	RenderImage(int width, int height, float x, float y);
	~RenderImage();
	
	RenderImage(const RenderImage&) = delete;
	RenderImage& operator=(const RenderImage&) = delete;

	void Clear(Color col);
	void SetPixel(int x, int y, Color col);
	void Line(int x0, int y0, int x1, int y1, Color col);
	
	void BufferImage();

	md5 GetHashCode();

	int GetWidth() const { return Width; }
	glm::mat4 GetMatrix() const { return mat; }
};

