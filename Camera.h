#pragma once
#include <DirectXMath.h>
using namespace DirectX;

class Camera
{
public:

	Camera();
	~Camera();

	void SetPosition(float x, float y, float z);
	void SetUp(float x, float y, float z);
	void SetRight(float x, float y, float z); 
	void SetLook(float x, float y, float z);

	XMFLOAT3 getPosition();
	void SetLens(float fovY, float aspect, float zn, float zf);

	XMMATRIX GetView()const;
	XMMATRIX GetProj()const;

	void Strafe(float d);
	void Walk(float d);

	void Pitch(float angle);
	void RotateY(float angle);

	void UpdateViewMatrix();
	XMFLOAT3 getmLook();
	XMFLOAT3 getmUp();
	XMFLOAT3 getmRight();
private:

	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	XMFLOAT4X4 mView =
	{
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f,
	};
	XMFLOAT4X4 mProj =
	{
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f,
	};
};
