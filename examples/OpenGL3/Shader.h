#pragma once

const static char *vertSource = \
	"#version 330 core\n"

	"layout(location = 0) in vec3 v_position;"

	"layout(std140) uniform uniformData"
	"{"
		"mat4 modelViewProjection;"
	"};"

	"void main()"
	"{"
		"gl_Position = modelViewProjection * vec4(v_position, 1);"
	"}";

const static char *fragSource = \
	"#version 330 core\n"

	"out vec4 out_color;"

	"void main()"
	"{"
		"out_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);"
	"}";