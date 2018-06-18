#include "RunGame.h"
#include <d3d11.h>
#include "MathHelper.h"
#include "DDSTextureLoader.h"
#include <stdlib.h>
#include "GeometryGenerator.h"
#include "GameMath.h"

RunGame* gRunGameApp = nullptr;

//回调函数，用于窗口初始化
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM IParam)
{
	return gRunGameApp->MsgProc(hwnd, msg, wParam, IParam);
}

RunGame::RunGame(HINSTANCE hInstance) :
	mhAppInst(hInstance),
	mWndCaption(L"跑酷游戏"),
	md3dDriveType(D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE),
	mClientHeight(800),
	mClientWidth(1200),
	mhMainWnd(0),
	mCarIndexCount(0),

	md3dDevice(nullptr),
	md3dImmediateContext(nullptr),
	mSwapChain(nullptr),
	mDepthStencilBuffer(nullptr),
	mRenderTargetView(nullptr),
	mDepthStencilView(nullptr),
	mBoxIB(nullptr),
	mBoxVB(nullptr),
	mCarIB(nullptr),
	mCarVB(nullptr),
	mFX(nullptr),
	mSkyFX(nullptr),
	mTech(nullptr),
	mSkyTech(nullptr),
	mInputLayout(nullptr),
	mEyePosW(0.f, 0.f, 0.f),
	mliftSRV(nullptr),
	mWallSRV(nullptr),
	mfxDiffuseMap(nullptr),
	mCubeMpSRV(nullptr),
	mfxTexTransform(nullptr),
	mCurRotateAngle(0.f),
	mFloorIB(nullptr),
	mFloorVB(nullptr),
	mFloorInputLayout(nullptr),
	mFloorTech(nullptr),
	mFloorFX(nullptr),
	mFloorTexSRV(nullptr),
	mFloorNormalTexSRV(nullptr)
{
	//初始化Viewport结构
	memset(&mScreenViewport, 0, sizeof(D3D11_VIEWPORT));
	memset(&mCubeMapViewport, 0, sizeof(D3D11_VIEWPORT));
	gRunGameApp = this;

	CurrentPos = EPlayerMoveDir::ECenter;
	NextPos = EPlayerMoveDir::ECenter;
	CurPosX = 0.f;    //玩家初始X相对位置为0
	NextPosX = 10.f;
	CurHeight = 0.f;
	DestHeight = 0.f;

	//初始化世界、观察、投影矩阵
	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);
	XMStoreFloat4x4(&mTexTransform, I);

	bInMove = false;
	bInJump = false;
	floorSpeed = 2.1f;
	ObstaclesInfo.clear();
	SpawnObstacleDistance =50.f;
	CarMoveDistancePerFrame = 0.f;
}

RunGame::~RunGame()
{
	//析构函数，清理内存，以免造成内存泄漏
	ReleaseCOM(mSwapChain);
	ReleaseCOM(mDepthStencilBuffer);
	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);

	if (md3dImmediateContext)
		md3dImmediateContext->ClearState();

	ReleaseCOM(md3dDevice);
	ReleaseCOM(md3dImmediateContext);
	ReleaseCOM(mFX);
	ReleaseCOM(mSkyFX);
	ReleaseCOM(mCarFX);
	//ReleaseCOM(mTech);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mCarIB);
	ReleaseCOM(mCarVB);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mCarInputLayout);
	ReleaseCOM(mliftSRV);
	ReleaseCOM(mWallSRV);
	ReleaseCOM(mCubeMpSRV);
	ReleaseCOM(mDynamicCubeMapDSV);
	ReleaseCOM(mDynamicCubeMapSRV);
	ReleaseCOM(mFloorFX);
	ReleaseCOM(mFloorVB);
	ReleaseCOM(mFloorIB);
	ReleaseCOM(mFloorInputLayout);
	ReleaseCOM(mFloorNormalTexSRV);
	ReleaseCOM(mFloorTexSRV);
	for (int i = 0; i < 6; i++)
	{
		ReleaseCOM(mDynamicCubeMapRTV[i]);
	}

	delete mSky;
}

//游戏运行期，就是游戏主循环
int RunGame::Run()
{
	MSG msg{ 0 };

	mTimer.Reset();

	//游戏主循环
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			mTimer.Tick();
			UpdateScene(mTimer.DeltaTime());
			DrawScene();
		}
	}

	return (int)msg.wParam;
}

