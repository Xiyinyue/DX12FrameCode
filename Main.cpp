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

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
//11111111111111111111111111111111111111111111
#pragma comment(lib, "assimp-vc142-mtd.lib")

using namespace Microsoft::WRL;

constexpr UINT MAXLIGHTS = 16;
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


std::vector<Material>mMaterial;
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
struct MaterialConstants
{
	DirectX::XMFLOAT3 Diffuse = { 1.0f, 1.0f, 1.0f };
	UINT matpad0;
	DirectX::XMFLOAT3 Specular = { 0.01f, 0.01f, 0.01f };
	UINT matpad1;
	DirectX::XMFLOAT3 Ambient = { 0.01f, 0.01f, 0.01f};
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


std::unordered_map<std::string, RenderItem>mObjRenderItems;
std::vector<RenderItem>TestTextureRenderItem;

const UINT FrameCount = 2;
UINT width = 800;
UINT height = 600;
HWND hwnd;

//管线对象
CD3DX12_VIEWPORT viewport(0.0f, 0.0f, width, height);
CD3DX12_RECT scissorRect(0, 0, width, height);
ComPtr<IDXGISwapChain3> swapChain;
ComPtr<ID3D12Device> device;
ComPtr<ID3D12Resource> renderTargets[FrameCount];
ComPtr<ID3D12CommandAllocator> commandAllocator;
ComPtr<ID3D12CommandQueue> commandQueue;
ComPtr<ID3D12RootSignature> rootSignature;
ComPtr<ID3D12DescriptorHeap> rtvHeap;
ComPtr<ID3D12DescriptorHeap> dsvHeap;
ComPtr<ID3D12DescriptorHeap> cbvsrvHeap;

ComPtr<ID3D12PipelineState> pipelineState;
ComPtr<ID3D12PipelineState> debugPipelineState;
ComPtr<ID3D12PipelineState> FinalPipelineState;


ComPtr<ID3D12GraphicsCommandList> commandList;
UINT rtvDescriptorSize;

ComPtr<ID3D12Resource> vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
ComPtr<ID3D12Resource> indexBuffer;
D3D12_INDEX_BUFFER_VIEW indexBufferView;

ComPtr<ID3D12Resource> depthStencilBuffer;

ComPtr<ID3D12Resource> constantBuffer;
ComPtr<ID3D12Resource>ObjCBResource;

ComPtr<ID3D12Resource> MaterialCBResource;

UINT8* pCbvDataBegin;
UINT8* pobjCbvDataBegin;
UINT8* pMatCbvDataBegin;
UINT meshIndex = 0;
 UINT ObjCBsize;
 UINT matCBsize;
UINT mCbvSrvUavDescriptorSize = 0;
// 同步对象
UINT frameIndex;
HANDLE fenceEvent;
ComPtr<ID3D12Fence> fence;
UINT64 fenceValue;

//time
float offset;
bool isOffset = false;


//
ComPtr<ID3D12Resource> textureBuffer;
ComPtr<ID3D12Resource> textureBufferUploadHeap;
UINT cbvsrvDescriptorSize;
BYTE* imageData;
//

ComPtr<ID3D12Resource>GBufferResources[5];

//
POINT lastMousePos;
Camera camera;
//

//
UINT VertexOffset = 0;
UINT IndexOffset = 0;
//
struct Mesh
{
	std::string name;
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
};


std::vector<Mesh> meshes;
std::unordered_map<std::string, Mesh>mMesh;
std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

void OnDestroy();
void OnRender();
void LoadModels(const char* modelFilename);
void InitD3DResource();
void OnUpdate();
void OnKeyboardInput();
void OnMouseMove();
void PopulateCommandList();
void WaitForPreviousFrame();
void LoadAsset();
void GenerateTestTextureRenderItem(float x, float y, float w, float h, float depth);
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target);
void LoadModels(const char* modelFilename)
{
	UINT IndexLocation = 0;
	UINT VertexLocation = 0;
	assert(modelFilename != nullptr);
	const std::string filePath(modelFilename);

	Assimp::Importer importer;
	const std::uint32_t flags{ aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded };
	const aiScene* scene{ importer.ReadFile(filePath.c_str(), flags) };
	assert(scene != nullptr);

	assert(scene->HasMeshes());
	bool DefaultFlag = true;
	assert(scene->HasMaterials());
	for (std::uint32_t i = 0U; i < scene->mNumMaterials; ++i) {

		Material tempMaterial;
		aiMaterial* mat{ scene->mMaterials[i] };
		aiColor3D MaterialColor;
		mat->Get(AI_MATKEY_COLOR_AMBIENT, MaterialColor);
		tempMaterial.ambient = XMFLOAT3(MaterialColor.r, MaterialColor.g, MaterialColor.b);
		tempMaterial.name = (mat->GetName().C_Str());
		mat->Get(AI_MATKEY_COLOR_DIFFUSE, MaterialColor);//ka
		
		tempMaterial.diffuse = XMFLOAT3(MaterialColor.r, MaterialColor.g, MaterialColor.b);

		mat->Get(AI_MATKEY_COLOR_SPECULAR, MaterialColor);
	tempMaterial.specular = XMFLOAT3(MaterialColor.r, MaterialColor.g, MaterialColor.b);
		//mat->Get(AI_MATKEY_, MaterialColor);
		

		mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, MaterialColor);
		//mat->Get()
		

		mMaterial.push_back(tempMaterial);
	
	}
	for (std::uint32_t i = 0U; i < scene->mNumMeshes; ++i)
	{
		aiMesh* mesh{ scene->mMeshes[i] };
		assert(mesh != nullptr);
		Mesh tempMesh;
		{
			// Positions
			const std::size_t numVertices{ mesh->mNumVertices };
			assert(numVertices > 0U);
			tempMesh.vertices.resize(numVertices);
			for (std::uint32_t i = 0U; i < numVertices; ++i)
			{
				tempMesh.vertices[i].position = XMFLOAT3(reinterpret_cast<const float*>(&mesh->mVertices[i]));
				tempMesh.vertices[i].Normal= XMFLOAT3(reinterpret_cast<const float*>(&mesh->mNormals[i]));
				//tempMesh.vertices[i].TexCoordinate= XMFLOAT3(reinterpret_cast<const float*>(&mesh->mTextureCoords));
			}
			// Indices

			const std::uint32_t numFaces{ mesh->mNumFaces };
			assert(numFaces > 0U);
			for (std::uint32_t i = 0U; i < numFaces; ++i)
			{
				const aiFace* face = &mesh->mFaces[i];
				assert(face != nullptr);
				// We only allow triangles
				assert(face->mNumIndices == 3U);

				tempMesh.indices.push_back(face->mIndices[0U]);
				tempMesh.indices.push_back(face->mIndices[1U]);
				tempMesh.indices.push_back(face->mIndices[2U]);
			}
		
			if (mesh->HasTextureCoords(0U))
			{
				assert(mesh->GetNumUVChannels() == 1U);
				const aiVector3D* aiTextureCoordinates{ mesh->mTextureCoords[0U] };
				assert(aiTextureCoordinates != nullptr);
				for (std::uint32_t i = 0U; i < numVertices; i++)
				{
					tempMesh.vertices[i].TexCoordinate = XMFLOAT2(reinterpret_cast<const float*>(&aiTextureCoordinates[i]));
				}
			}	
			mObjRenderItems[mesh->mName.C_Str()].BaseVertexLocation=VertexOffset;
			mObjRenderItems[mesh->mName.C_Str()].StartIndexLocation= IndexOffset;
			mObjRenderItems[mesh->mName.C_Str()].IndicesCount = numFaces * 3;
			mObjRenderItems[mesh->mName.C_Str()].mat.matCBIndex= mesh->mMaterialIndex;
			mObjRenderItems[mesh->mName.C_Str()].ObjCBIndex= meshIndex;
			VertexOffset += numVertices;
			IndexOffset += (numFaces * 3);
		}
		tempMesh.name = mesh->mName.C_Str();
		meshes.push_back(tempMesh);
		meshIndex++;
	}

}

void OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - lastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - lastMousePos.y));

		camera.Pitch(dy);
		camera.RotateY(dx);
	}

	lastMousePos.x = x;
	lastMousePos.y = y;
}

void OnKeyboardInput()
{
	const float dt = 0.01f;

	if (GetAsyncKeyState('W') & 0x8000)
		camera.Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		camera.Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		camera.Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		camera.Strafe(10.0f * dt);

	camera.UpdateViewMatrix();
}

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

std::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
	HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
	HRESULT Error() const { return m_hr; }
private:
	const HRESULT m_hr;
};

void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw HrException(hr);
	}
}

IDXGIAdapter1* GetSupportedAdapter(ComPtr<IDXGIFactory4>& dxgiFactory, const D3D_FEATURE_LEVEL featureLevel)
{
	IDXGIAdapter1* adapter = nullptr;
	for (std::uint32_t adapterIndex = 0U; ; ++adapterIndex)
	{
		IDXGIAdapter1* currentAdapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == dxgiFactory->EnumAdapters1(adapterIndex, &currentAdapter))
		{
			break;
		}

		const HRESULT hres = D3D12CreateDevice(currentAdapter, featureLevel, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hres))
		{
			adapter = currentAdapter;
			break;
		}

		currentAdapter->Release();
	}

	return adapter;
}

