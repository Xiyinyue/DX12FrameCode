#include "SampleCode.h"
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
//11111111111111111111111111111111111111111111
#pragma comment(lib, "assimp-vc142-mtd.lib")



HWND hwnd;
auto sample=std::make_unique<SampleCode>();

void SampleCode::LoadModels(const char* modelFilename)
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
				tempMesh.vertices[i].Normal = XMFLOAT3(reinterpret_cast<const float*>(&mesh->mNormals[i]));
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
			mObjRenderItems[mesh->mName.C_Str()].BaseVertexLocation = VertexOffset;
			mObjRenderItems[mesh->mName.C_Str()].StartIndexLocation = IndexOffset;
			mObjRenderItems[mesh->mName.C_Str()].IndicesCount = numFaces * 3;
			mObjRenderItems[mesh->mName.C_Str()].mat.matCBIndex = mesh->mMaterialIndex;
			mObjRenderItems[mesh->mName.C_Str()].ObjCBIndex = meshIndex;
			VertexOffset += numVertices;
			IndexOffset += (numFaces * 3);
		}
		tempMesh.name = mesh->mName.C_Str();
		meshes.push_back(tempMesh);
		meshIndex++;
	}

}

void SampleCode::OnMouseMove(WPARAM btnState, int x, int y)
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

void SampleCode::OnKeyboardInput()
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


void SampleCode::WaitForPreviousFrame()
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

void SampleCode::InitD3DResource()
{

#if defined(_DEBUG)
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif 

	Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
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

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
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
		rtvHeapDesc.NumDescriptors = FrameCount + 5;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		//
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 2;
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

void SampleCode::BuildRootSignature()
{
	mCbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	{

		CD3DX12_DESCRIPTOR_RANGE ranges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[4];

		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, 0);

		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[1].InitAsConstantBufferView(1);
		rootParameters[2].InitAsShaderResourceView(0);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);


		auto staticSamplers = GetStaticSamplers();
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(4, rootParameters, (UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
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
void SampleCode::BuildShadersAndInputLayout()
{
	{
		// 		ComPtr<ID3DBlob> vertexShader;
		// 		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		mShaders["GBufferVS"] = CompileShader(L"Assets/Shader/GBufferGenerate.hlsl", nullptr, "VS", "vs_5_0");
		mShaders["GBufferPS"] = CompileShader(L"Assets/Shader/GBufferGenerate.hlsl", nullptr, "PS", "ps_5_0");

		mShaders["FinalVS"] = CompileShader(L"Assets/Shader/FinalCompute.hlsl", nullptr, "VS", "vs_5_0");
		mShaders["FinalPS"] = CompileShader(L"Assets/Shader/FinalCompute.hlsl", nullptr, "PS", "ps_5_0");

		mShaders["shadowMappingVS"]= CompileShader(L"Assets/Shader/ShadowMapGenerate.hlsl", nullptr, "VSOnlyMain", "vs_5_0");

		mInputLayout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } ,
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0  }
		};

	}

}