bool RunGame::Init()
{
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	mCarDirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCarDirLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mCarDirLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mCarDirLights[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	mCarDirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mCarDirLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	mCarDirLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	mCarDirLights[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	mCarDirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mCarDirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCarDirLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mCarDirLights[2].Direction = XMFLOAT3(0.0f, 0.707f, 0.707f);

	// 直射光
	mDirLight.Ambient = XMFLOAT4(0.5f, 0.5f, 0.8f, 1.0f);
	mDirLight.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mDirLight.Specular = XMFLOAT4(0.4f, 0.1f, 0.f, 1.0f);
	mDirLight.Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	// 点光源
	mPointLight.Ambient = XMFLOAT4(0.f, 1.f, 0.f, 1.0f);
	mPointLight.Diffuse = XMFLOAT4(0.f, 1.f, 0.f, 1.0f);
	mPointLight.Specular = XMFLOAT4(0.f, 1.f, 0.f, 1.0f);
	mPointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mPointLight.Range = 25.0f;

	// 聚光灯（类似手电筒）
	mSpotLight.Ambient = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	mSpotLight.Diffuse = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	mSpotLight.Specular = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	mSpotLight.Att = XMFLOAT3(0.5f, 0.0f, 0.0f);
	mSpotLight.Spot = 96.0f;
	mSpotLight.Range = 10000.0f;

	//方块的材质
	mBoxMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	//汽车材质
	mCarMat.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCarMat.Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCarMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	mCarMat.Reflect = XMFLOAT4(1.f, 1.f, 1.f, 1.0f);

	//地面材质
	mFloorMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mFloorMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mFloorMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
	mFloorMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	mSky = new Sky(md3dDevice, this, L"Texture/grasscube1024.dds", 5000.0f);

	//创建纹理资源，这里直接使用DDS，为了提高利率，微软已经开源该库，见DDSTextureLoader.h
	DirectX::CreateDDSTextureFromFile(md3dDevice, L"Texture/checkboard.dds", nullptr, &mliftSRV);
	DirectX::CreateDDSTextureFromFile(md3dDevice, L"Texture/brick01.dds", nullptr, &mWallSRV);
	DirectX::CreateDDSTextureFromFile(md3dDevice, L"Texture/floor.dds", nullptr, &mFloorTexSRV);
	DirectX::CreateDDSTextureFromFile(md3dDevice, L"Texture/floor_nmap.dds", nullptr, &mFloorNormalTexSRV);

	BuildBoxBuffer();      //创建方块顶点、索引缓冲
	BuildCarBuffer();
	BuildFloorBuffer();
	BuildBoxFX();       //创建着色器
	BuildCubemapFX();
	BuildCarFX();
	BuildFloorFX();
	SetInputLayout();     //创建输入布局

	CreateDynamicCubeMapResource();

	BuildCubeFaceCamera(0.f, 2.f, 0.f);

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, static_cast<float>(mClientWidth / mClientHeight), 1.0f, 1000.0f);       //创建投影矩阵
	XMStoreFloat4x4(&mProj, P);

	mEyePosW = XMFLOAT3(0.f, 10.f, -30.f);
	XMVECTOR pos = XMVectorSet(0.f, 10.f, -30.f, 1.f);
	XMVECTOR target = XMVectorSet(0.f, 5.f, 0.f, 1.f);
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	mCam.LookAt(pos, target, up);
	mCam.SetLens(0.25f*MathHelper::Pi, static_cast<float>(mClientWidth / mClientHeight), 1.0f, 1000.0f);
	mCam.UpdateViewMatrix();

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);      //创建观察矩阵
	XMStoreFloat4x4(&mView, V);       //获取观察矩阵
 //初始化成功
	return true;
}

LRESULT RunGame::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		//窗口激活
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mTimer.Stop();
		}
		else
		{
			mTimer.Start();
		}
		return 0;

		//关闭窗口
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}


	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool RunGame::InitMainWindow()
{
	//初始化窗口类
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"GameWndClassName";

	//注册窗口
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"窗口注册失败", 0, 0);
		return false;
	}

	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	//创建窗口
	mhMainWnd = CreateWindow(L"GameWndClassName", mWndCaption.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);

	if (!mhMainWnd)
	{
		MessageBox(0, L"创建窗口失败", 0, 0);
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);
	SetWindowText(mhMainWnd, L" ");       //显示操作方法

	return true;
}

bool RunGame::InitDirect3D()
{
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
		nullptr,
		md3dDriveType,
		0,
		0,
		0,
		0,
		D3D11_SDK_VERSION,
		&md3dDevice,
		&featureLevel,
		&md3dImmediateContext);

		if (FAILED(hr))
		{
			MessageBox(0, L"D3D设备创建失败", 0, 0);
			return false;
		}

		if (featureLevel != D3D_FEATURE_LEVEL_11_0)
		{
			MessageBox(0, L"该设备不支持D3D11特性", 0, 0);
			return false;
		}

		md3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality);   //获取该设备支持多重采样等级

																									 //交换链初始化结构
		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = mClientWidth;
		sd.BufferDesc.Height = mClientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 60; //刷新率
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;       //缓冲区像素格式
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = 4;   //使用多重采样抗锯齿，不然效果太差
		sd.SampleDesc.Quality = m4xMsaaQuality - 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    //用于渲染目标
		sd.BufferCount = 1;        //缓冲区数目，正常指定一个
		sd.OutputWindow = mhMainWnd;    //指定输出窗口
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;

		//下面是就是开始创建交换链
		IDXGIDevice* dxgiDevice = 0;
		md3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

		IDXGIAdapter* dxgiAdapter = 0;
		dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

		IDXGIFactory* dxgiFactory = 0;
		dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

		dxgiFactory->CreateSwapChain(md3dDevice, &sd, &mSwapChain);   //创建交换链

		ReleaseCOM(dxgiDevice);
		ReleaseCOM(dxgiAdapter);
		ReleaseCOM(dxgiFactory);

		ID3D11Texture2D* backBuffer;       //缓冲区
		mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
		md3dDevice->CreateRenderTargetView(backBuffer, nullptr, &mRenderTargetView);   //创建RenderTarget
		ReleaseCOM(backBuffer);

		D3D11_TEXTURE2D_DESC depthStencilDesc;

		depthStencilDesc.Width = mClientWidth;
		depthStencilDesc.Height = mClientHeight;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		depthStencilDesc.SampleDesc.Count = 4;
		depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality - 1;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		//先创建Texture2D缓冲区，然后把深度模板缓冲绑定到上面
		md3dDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer);
		md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, 0, &mDepthStencilView);

		//绑定RenderTarget和深度模板缓冲到渲染管道


		//设置视口
		mScreenViewport.TopLeftX = 0;
		mScreenViewport.TopLeftY = 0;
		mScreenViewport.Width = static_cast<float>(mClientWidth);
		mScreenViewport.Height = static_cast<float>(mClientHeight);
		mScreenViewport.MinDepth = 0.0f;
		mScreenViewport.MaxDepth = 1.0f;

		return true;
}

