#include "RenderImage.h"

#include <cstring>
#include <stdexcept>

#define GL_RGB 0x1907
#define GL_NEAREST 0x2600
#define GL_BGR 0x80E0

void _glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
typedef void(APIENTRYP PFNGLTEXSUBIMAGE2DPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D = _glTexSubImage2D;
void _glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels) {
	glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)imgl3wGetProcAddress("glTexSubImage2D");
	return glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

RenderImage::RenderImage(const int width, const int height) {
	glGenTextures(1, &textureID);
	Width = width;
	Height = height;

	imgData = new Color[width * height];
	memset(imgData, 0, width * height * sizeof(Color));

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_BGR, GL_UNSIGNED_BYTE, imgData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

RenderImage::~RenderImage() {
	glDeleteTextures(1, &textureID);
	delete[] imgData;
}

int RenderImage::GetWidth() {
	return Width;
}
int RenderImage::GetHeight() {
	return Height;
}

void RenderImage::Clear(Color col) {
	for(int i = 0; i < Width * Height; ++i) {
		imgData[i] = col;
	}
}

void RenderImage::SetPixel(const int x, const int y, const Color col) {
	if(x >= 0 && x < Width && y >= 0 && y < Height) {
		imgData[x + y * Width] = col;
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
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, GL_RGB, GL_UNSIGNED_BYTE, imgData);
}
