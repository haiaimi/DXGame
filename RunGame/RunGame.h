#pragma once

#include "d3d11.h"
#include "DirectXMath.h"
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <DirectXPackedVector.h>
#include "GameTimer.h"
#include "d3dx11effect.h"
#include "LightHelper.h"
#include "Sky.h"
#include "Camera.h"
#include "xnacollision.h"
#include "BlurFilter.h"

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }        //COM����ͷ�

class Sky;
class Camera;

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace XNA;

//��ͨ����ṹ
struct Vertex
{
	Vertex() {};

	XMFLOAT3 Pos;       //����λ��
	XMFLOAT3 Normal;    //���㷨��
	XMFLOAT2 TexCoord;  //��ͼ����

	Vertex(
		float px, float py, float pz,     //����
		float nx, float ny, float nz,     //���߷���
		float u, float v             //uv����
	) :
		Pos(px, py, pz),
		Normal(nx, ny, nz),
		TexCoord(u, v)
	{}
};

//��������ͼ�����ͼ�Ľṹ
struct PosNormalTexTan
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;
	XMFLOAT3 TangentU;
};


//�����ϰ�����Ϣ����
struct Obstacle
{
	XMFLOAT3 ObstaclePos;       //�ϰ���ĵ�ǰλ��
};

//������ڵ�λ�ã���Ҫ������λ��
enum EPlayerMoveDir
{
	ELeft = 0,      //��
	ECenter = 1,    //��
	ERight = 2      //��
};

class RunGame
{
public:
	RunGame(HINSTANCE hInstance);
	virtual ~RunGame();

	/**��Ⱦ����,����ִ��֡ѭ��*/
	int Run();

	/**��ʼ��D3D�豸�ȵ�*/
	bool Init();

	/**���»���*/
	void UpdateScene(float dt);
	void DrawScene();

	void DrawScene(const Camera& inCam, bool drawCar);
	/**
	* wParam��IParam�ڲ�ͬ��Message���в�ͬ�����ã��� WM_SIZE�¾��д��ڳߴ���Ϣ
	* @Param wParam ��Ӧ32λ���޷���������
	*/
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM IParam);

	/**���������建��*/
	void BuildBoxBuffer();

	/**��������������*/
	void BuildCarBuffer();

	/**�������滺����*/
	void BuildFloorBuffer();

	/**����Ч���ļ�*/
	void BuildBoxFX();

	void BuildCubemapFX();

	void BuildFloorFX();

	void BuildCarFX();
	void CreateDynamicCubeMapResource();
	void BuildCubeFaceCamera(float x, float y, float z);
	/**�������벼��*/
	void SetInputLayout();

	/**�������ϵ�����Ƕȣ���ģ����ձ仯Ч��*/
	void UpdateUpCamera();

	/**����ģ��������Դ����������/�������壬��ɫ���������Լ���ɫ����Դ����*/
	void CreateBlurResource();

	void DrawScreenBlur();      //������Ļ��˹ģ��Ч��

protected:
	bool InitMainWindow();
	bool InitDirect3D();

