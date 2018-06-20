#include "BlurFilter.h"
#include "Effects.h"
#include "RunGame.h"

BlurFilter::BlurFilter()
  : mBlurredOutputTexSRV(0), mBlurredOutputTexUAV(0)
{
}

BlurFilter::~BlurFilter()
{
	ReleaseCOM(mBlurredOutputTexSRV);
	ReleaseCOM(mBlurredOutputTexUAV);
}

ID3D11ShaderResourceView* BlurFilter::GetBlurredOutput()
{
	return mBlurredOutputTexSRV;
}

void BlurFilter::SetGaussianWeights(float sigma)
{
	float d = 2.0f*sigma*sigma;

	float weights[9];
	float sum = 0.0f;
	for(int i = 0; i < 8; ++i)
	{
		float x = (float)i;
		weights[i] = expf(-x*x/d);

		sum += weights[i];
	}

	for(int i = 0; i < 8; ++i)
	{
		weights[i] /= sum;
	}

	//curGame->mBlurFX->SetWeights(weights);
}

void BlurFilter::SetWeights(const float weights[9])
{
	//Effects::BlurFX->SetWeights(weights);
}

void BlurFilter::Init(ID3D11Device* device, RunGame* inGame, UINT width, UINT height, DXGI_FORMAT format)
{
	// Start fresh.
	ReleaseCOM(mBlurredOutputTexSRV);
	ReleaseCOM(mBlurredOutputTexUAV);

	mWidth = width;
	mHeight = height;
	mFormat = format;
	curGame = inGame;

	//创建所用资源
	D3D11_TEXTURE2D_DESC blurredTexDesc;
	blurredTexDesc.Width     = width;
	blurredTexDesc.Height    = height;
    blurredTexDesc.MipLevels = 1;
    blurredTexDesc.ArraySize = 1;
	blurredTexDesc.Format    = format;
	blurredTexDesc.SampleDesc.Count   = 1;
	blurredTexDesc.SampleDesc.Quality = 0;
    blurredTexDesc.Usage     = D3D11_USAGE_DEFAULT;
    blurredTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    blurredTexDesc.CPUAccessFlags = 0;
    blurredTexDesc.MiscFlags      = 0;

	ID3D11Texture2D* blurredTex = 0;
	HR(device->CreateTexture2D(&blurredTexDesc, 0, &blurredTex));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	HR(device->CreateShaderResourceView(blurredTex, &srvDesc, &mBlurredOutputTexSRV));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	HR(device->CreateUnorderedAccessView(blurredTex, &uavDesc, &mBlurredOutputTexUAV));

	//释放引用
	ReleaseCOM(blurredTex);
}

void BlurFilter::BlurInPlace(ID3D11DeviceContext* dc, 
							 ID3D11ShaderResourceView* inputSRV, 
	                         ID3D11UnorderedAccessView* inputUAV,
							 int blurCount)
{
	for(int i = 0; i < blurCount; ++i)
	{
		//先进行横向滤波
		D3DX11_TECHNIQUE_DESC techDesc;
		curGame->mHorzBlurTech->GetDesc( &techDesc );
		for(UINT p = 0; p < techDesc.Passes; ++p)
		{
			curGame->mfxBlurInputMap->SetResource(inputSRV);
			curGame->mfxBlurOutputMap->SetUnorderedAccessView(mBlurredOutputTexUAV);
			curGame->mHorzBlurTech->GetPassByIndex(p)->Apply(0, dc);
			
			//计算着色器线程组的大小
			UINT numGroupsX = (UINT)ceilf(mWidth / 256.0f);
			dc->Dispatch(numGroupsX, mHeight, 1);       //执行计算着色器
		}
	
		// 解绑资源，因为这些资源后面会用于写入
		ID3D11ShaderResourceView* nullSRV[1] = { 0 };
		dc->CSSetShaderResources( 0, 1, nullSRV );

		ID3D11UnorderedAccessView* nullUAV[1] = { 0 };
		dc->CSSetUnorderedAccessViews( 0, 1, nullUAV, 0 );
	
		// 纵向滤波
		curGame->mVertBlurTech->GetDesc( &techDesc );
		for(UINT p = 0; p < techDesc.Passes; ++p)
		{
			curGame->mfxBlurInputMap->SetResource(mBlurredOutputTexSRV);
			curGame->mfxBlurOutputMap->SetUnorderedAccessView(inputUAV);
			curGame->mVertBlurTech->GetPassByIndex(p)->Apply(0, dc);

			UINT numGroupsY = (UINT)ceilf(mHeight / 256.0f);
			dc->Dispatch(mWidth, numGroupsY, 1);     //执行计算着色器
		}
	
		dc->CSSetShaderResources( 0, 1, nullSRV );
		dc->CSSetUnorderedAccessViews( 0, 1, nullUAV, 0 );
	}

	// 关闭计算着色器
	dc->CSSetShader(0, 0, 0);
}