void RunGame::UpdateScene(float dt)
{
	mSky->DeltaTime = dt;

	EPlayerMoveDir Temp;

	if (!bInMove)
		Temp = CurrentPos;
	else
		Temp = NextPos;

	BYTE KeyStatus[256] = { 0 };
	GetKeyboardState(KeyStatus);

	
		//以此判断两个键是否被按下
		if (KeyStatus['A'] == 1)
		{
			keyInputCount++;
			bInMove = true;
			if ((Temp - 1) < 0)
				NextPos = Temp;
			else
				NextPos = EPlayerMoveDir(Temp - 1);
		}

		if (KeyStatus['D'] == 1)
		{

			bInMove = true;
			if ((Temp + 1) > 2)
				NextPos = Temp;
			else
				NextPos = EPlayerMoveDir(Temp + 1);
		}
	

	if (KeyStatus[VK_SPACE] == 1)
	{
		bInJump = true;
		DestHeight = 10.f;
	}

	//恢复这两个键的状态
	KeyStatus['A'] = -1;
	KeyStatus['D'] = -1;
	KeyStatus[VK_SPACE] = -1;
	SetKeyboardState(KeyStatus);

	switch (NextPos)
	{
	case ELeft:
		NextPosX = 10.f;
		break;

	case ECenter:
		NextPosX = 0.f;
		break;

	case ERight:
		NextPosX = -10.f;
		break;
	}

	int overDistance = rand() % 10;

	if (SpawnObstacleDistance >= 30.f + overDistance)     //此时可以加入新的障碍物
	{
		SpawnObstacleDistance = 0.f;
		EPlayerMoveDir i = EPlayerMoveDir(rand() % 3);
		XMFLOAT3 SpawnLoc(0.f, 0.f, 100.f);

		switch (i)
		{
			case ELeft:
				SpawnLoc.x = -10.f;
				break;

			case ECenter:
				SpawnLoc.x = 0.f;
				break;

			case ERight:
				SpawnLoc.x = 10.f;
				break;
		}

		ObstaclesInfo.push_back(SpawnLoc);
	}

	//if (mTimer.TotalTime() > 10.f)
		floorSpeed += 0.01f * mTimer.DeltaTime();
}

void RunGame::DrawScene()
{
	ID3D11RenderTargetView* renderTargets[1];

	static float PreFloorSpeed = 2.f;
	static float PreTotalTime = 0.f;

	floorMovePerFrame = -mTimer.TotalTime()*floorSpeed;
	SpawnObstacleDistance = SpawnObstacleDistance + (100.f / 10.f)*(-floorMovePerFrame - PreTotalTime * PreFloorSpeed);      //这步十分重要，不能直接与floorSpeed，要考虑到材质偏移和TotalTime也有关系
	CarMoveDistancePerFrame = (100.f / 10.f)*(-floorMovePerFrame - PreTotalTime * PreFloorSpeed);

	PreFloorSpeed = floorSpeed;
	PreTotalTime = mTimer.TotalTime();
	

	// 下面使用特定的Viewport来渲染6个面到RenderTarget上（使用6个不同的相机）
	md3dImmediateContext->RSSetViewports(1, &mCubeMapViewport);
	for (int i = 0; i < 6; ++i)
	{
		//下面的渲染不直接显示在屏幕上
		md3dImmediateContext->ClearRenderTargetView(mDynamicCubeMapRTV[i], reinterpret_cast<const float*>(&Colors::White));
		md3dImmediateContext->ClearDepthStencilView(mDynamicCubeMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		renderTargets[0] = mDynamicCubeMapRTV[i];
		md3dImmediateContext->OMSetRenderTargets(1, renderTargets, mDynamicCubeMapDSV);     //设置新的RenderTarget
																							//渲染到当前RenderTarget
		DrawScene(mCubeMapCamera[i], false);
	}

	md3dImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
	md3dImmediateContext->RSSetViewports(1, &mScreenViewport);
	md3dImmediateContext->GenerateMips(mDynamicCubeMapSRV);    //生成层次图

	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);      //清理RenderTarget和深度模板缓冲进行绘制下一帧

	//md3dImmediateContext->IASetInputLayout(mInputLayout);
	//md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   //绘制的基本图元为三角形

	//UINT stride = sizeof(Vertex);   //顶点结构的大小
	//UINT offset = 0;
	//md3dImmediateContext->IASetVertexBuffers(0, 1, &mCarVB, &stride, &offset);    //设置顶点缓冲
	//md3dImmediateContext->IASetIndexBuffer(mCarIB, DXGI_FORMAT_R32_UINT, 0);       //设置索引缓冲

	//XMMATRIX world = XMLoadFloat4x4(&mWorld);
	//XMMATRIX view = XMLoadFloat4x4(&mView);
	//XMMATRIX proj = XMLoadFloat4x4(&mProj);

	////为效果文件缓冲区变量设置参数,这里是三个光源的参数
	//mfxDirLight->SetRawValue(&mDirLight, 0, sizeof(mDirLight));
	//mfxPointLight->SetRawValue(&mPointLight, 0, sizeof(mPointLight));
	//mfxSpotLight->SetRawValue(&mSpotLight, 0, sizeof(mSpotLight));

	////下面就是渲染部分
	//D3DX11_TECHNIQUE_DESC techDesc;
	//mTech->GetDesc(&techDesc);
	//for (UINT p = 0; p < techDesc.Passes; ++p)
	//{
	//	///绘制电梯部分
	//	XMMATRIX scale = XMMatrixScaling(10.f, 2.f, 10.f);      //方块尺寸
	//	mfxEyePosW->SetRawValue(&mEyePosW, 0, sizeof(XMFLOAT3));
	//	XMMATRIX offset = XMMatrixTranslation(-0.1f, liftHeight - 19.999f, 0.f);
	//	XMMATRIX worldViewProj = world * scale * offset * view * proj;     //获取当前世界观察矩阵， 这里一定要注意相乘的顺序，scale一定要在offset前面
	//	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world*scale);   //保持网格因为Scale变化造成发现异常

	//	//同样是设置着色器里的缓冲区参数
	//	//mfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
	//	//mfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
	//	//mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
	//	//mfxMaterial->SetRawValue(&mBoxMat, 0, sizeof(mBoxMat));
	//	//mfxTexTransform->SetMatrix(reinterpret_cast<const float*>(&XMMatrixScaling(2.f, 2.f, 0.f)));   //调整纹理大小，以免导致拉伸
	//	//mfxDiffuseMap->SetResource(mliftSRV);  //设置纹理

	//	//mfxEyePosW->SetRawValue(&mEyePosW, 0, sizeof(mEyePosW));
	//	//XMMATRIX worldViewProj = world * view * proj;     //获取当前世界观察矩阵， 这里一定要注意相乘的顺序，scale一定要在offset前面
	//	//XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);   //保持网格因为Scale变化造成发现异常

	//	XMMATRIX CarRotation = XMMatrixRotationRollPitchYaw(0.f, 179.1f, 0.f);
	//	worldViewProj = world * CarRotation * view * proj;
	//	mfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
	//	mfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
	//	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
	//	mfxMaterial->SetRawValue(&mBoxMat, 0, sizeof(mBoxMat));
	//	mfxDiffuseMap->SetResource(mliftSRV);  //设置纹理

	//	mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);    //这里一定要Apply这样才能绘制新的图形
	//	md3dImmediateContext->DrawIndexed(mCarIndexCount, 0, 0);
	//}

	//mSky->Draw(md3dImmediateContext, mCam);
	
	DrawScene(mCam, true);

	md3dImmediateContext->RSSetState(0);
	md3dImmediateContext->OMSetDepthStencilState(0, 0);
	mSwapChain->Present(0, 0);     //显示BackBuffer
}


