#pragma once
#include <cstdint>
#include <GL/glew.h>

struct Color {
	uint8_t R, G, B;
};

class RenderImage {
private:
	GLuint textureID;
	int Width, Height;
	Color* imgData;
public:
	RenderImage(int width, int height);
	~RenderImage();
	
	RenderImage(const RenderImage&) = delete;
	RenderImage& operator=(const RenderImage&) = delete;

	int GetWidth();
	int GetHeight();
	
	void Clear(Color col);
	void SetPixel(int x, int y, Color col);
	void Line(int x0, int y0, int x1, int y1, Color col);

	GLuint GetTextureId() const { return textureID; };
	void BufferImage();
};