public:
	HINSTANCE mhAppInst;
	HWND mhMainWnd;

	GameTimer mTimer; //��Ϸ��ʱ����ʹ���˿�Դ���룬��GameTimer

	ID3D11Device* md3dDevice;     //�豸ָ��
	ID3D11DeviceContext* md3dImmediateContext;    //�豸������
	IDXGISwapChain* mSwapChain;    //������
	ID3D11Texture2D* mDepthStencilBuffer;     //���ģ�建��
	ID3D11RenderTargetView* mRenderTargetView;     //RenderTarget
	ID3D11DepthStencilView* mDepthStencilView;
	D3D11_VIEWPORT mScreenViewport;
	ID3DX11Effect* mFX;
	ID3DX11Effect* mSkyFX;

	ID3D11Buffer* mBoxVB;    //���㻺��
	ID3D11Buffer* mBoxIB;    //��������
	ID3D11Buffer* mCarVB;    //�������㻺��
	ID3D11Buffer* mCarIB;    //������������

	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectTechnique* mSkyTech;   
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;   //����Shader�е�����۲�ͶӰ������
	ID3DX11EffectMatrixVariable* mfxWorld;         //�������
	ID3DX11EffectMatrixVariable* mfxWorldInvTranspose;     //���ڷ���ת��
	ID3DX11EffectVectorVariable* mfxEyePosW;          //���������λ��
	ID3DX11EffectMatrixVariable* mfxTexTransform;      //����任����Ҫ��������ת
	ID3DX11EffectMatrixVariable* mfxSkyWorldViewProj;
	ID3DX11EffectShaderResourceVariable* mfxDiffuseMap;   //������Դ
	ID3DX11EffectShaderResourceVariable* mfxCubeMap;        //��պ�������
	ID3DX11EffectVariable* mfxDirLight;
	ID3DX11EffectVariable* mfxPointLight;
	ID3DX11EffectVariable* mfxSpotLight;
	ID3DX11EffectVariable* mfxMaterial;
	
	ID3D11ShaderResourceView* mliftSRV;
	ID3D11ShaderResourceView* mWallSRV;

	ID3D11ShaderResourceView* mCubeMpSRV;       //��պ��Ӷ�Ӧ����Դ

	ID3D11InputLayout* mInputLayout;       //���벼��
	ID3D11InputLayout* mSkyInputLayout;     //��պ������벼��
	ID3D11InputLayout* mCarInputLayout;      //�������벼��

	//��������������Ӧ����Դ  

	ID3DX11Effect* mCarFX;        //����Ч���ļ�
	ID3DX11EffectTechnique* mCarTech;       //������ɫ����Ӧ��Tech

	ID3DX11EffectVariable* mCarfxDirLight;
	ID3DX11EffectVariable* mCarfxMaterial;   //��������
	ID3DX11EffectVectorVariable* mCarfxEyePosW;
	ID3DX11EffectMatrixVariable* mCarfxWorldViewProj;   //����Shader�е�����۲�ͶӰ������
	ID3DX11EffectMatrixVariable* mCarfxWorld;         //�������
	ID3DX11EffectMatrixVariable* mCarfxWorldInvTranspose;     //���ڷ���ת��
	ID3DX11EffectMatrixVariable* mCarfxTexTransform;
	ID3DX11EffectShaderResourceVariable* mCarfxCubeMap;

	ID3D11DepthStencilView* mDynamicCubeMapDSV;      //��Ȼ�����
	ID3D11RenderTargetView* mDynamicCubeMapRTV[6] = { nullptr };   //6��RenderTarget��ÿ����Ӧһ����
	ID3D11ShaderResourceView* mDynamicCubeMapSRV;
	D3D11_VIEWPORT mCubeMapViewport;     //RenderTarget�ӿڴ�С

	static const int CubeMapSize = 512;       //Ĭ��512����
	DirectionalLight mCarDirLights[3];
	Camera mCubeMapCamera[6];      //6����ͬ��������

	//�����ǵ�������Ӧ����Դ
	ID3D11Buffer* mFloorVB;     //����
	ID3D11Buffer* mFloorIB;     //����
	ID3D11InputLayout* mFloorInputLayout;     //����ģ�����벼��
	ID3DX11Effect* mFloorFX;
	ID3DX11EffectTechnique* mFloorTech;
	ID3DX11EffectMatrixVariable* mFloorfxViewProj;
	ID3DX11EffectMatrixVariable* mFloorfxWorldViewPoj;
	ID3DX11EffectMatrixVariable* mFloorfxWorld;
	ID3DX11EffectMatrixVariable* mFloorfxWorldInvTranspose;
	ID3DX11EffectMatrixVariable* mFloorfxTexTransform;
	ID3DX11EffectVectorVariable* mFloorfxEyePosW;
	ID3DX11EffectVariable* mFloorfxDirLights;
	ID3DX11EffectVariable* mFloorfxMat;
	ID3DX11EffectScalarVariable* mFloorfxHeightScale;
	ID3DX11EffectScalarVariable* mFloorfxMaxTessDistance;
	ID3DX11EffectScalarVariable* mFloorfxMinTessDistance;
	ID3DX11EffectScalarVariable* mFloorfxMaxTessFactor;
	ID3DX11EffectScalarVariable* mFloorfxMinTessFactor;

	ID3DX11EffectShaderResourceVariable* mFloorfxDiffuseMap;
	ID3DX11EffectShaderResourceVariable* mFloorfxNormalMap;
	ID3D11ShaderResourceView* mFloorNormalTexSRV;     //������ͼ
	ID3D11ShaderResourceView* mFloorTexSRV;           //������ͼ
	int mFloorIndexCount;       //������Ŀ
	Material mFloorMat;

	//�����Ǹ�˹ģ����Ӧ����Դ
	ID3D11Buffer* mBlurVB;
	ID3D11Buffer* mBlurIB;
	ID3DX11Effect* mBlurFX;
	ID3DX11EffectTechnique* mHorzBlurTech;    //�����˲�
	ID3DX11EffectTechnique* mVertBlurTech;     //�����˲�
	ID3DX11EffectTechnique* mBlurScreenTech;
	ID3DX11EffectShaderResourceVariable* mfxBlurInputMap;
	ID3DX11EffectUnorderedAccessViewVariable* mfxBlurOutputMap;
	ID3D11ShaderResourceView* mBlurSRV; 
	ID3D11UnorderedAccessView* mBlurUAV;
	ID3D11RenderTargetView* mBlurRTV;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;
	XMFLOAT4X4 mTexTransform;
	XMFLOAT3 mEyePosW;

	DirectionalLight mDirLight;
	PointLight mPointLight;
	SpotLight mSpotLight;
	Material mBoxMat;
	Material mCarMat;     //��������

	Camera mCam;
	Sky* mSky;
	BlurFilter mBlurFilter;
	std::wstring mWndCaption;
	D3D_DRIVER_TYPE md3dDriveType;   //�������ͣ�ͨ�����Ի����Կ���������Ӳ������
	UINT m4xMsaaQuality;        //����ݵȼ�
	int mClientWidth;
	int mClientHeight;
	int mCarIndexCount;