void RunGame::DrawScene(const Camera& inCam, bool drawCar)
{
	//绘制天空盒子
	UpdateUpCamera();     //更新摄像机
	mSky->Draw(md3dImmediateContext, inCam);

	md3dImmediateContext->RSSetState(0);
	md3dImmediateContext->OMSetDepthStencilState(0, 0);

	//绘制地板
	UINT stride = sizeof(PosNormalTexTan);
	UINT offset = 0;
	XMMATRIX scale = XMMatrixScaling(1.f, 1.f, 1.f);      //尺寸
	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view = inCam.View();
	XMMATRIX proj = inCam.Proj();
	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world*scale);   //保持网格因为Scale变化造成发现异常
	XMMATRIX worldViewProj;    //获取当前世界观察矩阵， 这里一定要注意相乘的顺序，scale一定要在offset前面
	XMMATRIX TexTransform = XMMatrixScaling(7.f, 10.f, 1.f);
	XMMATRIX FloorOffset = XMMatrixTranslation(0.f, -2.5f, 40.f);
	XMMATRIX TexOffset = XMMatrixTranslation(0.f, floorMovePerFrame, 0.f);
	TexTransform = TexTransform * TexOffset;

	md3dImmediateContext->IASetInputLayout(mFloorInputLayout);
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mFloorVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mFloorIB, DXGI_FORMAT_R32_UINT, 0);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

	mFloorfxDirLights->SetRawValue(&mCarDirLights, 0, 3 * sizeof(DirectionalLight));
	mFloorfxMat->SetRawValue(&mFloorMat, 0, sizeof(Material));
	mFloorfxHeightScale->SetFloat(0.06f);
	mFloorfxMaxTessDistance->SetFloat(25.f);
	mFloorfxMinTessDistance->SetFloat(1.f);
	mFloorfxMaxTessFactor->SetFloat(40.f);
	mFloorfxMinTessFactor->SetFloat(1.f);

	world = world * FloorOffset;

	D3DX11_TECHNIQUE_DESC techDesc, techDesc1, techDesc2;
	mFloorTech->GetDesc(&techDesc);
	for (UINT i = 0; i < techDesc.Passes; ++i)
	{
		worldViewProj = world * view * proj;
		mFloorfxEyePosW->SetRawValue(&inCam.GetPosition(), 0, sizeof(XMFLOAT3));
		mFloorfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
		mFloorfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
		mFloorfxViewProj->SetMatrix(reinterpret_cast<float*>(&(view*proj)));
		mFloorfxWorldViewPoj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mFloorfxTexTransform->SetMatrix(reinterpret_cast<float*>(&TexTransform));
		mFloorfxDiffuseMap->SetResource(mFloorTexSRV);
		mFloorfxNormalMap->SetResource(mFloorNormalTexSRV);

		mFloorTech->GetPassByIndex(i)->Apply(0, md3dImmediateContext);
		md3dImmediateContext->DrawIndexed(mFloorIndexCount, 0, 0);
	}

	//关闭细分
	md3dImmediateContext->HSSetShader(0, 0, 0);
	md3dImmediateContext->DSSetShader(0, 0, 0);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//绘制障碍物（方块）
	stride = sizeof(Vertex);   //顶点结构的大小
	offset = 0;

	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);    //设置顶点缓冲
	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);       //设置索引缓冲
	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   //绘制的基本图元为三角形

	world = XMLoadFloat4x4(&mWorld);
	view = inCam.View();
	proj = inCam.Proj();

	//为效果文件缓冲区变量设置参数,这里是三个光源的参数
	mfxDirLight->SetRawValue(&mDirLight, 0, sizeof(mDirLight));
	mfxPointLight->SetRawValue(&mPointLight, 0, sizeof(mPointLight));
	mfxSpotLight->SetRawValue(&mSpotLight, 0, sizeof(mSpotLight));
	mfxMaterial->SetRawValue(&mBoxMat, 0, sizeof(mBoxMat));
	mfxEyePosW->SetRawValue(&inCam.GetPosition(), 0, sizeof(XMFLOAT3));
	mfxTexTransform->SetMatrix(reinterpret_cast<float*>(&XMMatrixIdentity()));
	mfxDiffuseMap->SetResource(mWallSRV);  //设置纹理

	scale = XMMatrixScaling(10.f, 7.f, 5.f);      //方块尺寸

	for (size_t i = 0; i < ObstaclesInfo.size(); ++i)
	{
		XMFLOAT3 ObstacleLoc = ObstaclesInfo[i];
		ObstacleLoc.z -= CarMoveDistancePerFrame;
		XMMATRIX boxOffset = XMMatrixTranslation(ObstacleLoc.x, ObstacleLoc.y, ObstacleLoc.z);

		worldViewProj = world * scale * boxOffset * view * proj;     //获取当前世界观察矩阵， 这里一定要注意相乘的顺序，scale一定要在offset前面
		worldInvTranspose = MathHelper::InverseTranspose(world*scale);   //保持网格因为Scale变化造成发现异常

		//下面就是渲染部分
		mTech->GetDesc(&techDesc2);
		for (UINT p = 0; p < techDesc2.Passes; ++p)
		{
			mfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
			mfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
			mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

			mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);    //这里一定要Apply这样才能绘制新的图形
			md3dImmediateContext->DrawIndexed(36, 0, 0);
		}
		
		if (ObstacleLoc.z < -30.f)
			ObstaclesInfo.erase(ObstaclesInfo.begin());    //该障碍物已经不需要绘制
		if (drawCar)
			ObstaclesInfo[i] = ObstacleLoc;
	}

	//绘制汽车
	if (drawCar)
	{
		md3dImmediateContext->IASetInputLayout(mInputLayout);
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   //绘制的基本图元为三角形

		UINT stride = sizeof(Vertex);   //顶点结构的大小
		UINT offset = 0;
		
		md3dImmediateContext->IASetVertexBuffers(0, 1, &mCarVB, &stride, &offset);    //设置顶点缓冲
		md3dImmediateContext->IASetIndexBuffer(mCarIB, DXGI_FORMAT_R32_UINT, 0);       //设置索引缓冲

		//为效果文件缓冲区变量设置参数,这里是三个光源的参数`
		mCarfxDirLight->SetRawValue(&mCarDirLights, 0, sizeof(mCarDirLights));
		if (bInMove)
		{
			CurPosX = GameMath::FInterpTo(CurPosX, NextPosX, mTimer.DeltaTime(), 8.f);

			if (abs(NextPosX - CurPosX) < 0.1f)
			{
				CurPosX = NextPosX;
				CurrentPos = NextPos;
				bInMove = false;
			}
		}

		if (bInJump)
		{
			CurHeight = GameMath::FInterpTo(CurHeight, DestHeight, mTimer.DeltaTime(), 7.f);

			if (abs(DestHeight - CurHeight) < 0.05f)
			{
				DestHeight = 0.f;
			}
		}

		XMMATRIX offsetXY = XMMatrixTranslation(CurPosX, CurHeight, 0.f);

		//下面就是渲染部分
		mCarTech->GetDesc(&techDesc1);
		for (UINT p = 0; p < techDesc1.Passes; ++p)
		{
			world = XMLoadFloat4x4(&mWorld)*offsetXY;
			mCarfxEyePosW->SetRawValue(&mEyePosW, 0, sizeof(XMFLOAT3));
			XMMATRIX offset = XMMatrixTranslation(0.f, 0.f, 0.f);
		
			XMMATRIX CarRotation = XMMatrixRotationRollPitchYaw(0.f, 179.1f, 0.f);
			worldViewProj = world * CarRotation * view * proj;
			mCarfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
			mCarfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
			mCarfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
			mCarfxMaterial->SetRawValue(&mCarMat, 0, sizeof(mCarMat));
			mCarfxCubeMap->SetResource(mDynamicCubeMapSRV);
			mCarfxDirLight->SetRawValue(mCarDirLights, 0, sizeof(mCarDirLights));
			mCarfxTexTransform->SetMatrix(reinterpret_cast<float*>(&TexTransform));

			mCarTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);    //这里一定要Apply这样才能绘制新的图形
			md3dImmediateContext->DrawIndexed(mCarIndexCount, 0, 0);
		}
	}

	for (size_t i = 0; i < ObstaclesInfo.size(); ++i)
	{
		AxisAlignedBox obstacleTemp, carTemp;
		XMMATRIX boxScale = XMMatrixScaling(10.f, 7.f, 5.f);
	
		obstacleTemp.Center = ObstaclesInfo[i];
		carTemp.Extents = carCollision.Extents;
		XMVECTOR extent = XMVector3Transform(XMLoadFloat3(&obstacleCollision.Extents), boxScale);
		XMVECTOR carCenter = XMVector3Transform(XMLoadFloat3(&carCollision.Center), XMMatrixTranslation(CurPosX, CurHeight, 0.f)*XMMatrixRotationRollPitchYaw(0.f, 179.1f, 0.f));
		XMStoreFloat3(&obstacleTemp.Extents, extent);
		XMStoreFloat3(&carTemp.Center, carCenter);

		if (XNA::IntersectAxisAlignedBoxAxisAlignedBox(&carTemp, &obstacleTemp))
		{
			blockNum++;
		}

		//下面代码用于调试
		wchar_t str[100];
		int t = NextPos;
		int k = CurrentPos;
		swprintf(str, 100, L"CenterX: %d, FPS: %2.2f, Floor Speed: %2.2f, DeltaTime: %2.4f",blockNum, 1.f / mTimer.DeltaTime(), floorSpeed, mTimer.DeltaTime());

		SetWindowText(mhMainWnd, str);
	}

	

}
void RunGame::BuildBoxBuffer()
{
	Vertex v[24];

	std::vector<Vertex> Vertices;
	std::vector<UINT> Indices;

	float w2 = 0.5f;
	float h2 = 0.5f;
	float d2 = 0.5f;

	//下面依次填充六个面的顶点
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);

	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	Vertices.assign(&v[0], &v[24]);

	//下面填充索引
	UINT i[36];

	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	Indices.assign(&i[0], &i[36]);

	//创建顶点缓冲
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * 24;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &Vertices[0];
	md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB);

	//创建索引缓冲
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * 36;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &Indices[0];
	md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB);

	obstacleCollision.Center = XMFLOAT3(0.f, 0.f, 0.f);
	obstacleCollision.Extents = XMFLOAT3(0.5f, 0.5f, 0.5f);
}

