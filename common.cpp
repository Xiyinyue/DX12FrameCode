#pragma once
#include "common.h"

// 定义函数和变量
DirectX::XMMATRIX InitMatrix4X4()
{
	DirectX::XMMATRIX I(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	return I;
}

DirectX::XMFLOAT4X4 InitFloat4X4()
{
	DirectX::XMFLOAT4X4 I(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	return I;
}

UINT width = 800;
UINT height = 600;