void WaitForPreviousFrame()
{
	const UINT64 tempFenceValue = fenceValue;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), tempFenceValue));
	fenceValue++;

	if (fence->GetCompletedValue() < tempFenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(tempFenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void InitD3DResource()
{

#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif 

	ComPtr<IDXGIFactory4> mDxgiFactory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(mDxgiFactory.GetAddressOf())));

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	IDXGIAdapter1* adapter = nullptr;
	for (std::uint32_t i = 0U; i < _countof(featureLevels); ++i)
	{
		adapter = GetSupportedAdapter(mDxgiFactory, featureLevels[i]);
		if (adapter != nullptr)
		{
			break;
		}
	}

	if (adapter != nullptr)
	{
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(device.GetAddressOf()));
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(mDxgiFactory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	ThrowIfFailed(swapChain1.As(&swapChain));
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount+5;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		//
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
		//
		D3D12_DESCRIPTOR_HEAP_DESC cbvsrvHeapDesc = {};
		// sceneCB  ObjCB  MaterialCB 
		cbvsrvHeapDesc.NumDescriptors = 7;
		cbvsrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvsrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&cbvsrvHeapDesc, IID_PPV_ARGS(&cbvsrvHeap)));
		cbvsrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT n = 0; n < FrameCount; n++)
	{
		ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
		device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	


	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
}

void BuildRootSignature()
{
	mCbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	{

		CD3DX12_DESCRIPTOR_RANGE ranges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[4];

		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1,0);

		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[1].InitAsConstantBufferView(1);
		rootParameters[2].InitAsShaderResourceView(0);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);


		auto staticSamplers = GetStaticSamplers();
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(4, rootParameters, (UINT)staticSamplers.size(), staticSamplers.data(), 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf())));
	}


}
void BuildShadersAndInputLayout()
{
	{
		// 		ComPtr<ID3DBlob> vertexShader;
		// 		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		
		mShaders["standardVS"] = CompileShader(L"Assets/Shader/shaders.hlsl", nullptr, "VSMain", "vs_5_0");
		mShaders["opaquePS"] = CompileShader(L"Assets/Shader/shaders.hlsl", nullptr, "PSMain", "ps_5_0");

		mShaders["debugVS"]= CompileShader(L"Assets/Shader/debug.hlsl", nullptr, "VS", "vs_5_0");
		mShaders["debugPS"] = CompileShader(L"Assets/Shader/debug.hlsl", nullptr, "PS", "ps_5_0");

		mShaders["FinalVS"] = CompileShader(L"Assets/Shader/FinalCompute.hlsl", nullptr, "VS", "vs_5_0");
		mShaders["FinalPS"] = CompileShader(L"Assets/Shader/FinalCompute.hlsl", nullptr, "PS", "ps_5_0");

		mInputLayout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } ,
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0  }
		};

	}

}


