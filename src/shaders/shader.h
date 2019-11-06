#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <GL/glew.h>

GLuint CreateShader(GLuint type, const char* path) {
	GLuint shaderId = glCreateShader(type);

	std::ifstream stream(path, std::ios::in);
	if(!stream.is_open()) {
		std::cerr << "Failed to open " << path << std::endl;
		throw std::logic_error("Failed to open path");
	}

	std::string code;
	std::stringstream sstr;
	sstr << stream.rdbuf();
	code = sstr.str();
	
	stream.close();

	const char* cstr = code.c_str();
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

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {
	// Create the shaders
	GLuint VertexShaderID = CreateShader(GL_VERTEX_SHADER, vertex_file_path);
	GLuint FragmentShaderID = CreateShader(GL_FRAGMENT_SHADER, fragment_file_path);

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
