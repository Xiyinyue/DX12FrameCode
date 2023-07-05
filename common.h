#pragma once
#ifndef COMMON_H
#define COMMON_H
#include<Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <wincodec.h>
#include <windowsx.h>
#include<iostream>
#include <unordered_map>
#include "../DXUtils/d3dx12.h"
#include "Camera.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <array>
template <typename T>
constexpr UINT CalcConstantBufferByteSize()
{
	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.  We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	UINT byteSize = sizeof(T);
	return (byteSize + 255) & ~255;
}


struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoordinate;
};

struct Light
{
	DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
	float FalloffStart = 1.0f;                          // point/spot light only
	DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
	float FalloffEnd = 5.0f;                           // point/spot light only
	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
	float SpotPower = 64.0f;                            // spot light only

};
constexpr UINT MAXLIGHTS = 16;

extern DirectX::XMMATRIX InitMatrix4X4();
extern DirectX::XMFLOAT4X4 InitFloat4X4();
extern UINT width;
extern UINT height;
struct SceneConstantBuffer
{
	XMFLOAT4X4 MVP;
	XMFLOAT4X4 gWorld;
	XMFLOAT3 ViewPos;
	UINT scpad1;
	XMFLOAT3 Look;
	UINT scpad2;
	XMFLOAT3 Up;
	UINT scpad3;
	XMFLOAT3 Right;
	UINT scpad4;
	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
	Light Lights[MAXLIGHTS];
};
struct ObjConstantBuffer
{
	XMFLOAT4X4 WorldTransform;
	XMFLOAT4X4 TexTransform;
	UINT MaterialIndex;
	UINT     CBPad1;
	UINT     CBPad2;
	UINT     CBPad3;
};

struct Material
{
	std::string name;
	UINT matCBIndex;
	UINT DiffuseSrvHeapIndex;
	UINT NormalSrvHeapIndex;
	float Frenel;
	float Roughness;
	XMFLOAT4X4 matTransForm;
	XMFLOAT3 ambient;
	XMFLOAT3 specular;
	XMFLOAT3 diffuse;
};

struct Mesh
{
	std::string name;
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
};

struct MaterialConstants
{
	DirectX::XMFLOAT3 Diffuse = { 1.0f, 1.0f, 1.0f };
	UINT matpad0;
	DirectX::XMFLOAT3 Specular = { 0.01f, 0.01f, 0.01f };
	UINT matpad1;
	DirectX::XMFLOAT3 Ambient = { 0.01f, 0.01f, 0.01f };
	UINT matpad2;
	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = InitFloat4X4();
};

//draw directly operate the Instance: 
//IndexCount 
//StartIndexLocation 
//BaseVertexLocation
struct  RenderItem
{
	UINT BaseVertexLocation = 0;
	UINT StartIndexLocation = 0;
	UINT IndicesCount = 0;
	Material mat;
	UINT ObjCBIndex = -1;
	UINT matIndex = 0;

};
static const UINT FrameCount = 2;
Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target);


#endif