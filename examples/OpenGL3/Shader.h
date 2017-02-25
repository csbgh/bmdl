#pragma once

const static char *vertSource = \
	"#version 330 core\n"

	"layout(location = 0) in vec3 v_position;"
	"layout(location = 1) in vec3 v_normal;"
	"layout(location = 2) in vec2 v_uv;"

	"layout(std140) uniform uniformData"
	"{"
	"mat4 modelViewProjection;"
	"};"

	"out vec3 normal;"
	"out vec2 uv;"

	"void main()"
	"{"
		"normal = v_normal;"
		"uv = v_uv;"
		"gl_Position = modelViewProjection * vec4(v_position, 1);"
	"}";

const static char *fragSource = \
	"#version 330 core\n"

	"in vec3 normal;"
	"in vec2 uv;"

	"out vec4 out_color;"

	"void main()"
	"{"
		"float total = floor(uv.x * 32.0f) + floor(uv.y * 48.0f);"
		"bool isEven = mod(total,2.0)==0.0;"
		"float uvMult = (isEven) ? 0.75f : 1.0f;"
		"vec3 scaledNorm = (normal + 1.0f) * 0.5f;"
		"out_color = vec4(scaledNorm.xyz * uvMult, 1.0f);"
	"}";