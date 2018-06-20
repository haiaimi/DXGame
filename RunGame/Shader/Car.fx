#include "LightHelper.fx"
 
cbuffer cbPerFrame
{
	DirectionalLight gDirLights[3];
	float3 gEyePosW;
}; 

cbuffer cbPerObject
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
	float4x4 gTexTransform;
	Material gMaterial;
}; 

TextureCube gCubeMap;

SamplerState samAnisotropic
{
	Filter = ANISOTROPIC;
	MaxAnisotropy = 4;

	AddressU = WRAP;
	AddressV = WRAP;
};
 
struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex     : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 Tex     : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// 局部坐标转换到世界坐标
	vout.PosW    = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);
		
	// 转换到NDK控件
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// 计算材质偏移
	vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;

	return vout;
}
 
float4 PS(VertexOut pin, 
          uniform int gLightCount, 
		  uniform bool gUseTexure, 
		  uniform bool gAlphaClip, 
		  uniform bool gFogEnabled, 
		  uniform bool gReflectionEnabled) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);

	float3 toEye = gEyePosW - pin.PosW;

	float distToEye = length(toEye);

	// 单位化到眼睛的向量
	toEye /= distToEye;
	
    float4 texColor = float4(1, 1, 1, 1);
	 
	float4 litColor = texColor;
	if( gLightCount > 0  )
	{  
		// 初始化
		float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 spec    = float4(0.0f, 0.0f, 0.0f, 0.0f);

		//计算光照影响
		[unroll]
		for(int i = 0; i < gLightCount; ++i)
		{
			float4 A, D, S;
			ComputeDirectionalLight(gMaterial, gDirLights[i], pin.NormalW, toEye, 
				A, D, S);

			ambient += A;
			diffuse += D;
			spec    += S;
		}

		litColor = texColor*(ambient + diffuse) + spec;

		if( gReflectionEnabled )
		{
			//下面是透明效果
			//float3 incident = -toEye;
			//float3 reflectionVector = reflect(incident, pin.NormalW);   //计算反射向量
			//float3 refractionVector = refract(incident, pin.NormalW, 0.97f);   //计算折射方向，第三个参数是折射因素（两种介质反射率相除）
			//float4 reflectionColor  = gCubeMap.Sample(samAnisotropic, reflectionVector);    //反射向量取样
			//float4 refractionColor = gCubeMap.Sample(samAnisotropic, refractionVector);     //折射方向取样

			//litColor += gMaterial.Reflect*refractionColor;  //这里根据不同的要求选择不同的反射参数

			//镜面反射
			float3 incident = -toEye;
			float3 reflectionVector = reflect(incident, pin.NormalW);
			float4 reflectionColor = gCubeMap.Sample(samAnisotropic, reflectionVector);

			litColor += gMaterial.Reflect*reflectionColor;
		}
	}

	// Common to take alpha from diffuse material and texture.
	litColor.a = gMaterial.Diffuse.a * texColor.a;

    return litColor;
}


technique11 Light3
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(3, false, true, false, true) ) );
    }
}