void BuildPipelineState()
{


	{
		BuildShadersAndInputLayout();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { mInputLayout.data(),(UINT)mInputLayout.size() };
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
		};
		psoDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
		};
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
	}

	




 	{
 		D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPipelineDesc = {};
 		debugPipelineDesc.InputLayout = { mInputLayout.data(),(UINT)mInputLayout.size() };
 		debugPipelineDesc.pRootSignature = rootSignature.Get();
 		debugPipelineDesc.VS = {
 		reinterpret_cast<BYTE*>(mShaders["debugVS"]->GetBufferPointer()),
 		mShaders["debugVS"]->GetBufferSize()
 		};
 		debugPipelineDesc.PS = {
 		reinterpret_cast<BYTE*>(mShaders["debugPS"]->GetBufferPointer()),
 		mShaders["debugPS"]->GetBufferSize()
 		};
 		debugPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
 		debugPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
 		debugPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
 		debugPipelineDesc.SampleMask = UINT_MAX;
 		debugPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
 		debugPipelineDesc.NumRenderTargets = 5;
 		debugPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		debugPipelineDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		debugPipelineDesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		debugPipelineDesc.RTVFormats[3] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		debugPipelineDesc.RTVFormats[4] = DXGI_FORMAT_R32G32B32A32_FLOAT;
	
 		debugPipelineDesc.SampleDesc.Count = 1;
 		debugPipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
 		ThrowIfFailed(device->CreateGraphicsPipelineState(&debugPipelineDesc, IID_PPV_ARGS(&debugPipelineState)));
 	}


	{
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC FinalPSODesc = {};
			FinalPSODesc.InputLayout = { mInputLayout.data(),(UINT)mInputLayout.size() };
			FinalPSODesc.pRootSignature = rootSignature.Get();
			FinalPSODesc.VS = {
			reinterpret_cast<BYTE*>(mShaders["FinalVS"]->GetBufferPointer()),
			mShaders["FinalVS"]->GetBufferSize()
			};
			FinalPSODesc.PS = {
			reinterpret_cast<BYTE*>(mShaders["FinalPS"]->GetBufferPointer()),
			mShaders["FinalPS"]->GetBufferSize()
			};
			FinalPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			FinalPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			FinalPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			FinalPSODesc.SampleMask = UINT_MAX;
			FinalPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			FinalPSODesc.NumRenderTargets = 1;
			FinalPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

			FinalPSODesc.SampleDesc.Count = 1;
			FinalPSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			ThrowIfFailed(device->CreateGraphicsPipelineState(&FinalPSODesc, IID_PPV_ARGS(&FinalPipelineState)));

		}
	
	
	
	}



	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList)));





	ThrowIfFailed(commandList->Close());
}
void BuildVertexIndexBuffer()
{
	{
		// 		const UINT vertexBufferSize = sizeof(meshes[0].vertices);
		// 		const UINT indexBufferSize = sizeof(meshes[0].indices);
		std::uint32_t vertexBufferSize = 0;
		std::uint32_t indexBufferSize = 0;
		for (int i = 0; i < meshes.size(); i++)
		{
			vertexBufferSize += meshes[i].vertices.size();
			indexBufferSize += meshes[i].indices.size();
		}
		std::vector<Vertex>Vertices;
		std::vector<std::uint32_t>Indices;
		//sizeofindices = indexBufferSize;
		vertexBufferSize *= sizeof(Vertex);
		indexBufferSize *= sizeof(std::uint32_t);

		for (int i = 0; i < meshes.size(); i++)
		{
			Vertices.insert(Vertices.end(), std::begin(meshes[i].vertices), std::end(meshes[i].vertices));
			Indices.insert(Indices.end(), std::begin(meshes[i].indices), std::end(meshes[i].indices));
		}

		CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC vertexResourceDes = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		CD3DX12_RESOURCE_DESC indexResourceDes = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&vertexResourceDes,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer)));
		vertexBuffer->SetName(L"vertexBuffer");
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&indexResourceDes,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&indexBuffer)));
		indexBuffer->SetName(L"IndexBuffer");

		UINT8* pDataBegin;
		CD3DX12_RANGE readRange(0, 0);

		ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
		memcpy(pDataBegin, &Vertices[0], vertexBufferSize);
		vertexBuffer->Unmap(0, nullptr);

		ThrowIfFailed(indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
		memcpy(pDataBegin, &Indices[0], indexBufferSize);
		indexBuffer->Unmap(0, nullptr);

		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(Vertex);
		vertexBufferView.SizeInBytes = vertexBufferSize;

		indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = indexBufferSize;
	}
}
void BuildDepthStencilBuffer()
{
	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heapProperties2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC tex2D = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties2,
		D3D12_HEAP_FLAG_NONE,
		&tex2D,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&depthStencilBuffer)));

	device->CreateDepthStencilView(depthStencilBuffer.Get(), &depthStencilDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());


}

