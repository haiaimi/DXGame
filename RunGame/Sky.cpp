//***************************************************************************************
// Sky.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Sky.h"
#include "GeometryGenerator.h"
#include "Camera.h"
#include "Effects.h"

Sky::Sky(ID3D11Device* device, RunGame* mainApp, const std::wstring& cubemapFilename, float skySphereRadius)
{
	CreateDDSTextureFromFile(device, cubemapFilename.c_str(), 0, &mCubeMapSRV);
	CurScale = XMFLOAT3(1.f, 1.f, 1.f);

	CurGame = mainApp;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator geoGen;
	geoGen.CreateSphere(skySphereRadius, 30, 30, sphere);

	std::vector<XMFLOAT3> vertices(sphere.Vertices.size());

	for (size_t i = 0; i < sphere.Vertices.size(); ++i)
	{
		vertices[i] = sphere.Vertices[i].Position;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(XMFLOAT3) * vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];

	device->CreateBuffer(&vbd, &vinitData, &mVB);


	mIndexCount = sphere.Indices.size();

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(USHORT) * mIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	std::vector<USHORT> indices16;
	indices16.assign(sphere.Indices.begin(), sphere.Indices.end());

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices16[0];

	device->CreateBuffer(&ibd, &iinitData, &mIB);
}

Sky::~Sky()
{
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mCubeMapSRV);

	CurGame = nullptr;
}

ID3D11ShaderResourceView* Sky::CubeMapSRV()
{
	return mCubeMapSRV;
}

void Sky::Draw(ID3D11DeviceContext* dc, const Camera& camera)
{
	// center Sky about eye in world space
	//XMFLOAT3 eyePos = CurGame->mEyePosW;
	//XMMATRIX T = XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);
	//CurScale = XMFLOAT3(CurScale.x - DeltaTime * 0.01, CurScale.y - DeltaTime * 0.01, CurScale.z - DeltaTime * 0.01);
	//
	//XMMATRIX WVP = XMMatrixMultiply(T /* XMMatrixScaling(CurScale.x, CurScale.y, CurScale.z)*/, XMMatrixMultiply(XMLoadFloat4x4(&CurGame->mView), XMLoadFloat4x4(&CurGame->mProj)));

	XMFLOAT3 eyePos = camera.GetPosition();
	XMMATRIX T = XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);
	XMMATRIX Rotation = XMMatrixRotationRollPitchYaw(0.f, CurGame->mTimer.TotalTime()*0.005f, 0.f);
	XMMATRIX WVP = XMMatrixMultiply(T*Rotation, camera.ViewProj());

	CurGame->mfxSkyWorldViewProj->SetMatrix(reinterpret_cast<const float*>(&WVP));
	CurGame->mfxCubeMap->SetResource(mCubeMapSRV);

	UINT stride = sizeof(XMFLOAT3);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	dc->IASetIndexBuffer(mIB, DXGI_FORMAT_R16_UINT, 0);
	dc->IASetInputLayout(CurGame->mSkyInputLayout);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DX11_TECHNIQUE_DESC techDesc;
	CurGame->mSkyTech->GetDesc(&techDesc);

	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		ID3DX11EffectPass* pass = CurGame->mSkyTech->GetPassByIndex(p);

		pass->Apply(0, dc);

		dc->DrawIndexed(mIndexCount, 0, 0);
	}
}