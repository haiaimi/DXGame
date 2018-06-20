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

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }        //COM组件释放

class Sky;
class Camera;

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace XNA;

//普通顶点结构
struct Vertex
{
	Vertex() {};

	XMFLOAT3 Pos;       //顶点位置
	XMFLOAT3 Normal;    //顶点法线
	XMFLOAT2 TexCoord;  //贴图坐标

	Vertex(
		float px, float py, float pz,     //顶点
		float nx, float ny, float nz,     //法线方向
		float u, float v             //uv坐标
	) :
		Pos(px, py, pz),
		Normal(nx, ny, nz),
		TexCoord(u, v)
	{}
};

//带法线贴图和深度图的结构
struct PosNormalTexTan
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;
	XMFLOAT3 TangentU;
};


//管理障碍物信息的类
struct Obstacle
{
	XMFLOAT3 ObstaclePos;       //障碍物的当前位置
};

//玩家所在的位置，主要就三个位置
enum EPlayerMoveDir
{
	ELeft = 0,      //左
	ECenter = 1,    //中
	ERight = 2      //右
};

class RunGame
{
public:
	RunGame(HINSTANCE hInstance);
	virtual ~RunGame();

	/**渲染运行,里面执行帧循环*/
	int Run();

	/**初始化D3D设备等等*/
	bool Init();

	/**更新画面*/
	void UpdateScene(float dt);
	void DrawScene();

	void DrawScene(const Camera& inCam, bool drawCar);
	/**
	* wParam和IParam在不同的Message下有不同的作用，如 WM_SIZE下就有窗口尺寸信息
	* @Param wParam 对应32位的无符号整型数
	*/
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM IParam);

	/**创建正方体缓冲*/
	void BuildBoxBuffer();

	/**创建汽车缓冲区*/
	void BuildCarBuffer();

	/**创建地面缓冲区*/
	void BuildFloorBuffer();

	/**创建效果文件*/
	void BuildBoxFX();

	void BuildCubemapFX();

	void BuildFloorFX();

	void BuildCarFX();
	void CreateDynamicCubeMapResource();
	void BuildCubeFaceCamera(float x, float y, float z);
	/**设置输入布局*/
	void SetInputLayout();

	/**更新向上的相机角度，以模拟天空变化效果*/
	void UpdateUpCamera();

	/**创建模糊所用资源，包括顶点/索引缓冲，着色器创建，以及着色器资源创建*/
	void CreateBlurResource();

	void DrawScreenBlur();      //绘制屏幕高斯模糊效果

protected:
	bool InitMainWindow();
	bool InitDirect3D();

