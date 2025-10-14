#pragma once
#include <cassert>
#include <vector>

#include <GLFW/glfw3.h>

struct Color {
	uint8_t R, G, B;
};

class Texture {
  private:
	GLuint textureID;
	int Width, Height;
	std::vector<Color> imgData;

  public:
	Texture(int width, int height) {
		glGenTextures(1, &textureID);
		Width = width;
		Height = height;

		imgData.resize(width * height);

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, imgData.data());

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	~Texture() {
		glDeleteTextures(1, &textureID);
	}

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

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

	GLuint GetTextureId() const { return textureID; }
	void BufferImage() const {
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, GL_RGB, GL_UNSIGNED_BYTE, imgData.data());
	}
};