void SampleCode::BuildPipelineState()
{

	
	{
		BuildShadersAndInputLayout();
	}


	// build Render a Gbuffer Pso
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC GBufferPSODesc = {};
		GBufferPSODesc.InputLayout = { mInputLayout.data(),(UINT)mInputLayout.size() };
		GBufferPSODesc.pRootSignature = rootSignature.Get();
		GBufferPSODesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["GBufferVS"]->GetBufferPointer()),
		mShaders["GBufferVS"]->GetBufferSize()
		};
		GBufferPSODesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["GBufferPS"]->GetBufferPointer()),
		mShaders["GBufferPS"]->GetBufferSize()
		};
		GBufferPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		GBufferPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		GBufferPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		GBufferPSODesc.SampleMask = UINT_MAX;
		GBufferPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GBufferPSODesc.NumRenderTargets = 5;
		GBufferPSODesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		GBufferPSODesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		GBufferPSODesc.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		GBufferPSODesc.RTVFormats[3] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		GBufferPSODesc.RTVFormats[4] = DXGI_FORMAT_R32G32B32A32_FLOAT;

		GBufferPSODesc.SampleDesc.Count = 1;
		GBufferPSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&GBufferPSODesc, IID_PPV_ARGS(&GBufferPSO)));
	}


	{
		//build Final Render a quad 's PSO
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
			ThrowIfFailed(device->CreateGraphicsPipelineState(&FinalPSODesc, IID_PPV_ARGS(&FinalPSO)));

		
		

	  // PSO for shadow map pass.
	  //

<<<<<<< Updated upstream
			D3D12_RASTERIZER_DESC mRasterizerStateShadow;
			mRasterizerStateShadow.FillMode = D3D12_FILL_MODE_SOLID;
			mRasterizerStateShadow.CullMode = D3D12_CULL_MODE_BACK;
			mRasterizerStateShadow.FrontCounterClockwise = FALSE;
			mRasterizerStateShadow.SlopeScaledDepthBias = 10.0f;
			mRasterizerStateShadow.DepthBias = 0.05f;
			mRasterizerStateShadow.DepthClipEnable = FALSE;
			mRasterizerStateShadow.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			mRasterizerStateShadow.MultisampleEnable = FALSE;
			mRasterizerStateShadow.AntialiasedLineEnable = FALSE;
			mRasterizerStateShadow.ForcedSampleCount = 0;
			mRasterizerStateShadow.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
=======


// 			CD3DX12_RASTERIZER_DESC mRasterizerStateShadow = {};
// 			mRasterizerStateShadow.FillMode = D3D12_FILL_MODE_SOLID;
// 			mRasterizerStateShadow.CullMode = D3D12_CULL_MODE_BACK;
// 
// 			mRasterizerStateShadow.SlopeScaledDepthBias = 1.0f;
// 			mRasterizerStateShadow.DepthBias = 10000.0f;
// 
// 			mRasterizerStateShadow.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
// 		
>>>>>>> Stashed changes

			D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = FinalPSODesc;
		/*	smapPsoDesc.RasterizerState = mRasterizerStateShadow;*/
			smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
			smapPsoDesc.RasterizerState.DepthBias = 100.0f;
			smapPsoDesc.pRootSignature = rootSignature.Get();
			smapPsoDesc.VS =
			{
				reinterpret_cast<BYTE*>(mShaders["shadowMappingVS"]->GetBufferPointer()),
				mShaders["shadowMappingVS"]->GetBufferSize()
			};
			smapPsoDesc.PS = { 
				nullptr,
				0 
			};
			// Shadow map pass does not have a render target.
			smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
			smapPsoDesc.NumRenderTargets = 0;
			ThrowIfFailed(device->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&ShadowMappingPSO)));

		
		
		
		
		
		}


	}



	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), FinalPSO.Get(), IID_PPV_ARGS(&commandList)));





	ThrowIfFailed(commandList->Close());
}
void SampleCode::BuildVertexIndexBuffer()
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
void SampleCode::BuildDepthStencilBuffer()
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

void SampleCode::LoadAsset()
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




	const UINT objCBsize = CalcConstantBufferByteSize<ObjConstantBuffer>();
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(objCBsize * meshIndex),
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

void SampleCode::RenderGBuffer()
{

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());


	auto cbaddress = ObjCBResource->GetGPUVirtualAddress();
	auto matCBAdress = MaterialCBResource->GetGPUVirtualAddress();
	const float clearColor[] = { 0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f };

	commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);

	commandList->SetGraphicsRootDescriptorTable(0, cbvsrvHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootShaderResourceView(2, matCBAdress);
	commandList->SetGraphicsRootConstantBufferView(1, cbaddress);

	
		commandList->SetPipelineState(GBufferPSO.Get());

		CD3DX12_CPU_DESCRIPTOR_HANDLE GBufferRtvHanle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), 2, rtvDescriptorSize);
		for (int i = 0; i < _countof(GBufferResources); i++)
		{
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GBufferResources[i].Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
		}


		commandList->ClearRenderTargetView(GBufferRtvHanle, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), 3, rtvDescriptorSize), clearColor, 0, nullptr);
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

}