void LoadAsset()
{
	BuildRootSignature();
	BuildPipelineState();
	BuildVertexIndexBuffer();
	BuildDepthStencilBuffer();




		CD3DX12_RANGE readRange(0, 0);
		CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const UINT constantBufferSize = CalcConstantBufferByteSize<SceneConstantBuffer>();
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constantBuffer)));
		constantBuffer->SetName(L"constantBuffer");


		CD3DX12_CPU_DESCRIPTOR_HANDLE hCbvSrvHeap(cbvsrvHeap->
			GetCPUDescriptorHandleForHeapStart());
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constantBufferSize;
		device->CreateConstantBufferView(&cbvDesc, hCbvSrvHeap);
		ThrowIfFailed(constantBuffer->Map(0, &readRange, 
			reinterpret_cast<void**>(&pCbvDataBegin)));




		const UINT objCBsize =CalcConstantBufferByteSize<ObjConstantBuffer>();
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(objCBsize*meshIndex),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&ObjCBResource)));

		cbvDesc.BufferLocation = ObjCBResource->GetGPUVirtualAddress();
		hCbvSrvHeap.Offset(1, mCbvSrvUavDescriptorSize);
		cbvDesc.SizeInBytes = objCBsize;
		device->CreateConstantBufferView(&cbvDesc, hCbvSrvHeap);
		ThrowIfFailed(ObjCBResource->Map(0, &readRange, 
			reinterpret_cast<void**>(&pobjCbvDataBegin)));
		ObjCBResource->SetName(L"objCB");




		
		matCBsize = sizeof(MaterialConstants);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mMaterial.size() * matCBsize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&MaterialCBResource)));

	
		//D3D12_SHADER_BUFFER_DESC;
		ThrowIfFailed(MaterialCBResource->Map(0, &readRange,
			reinterpret_cast<void**>(&pMatCbvDataBegin)));
		MaterialCBResource->SetName(L"MaterialCBResource");

	hCbvSrvHeap.Offset(1, mCbvSrvUavDescriptorSize);
	//目前还没有用过上一个shader resource 的view 
	//我们看看堆里这个view 是否用过

	{
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = 800;
		texDesc.Height = 600;
		texDesc.DepthOrArraySize = 1;
		texDesc.MipLevels = 1;
		texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		
		const float clearColor[] = { 0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f };
		CD3DX12_CLEAR_VALUE GBufferClearValue = { DXGI_FORMAT_R32G32B32A32_FLOAT,clearColor };


		for (int i = 0; i < 5; i++) {
			ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				&GBufferClearValue,
				IID_PPV_ARGS(&GBufferResources[i])));
		}



		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		auto hGBufferRtvHeapStartCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), 2, rtvDescriptorSize);
		for (int i = 0; i < _countof(GBufferResources); i++)
		{
			device->CreateRenderTargetView(GBufferResources[i].Get(), &rtvDesc, hGBufferRtvHeapStartCpu);
			device->CreateShaderResourceView(GBufferResources[i].Get(), &srvDesc, hCbvSrvHeap);
			hGBufferRtvHeapStartCpu.Offset(1, rtvDescriptorSize);
			hCbvSrvHeap.Offset(1, mCbvSrvUavDescriptorSize);
		}


	}



	{
		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		fenceValue = 1;

		fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
		WaitForPreviousFrame();
	}
}