private:
	/**��ǰ����������ת�Ƕ�*/
	float mCurRotateAngle;


	//��������Ϸ�߼����ֵı���
	bool bInMove;
	bool bInJump;
	EPlayerMoveDir CurrentPos;       //��ҵ�ǰ���ڵ�λ��
	EPlayerMoveDir NextPos;          //����������һ��Ŀ��λ��
	float CurPosX;   //���X�᷽���λ�ã�ע���������λ��
	float NextPosX;  //���X��Ŀ��λ�ã���Ӧ�������NextPos 

	float CurHeight;
	float DestHeight;
	int keyInputCount = 0;

	std::vector<XMFLOAT3> ObstaclesInfo;        //�洢�ϰ�����Ϣ������
	float SpawnObstacleDistance;          //����ÿ����һ���ϰ�����������
	float floorSpeed;
	float CarMoveDistancePerFrame;
	float floorMovePerFrame;

	//��������������ײ��Χ��������룩
	AxisAlignedBox carCollision;
	AxisAlignedBox obstacleCollision;

	int blockNum = 0;     //��ײ���������ڲ���

	bool quitGame;
	bool bRestart;    //�Ƿ�������Ϸ
	bool bDrawBlur;    //�Ƿ���Ƹ�˹ģ��Ч������Ϸ����ʱ��ʾ��

	float mPlayerScore;    //��ҵ÷�
};


