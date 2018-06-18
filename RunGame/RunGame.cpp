#include "RunGame.h"
#include <d3d11.h>
#include "MathHelper.h"
#include "DDSTextureLoader.h"
#include <stdlib.h>
#include "GeometryGenerator.h"
#include "GameMath.h"

RunGame* gRunGameApp = nullptr;

//�ص����������ڴ��ڳ�ʼ��
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM IParam)
{
	return gRunGameApp->MsgProc(hwnd, msg, wParam, IParam);
}

RunGame::RunGame(HINSTANCE hInstance) :
	mhAppInst(hInstance),
	mWndCaption(L"�ܿ���Ϸ"),
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
	//��ʼ��Viewport�ṹ
	memset(&mScreenViewport, 0, sizeof(D3D11_VIEWPORT));
	memset(&mCubeMapViewport, 0, sizeof(D3D11_VIEWPORT));
	gRunGameApp = this;

	CurrentPos = EPlayerMoveDir::ECenter;
	NextPos = EPlayerMoveDir::ECenter;
	CurPosX = 0.f;    //��ҳ�ʼX���λ��Ϊ0
	NextPosX = 10.f;
	CurHeight = 0.f;
	DestHeight = 0.f;

	//��ʼ�����硢�۲졢ͶӰ����
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
	//���������������ڴ棬��������ڴ�й©
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

//��Ϸ�����ڣ�������Ϸ��ѭ��
int RunGame::Run()
{
	MSG msg{ 0 };

	mTimer.Reset();

	//��Ϸ��ѭ��
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

	// ֱ���
	mDirLight.Ambient = XMFLOAT4(0.5f, 0.5f, 0.8f, 1.0f);
	mDirLight.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mDirLight.Specular = XMFLOAT4(0.4f, 0.1f, 0.f, 1.0f);
	mDirLight.Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	// ���Դ
	mPointLight.Ambient = XMFLOAT4(0.f, 1.f, 0.f, 1.0f);
	mPointLight.Diffuse = XMFLOAT4(0.f, 1.f, 0.f, 1.0f);
	mPointLight.Specular = XMFLOAT4(0.f, 1.f, 0.f, 1.0f);
	mPointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mPointLight.Range = 25.0f;

	// �۹�ƣ������ֵ�Ͳ��
	mSpotLight.Ambient = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	mSpotLight.Diffuse = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	mSpotLight.Specular = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	mSpotLight.Att = XMFLOAT3(0.5f, 0.0f, 0.0f);
	mSpotLight.Spot = 96.0f;
	mSpotLight.Range = 10000.0f;

	//����Ĳ���
	mBoxMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	//��������
	mCarMat.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCarMat.Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mCarMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	mCarMat.Reflect = XMFLOAT4(1.f, 1.f, 1.f, 1.0f);

	//�������
	mFloorMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mFloorMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mFloorMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
	mFloorMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	mSky = new Sky(md3dDevice, this, L"Texture/grasscube1024.dds", 5000.0f);

	//����������Դ������ֱ��ʹ��DDS��Ϊ��������ʣ�΢���Ѿ���Դ�ÿ⣬��DDSTextureLoader.h
	DirectX::CreateDDSTextureFromFile(md3dDevice, L"Texture/checkboard.dds", nullptr, &mliftSRV);
	DirectX::CreateDDSTextureFromFile(md3dDevice, L"Texture/brick01.dds", nullptr, &mWallSRV);
	DirectX::CreateDDSTextureFromFile(md3dDevice, L"Texture/floor.dds", nullptr, &mFloorTexSRV);
	DirectX::CreateDDSTextureFromFile(md3dDevice, L"Texture/floor_nmap.dds", nullptr, &mFloorNormalTexSRV);

	BuildBoxBuffer();      //�������鶥�㡢��������
	BuildCarBuffer();
	BuildFloorBuffer();
	BuildBoxFX();       //������ɫ��
	BuildCubemapFX();
	BuildCarFX();
	BuildFloorFX();
	SetInputLayout();     //�������벼��

	CreateDynamicCubeMapResource();

	BuildCubeFaceCamera(0.f, 2.f, 0.f);

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, static_cast<float>(mClientWidth / mClientHeight), 1.0f, 1000.0f);       //����ͶӰ����
	XMStoreFloat4x4(&mProj, P);

	mEyePosW = XMFLOAT3(0.f, 10.f, -30.f);
	XMVECTOR pos = XMVectorSet(0.f, 10.f, -30.f, 1.f);
	XMVECTOR target = XMVectorSet(0.f, 5.f, 0.f, 1.f);
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	mCam.LookAt(pos, target, up);
	mCam.SetLens(0.25f*MathHelper::Pi, static_cast<float>(mClientWidth / mClientHeight), 1.0f, 1000.0f);
	mCam.UpdateViewMatrix();

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);      //�����۲����
	XMStoreFloat4x4(&mView, V);       //��ȡ�۲����
 //��ʼ���ɹ�
	return true;
}

