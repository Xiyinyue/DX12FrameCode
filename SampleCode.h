#pragma once
#include"common.h"
class ShadowMapClass
{
public:
	Microsoft::WRL::ComPtr<ID3D12Resource>mSMResource;
	UINT SrvHeapindex;
	UINT DsvHeapindex;
	ID3D12Resource* ShadowMapClass::Get();
};
ID3D12Resource* ShadowMapClass::Get()
{
	return mSMResource.Get();
}




class GBufferClass
{
public:
	Microsoft::WRL::ComPtr<ID3D12Resource>GBufferResource;
	UINT SrvHeapindex;
	UINT DsvHeapindex;
	UINT rtvHeapindex;
	ID3D12Resource* GBufferClass::Get();
};
ID3D12Resource* GBufferClass::Get()
{
	return GBufferResource.Get();

}







class SampleCode
{
public:

	std::vector<Material>mMaterial;
	std::unordered_map<std::string, RenderItem>mObjRenderItems;
	std::vector<RenderItem>TestTextureRenderItem;
	//管线对象

	CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, width, height);
	CD3DX12_RECT scissorRect = CD3DX12_RECT(0, 0, width, height);
	Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvsrvHeap;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GBufferPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> FinalPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> ShadowMappingPSO;
	


	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	UINT rtvDescriptorSize;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>ObjCBResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> MaterialCBResource;
	GBufferClass mGBuffer[5];
	ShadowMapClass mShadowMap;



	UINT8* pCbvDataBegin;
	UINT8* pobjCbvDataBegin;
	UINT8* pMatCbvDataBegin;
	UINT meshIndex = 0;
	UINT ObjCBsize;
	UINT matCBsize;
	UINT mCbvSrvUavDescriptorSize = 0;
	UINT DsvHeapDescriptorSize = 0;
	UINT CurrentDsvHeapIndex = 0;
	UINT CurrentRtvHeapIndex = 0;
	UINT CurrentCbvSrvUavHeapIndex = 0;
	// 同步对象
	UINT frameIndex;
	HANDLE fenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue;

	//time
	float offset;
	bool isOffset = false;


	//
	Microsoft::WRL::ComPtr<ID3D12Resource> textureBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureBufferUploadHeap;
	UINT cbvsrvDescriptorSize;
	BYTE* imageData;
	//



	//
	POINT lastMousePos;
	Camera camera;
	//

	//
	UINT VertexOffset = 0;
	UINT IndexOffset = 0;
	//
	std::vector<Mesh> meshes;
	std::unordered_map<std::string, Mesh>mMesh;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
public:
	void OnDestroy();
	void OnRender();
	void LoadModels(const char* modelFilename);
	void InitD3DResource();
	void OnUpdate();
	void OnKeyboardInput();
	void OnMouseMove(WPARAM btnState, int x, int y);
	void FinalRender();
	void WaitForPreviousFrame();
	void BuildDescriptors();
	void RenderShadowMap();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPipelineState();
	void BuildVertexIndexBuffer();
	void BuildDepthStencilBuffer();
	void GenerateTestTextureRenderItem(float x, float y, float w, float h, float depth);
	void RenderGBuffer();
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
};