void RunGame::BuildCarBuffer()
{
	//使用文件流读取文件
	std::ifstream fin("Models/car.txt");

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	//每遇到空格或换行就停止流入
	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		//文件中每行就有一个Pos和Normal，下面就是依次读入
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	mCarIndexCount = 3 * tcount;
	std::vector<UINT> indices(mCarIndexCount);
	for (UINT i = 0; i < tcount; ++i)
	{
		//读取模型的顶点索引
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();    //关闭文件流

	//创建顶点缓冲
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * vcount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	md3dDevice->CreateBuffer(&vbd, &vinitData, &mCarVB);

	//创建索引缓冲
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mCarIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	md3dDevice->CreateBuffer(&ibd, &iinitData, &mCarIB);

	XMFLOAT3 maxPos = vertices[0].Pos;
	XMFLOAT3 minPos = vertices[0].Pos;
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		XMFLOAT3 comparedPos = vertices[i].Pos;
		if (comparedPos.x > maxPos.x)maxPos.x = comparedPos.x;
		if (comparedPos.y > maxPos.y)maxPos.y = comparedPos.y;
		if (comparedPos.z > maxPos.z)maxPos.z = comparedPos.z;

		if (comparedPos.x < minPos.x)minPos.x = comparedPos.x;
		if (comparedPos.y < minPos.y)minPos.y = comparedPos.y;
		if (comparedPos.z < minPos.z)minPos.z = comparedPos.z;
	}

	XMVECTOR min = XMLoadFloat3(&minPos);
	XMVECTOR max = XMLoadFloat3(&maxPos);

	//由于游戏中汽车旋转了180度，所以这里也要将这个包围盒进行旋转
	XMMATRIX carRotation = XMMatrixRotationRollPitchYaw(0.f, 179.1f, 0.f);
	//min = XMVector3Transform(min, carRotation);     //汽车中心跟着旋转
	//max = XMVector3Transform(max, carRotation);
	
	XMStoreFloat3(&carCollision.Center, (min + max)*0.5f);
	XMStoreFloat3(&carCollision.Extents, (max - min)*0.5f);
}