LRESULT RunGame::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		//���ڼ���
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

		//�رմ���
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}


	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool RunGame::InitMainWindow()
{
	//��ʼ��������
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

	//ע�ᴰ��
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"����ע��ʧ��", 0, 0);
		return false;
	}

	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	//��������
	mhMainWnd = CreateWindow(L"GameWndClassName", mWndCaption.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);

	if (!mhMainWnd)
	{
		MessageBox(0, L"��������ʧ��", 0, 0);
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);
	SetWindowText(mhMainWnd, L" ");       //��ʾ��������

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
			MessageBox(0, L"D3D�豸����ʧ��", 0, 0);
			return false;
		}

		if (featureLevel != D3D_FEATURE_LEVEL_11_0)
		{
			MessageBox(0, L"���豸��֧��D3D11����", 0, 0);
			return false;
		}

		md3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality);   //��ȡ���豸֧�ֶ��ز����ȼ�

																									 //��������ʼ���ṹ
		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = mClientWidth;
		sd.BufferDesc.Height = mClientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 60; //ˢ����
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;       //���������ظ�ʽ
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = 4;   //ʹ�ö��ز�������ݣ���ȻЧ��̫��
		sd.SampleDesc.Quality = m4xMsaaQuality - 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    //������ȾĿ��
		sd.BufferCount = 1;        //��������Ŀ������ָ��һ��
		sd.OutputWindow = mhMainWnd;    //ָ���������
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;

		//�����Ǿ��ǿ�ʼ����������
		IDXGIDevice* dxgiDevice = 0;
		md3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

		IDXGIAdapter* dxgiAdapter = 0;
		dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

		IDXGIFactory* dxgiFactory = 0;
		dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

		dxgiFactory->CreateSwapChain(md3dDevice, &sd, &mSwapChain);   //����������

		ReleaseCOM(dxgiDevice);
		ReleaseCOM(dxgiAdapter);
		ReleaseCOM(dxgiFactory);

		ID3D11Texture2D* backBuffer;       //������
		mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
		md3dDevice->CreateRenderTargetView(backBuffer, nullptr, &mRenderTargetView);   //����RenderTarget
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

		//�ȴ���Texture2D��������Ȼ������ģ�建��󶨵�����
		md3dDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer);
		md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, 0, &mDepthStencilView);

		//��RenderTarget�����ģ�建�嵽��Ⱦ�ܵ�


		//�����ӿ�
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

	
		//�Դ��ж��������Ƿ񱻰���
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

	//�ָ�����������״̬
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

	if (SpawnObstacleDistance >= 30.f + overDistance)     //��ʱ���Լ����µ��ϰ���
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
	SpawnObstacleDistance = SpawnObstacleDistance + (100.f / 10.f)*(-floorMovePerFrame - PreTotalTime * PreFloorSpeed);      //�ⲽʮ����Ҫ������ֱ����floorSpeed��Ҫ���ǵ�����ƫ�ƺ�TotalTimeҲ�й�ϵ
	CarMoveDistancePerFrame = (100.f / 10.f)*(-floorMovePerFrame - PreTotalTime * PreFloorSpeed);

	PreFloorSpeed = floorSpeed;
	PreTotalTime = mTimer.TotalTime();
	

	// ����ʹ���ض���Viewport����Ⱦ6���浽RenderTarget�ϣ�ʹ��6����ͬ�������
	md3dImmediateContext->RSSetViewports(1, &mCubeMapViewport);
	for (int i = 0; i < 6; ++i)
	{
		//�������Ⱦ��ֱ����ʾ����Ļ��
		md3dImmediateContext->ClearRenderTargetView(mDynamicCubeMapRTV[i], reinterpret_cast<const float*>(&Colors::White));
		md3dImmediateContext->ClearDepthStencilView(mDynamicCubeMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		renderTargets[0] = mDynamicCubeMapRTV[i];
		md3dImmediateContext->OMSetRenderTargets(1, renderTargets, mDynamicCubeMapDSV);     //�����µ�RenderTarget
																							//��Ⱦ����ǰRenderTarget
		DrawScene(mCubeMapCamera[i], false);
	}

	md3dImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
	md3dImmediateContext->RSSetViewports(1, &mScreenViewport);
	md3dImmediateContext->GenerateMips(mDynamicCubeMapSRV);    //���ɲ��ͼ

	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);      //����RenderTarget�����ģ�建����л�����һ֡

	//md3dImmediateContext->IASetInputLayout(mInputLayout);
	//md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   //���ƵĻ���ͼԪΪ������

	//UINT stride = sizeof(Vertex);   //����ṹ�Ĵ�С
	//UINT offset = 0;
	//md3dImmediateContext->IASetVertexBuffers(0, 1, &mCarVB, &stride, &offset);    //���ö��㻺��
	//md3dImmediateContext->IASetIndexBuffer(mCarIB, DXGI_FORMAT_R32_UINT, 0);       //������������

	//XMMATRIX world = XMLoadFloat4x4(&mWorld);
	//XMMATRIX view = XMLoadFloat4x4(&mView);
	//XMMATRIX proj = XMLoadFloat4x4(&mProj);

	////ΪЧ���ļ��������������ò���,������������Դ�Ĳ���
	//mfxDirLight->SetRawValue(&mDirLight, 0, sizeof(mDirLight));
	//mfxPointLight->SetRawValue(&mPointLight, 0, sizeof(mPointLight));
	//mfxSpotLight->SetRawValue(&mSpotLight, 0, sizeof(mSpotLight));

	////���������Ⱦ����
	//D3DX11_TECHNIQUE_DESC techDesc;
	//mTech->GetDesc(&techDesc);
	//for (UINT p = 0; p < techDesc.Passes; ++p)
	//{
	//	///���Ƶ��ݲ���
	//	XMMATRIX scale = XMMatrixScaling(10.f, 2.f, 10.f);      //����ߴ�
	//	mfxEyePosW->SetRawValue(&mEyePosW, 0, sizeof(XMFLOAT3));
	//	XMMATRIX offset = XMMatrixTranslation(-0.1f, liftHeight - 19.999f, 0.f);
	//	XMMATRIX worldViewProj = world * scale * offset * view * proj;     //��ȡ��ǰ����۲���� ����һ��Ҫע����˵�˳��scaleһ��Ҫ��offsetǰ��
	//	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world*scale);   //����������ΪScale�仯��ɷ����쳣

	//	//ͬ����������ɫ����Ļ���������
	//	//mfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
	//	//mfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
	//	//mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
	//	//mfxMaterial->SetRawValue(&mBoxMat, 0, sizeof(mBoxMat));
	//	//mfxTexTransform->SetMatrix(reinterpret_cast<const float*>(&XMMatrixScaling(2.f, 2.f, 0.f)));   //���������С�����⵼������
	//	//mfxDiffuseMap->SetResource(mliftSRV);  //��������

	//	//mfxEyePosW->SetRawValue(&mEyePosW, 0, sizeof(mEyePosW));
	//	//XMMATRIX worldViewProj = world * view * proj;     //��ȡ��ǰ����۲���� ����һ��Ҫע����˵�˳��scaleһ��Ҫ��offsetǰ��
	//	//XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);   //����������ΪScale�仯��ɷ����쳣

	//	XMMATRIX CarRotation = XMMatrixRotationRollPitchYaw(0.f, 179.1f, 0.f);
	//	worldViewProj = world * CarRotation * view * proj;
	//	mfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
	//	mfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
	//	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
	//	mfxMaterial->SetRawValue(&mBoxMat, 0, sizeof(mBoxMat));
	//	mfxDiffuseMap->SetResource(mliftSRV);  //��������

	//	mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);    //����һ��ҪApply�������ܻ����µ�ͼ��
	//	md3dImmediateContext->DrawIndexed(mCarIndexCount, 0, 0);
	//}

	//mSky->Draw(md3dImmediateContext, mCam);
	
	DrawScene(mCam, true);

	md3dImmediateContext->RSSetState(0);
	md3dImmediateContext->OMSetDepthStencilState(0, 0);
	mSwapChain->Present(0, 0);     //��ʾBackBuffer
}