void PopulateCommandList()
{
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

	commandList->SetGraphicsRootSignature(rootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { cbvsrvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	auto cbaddress = ObjCBResource->GetGPUVirtualAddress();
	auto matCBAdress = MaterialCBResource->GetGPUVirtualAddress();
	commandList->SetGraphicsRootDescriptorTable(0, cbvsrvHeap->GetGPUDescriptorHandleForHeapStart());

	commandList->SetGraphicsRootShaderResourceView(2, matCBAdress);

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);


	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	//const float clearColor[] = { color[0], color[1], color[2], 1.0f };
	const float clearColor[] = { 0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f };
	//const float clearColor[] = { 0.0f ,0.0f ,0.0f, 1.000000000f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);

	commandList->SetGraphicsRootConstantBufferView(1, cbaddress );
//  	for (auto t : mObjRenderItems) {
// 
//  	commandList->SetGraphicsRootConstantBufferView(1, cbaddress + t.second.ObjCBIndex * ObjCBsize);
//  	commandList->DrawIndexedInstanced(t.second.IndicesCount, 1, t.second.StartIndexLocation, t.second.BaseVertexLocation, 0);
//  	}


	{	
		
		commandList->SetPipelineState(debugPipelineState.Get());
		GBufferResources[0]->SetName(L"GBufferResources0");
		GBufferResources[1]->SetName(L"GBufferResources1");

		CD3DX12_CPU_DESCRIPTOR_HANDLE GBufferRtvHanle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), 2, rtvDescriptorSize);
		for (int i = 0; i < _countof(GBufferResources); i++)
		{
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GBufferResources[i].Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
		}
		
		
 		commandList->ClearRenderTargetView(GBufferRtvHanle, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), 3, rtvDescriptorSize),clearColor, 0, nullptr);
 		commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		
		commandList->OMSetRenderTargets(5, &GBufferRtvHanle, true, &dsvHandle);

		for (auto t : mObjRenderItems)
		{
			commandList->SetGraphicsRootConstantBufferView(1, cbaddress + t.second.ObjCBIndex * ObjCBsize);
			commandList->DrawIndexedInstanced(t.second.IndicesCount, 1, t.second.StartIndexLocation, t.second.BaseVertexLocation, 0);
		}


		for (int i = 0; i < _countof(GBufferResources); i++)
		{
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GBufferResources[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		}


		//commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
 		//commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		commandList->SetPipelineState(FinalPipelineState.Get());
		cbvsrvHeap->GetGPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		commandList->SetGraphicsRootDescriptorTable(0, cbvsrvHeap->GetGPUDescriptorHandleForHeapStart());
		auto hsrv=CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvsrvHeap->GetGPUDescriptorHandleForHeapStart(), 2,cbvsrvDescriptorSize);
 		commandList->SetGraphicsRootDescriptorTable(3,hsrv);

		for (auto &t:TestTextureRenderItem)
	 	{
	 		commandList->DrawIndexedInstanced(t.IndicesCount, 1, t.StartIndexLocation, t.BaseVertexLocation, 0);
	 	}

		


	}


	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET , D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(commandList->Close());
}

void OnUpdate()
{

	if (offset <= 3.0f && isOffset)
	{
		offset += 0.01f;
		isOffset = true;
	}
	else
	{
		offset -= 0.01f;
		offset <= -3 ? isOffset = true : isOffset = false;

	}
	OnKeyboardInput();
	XMMATRIX v = camera.GetView();
	XMMATRIX p = camera.GetProj();

	XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	XMMATRIX rotation = XMMatrixRotationAxis({ 0.0f, 1.0f, 0.0f }, XMConvertToRadians(90.0f));
	XMMATRIX m = scale * rotation * translation;
	XMMATRIX MVP = m * v * p;

	SceneConstantBuffer passCBconstants;
	XMStoreFloat4x4(&passCBconstants.MVP, XMMatrixTranspose(MVP));
	XMStoreFloat4x4(&passCBconstants.gWorld, XMMatrixTranspose(m));
	passCBconstants.ViewPos = camera.getPosition();
	passCBconstants.Look = camera.getmLook();
	passCBconstants.Up = camera.getmUp();
	passCBconstants.Right = camera.getmRight(); 
	passCBconstants.AmbientLight = { 0.01f, 0.01f, 0.01f, 1.0f };
	passCBconstants.Lights[0].Position = { -0.0000,1.90000,0.00000 };
	passCBconstants.Lights[0].SpotPower = 0.8f;
	passCBconstants.Lights[0].Strength = { 1.0f,1.0f, 1.0f };
	memcpy(pCbvDataBegin, &passCBconstants, sizeof(passCBconstants));

	
	for (auto &t:mObjRenderItems)
	{
		ObjConstantBuffer tempobjCB;
		auto tempObjIndex = t.second.ObjCBIndex;
		tempobjCB.MaterialIndex = t.second.mat.matCBIndex;

		XMStoreFloat4x4(&tempobjCB.TexTransform , InitMatrix4X4());
		XMStoreFloat4x4(&tempobjCB.WorldTransform , InitMatrix4X4());
		memcpy(&pobjCbvDataBegin[tempObjIndex *ObjCBsize], &tempobjCB, sizeof(ObjConstantBuffer));

	}
	for (int i=0;i<mMaterial.size();i++)
	{
		MaterialConstants tempMat;
		tempMat.Ambient = mMaterial[i].ambient;
		tempMat.Diffuse = mMaterial[i].diffuse;
		tempMat.Specular = mMaterial[i].specular;
		XMStoreFloat4x4(&tempMat.MatTransform, InitMatrix4X4());
		memcpy(&pMatCbvDataBegin[i* matCBsize], &tempMat, sizeof(MaterialConstants));
	}
	
	


}

