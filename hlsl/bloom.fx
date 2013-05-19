//-----------------------------------------------------------------------------
// Effect File Variables
//-----------------------------------------------------------------------------

texture DiffuseA;
texture DiffuseB;
texture DiffuseC;
texture DiffuseD;
texture DiffuseE;
texture DiffuseF;
texture DiffuseG;
texture DiffuseH;
texture DiffuseI;
texture DiffuseJ;
texture DiffuseK;

sampler DiffuseSampler0 = sampler_state
{
	Texture   = <DiffuseA>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSampler1 = sampler_state
{
	Texture   = <DiffuseB>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSampler2 = sampler_state
{
	Texture   = <DiffuseC>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSampler3 = sampler_state
{
	Texture   = <DiffuseD>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSampler4 = sampler_state
{
	Texture   = <DiffuseE>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSampler5 = sampler_state
{
	Texture   = <DiffuseF>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSampler6 = sampler_state
{
	Texture   = <DiffuseG>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSampler7 = sampler_state
{
	Texture   = <DiffuseH>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSampler8 = sampler_state
{
	Texture   = <DiffuseI>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSampler9 = sampler_state
{
	Texture   = <DiffuseJ>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

sampler DiffuseSamplerA = sampler_state
{
	Texture   = <DiffuseK>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------

struct VS_OUTPUT
{
	float4 Position : POSITION;
	float4 Color : COLOR0;
	float4 TexCoord01 : TEXCOORD0;
	float4 TexCoord23 : TEXCOORD1;
	float4 TexCoord45 : TEXCOORD2;
	float4 TexCoord67 : TEXCOORD3;
	float4 TexCoord89 : TEXCOORD4;
	float2 TexCoordA : TEXCOORD5;
};

struct VS_INPUT
{
	float3 Position : POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Color : COLOR0;
	float4 TexCoord01 : TEXCOORD0;
	float4 TexCoord23 : TEXCOORD1;
	float4 TexCoord45 : TEXCOORD2;
	float4 TexCoord67 : TEXCOORD3;
	float4 TexCoord89 : TEXCOORD4;
	float2 TexCoordA : TEXCOORD5;
};

//-----------------------------------------------------------------------------
// Bloom Vertex Shader
//-----------------------------------------------------------------------------

uniform float2 TargetSize;
uniform float2 SourceSize;

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output = (VS_OUTPUT)0;
	
	Output.Position = float4(Input.Position.xyz, 1.0f);
	Output.Position.xy /= TargetSize;
	Output.Position.y = 1.0f - Output.Position.y;
	Output.Position.xy -= float2(0.5f, 0.5f);
	Output.Position.xy *= float2(2.0f, 2.0f);
	Output.Color = Input.Color;
	float2 inversePixel = 1.0f / TargetSize;
	float2 TexCoord = Input.Position.xy * inversePixel + float2(0.5f, 0.5f) * inversePixel;
	Output.TexCoord01.xy = TexCoord - inversePixel;
	Output.TexCoord01.zw = (((TexCoord - inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.5f;
	Output.TexCoord23.xy = (((TexCoord - inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.25f;
	Output.TexCoord23.zw = (((TexCoord - inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.125f;
	Output.TexCoord45.xy = (((TexCoord - inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.0625f;
	Output.TexCoord45.zw = (((TexCoord - inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.03125f;
	Output.TexCoord67.xy = (((TexCoord - inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.015625f;
	Output.TexCoord67.zw = (((TexCoord + inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.0078125f;
	Output.TexCoord89.xy = (((TexCoord + inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.00390625f;
	Output.TexCoord89.zw = (((TexCoord + inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.001953125f;
	Output.TexCoordA = (((TexCoord + inversePixel) - 0.5f) * 1.00f + 0.5f) * 1.0f;//0.0009765625f;

	return Output;
}

//-----------------------------------------------------------------------------
// Bloom Pixel Shader
//-----------------------------------------------------------------------------

uniform float4 Level0123Weight;
uniform float4 Level4567Weight;
uniform float3 Level89AWeight;

float4 ps_main(PS_INPUT Input) : COLOR
{
	float3 texel0 = tex2D(DiffuseSampler0, Input.TexCoord01.xy).rgb;
	float3 texel1 = tex2D(DiffuseSampler1, Input.TexCoord01.zw).rgb;
	float3 texel2 = tex2D(DiffuseSampler2, Input.TexCoord23.xy).rgb;
	float3 texel3 = tex2D(DiffuseSampler3, Input.TexCoord23.zw).rgb;
	float3 texel4 = tex2D(DiffuseSampler4, Input.TexCoord45.xy).rgb;
	float3 texel5 = tex2D(DiffuseSampler5, Input.TexCoord45.zw).rgb;
	float3 texel6 = tex2D(DiffuseSampler6, Input.TexCoord67.xy).rgb;
	float3 texel7 = tex2D(DiffuseSampler7, Input.TexCoord67.zw).rgb;
	float3 texel8 = tex2D(DiffuseSampler8, Input.TexCoord89.xy).rgb;
	float3 texel9 = tex2D(DiffuseSampler9, Input.TexCoord89.zw).rgb;
	float3 texelA = tex2D(DiffuseSamplerA, Input.TexCoordA).rgb;

	texel0 = texel0 * Level0123Weight.x;
	texel1 = texel1 * Level0123Weight.y;
	texel2 = texel2 * Level0123Weight.z;
	texel3 = texel3 * Level0123Weight.w;
	texel4 = texel4 * Level4567Weight.x;
	texel5 = texel5 * Level4567Weight.y;
	texel6 = texel6 * Level4567Weight.z;
	texel7 = texel7 * Level4567Weight.w;
	texel8 = texel8 * Level89AWeight.x;
	texel9 = texel9 * Level89AWeight.y;
	texelA = texelA * Level89AWeight.z;

	float4 sum = float4(texel0 + texel1 + texel2 + texel3 + texel4 +
	        texel5 + texel6 + texel7 + texel8 + texel9 + texelA, 1.0f);
	return sum;
}

//-----------------------------------------------------------------------------
// Downsample Effect
//-----------------------------------------------------------------------------

technique TestTechnique
{
	pass Pass0
	{
		Lighting = FALSE;

		Sampler[0] = <DiffuseSampler0>; // 2048x2048
		Sampler[1] = <DiffuseSampler1>; // 1024x1024
		Sampler[2] = <DiffuseSampler2>; // 512x512
		Sampler[3] = <DiffuseSampler3>; // 256x256
		Sampler[4] = <DiffuseSampler4>; // 128x128
		Sampler[5] = <DiffuseSampler5>; // 64x64
		Sampler[6] = <DiffuseSampler6>; // 32x32
		Sampler[7] = <DiffuseSampler7>; // 16x16
		Sampler[8] = <DiffuseSampler8>; // 8x8
		Sampler[9] = <DiffuseSampler9>; // 4x4
		Sampler[10] = <DiffuseSamplerA>; // 2x2

		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();
	}
}
