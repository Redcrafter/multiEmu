#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <GL/glew.h>

GLuint CreateShader(GLuint type, const char* path, bool isPath) {
	std::string code;

	if(isPath) {
		std::ifstream stream(path, std::ios::in);
		if(!stream.is_open()) {
			std::cerr << "Failed to open " << path << std::endl;
			throw std::logic_error("Failed to open path");
		}

		std::stringstream sstr;
		sstr << stream.rdbuf();
		code = sstr.str();

		stream.close();
	} else {
		code = path;
	}

	const char* cstr = code.c_str();

	GLuint shaderId = glCreateShader(type);
	glShaderSource(shaderId, 1, &cstr, NULL);
	glCompileShader(shaderId);

	GLint result = GL_FALSE;
	int infoLogLength;

	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
	if(infoLogLength > 0) {
		std::vector<char> errorMessage(infoLogLength + 1);
		glGetShaderInfoLog(shaderId, infoLogLength, NULL, &errorMessage[0]);
		printf("%s\n", &errorMessage[0]);

		throw std::runtime_error("Failed to compile shader");
	}

	return shaderId;
}

GLuint LoadShaders(const char* vertex, const char* fragment, bool isPath) {
	// Create the shaders
	GLuint VertexShaderID = CreateShader(GL_VERTEX_SHADER, vertex, isPath);
	GLuint FragmentShaderID = CreateShader(GL_FRAGMENT_SHADER, fragment, isPath);

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Link the program
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if(InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);

		throw std::runtime_error("Error linking program");
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

// TODO: copy contents of shaders into this file automatically
GLuint LoadShaders() {
	const char* vs =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vertexPos;\n"
		"layout(location = 1) in vec2 vertexUV;\n"
		"out vec2 UV;\n"
		"uniform mat4 MVP;\n"
		"void main() {\n"
		"	gl_Position =  MVP * vec4(vertexPos, 1);\n"
		"	UV = vertexUV;\n"
		"}";

	const char* fs =
		"#version 330 core\n"
		"in vec2 UV;\n"
		"out vec3 color;\n"
		"uniform sampler2D textureSampler;\n"
		"void main() {\n"
		"	color = texture(textureSampler, UV).rgb;\n"
		"}";

	return LoadShaders(vs, fs, false);
}