void RunGame::DrawScene(const Camera& inCam, bool drawCar)
{
	//������պ���
	UpdateUpCamera();     //���������
	mSky->Draw(md3dImmediateContext, inCam);

	md3dImmediateContext->RSSetState(0);
	md3dImmediateContext->OMSetDepthStencilState(0, 0);

	//���Ƶذ�
	UINT stride = sizeof(PosNormalTexTan);
	UINT offset = 0;
	XMMATRIX scale = XMMatrixScaling(1.f, 1.f, 1.f);      //�ߴ�
	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view = inCam.View();
	XMMATRIX proj = inCam.Proj();
	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world*scale);   //����������ΪScale�仯��ɷ����쳣
	XMMATRIX worldViewProj;    //��ȡ��ǰ����۲���� ����һ��Ҫע����˵�˳��scaleһ��Ҫ��offsetǰ��
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

	//�ر�ϸ��
	md3dImmediateContext->HSSetShader(0, 0, 0);
	md3dImmediateContext->DSSetShader(0, 0, 0);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//�����ϰ�����飩
	stride = sizeof(Vertex);   //����ṹ�Ĵ�С
	offset = 0;

	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);    //���ö��㻺��
	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);       //������������
	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   //���ƵĻ���ͼԪΪ������

	world = XMLoadFloat4x4(&mWorld);
	view = inCam.View();
	proj = inCam.Proj();

	//ΪЧ���ļ��������������ò���,������������Դ�Ĳ���
	mfxDirLight->SetRawValue(&mDirLight, 0, sizeof(mDirLight));
	mfxPointLight->SetRawValue(&mPointLight, 0, sizeof(mPointLight));
	mfxSpotLight->SetRawValue(&mSpotLight, 0, sizeof(mSpotLight));
	mfxMaterial->SetRawValue(&mBoxMat, 0, sizeof(mBoxMat));
	mfxEyePosW->SetRawValue(&inCam.GetPosition(), 0, sizeof(XMFLOAT3));
	mfxTexTransform->SetMatrix(reinterpret_cast<float*>(&XMMatrixIdentity()));
	mfxDiffuseMap->SetResource(mWallSRV);  //��������

	scale = XMMatrixScaling(10.f, 7.f, 5.f);      //����ߴ�

	for (size_t i = 0; i < ObstaclesInfo.size(); ++i)
	{
		XMFLOAT3 ObstacleLoc = ObstaclesInfo[i];
		ObstacleLoc.z -= CarMoveDistancePerFrame;
		XMMATRIX boxOffset = XMMatrixTranslation(ObstacleLoc.x, ObstacleLoc.y, ObstacleLoc.z);

		worldViewProj = world * scale * boxOffset * view * proj;     //��ȡ��ǰ����۲���� ����һ��Ҫע����˵�˳��scaleһ��Ҫ��offsetǰ��
		worldInvTranspose = MathHelper::InverseTranspose(world*scale);   //����������ΪScale�仯��ɷ����쳣

		//���������Ⱦ����
		mTech->GetDesc(&techDesc2);
		for (UINT p = 0; p < techDesc2.Passes; ++p)
		{
			mfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
			mfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
			mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

			mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);    //����һ��ҪApply�������ܻ����µ�ͼ��
			md3dImmediateContext->DrawIndexed(36, 0, 0);
		}
		
		if (ObstacleLoc.z < -30.f)
			ObstaclesInfo.erase(ObstaclesInfo.begin());    //���ϰ����Ѿ�����Ҫ����
		if (drawCar)
			ObstaclesInfo[i] = ObstacleLoc;
	}

	//��������
	if (drawCar)
	{
		md3dImmediateContext->IASetInputLayout(mInputLayout);
		md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   //���ƵĻ���ͼԪΪ������

		UINT stride = sizeof(Vertex);   //����ṹ�Ĵ�С
		UINT offset = 0;
		
		md3dImmediateContext->IASetVertexBuffers(0, 1, &mCarVB, &stride, &offset);    //���ö��㻺��
		md3dImmediateContext->IASetIndexBuffer(mCarIB, DXGI_FORMAT_R32_UINT, 0);       //������������

		//ΪЧ���ļ��������������ò���,������������Դ�Ĳ���`
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

		//���������Ⱦ����
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

			mCarTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);    //����һ��ҪApply�������ܻ����µ�ͼ��
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

		//����������ڵ���
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

	//�����������������Ķ���
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

	//�����������
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

	//�������㻺��
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * 24;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &Vertices[0];
	md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB);

	//������������
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
	//ʹ���ļ�����ȡ�ļ�
	std::ifstream fin("Models/car.txt");

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	//ÿ�����ո���о�ֹͣ����
	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		//�ļ���ÿ�о���һ��Pos��Normal������������ζ���
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
		//��ȡģ�͵Ķ�������
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();    //�ر��ļ���

	//�������㻺��
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * vcount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	md3dDevice->CreateBuffer(&vbd, &vinitData, &mCarVB);

	//������������
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

	//������Ϸ��������ת��180�ȣ���������ҲҪ�������Χ�н�����ת
	XMMATRIX carRotation = XMMatrixRotationRollPitchYaw(0.f, 179.1f, 0.f);
	//min = XMVector3Transform(min, carRotation);     //�������ĸ�����ת
	//max = XMVector3Transform(max, carRotation);
	
	XMStoreFloat3(&carCollision.Center, (min + max)*0.5f);
	XMStoreFloat3(&carCollision.Extents, (max - min)*0.5f);
}

