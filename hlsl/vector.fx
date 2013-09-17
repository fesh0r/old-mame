//-----------------------------------------------------------------------------
// Effect File Variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------

struct VS_OUTPUT
{
	float4 Position : POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
	float2 LineInfo : TEXCOORD1;
};

struct VS_INPUT
{
	float3 Position : POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
	float2 LineInfo : TEXCOORD1;
};

struct PS_INPUT
{
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
	float2 LineInfo : TEXCOORD1;
};

//-----------------------------------------------------------------------------
// Simple Vertex Shader
//-----------------------------------------------------------------------------

uniform float2 ScreenDims;
uniform float2 TimeParams;
uniform float3 LengthParams;

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output = (VS_OUTPUT)0;
	
	Output.Position = float4(Input.Position.xyz, 1.0f);
	Output.Position.xy /= ScreenDims;
	Output.Position.y = 1.0f - Output.Position.y;
	Output.Position.xy -= 0.5f;
	Output.Position *= float4(2.0f, 2.0f, 1.0f, 1.0f);
	Output.Color = Input.Color;
	Output.TexCoord = Input.Position.xy / ScreenDims;
	Output.LineInfo = Input.LineInfo;
	return Output;
}

//-----------------------------------------------------------------------------
// Simple Pixel Shader
//-----------------------------------------------------------------------------

// TimeParams.x: Frame time of the vector
// TimeParams.y: How much frame time affects the vector's fade
// LengthParams.y: How much length affects the vector's fade
// LengthParams.z: Size at which fade is maximum
float4 ps_main(PS_INPUT Input) : COLOR
{
	float timeModulate = lerp(1.0f, TimeParams.x, TimeParams.y) * 1.0;

	float lengthModulate = 1.0f - clamp(Input.LineInfo.x / LengthParams.z, 0.0f, 1.0f);
	float minLength = 2.0f - clamp(Input.LineInfo.x - 1.0f, 0.0f, 2.0f);
	lengthModulate = lerp(lengthModulate, 4.0f, minLength * 0.5f);
	lengthModulate = lerp(1.0f, timeModulate * lengthModulate, LengthParams.y) * 1.0;

	float4 outColor = Input.Color * float4(lengthModulate, lengthModulate, lengthModulate, 1.0f) * 2.0;
	return outColor;
}

//-----------------------------------------------------------------------------
// Simple Effect
//-----------------------------------------------------------------------------

technique TestTechnique
{
	pass Pass0
	{
		Lighting = FALSE;

		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
