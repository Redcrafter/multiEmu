#pragma once
#include <cassert>
// #include <cmath>
#include <cstdint>
#include <vector>

#include <GLFW/glfw3.h>

struct Color {
	uint8_t R, G, B;
};

class RenderImage {
  private:
	GLuint textureID;
	int Width, Height;
	std::vector<Color> imgData;

  public:
	RenderImage(int width, int height) {
		glGenTextures(1, &textureID);
		Width = width;
		Height = height;

		imgData.resize(width * height);

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, imgData.data());

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	~RenderImage() {
		glDeleteTextures(1, &textureID);
	}

	RenderImage(const RenderImage&) = delete;
	RenderImage& operator=(const RenderImage&) = delete;

	int GetWidth() const { return Width; }
	int GetHeight() const { return Height; }

	void Clear(Color col) {
		std::fill(imgData.begin(), imgData.end(), col);
	}
	void SetPixel(int x, int y, Color col) {
		assert(x >= 0 && x < Width && y >= 0 && y < Height);
		imgData[x + y * Width] = col;
	}
	Color GetPixel(int x, int y) const {
		assert(x >= 0 && x < Width && y >= 0 && y < Height);
		return imgData[x + y * Width];
	}

#if false
	void Line(int x0, int y0, int x1, int y1, Color col) {
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
#endif

	GLuint GetTextureId() const { return textureID; };
	void BufferImage() const {
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, GL_RGB, GL_UNSIGNED_BYTE, imgData.data());
	}
};
