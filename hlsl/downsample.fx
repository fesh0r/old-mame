//-----------------------------------------------------------------------------
// Effect File Variables
//-----------------------------------------------------------------------------

texture Diffuse;

sampler DiffuseSampler = sampler_state
{
	Texture   = <Diffuse>;
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
};

//-----------------------------------------------------------------------------
// Downsample Vertex Shader
//-----------------------------------------------------------------------------

uniform float2 ScreenSize;
uniform float2 TargetSize;
uniform float2 SourceSize;
uniform float2 PrimRatio;
uniform float BloomRescale;

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output = (VS_OUTPUT)0;
	
	Output.Position = float4(Input.Position.xyz, 1.0f);
	Output.Position.xy /= ScreenSize;
	Output.Position.y = 1.0f - Output.Position.y;
	Output.Position.xy -= float2(0.5f, 0.5f);
	Output.Position.xy *= float2(2.0f, 2.0f);
	Output.Color = Input.Color;
	float2 inversePixel = 1.0f / ScreenSize;
	Output.TexCoord01.xy = Input.Position.xy / ScreenSize + float2(0.5f, 0.5f) / TargetSize;
	Output.TexCoord01.zw = Input.Position.xy / ScreenSize + float2(1.5f, 0.5f) / TargetSize;
	Output.TexCoord23.xy = Input.Position.xy / ScreenSize + float2(0.5f, 1.5f) / TargetSize;
	Output.TexCoord23.zw = Input.Position.xy / ScreenSize + float2(1.5f, 1.5f) / TargetSize;

	return Output;
}

//-----------------------------------------------------------------------------
// Downsample Pixel Shader
//-----------------------------------------------------------------------------

float4 ps_main(PS_INPUT Input) : COLOR
{
	float4 texel0 = tex2D(DiffuseSampler, Input.TexCoord01.xy);
	float4 texel1 = tex2D(DiffuseSampler, Input.TexCoord01.zw);
	float4 texel2 = tex2D(DiffuseSampler, Input.TexCoord23.xy);
	float4 texel3 = tex2D(DiffuseSampler, Input.TexCoord23.zw);
	float4 outTexel = (texel0 + texel1 + texel2 + texel3) * BloomRescale;
	return float4(outTexel.rgb, 1.0f);
}

//-----------------------------------------------------------------------------
// Downsample Effect
//-----------------------------------------------------------------------------

technique TestTechnique
{
	pass Pass0
	{
		Lighting = FALSE;

		Sampler[0] = <DiffuseSampler>;

		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