public:
	HINSTANCE mhAppInst;
	HWND mhMainWnd;

	GameTimer mTimer; //游戏计时器，使用了开源代码，见GameTimer

	ID3D11Device* md3dDevice;     //设备指针
	ID3D11DeviceContext* md3dImmediateContext;    //设备上下文
	IDXGISwapChain* mSwapChain;    //交换链
	ID3D11Texture2D* mDepthStencilBuffer;     //深度模板缓冲
	ID3D11RenderTargetView* mRenderTargetView;     //RenderTarget
	ID3D11DepthStencilView* mDepthStencilView;
	D3D11_VIEWPORT mScreenViewport;
	ID3DX11Effect* mFX;
	ID3DX11Effect* mSkyFX;

	ID3D11Buffer* mBoxVB;    //顶点缓冲
	ID3D11Buffer* mBoxIB;    //索引缓冲
	ID3D11Buffer* mCarVB;    //汽车顶点缓冲
	ID3D11Buffer* mCarIB;    //汽车索引缓冲

	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectTechnique* mSkyTech;   
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;   //用于Shader中的世界观察投影机矩阵
	ID3DX11EffectMatrixVariable* mfxWorld;         //世界矩阵
	ID3DX11EffectMatrixVariable* mfxWorldInvTranspose;     //用于法线转换
	ID3DX11EffectVectorVariable* mfxEyePosW;          //摄像机所在位置
	ID3DX11EffectMatrixVariable* mfxTexTransform;      //纹理变换，如要让纹理旋转
	ID3DX11EffectMatrixVariable* mfxSkyWorldViewProj;
	ID3DX11EffectShaderResourceVariable* mfxDiffuseMap;   //纹理资源
	ID3DX11EffectShaderResourceVariable* mfxCubeMap;        //天空盒子纹理
	ID3DX11EffectVariable* mfxDirLight;
	ID3DX11EffectVariable* mfxPointLight;
	ID3DX11EffectVariable* mfxSpotLight;
	ID3DX11EffectVariable* mfxMaterial;
	
	ID3D11ShaderResourceView* mliftSRV;
	ID3D11ShaderResourceView* mWallSRV;

	ID3D11ShaderResourceView* mCubeMpSRV;       //天空盒子对应的资源

	ID3D11InputLayout* mInputLayout;       //输入布局
	ID3D11InputLayout* mSkyInputLayout;     //天空盒子输入布局
	ID3D11InputLayout* mCarInputLayout;      //汽车输入布局

	//下面是汽车所对应的资源  

	ID3DX11Effect* mCarFX;        //汽车效果文件
	ID3DX11EffectTechnique* mCarTech;       //汽车着色器对应的Tech

	ID3DX11EffectVariable* mCarfxDirLight;
	ID3DX11EffectVariable* mCarfxMaterial;   //汽车材质
	ID3DX11EffectVectorVariable* mCarfxEyePosW;
	ID3DX11EffectMatrixVariable* mCarfxWorldViewProj;   //用于Shader中的世界观察投影机矩阵
	ID3DX11EffectMatrixVariable* mCarfxWorld;         //世界矩阵
	ID3DX11EffectMatrixVariable* mCarfxWorldInvTranspose;     //用于法线转换
	ID3DX11EffectMatrixVariable* mCarfxTexTransform;
	ID3DX11EffectShaderResourceVariable* mCarfxCubeMap;

	ID3D11DepthStencilView* mDynamicCubeMapDSV;      //深度缓冲区
	ID3D11RenderTargetView* mDynamicCubeMapRTV[6] = { nullptr };   //6个RenderTarget，每个对应一个面
	ID3D11ShaderResourceView* mDynamicCubeMapSRV;
	D3D11_VIEWPORT mCubeMapViewport;     //RenderTarget视口大小

	static const int CubeMapSize = 512;       //默认512像素
	DirectionalLight mCarDirLights[3];
	Camera mCubeMapCamera[6];      //6个不同方向的相机

	//下面是地面所对应的资源
	ID3D11Buffer* mFloorVB;     //顶点
	ID3D11Buffer* mFloorIB;     //索引
	ID3D11InputLayout* mFloorInputLayout;     //地面模型输入布局
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
	ID3D11ShaderResourceView* mFloorNormalTexSRV;     //法线贴图
	ID3D11ShaderResourceView* mFloorTexSRV;           //纹理贴图
	int mFloorIndexCount;       //索引数目
	Material mFloorMat;

	//下面是高斯模糊对应的资源
	ID3D11Buffer* mBlurVB;
	ID3D11Buffer* mBlurIB;
	ID3DX11Effect* mBlurFX;
	ID3DX11EffectTechnique* mHorzBlurTech;    //横向滤波
	ID3DX11EffectTechnique* mVertBlurTech;     //纵向滤波
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
	Material mCarMat;     //汽车材质

	Camera mCam;
	Sky* mSky;
	BlurFilter mBlurFilter;
	std::wstring mWndCaption;
	D3D_DRIVER_TYPE md3dDriveType;   //驱动类型，通常电脑会有显卡，所以是硬件驱动
	UINT m4xMsaaQuality;        //抗锯齿等级
	int mClientWidth;
	int mClientHeight;
	int mCarIndexCount;

private:
	/**当前向上向量旋转角度*/
	float mCurRotateAngle;


	//下面是游戏逻辑部分的变量
	bool bInMove;
	bool bInJump;
	EPlayerMoveDir CurrentPos;       //玩家当前所在的位置
	EPlayerMoveDir NextPos;          //玩家移向的下一个目标位置
	float CurPosX;   //玩家X轴方向的位置，注意这是相对位置
	float NextPosX;  //玩家X轴目标位置，对应于上面的NextPos 

	float CurHeight;
	float DestHeight;
	int keyInputCount = 0;

	std::vector<XMFLOAT3> ObstaclesInfo;        //存储障碍物信息的容器
	float SpawnObstacleDistance;          //这是每生成一个障碍物的相隔距离
	float floorSpeed;
	float CarMoveDistancePerFrame;
	float floorMovePerFrame;

	//下面是汽车的碰撞包围，（轴对齐）
	AxisAlignedBox carCollision;
	AxisAlignedBox obstacleCollision;

	int blockNum = 0;     //碰撞次数，用于测试

	bool quitGame;
	bool bRestart;    //是否重启游戏
	bool bDrawBlur;    //是否绘制高斯模糊效果（游戏结束时显示）

	float mPlayerScore;    //玩家得分
};