void OnRender()
{
	PopulateCommandList();

	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(swapChain->Present(1, 0));

	WaitForPreviousFrame();
}
void OnDestroy()
{
	WaitForPreviousFrame();

	CloseHandle(fenceEvent);

	delete imageData;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		OnUpdate();
		OnRender();
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"HTRenderClass";

	RegisterClassEx(&windowClass);

	hwnd = CreateWindow(
		windowClass.lpszClassName,
		L"TTRenderEr",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	camera.SetPosition(-3.0f, 1.0f, 0.0f);
	ObjCBsize = CalcConstantBufferByteSize<ObjConstantBuffer>();
	InitD3DResource();
	LoadModels("Resources/anotherCornellBox.obj");
	GenerateTestTextureRenderItem(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
	//GenerateTestTextureRenderItem(0.0f, -0.5, 0.5f, 0.5f, 0.0f);
	LoadAsset();

	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{

		}
	}

	OnDestroy();

	return 0;
}
ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	//ThrowIfFailed(hr);

	return byteCode;
}
void GenerateTestTextureRenderItem(float x, float y, float w, float h, float depth)
{
	Mesh tempMesh;
	tempMesh.vertices.resize(4);
	tempMesh.indices.resize(6);

	tempMesh.vertices[0].position = { XMFLOAT3(x, y - h, depth) };
	tempMesh.vertices[0].Normal = { XMFLOAT3(0.0f, 0.0f, -1.0f) };
	tempMesh.vertices[0].TexCoordinate = { XMFLOAT2(0.0f, 1.0f) };

	tempMesh.vertices[1].position = { XMFLOAT3(x, y, depth) };
	tempMesh.vertices[1].Normal = { XMFLOAT3(0.0f, 0.0f, -1.0f) };
	tempMesh.vertices[1].TexCoordinate = { XMFLOAT2(0.0f, 0.0f) };

	tempMesh.vertices[2].position = { XMFLOAT3(x + w, y, depth) };
	tempMesh.vertices[2].Normal = { XMFLOAT3(0.0f, 0.0f, -1.0f) };
	tempMesh.vertices[2].TexCoordinate = { XMFLOAT2(1.0f, 0.0f) };

	tempMesh.vertices[3].position = { XMFLOAT3(x + w, y - h, depth) };
	tempMesh.vertices[3].Normal = { XMFLOAT3(0.0f, 0.0f, -1.0f) };
	tempMesh.vertices[3].TexCoordinate = { XMFLOAT2(1.0f, 1.0f) };

	tempMesh.indices[0] = 0;
	tempMesh.indices[1] = 1;
	tempMesh.indices[2] = 2;

	tempMesh.indices[3] = 0;
	tempMesh.indices[4] = 2;
	tempMesh.indices[5] = 3;

	meshes.push_back(tempMesh);
	RenderItem tempRenderItem;
	tempRenderItem.BaseVertexLocation = VertexOffset;
	tempRenderItem.StartIndexLocation = IndexOffset;
	tempRenderItem.IndicesCount = 6;
	TestTextureRenderItem.push_back(tempRenderItem);
	VertexOffset += 4;
	IndexOffset += 6;

}
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow
	};
}