void RunGame::BuildFloorBuffer()
{
	GeometryGenerator::MeshData grid;  //地面网格数据
	
	GeometryGenerator geoGen;
	geoGen.CreateGrid(30.f, 100.f, 30, 40, grid);       //创建网格缓冲区

	mFloorIndexCount = grid.Indices.size();

	std::vector<PosNormalTexTan> vertices(grid.Vertices.size());

	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		vertices[i].Pos      = grid.Vertices[i].Position;
		vertices[i].Normal   = grid.Vertices[i].Normal;
		vertices[i].Tex      = grid.Vertices[i].TexC;
		vertices[i].TangentU = grid.Vertices[i].TangentU;
	}

	//创建地面的顶点缓冲
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(PosNormalTexTan) * grid.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	md3dDevice->CreateBuffer(&vbd, &vinitData, &mFloorVB);

	std::vector<UINT> indices;
	indices = grid.Indices;

	//创建地面索引缓冲
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mFloorIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	md3dDevice->CreateBuffer(&ibd, &iinitData, &mFloorIB);

}

void RunGame::BuildBoxFX()
{
	std::ifstream fin(L"Shader/Lift.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();     //获取该文件的大小
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compileShader(size);

	fin.read(&compileShader[0], size);
	fin.close();

	//创建Shader
	D3DX11CreateEffectFromMemory(&compileShader[0], size, 0, md3dDevice, &mFX);

	mTech = mFX->GetTechniqueByName("LiftTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix(); //从效果文件中获取矩阵变量
	mfxWorld = mFX->GetVariableByName("gWorld")->AsMatrix();     //获取world变量
	mfxWorldInvTranspose = mFX->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	mfxTexTransform = mFX->GetVariableByName("gTexTransform")->AsMatrix();

	//获取光线变量
	mfxEyePosW = mFX->GetVariableByName("gEyePosW")->AsVector();
	mfxDirLight = mFX->GetVariableByName("gDirLight");
	mfxPointLight = mFX->GetVariableByName("gPointLight");
	mfxSpotLight = mFX->GetVariableByName("gSpotLight");
	mfxMaterial = mFX->GetVariableByName("gMaterial");
	mfxDiffuseMap = mFX->GetVariableByName("gDiffuseMap")->AsShaderResource();

}

void RunGame::BuildCubemapFX()
{
	std::ifstream fin(L"Shader/Sky.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();     //获取该文件的大小
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compileShader(size);

	fin.read(&compileShader[0], size);
	fin.close();

	D3DX11CreateEffectFromMemory(&compileShader[0], size, 0, md3dDevice, &mSkyFX);

	mSkyTech = mSkyFX->GetTechniqueByName("SkyTech");
	mfxSkyWorldViewProj = mSkyFX->GetVariableByName("gWorldViewProj")->AsMatrix();
	mfxCubeMap = mSkyFX->GetVariableByName("gCubeMap")->AsShaderResource();
}

void RunGame::BuildFloorFX()
{
	std::ifstream fin(L"Shader/Floor.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();     //获取该文件的大小
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compileShader(size);

	fin.read(&compileShader[0], size);
	fin.close();

	D3DX11CreateEffectFromMemory(&compileShader[0], size, 0, md3dDevice, &mFloorFX);

	mFloorTech = mFloorFX->GetTechniqueByName("Light3TexNormalDisplace");

	mFloorfxViewProj = mFloorFX->GetVariableByName("gViewProj")->AsMatrix();
	mFloorfxWorldViewPoj = mFloorFX->GetVariableByName("gWorldViewProj")->AsMatrix();
	mFloorfxWorld = mFloorFX->GetVariableByName("gWorld")->AsMatrix();
	mFloorfxWorldInvTranspose = mFloorFX->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	mFloorfxTexTransform = mFloorFX->GetVariableByName("gTexTransform")->AsMatrix();
	mFloorfxEyePosW = mFloorFX->GetVariableByName("gEyePosW")->AsVector();
	mFloorfxDirLights = mFloorFX->GetVariableByName("gDirLights");
	mFloorfxMat = mFloorFX->GetVariableByName("gMaterial");
	mFloorfxHeightScale = mFloorFX->GetVariableByName("gHeightScale")->AsScalar();
	mFloorfxMaxTessDistance = mFloorFX->GetVariableByName("gMaxTessDistance")->AsScalar();
	mFloorfxMinTessDistance = mFloorFX->GetVariableByName("gMinTessDistance")->AsScalar();
	mFloorfxMaxTessFactor = mFloorFX->GetVariableByName("gMaxTessFactor")->AsScalar();
	mFloorfxMinTessFactor = mFloorFX->GetVariableByName("gMinTessFactor")->AsScalar();

	mFloorfxDiffuseMap = mFloorFX->GetVariableByName("gDiffuseMap")->AsShaderResource();
	mFloorfxNormalMap = mFloorFX->GetVariableByName("gNormalMap")->AsShaderResource();
}

void RunGame::BuildCarFX()
{
	std::ifstream fin(L"Shader/Car.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();     //获取该文件的大小
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compileShader(size);

	fin.read(&compileShader[0], size);
	fin.close();

	D3DX11CreateEffectFromMemory(&compileShader[0], size, 0, md3dDevice, &mCarFX);

	mCarTech = mCarFX->GetTechniqueByName("Light3");
	mCarfxDirLight = mCarFX->GetVariableByName("gDirLights");
	mCarfxEyePosW = mCarFX->GetVariableByName("gEyePosw")->AsVector();
	mCarfxMaterial = mCarFX->GetVariableByName("gMaterial");
	mCarfxWorld = mCarFX->GetVariableByName("gWorld")->AsMatrix();
	mCarfxWorldInvTranspose = mCarFX->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	mCarfxWorldViewProj = mCarFX->GetVariableByName("gWorldViewProj")->AsMatrix();
	mCarfxTexTransform = mCarFX->GetVariableByName("gTexTransform")->AsMatrix();
	mCarfxCubeMap = mCarFX->GetVariableByName("gCubeMap")->AsShaderResource();
}

void RunGame::CreateDynamicCubeMapResource()
{
	// 创建含有6张Texture的TextureCube

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = CubeMapSize;
	texDesc.Height = CubeMapSize;
	texDesc.MipLevels = 0;
	texDesc.ArraySize = 6;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;

	ID3D11Texture2D* cubeTex = nullptr;
	md3dDevice->CreateTexture2D(&texDesc, nullptr, &cubeTex);

	// 创建RenderTarget
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.MipSlice = 0;

	for (int i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;   //创建贴图的序号
		md3dDevice->CreateRenderTargetView(cubeTex, &rtvDesc, &mDynamicCubeMapRTV[i]);      //创建RenderTarget
	}

	//创建着色器资源

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = -1;

	md3dDevice->CreateShaderResourceView(cubeTex, &srvDesc, &mDynamicCubeMapSRV);    //创建CubeMap

	ReleaseCOM(cubeTex);

	// 下面就是创建对应CubeMap的深度模板缓冲
	D3D11_TEXTURE2D_DESC depthTexDesc;
	depthTexDesc.Width = CubeMapSize;
	depthTexDesc.Height = CubeMapSize;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.ArraySize = 1;
	depthTexDesc.SampleDesc.Count = 1;
	depthTexDesc.SampleDesc.Quality = 0;
	depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTexDesc.CPUAccessFlags = 0;
	depthTexDesc.MiscFlags = 0;

	ID3D11Texture2D* depthTex = nullptr;
	md3dDevice->CreateTexture2D(&depthTexDesc, nullptr, &depthTex);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = depthTexDesc.Format;
	dsvDesc.Flags = 0;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateDepthStencilView(depthTex, &dsvDesc, &mDynamicCubeMapDSV);    //创建深度模板缓冲

	ReleaseCOM(depthTex);

	//对应的Viewport属性
	mCubeMapViewport.TopLeftX = 0.0f;
	mCubeMapViewport.TopLeftY = 0.0f;
	mCubeMapViewport.Width = (float)CubeMapSize;
	mCubeMapViewport.Height = (float)CubeMapSize;
	mCubeMapViewport.MinDepth = 0.0f;
	mCubeMapViewport.MaxDepth = 1.0f;
}

void RunGame::BuildCubeFaceCamera(float x, float y, float z)
{
	XMFLOAT3 center(x, y, z);
	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);

	// 摄像机的6个朝向，分别记录6个面的画面
	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f, y, z), // +X
		XMFLOAT3(x - 1.0f, y, z), // -X
		XMFLOAT3(x, y - 1.0f, z), // +Y
		XMFLOAT3(x, y + 1.0f, z), // -Y
		XMFLOAT3(x, y, z - 1.0f), // +Z
		XMFLOAT3(x, y + 0.2f, z + 1.0f)  // -Z
	};

	// 摄像机上方向 向量
	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f, -1.0f, 0.0f),  // +X
		XMFLOAT3(0.0f, -1.0f, 0.0f),  // -X
		XMFLOAT3(0.0f, 0.0f, 1.0f),  //-Y
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		XMFLOAT3(0.0f, 1.0f, 0.0f),	 // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f)	 // -Z
	};

	//下面就是创建6个方向的摄像机
	for (int i = 0; i < 6; ++i)
	{
		mCubeMapCamera[i].LookAt(center, targets[i], ups[i]);
		mCubeMapCamera[i].SetLens(0.5f*XM_PI, 1.0f, 0.1f, 1000.0f);
		mCubeMapCamera[i].UpdateViewMatrix();
	}
}