void RunGame::BuildFloorBuffer()
{
	GeometryGenerator::MeshData grid;  //������������
	
	GeometryGenerator geoGen;
	geoGen.CreateGrid(30.f, 100.f, 30, 40, grid);       //�������񻺳���

	mFloorIndexCount = grid.Indices.size();

	std::vector<PosNormalTexTan> vertices(grid.Vertices.size());

	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		vertices[i].Pos      = grid.Vertices[i].Position;
		vertices[i].Normal   = grid.Vertices[i].Normal;
		vertices[i].Tex      = grid.Vertices[i].TexC;
		vertices[i].TangentU = grid.Vertices[i].TangentU;
	}

	//��������Ķ��㻺��
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

	//����������������
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
	int size = (int)fin.tellg();     //��ȡ���ļ��Ĵ�С
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compileShader(size);

	fin.read(&compileShader[0], size);
	fin.close();

	//����Shader
	D3DX11CreateEffectFromMemory(&compileShader[0], size, 0, md3dDevice, &mFX);

	mTech = mFX->GetTechniqueByName("LiftTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix(); //��Ч���ļ��л�ȡ�������
	mfxWorld = mFX->GetVariableByName("gWorld")->AsMatrix();     //��ȡworld����
	mfxWorldInvTranspose = mFX->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	mfxTexTransform = mFX->GetVariableByName("gTexTransform")->AsMatrix();

	//��ȡ���߱���
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
	int size = (int)fin.tellg();     //��ȡ���ļ��Ĵ�С
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
	int size = (int)fin.tellg();     //��ȡ���ļ��Ĵ�С
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
	int size = (int)fin.tellg();     //��ȡ���ļ��Ĵ�С
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
	// ��������6��Texture��TextureCube

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

	// ����RenderTarget
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.MipSlice = 0;

	for (int i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;   //������ͼ�����
		md3dDevice->CreateRenderTargetView(cubeTex, &rtvDesc, &mDynamicCubeMapRTV[i]);      //����RenderTarget
	}

	//������ɫ����Դ

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = -1;

	md3dDevice->CreateShaderResourceView(cubeTex, &srvDesc, &mDynamicCubeMapSRV);    //����CubeMap

	ReleaseCOM(cubeTex);

	// ������Ǵ�����ӦCubeMap�����ģ�建��
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
	md3dDevice->CreateDepthStencilView(depthTex, &dsvDesc, &mDynamicCubeMapDSV);    //�������ģ�建��

	ReleaseCOM(depthTex);

	//��Ӧ��Viewport����
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

	// �������6�����򣬷ֱ��¼6����Ļ���
	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f, y, z), // +X
		XMFLOAT3(x - 1.0f, y, z), // -X
		XMFLOAT3(x, y - 1.0f, z), // +Y
		XMFLOAT3(x, y + 1.0f, z), // -Y
		XMFLOAT3(x, y, z - 1.0f), // +Z
		XMFLOAT3(x, y + 0.2f, z + 1.0f)  // -Z
	};

	// ������Ϸ��� ����
	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f, -1.0f, 0.0f),  // +X
		XMFLOAT3(0.0f, -1.0f, 0.0f),  // -X
		XMFLOAT3(0.0f, 0.0f, 1.0f),  //-Y
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		XMFLOAT3(0.0f, 1.0f, 0.0f),	 // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f)	 // -Z
	};

	//������Ǵ���6������������
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

	//���Ϸ��������ǵ�����
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
	//���ò��ֲ���
	D3D11_INPUT_ELEMENT_DESC descs[3] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	//D3DЧ����ؿ⣬΢���Ѿ���Դ
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

	//�����������벼��
	D3DX11_PASS_DESC passDesc1;
	mFloorTech->GetPassByIndex(0)->GetDesc(&passDesc1);
	md3dDevice->CreateInputLayout(PosNormalTexTan_, 4, passDesc1.pIAInputSignature, passDesc1.IAInputSignatureSize, &mFloorInputLayout);
}