void SampleCode::FinalRender()
{
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), FinalPSO.Get()));

	commandList->SetGraphicsRootSignature(rootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { cbvsrvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);


	auto cbaddress = ObjCBResource->GetGPUVirtualAddress();
	auto matCBAdress = MaterialCBResource->GetGPUVirtualAddress();




	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());



	const float clearColor[] = { 0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f };

	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);

	commandList->SetGraphicsRootDescriptorTable(0, cbvsrvHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootShaderResourceView(2, matCBAdress);
	commandList->SetGraphicsRootConstantBufferView(1, cbaddress);

	{
		RenderGBuffer();


		commandList->SetPipelineState(FinalPSO.Get());
		cbvsrvHeap->GetGPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		commandList->SetGraphicsRootDescriptorTable(0, cbvsrvHeap->GetGPUDescriptorHandleForHeapStart());
		auto hsrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvsrvHeap->GetGPUDescriptorHandleForHeapStart(), 2, cbvsrvDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(3, hsrv);

		for (auto& t : TestTextureRenderItem)
		{
			commandList->DrawIndexedInstanced(t.IndicesCount, 1, t.StartIndexLocation, t.BaseVertexLocation, 0);
		}




	}


	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(commandList->Close());
}

void SampleCode::OnUpdate()
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



	XMFLOAT3 lightPosition = { 0.25f, 5.8f, 0.0f };
	XMVECTOR eyePosition = XMLoadFloat3(&lightPosition);
	XMVECTOR direction = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	XMVECTOR upDirection = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	XMMATRIX lightViewMatrix = XMMatrixLookToLH(eyePosition, direction, upDirection);

	//v = lightViewMatrix;


	XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	XMMATRIX rotation = XMMatrixRotationAxis({ 0.0f, 1.0f, 0.0f }, XMConvertToRadians(90.0f));
	XMMATRIX m = scale * rotation * translation;
	XMMATRIX MVP = m * v * p;

	SceneConstantBuffer passCBconstants;
	XMStoreFloat4x4(&passCBconstants.MVP, XMMatrixTranspose(MVP));
	XMStoreFloat4x4(&passCBconstants.gWorld, XMMatrixTranspose(m));

	//下面是创建光源视图矩阵
	// 为了创建一个视图矩阵来变换每个物体，把它们变换到从光源视角可见的空间中，
	// 我们将使用XMMatrixLookToRH函数；这次从光源的位置看向场景中央。
	//注意Direction不能为0，
<<<<<<< Updated upstream
	XMFLOAT3 lightPosition= { -0.0000,1.90000,0.00000 };
	XMVECTOR eyePosition = XMLoadFloat3(&lightPosition);
	XMVECTOR direction = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	XMVECTOR upDirection = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX lightViewMatrix = XMMatrixLookToRH(eyePosition, direction, upDirection);

=======

// 	XMFLOAT3 lightPosition = { 0.0000f, 2.0f, 0.0f };
// 	XMVECTOR eyePosition = XMLoadFloat3(&lightPosition);
// 	XMVECTOR direction = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
// 	XMVECTOR upDirection = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
// 	XMMATRIX lightViewMatrix = XMMatrixLookToLH(eyePosition, direction, upDirection);

	
>>>>>>> Stashed changes
	//步骤2：创建光源投影矩阵
	// 因为我们使用的是一个所有光线都平行的定向光。
	// 出于这个原因，我们将为光源使用正交投影矩阵，透视图将没有任何变形：

<<<<<<< Updated upstream
	float nearPlane = 1.0f;
	float farPlane = 100.0f;
	float Oriwidth = 10.0f;
	float Oriheight = 10.0f;

	// 步骤3：创建光照空间矩阵
	XMMATRIX lightProjectionMatrix = XMMatrixOrthographicRH(Oriwidth, Oriheight, nearPlane, farPlane);