void RunGame::UpdateUpCamera()
{
	BuildCubeFaceCamera(CurPosX, CurHeight + 2.f, 0.f);

	//向上方向的相机是第三个
	XMFLOAT3 finalTarget;
	XMMATRIX targetOffset = XMMatrixTranslation(1.f, 5.f, 0.f);
	XMMATRIX targetRotateY = XMMatrixRotationY(-mTimer.TotalTime()*0.2f);
	XMVECTOR target = XMVectorSet(0.f, 0.f, 0.f, 1.f);
 	 
	XMVECTOR dest = XMVector3Transform(target, XMMatrixMultiply(targetOffset, targetRotateY));
	XMStoreFloat3(&finalTarget, dest);

	mCubeMapCamera[2].LookAt(XMFLOAT3(0.f,2.f,0.f), finalTarget, XMFLOAT3(0.0f, 0.0f, -1.0f));
	mCubeMapCamera[2].SetLens(0.5f*XM_PI, 1.0f, 0.1f, 1000.0f);
	mCubeMapCamera[2].UpdateViewMatrix();

	mCubeMapCamera[3].LookAt(XMFLOAT3(0.f, 2.f, 0.f), finalTarget, XMFLOAT3(0.0f, 0.0f, -1.0f));
	mCubeMapCamera[3].SetLens(0.5f*XM_PI, 1.0f, 0.1f, 1000.0f);
	mCubeMapCamera[3].UpdateViewMatrix();
}

void RunGame::SetInputLayout()
{
	//设置布局参数
	D3D11_INPUT_ELEMENT_DESC descs[3] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	//D3D效果相关库，微软已经开源
	D3DX11_PASS_DESC passDesc;
	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	md3dDevice->CreateInputLayout(descs, 3, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout);

	D3D11_INPUT_ELEMENT_DESC CubePos[1] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	mSkyTech->GetPassByIndex(0)->GetDesc(&passDesc);
	md3dDevice->CreateInputLayout(CubePos, 1, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mSkyInputLayout); 

	D3D11_INPUT_ELEMENT_DESC PosNormalTexTan_[4] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	//创建地面输入布局
	D3DX11_PASS_DESC passDesc1;
	mFloorTech->GetPassByIndex(0)->GetDesc(&passDesc1);
	md3dDevice->CreateInputLayout(PosNormalTexTan_, 4, passDesc1.pIAInputSignature, passDesc1.IAInputSignatureSize, &mFloorInputLayout);
}


