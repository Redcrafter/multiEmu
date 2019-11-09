#include "RenderImage.h"
#include <memory>
#include <stdexcept>

RenderImage::RenderImage(const int width, const int height) {
	glGenTextures(1, &textureID);
	Width = width;
	Height = height;

	imgData = new Color[width * height];
	std::memset(imgData, 0, width * height * sizeof(Color));

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_BGR, GL_UNSIGNED_BYTE, imgData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

RenderImage::~RenderImage() {
	delete[] imgData;
}

void RenderImage::Clear(Color col) {
	changed = true;
	for(int i = 0; i < Width * Height; ++i) {
		imgData[i] = col;
	}
}

void RenderImage::SetPixel(const int x, const int y, const Color col) {
	if(x >= 0 && x < Width && y >= 0 && y < Height) {
		imgData[x + y * Width] = col;
		changed = true;
	}
}

void RenderImage::Line(int x0, int y0, int x1, int y1, Color col) {
	const int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	const int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2;

	while(true) {
		SetPixel(x0, y0, col);
		if(x0 == x1 && y0 == y1)
			break;
		
		e2 = 2 * err;
		if(e2 > dy) {
			err += dy;
			x0 += sx;
		}

		if(e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
}

void RenderImage::BufferImage() {
	glBindTexture(GL_TEXTURE_2D, textureID);
	if(changed) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, GL_RGB, GL_UNSIGNED_BYTE, imgData);
	}
}

md5 RenderImage::GetHashCode() {
	return md5(reinterpret_cast<char*>(&imgData), Width * Height * sizeof(Color));
}