=======
	float nearPlane =0.1f;  // 根据实际需要调整近平面的值
	float farPlane = 1000.0f;  // 根据实际需要调整远平面的值
	float aspectRatio = width / height;  // 屏幕宽高比，根据实际需要替换为正确的值
	float fovAngle = XM_PIDIV4;  // 光照投影的视野角度，根据实际需要调整
	// 步骤3：创建光照空间矩阵
	XMMATRIX lightProjectionMatrix = XMMatrixPerspectiveFovLH(0.25f * 3.1415926f, 1.0f, 1.0f, 1000.0f); 
	//XMMATRIX lightProjectionMatrix = XMMatrixPerspectiveFovLH(fovAngle, aspectRatio, nearPlane, farPlane);
>>>>>>> Stashed changes
	XMMATRIX lightSpaceMatrix = lightViewMatrix * lightProjectionMatrix;

	//二者相结合为我们提供了一个光空间的变换矩阵，
	//它将每个世界空间坐标变换到光源处所见到的那个空间；这正是我们渲染深度贴图所需要的。
	// 

	XMStoreFloat4x4(&passCBconstants.LightSpaceMatrix, XMMatrixTranspose(lightSpaceMatrix));



	passCBconstants.ViewPos = DirectX::XMFLOAT4{ camera.getPosition().x,camera.getPosition().y,camera.getPosition().z,0.0f };
	passCBconstants.Look = DirectX::XMFLOAT4{ camera.getmLook().x,camera.getmLook().y,camera.getmLook().z,0.0f };
	passCBconstants.Up = DirectX::XMFLOAT4{ camera.getmUp().x,camera.getmUp().y,camera.getmUp().z,0.0f };
	passCBconstants.Right=DirectX::XMFLOAT4{ camera.getmRight().x,camera.getmRight().y,camera.getmRight().z,0.0f };
	passCBconstants.AmbientLight = { 0.01f, 0.01f, 0.01f, 1.0f };
	passCBconstants.Lights[0].Position = { 0.25,1.8,0.00000 };
	passCBconstants.Lights[0].SpotPower = 0.8f;
	passCBconstants.Lights[0].Strength = { 1.0f,1.0f, 1.0f };


	memcpy(pCbvDataBegin, &passCBconstants, sizeof(passCBconstants));


	for (auto& t : mObjRenderItems)
	{
		ObjConstantBuffer tempobjCB;
		auto tempObjIndex = t.second.ObjCBIndex;
		tempobjCB.MaterialIndex = t.second.mat.matCBIndex;

		XMStoreFloat4x4(&tempobjCB.TexTransform, InitMatrix4X4());
		XMStoreFloat4x4(&tempobjCB.WorldTransform, InitMatrix4X4());
		memcpy(&pobjCbvDataBegin[tempObjIndex * ObjCBsize], &tempobjCB, sizeof(ObjConstantBuffer));

	}
	for (int i = 0; i < mMaterial.size(); i++)
	{
		MaterialConstants tempMat;
		tempMat.Ambient = mMaterial[i].ambient;
		tempMat.Diffuse = mMaterial[i].diffuse;
		tempMat.Specular = mMaterial[i].specular;
		XMStoreFloat4x4(&tempMat.MatTransform, InitMatrix4X4());
		memcpy(&pMatCbvDataBegin[i * matCBsize], &tempMat, sizeof(MaterialConstants));
	}




}

void SampleCode::OnRender()
{
	FinalRender();

	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(swapChain->Present(1, 0));

	WaitForPreviousFrame();
}
void SampleCode::OnDestroy()
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
		sample->OnUpdate();
		sample->OnRender();
		return 0;
	case WM_MOUSEMOVE:
		sample->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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

	sample->camera.SetPosition(-3.0f, 1.0f, 0.0f);
	sample->ObjCBsize = CalcConstantBufferByteSize<ObjConstantBuffer>();
	sample->InitD3DResource();
	sample->LoadModels("Resources/anotherCornellBox.obj");
	sample->GenerateTestTextureRenderItem(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
	//GenerateTestTextureRenderItem(0.0f, -0.5, 0.5f, 0.5f, 0.0f);
	sample->LoadAsset();

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

	sample->OnDestroy();

	return 0;
}
void SampleCode::GenerateTestTextureRenderItem(float x, float y, float w, float h, float depth)
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
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> SampleCode::GetStaticSamplers()
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
