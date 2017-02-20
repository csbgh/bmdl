#pragma once

const static char *vertSource = \
	"#version 330 core\n"

	"layout(location = 0) in vec3 v_position;"
	"layout(location = 1) in vec3 v_normal;"

	"layout(std140) uniform uniformData"
	"{"
		"mat4 modelViewProjection;"
	"};"

	"out vec3 normal;"

	"void main()"
	"{"
		"normal = v_normal;"
		"gl_Position = modelViewProjection * vec4(v_position, 1);"
	"}";

const static char *fragSource = \
	"#version 330 core\n"

	"in vec3 normal;"

	"out vec4 out_color;"

	"void main()"
	"{"
		"vec3 scaledNorm = (normal + 1.0f) * 0.5f;"
		"out_color = vec4(scaledNorm.xyz, 1.0f);"
	"}";