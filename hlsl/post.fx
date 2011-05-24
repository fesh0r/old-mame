//-----------------------------------------------------------------------------
// Scanline & Shadowmask Effect
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

texture Shadow;

sampler ShadowSampler = sampler_state
{
	Texture   = <Shadow>;
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	AddressW = WRAP;
};

//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------

struct VS_OUTPUT
{
	float4 Position : POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
	float2 ExtraInfo : TEXCOORD1;
};

struct VS_INPUT
{
	float4 Position : POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
	float2 ExtraInfo : TEXCOORD1;
};

struct PS_INPUT
{
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
	float2 ExtraInfo : TEXCOORD1;
};

//-----------------------------------------------------------------------------
// Scanline & Shadowmask Vertex Shader
//-----------------------------------------------------------------------------

uniform float TargetWidth;
uniform float TargetHeight;

uniform float RawWidth;
uniform float RawHeight;

uniform float WidthRatio;
uniform float HeightRatio;

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output = (VS_OUTPUT)0;
	
	Output.Position = float4(Input.Position.xyz, 1.0f);
	Output.Position.x /= TargetWidth;
	Output.Position.y /= TargetHeight;
	Output.Position.y = 1.0f - Output.Position.y;
	Output.Position.x -= 0.5f;
	Output.Position.y -= 0.5f;
	Output.Position *= float4(2.0f, 2.0f, 1.0f, 1.0f);
	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord + 0.5f / float2(RawWidth, RawHeight);
	Output.ExtraInfo = Input.ExtraInfo;

	//float Zoom = 1.0f;
	//Output.TexCoord /= Zoom;
	//Output.TexCoord += float2(0.175f * (1.0f - 1.0f / Zoom) / WidthRatio, 0.175f * (1.0f - 1.0f / Zoom) / HeightRatio);
	return Output;
}

//-----------------------------------------------------------------------------
// Post-Processing Pixel Shader
//-----------------------------------------------------------------------------

uniform float PI = 3.14159265f;

uniform float PincushionAmount = 0.00f;
uniform float CurvatureAmount = 0.08f;

uniform float ScanlineAmount = 1.0f;
uniform float ScanlineScale = 1.0f;
uniform float ScanlineBrightScale = 1.0f;
uniform float ScanlineBrightOffset = 1.0f;
uniform float ScanlineOffset = 1.0f;

uniform float UseShadow = 0.0f;
uniform float ShadowBrightness = 1.0f;
uniform float ShadowPixelSizeX = 3.0f;
uniform float ShadowPixelSizeY = 3.0f;
uniform float ShadowMaskSizeX = 3.0f;
uniform float ShadowMaskSizeY = 3.0f;
uniform float ShadowU = 0.375f;
uniform float ShadowV = 0.375f;
uniform float ShadowWidth = 8.0f;
uniform float ShadowHeight = 8.0f;

float4 ps_main(PS_INPUT Input) : COLOR
{
	float2 Ratios = float2(WidthRatio, HeightRatio);

	// -- Screen Pincushion Calculation --
	float2 PinViewpointOffset = float2(0.0f, 0.0f);
	float2 PinUnitCoord = (Input.TexCoord + PinViewpointOffset) * Ratios * 2.0f - 1.0f;
	float PincushionR2 = pow(length(PinUnitCoord), 2.0f) / pow(length(Ratios), 2.0f);
	float2 PincushionCurve = PinUnitCoord * PincushionAmount * PincushionR2;
	float2 BaseCoord = Input.TexCoord;
	BaseCoord -= 0.5f / Ratios;
	BaseCoord *= 1.0f - PincushionAmount * Ratios * 0.2f; // Warning: Magic constant
	BaseCoord += 0.5f / Ratios;
	BaseCoord += PincushionCurve;

	float2 CurveViewpointOffset = float2(0.2f, 0.0f);
	float2 CurveUnitCoord = (Input.TexCoord + CurveViewpointOffset) * 2.0f - 1.0f;
	float CurvatureR2 = pow(length(CurveUnitCoord),2.0f) / pow(length(Ratios), 2.0f);
	float2 CurvatureCurve = CurveUnitCoord * CurvatureAmount * CurvatureR2;
	float2 ScreenCurveCoord = Input.TexCoord + CurvatureCurve;

	float2 CurveClipUnitCoord = Input.TexCoord * Ratios * 2.0f - 1.0f;
	float CurvatureClipR2 = pow(length(CurveClipUnitCoord),2.0f) / pow(length(Ratios), 2.0f);
	float2 CurvatureClipCurve = CurveClipUnitCoord * CurvatureAmount * CurvatureClipR2;
	float2 ScreenClipCoord = Input.TexCoord;
	ScreenClipCoord -= 0.5f / Ratios;
	ScreenClipCoord *= 1.0f - CurvatureAmount * Ratios * 0.2f; // Warning: Magic constant
	ScreenClipCoord += 0.5f / Ratios;
	ScreenClipCoord += CurvatureClipCurve;

	// RGB Pincushion Calculation
	float3 PincushionCurveX = PinUnitCoord.x * PincushionAmount * PincushionR2;
	float3 PincushionCurveY = PinUnitCoord.y * PincushionAmount * PincushionR2;

	float4 BaseTexel = tex2D(DiffuseSampler, BaseCoord);

	// -- Alpha Clipping (1px border in drawd3d does not work for some reason) --
	clip((BaseCoord.x < WidthRatio / RawWidth) ? -1 : 1);
	clip((BaseCoord.y < HeightRatio / RawHeight) ? -1 : 1);
	clip((BaseCoord.x > (1.0f / WidthRatio + 1.0f / RawWidth)) ? -1 : 1);
	clip((BaseCoord.y > (1.0f / HeightRatio + 1.0f / RawHeight)) ? -1 : 1);

	// -- Scanline Simulation --
	float3 ScanBrightness = lerp(1.0f, abs(sin(((BaseCoord.y * Ratios.y * RawHeight * ScanlineScale) * PI + ScanlineOffset * RawHeight))) * ScanlineBrightScale + ScanlineBrightOffset, ScanlineAmount);
	float3 Scanned = BaseTexel.rgb * ScanBrightness;

	float2 ShadowDims = float2(ShadowWidth, ShadowHeight);
	float2 ShadowUV = float2(ShadowU, ShadowV);
	float2 ShadowMaskSize = float2(ShadowMaskSizeX, ShadowMaskSizeY);
	float2 ShadowFrac = frac(ScreenCurveCoord * ShadowMaskSize * Ratios * 0.5f);
	float2 ShadowCoord = ShadowFrac * ShadowUV + 1.5f / ShadowDims;
	float3 ShadowTexel = lerp(1.0f, tex2D(ShadowSampler, ShadowCoord), UseShadow);
	
	// -- Final Pixel --
	float4 Output = lerp(Input.Color, float4(Scanned * lerp(1.0f, ShadowTexel * 1.25f, ShadowBrightness), BaseTexel.a) * Input.Color, Input.ExtraInfo.x);
	
	return Output;
}

//-----------------------------------------------------------------------------
// Scanline & Shadowmask Effect
//-----------------------------------------------------------------------------

technique ScanMaskTechnique
{
	pass Pass0
	{
		Lighting = FALSE;

		//Sampler[0] = <DiffuseSampler>;

		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();
	}
}
