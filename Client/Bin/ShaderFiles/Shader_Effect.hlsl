#include "../../../EngineSDK/hlsl/Engine_Shader_Defines.hlsli"

float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix, g_OldViewMatrix, g_OldWorldMatrix;
Texture2D g_DiffuseTexture;
Texture2D g_NormalTexture;
Texture2D g_NoiseTexture;
Texture2D g_MaskTexture;

float g_fWeightX;
float g_fWeightY;
float g_TimeX;
float g_TimeY;
float g_StartTexcoordX;
float g_StartTexcoordY;
float g_DissolveAmount;
bool g_bUsing_Noise; //0 Noise True, Texcoord

float g_MaskCountX;
float g_MaskCountY;

//float g_fRGB_R;
//float g_fRGB_G;
//float g_fRGB_B;
float3 g_vRGB;


vector g_vCamPosition;

bool g_bTexcoordX;
bool g_bTexcoordY;
float g_fTexcoordLerpX;
float g_fTexcoordLerpY;

float g_fMaxTimer;

bool g_bMinus_X;
bool g_bMinus_Y;
bool g_bGray;


float4 g_ModelPosition;
float g_fExplosionPower;
float g_fFallingTime;
float g_fModelHeightCenterY;




struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
	
	   
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
};

struct VS_OUT_MotionBlur
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
	   
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
    
    float4 vDir : TEXCOORD3;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    matrix matWV, matWVP;

    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);
    
    
    
    Out.vPosition = mul(vector(In.vPosition, 1.f), matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    Out.vProjPos = Out.vPosition;
	
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), g_WorldMatrix));
    Out.vBinormal = vector(normalize(cross(Out.vNormal.xyz, Out.vTangent.xyz)), 0.f); // 외적 순서 중요하다 왜냐하면 순서바뀌면 binormal이 - 축으로 설정되니깐 
	
    return Out;
}

VS_OUT VS_EXPLOSION(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    matrix matWV, matWVP;

    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);
    
    float3 vPos = In.vPosition;
    
    float2 noiseUV = float2(vPos.x * 0.5f + g_TimeX * 0.2, vPos.y * 0.5f);
    
    float wave = sin(vPos.y * 10 + g_TimeX * 5) * 0.1;
    float offset = (wave) * 0.2;
    
    float angle = vPos.y * 4.0f + g_TimeX * 2.f;
    float2 dir = float2(cos(angle), sin(angle));
    vPos.xz += dir * offset;
    
    Out.vPosition = mul(vector(vPos, 1.f), matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix));
    Out.vTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y + g_TimeX * 0.5f);
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    Out.vProjPos = float4(vPos, 1.f);
	
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), g_WorldMatrix));
    Out.vBinormal = vector(normalize(cross(Out.vNormal.xyz, Out.vTangent.xyz)), 0.f);
	
    return Out;
}

VS_OUT_MotionBlur VS_MAIN_MOTIONBLUR(VS_IN In)
{
    VS_OUT_MotionBlur Out = (VS_OUT_MotionBlur) 0;

    matrix matWV, matWVP;

    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);

    Out.vPosition = mul(vector(In.vPosition, 1.f), matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    Out.vProjPos = Out.vPosition;
    
    //motion blur를 위한 추가작업시작
    float4 vNewPos = Out.vPosition;
    float4 vOldPos = mul(vector(In.vPosition, 1.f), g_OldWorldMatrix);
    vOldPos = mul(vOldPos, g_OldViewMatrix);
    vOldPos = mul(vOldPos, g_ProjMatrix);
	
    float3 vDir = vNewPos.xyz - vOldPos.xyz;
    
    float a = dot(normalize(vDir), normalize(Out.vNormal.xyz));
    if (a < 0.f)
        Out.vPosition = vOldPos;
    else
        Out.vPosition = vNewPos;
    
    float2 vVelocity = (vNewPos.xy / vNewPos.w) - (vOldPos.xy / vOldPos.w);
    Out.vDir.x = vVelocity.x * 0.5f;
    Out.vDir.y = vVelocity.y * (-0.5f);
    Out.vDir.z = Out.vPosition.z;
    Out.vDir.w = Out.vPosition.w;
    //motion blur를 위한 추가작업끝
    
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), g_WorldMatrix));
    Out.vBinormal = vector(normalize(cross(Out.vNormal.xyz, Out.vTangent.xyz)), 0.f);
	
    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
	
		
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
};

struct PS_IN_MOTIONBLUR
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
	   
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
    
    float4 vDir : TEXCOORD3;
};

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
    float4 vNormal : SV_TARGET1;
    float4 vDepth : SV_TARGET2;
};

struct PS_OUT_DISTORTION
{
    float4 vDistortion : SV_TARGET0;
};

struct PS_OUT_GLOW
{
    float4 vGlow : SV_TARGET0;
};

struct PS_OUT_MOTIONBLUR
{
    float4 vMotionBlur : SV_TARGET0;
};

struct PS_OUT_WEIGHTBLEND
{
    float4 vDiffuse : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;

    
    
    //vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
	
    //vector vMask = g_MaskTexture.Sample(LinearSampler, In.vTexcoord);
    vector vMask = g_MaskTexture.Sample(LinearSampler, In.vTexcoord);
    
    float fMask = vMask.r;
    
    //if (vMtrlDiffuse.a < 0.1f)
    //    discard;
	
    //float4 vNormalDesc  = g_NormalTexture.Sample(LinearSampler, In.vTexcoord);	
    float4 vNormalDesc = g_NormalTexture.Sample(LinearSampler, In.vTexcoord);
	

	/* 탄젠트 스페이스에 존재하는 노멀이다. */	
    float3 vNormal = vNormalDesc.xyz * 2.f - 1.f;
	
	
	/* 월드 스페이스상의 노말로 변환하자. */
    float3x3 WorldMatrix = float3x3(In.vTangent.xyz, In.vBinormal.xyz, In.vNormal.xyz);
    vNormal = normalize(mul(vNormal, WorldMatrix));
	

    Out.vDiffuse = vMtrlDiffuse * fMask;
    Out.vDiffuse.a = 1.f;
    Out.vNormal = vector(vNormal * 0.5f + 0.5f, 0.f);
    //Out.vNormal  = vector(In.vNormal.xyz * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w, 0.f, 0.f);
	
    return Out;
}


PS_OUT PS_MAIN_DISSOLVE(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;

    float2 vMaskTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y);
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    float2 vMtrlTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, vMtrlTexcoord) * fMask;
    
    //if (vMtrlDiffuse.a < 0.1f)  
    //    discard;
   
    /* Dissolve 관련 */ 
    
    // 노이즈 텍스처에서 샘플링 (UV 스케일, 타임 등을 이용해 변형 가능)
    float2 noiseUV = In.vTexcoord + float2(g_TimeX * 0.1f, g_TimeY * 0.1f);
    float noiseValue = g_NoiseTexture.Sample(LinearSampler, noiseUV).r;
    
    
    //(선택) Dissolve 경계를 검출해 Edge Color를 보정 
    // 경계는 (noiseValue)가 gDissolveAmount 근처에 있을 때 
    // 예) edgeWidth 범위 내에서만 효과 
    float g_EdgeWidth = 0.3f;
    float edgeFactor = smoothstep(g_DissolveAmount - g_EdgeWidth, g_DissolveAmount, noiseValue);
    
    //edgeFactor를 이용해 경계 색상 블랜딩 
    //edgeFactor가 1이면 경계 밖(정상), 0이면 경계 안 (완전 클립)    
    //경계 = edgeFactor가 0~1 사이를 지나는 부분
    //(좀 더 심플하게 하려면 saturate, lerp를 이용) 
    
    float4 g_EdgeColor = { 1.f, 1.f, 0.f, 1.f };
    float edgeStrength = 1.0 - edgeFactor; // 경계 부근에서 1
    float4 edgeBlend = lerp(vMtrlDiffuse, g_EdgeColor, edgeStrength);
    float4 finalColor = lerp(vMtrlDiffuse, edgeBlend, edgeStrength);
    
    // Dissolve 조건 
    // noiseValue < gDissolveAmount 이면 픽셀을 버린다 ( 투명 처리 ) 
    // clip(x)를 호출하면 x < 0 일 때 해당 픽셀이 버려짐. 
    if (noiseValue < g_DissolveAmount)
    {
           // 예를 들어, noiseValue가 g_DissolveAmount 근처일 때는 edge 효과를 유지하고,
            // 그보다 많이 낮으면 투명하게 만듦.
        float alphaFactor = saturate((noiseValue - (g_DissolveAmount - g_EdgeWidth)) / g_EdgeWidth);
        finalColor.a *= alphaFactor;
        
        

         // 필요에 따라, alpha가 너무 낮으면 완전히 폐기할 수도 있음
        if (finalColor.a < 0.01f) 
            clip(-1);
       
    }
    
    // (선택) Dissolve 경계를 검출해 Edge Color를 보정 
    // 경계는 (noiseValue)가 gDissolveAmount 근처에 있을 때 
    // 예) edgeWidth 범위 내에서만 효과 
    //float edgeFactor = smoothstep(g_DissolveAmount - g_EdgeWidth, g_DissolveAmount, noiseValue);    
    
    // edgeFactor를 이용해 경계 색상 블랜딩 
    // edgeFactor가 1이면 경계 밖(정상), 0이면 경계 안 (완전 클립) 
    // 경계 = edgeFactor가 0~1 사이를 지나는 부분
    // (좀 더 심플하게 하려면 saturate, lerp를 이용)
    
    //float  edgeStrength = 1.0 - edgeFactor; // 경계 부근에서 1
    //float4 edgeBlend = lerp(vMtrlDiffuse, g_EdgeColor, edgeStrength);    
    //float4 finalColor = lerp(vMtrlDiffuse, edgeBlend, edgeStrength);
    
    /* ////////////////////////////////// */
    
    float4 vNormalDesc = g_NormalTexture.Sample(LinearSampler, In.vTexcoord);
	
    /* 탄젠트 스페이스에 존재하는 노멀이다. 지금 (0~1 ) UnNormal로 저장 되어 있음 */  
    float3 vNormal = vNormalDesc.xyz * 2.f - 1.f;

   /* 월드 스페이스상의 노말로 변환하자. */
    float3x3 WorldMatrix = float3x3(In.vTangent.xyz, In.vBinormal.xyz, In.vNormal.xyz);
    
    vNormal = normalize(mul(vNormal, WorldMatrix));
    
    //Out.vDiffuse = vMtrlDiffuse;
    /* 디졸브 관련 */
    float blendFactor = 0.5; // 0 ~ 1 사이의 값, 효과의 강도 조절
    
    //float3 vRGB = float3(g_fRGB_R, g_fRGB_G, g_fRGB_B);
    
    Out.vDiffuse = lerp(vMtrlDiffuse, finalColor, blendFactor) * vector(g_vRGB, 1.f);
    Out.vNormal = vector(vNormal * 0.5f + 0.5f, 0.f);
    
    //Out.vNormal = vector(In.vNormal.xyz * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w, 0.f, 0.f);

    //Out.vDiffuse = 1.f;

    return Out;
}

PS_OUT_DISTORTION PS_MAIN_DISTORTION(PS_IN In)
{
    PS_OUT_DISTORTION Out = (PS_OUT_DISTORTION) 0;

    //아마 이런 작업이 필요할것임
    
    //vector vNoise = g_DistortionTexture.Sample(LinearSampler, In.vTexCoord * g_fTimer);
    //
    //float fWeight = vNoise.r * 0.9f;
    //
    //Out.vColor = g_NoiseTexture.Sample(LinearSampler, In.vTexCoord) * vNoise;
    vector vNoise = 0.f;
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, In.vTexcoord);
    float fMask = vMask.r;
    
    float2 vTexcoord = float2(In.vTexcoord.x * g_TimeX + g_StartTexcoordX, In.vTexcoord.y * g_TimeY + g_StartTexcoordY);
    
    if (true == g_bUsing_Noise)
        vNoise = g_NoiseTexture.Sample(LinearSampler, vTexcoord);
    else
        vNoise = g_DiffuseTexture.Sample(LinearSampler, vTexcoord);
    
    float fWeightX = vNoise.r * g_fWeightX;
    float fWeightY = vNoise.r * g_fWeightY;
    
    vTexcoord = float2(In.vTexcoord.x * fWeightX, In.vTexcoord.y * fWeightY);
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, vTexcoord) * fMask;
    
    //여기까지 지워야할지도
    
	
    if (vMtrlDiffuse.a < 0.1f)
        discard;
	
    Out.vDistortion = vMtrlDiffuse;
	
    return Out;
}

PS_OUT_GLOW PS_MAIN_GLOW(PS_IN In)
{
    PS_OUT_GLOW Out = (PS_OUT_GLOW) 0;
    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    vector vNoise = 0.f;
    
    float2 vTexcoord = 0;
    
    vTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y);
    
    if (true == g_bUsing_Noise)
        vNoise = g_NoiseTexture.Sample(LinearSampler_Clamp, vTexcoord);
    else
        vNoise = g_DiffuseTexture.Sample(LinearSampler_Clamp, vTexcoord);
    
    float fWeightX = vNoise.r * g_fWeightX;
    float fWeightY = vNoise.r * g_fWeightY;
    
    vTexcoord = float2((g_TimeX + g_StartTexcoordX) * fWeightX, (g_TimeY + g_StartTexcoordY) * fWeightY);
    
    if (true == g_bTexcoordX)
        vTexcoord.x = lerp(0.5f, vTexcoord.x, (1.f - vTexcoord.y) * g_fTexcoordLerpX);
    if (true == g_bTexcoordY)
        vTexcoord.y = lerp(0.5f, vTexcoord.y, (1.f - vTexcoord.x) * g_fTexcoordLerpY);
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler_Clamp, vTexcoord) * fMask;
	
    if (vMtrlDiffuse.a < 0.1f)
        discard;
    
    float3 vRGB = float3(g_vRGB);
	
    Out.vGlow = vMtrlDiffuse * vector(vRGB, 1.f);
    
    return Out;
}

PS_OUT PS_MAIN_BLOOD(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;

    float2 vMaskTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y);
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    float2 vMtrlTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, vMtrlTexcoord) * fMask;
    
    float2 noiseUV = In.vTexcoord + float2(g_TimeX * 0.1f, g_TimeY * 0.1f);
    float noiseValue = g_NoiseTexture.Sample(LinearSampler, noiseUV).r;
    
    float g_EdgeWidth = 0.3f;
    float edgeFactor = smoothstep(g_DissolveAmount - g_EdgeWidth, g_DissolveAmount, noiseValue);
    
    float4 g_EdgeColor = { 0.f, 0.f, 0.f, 1.f };
    float edgeStrength = 1.0 - edgeFactor; // 경계 부근에서 1
    float4 edgeBlend = lerp(vMtrlDiffuse, g_EdgeColor, edgeStrength);
    float4 finalColor = lerp(vMtrlDiffuse, edgeBlend, edgeStrength);
    
    if (noiseValue < g_DissolveAmount)
    {
        float alphaFactor = saturate((noiseValue - (g_DissolveAmount - g_EdgeWidth)) / g_EdgeWidth);
        finalColor.a *= alphaFactor;
        
        if (finalColor.a < 0.01f) 
            clip(-1);
    }
    
    float4 vNormalDesc = g_NormalTexture.Sample(LinearSampler, In.vTexcoord);
	
    float3 vNormal = vNormalDesc.xyz * 2.f - 1.f;

    float3x3 WorldMatrix = float3x3(In.vTangent.xyz, In.vBinormal.xyz, In.vNormal.xyz);
    
    vNormal = normalize(mul(vNormal, WorldMatrix));
    
    float blendFactor = 0.2f; // 0 ~ 1 사이의 값, 효과의 강도 조절
    
    Out.vDiffuse = lerp(vMtrlDiffuse, finalColor, blendFactor) * vector(g_vRGB, 1.f);
    Out.vNormal = vector(vNormal * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w, 0.f, 0.f);
    Out.vDiffuse.a = 1.f;
    
    
    return Out;
}

PS_OUT_DISTORTION PS_MAIN_ROUND(PS_IN In)
{
    PS_OUT_DISTORTION Out = (PS_OUT_DISTORTION) 0;

    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    float2 vRound = In.vTexcoord - float2(0.5f, 0.5f);
    float fDist = length(vRound);
    
    float fWave = sin((fDist - g_TimeX * g_TimeY) * 30) * exp(-fDist * 8) * g_DissolveAmount;
    
    float3 vNoise = g_NoiseTexture.Sample(LinearSampler, In.vTexcoord).rgb * 2 - 1.f;
    
    float2 vOffset = vNoise.xy * fWave;
    
    float4 vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord + vOffset) * vMask;
	
    if (vMtrlDiffuse.a < 0.1f)
        discard;
	
    Out.vDistortion = vMtrlDiffuse;
	
    return Out;
}

PS_OUT_GLOW PS_MAIN_ROAR(PS_IN In)
{
    PS_OUT_GLOW Out = (PS_OUT_GLOW) 0;
    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    float fTimeX = g_TimeX;
    float fTimeY = g_TimeY;
    
    if (true == g_bMinus_X)
        fTimeX = g_fMaxTimer - fTimeX;
    
    if (true == g_bMinus_Y)
        fTimeY = g_fMaxTimer - fTimeY;
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    vector vNoise = 0.f;
    
    float2 vTexcoord = 0;
    
    vTexcoord = float2(In.vTexcoord.x * fTimeX + g_StartTexcoordX, In.vTexcoord.y * fTimeY + g_StartTexcoordY);
    
    float fTimerX = (fTimeX / g_fMaxTimer);
    
    float fTimerY = (fTimeY / g_fMaxTimer);
    
    if (vMaskTexcoord.x < fTimerX)
        discard;
    
    if (vMaskTexcoord.y > fTimerY)
        discard;
    
    if (true == g_bUsing_Noise)
        vNoise = g_NoiseTexture.Sample(LinearSampler, In.vTexcoord);
    else
        vNoise = g_DiffuseTexture.Sample(LinearSampler_Clamp, 1.f - vTexcoord);
    
    float fWeightX = vNoise.r * g_fWeightX;
    float fWeightY = vNoise.r * g_fWeightY;
    
    vTexcoord = float2((fTimeX + g_StartTexcoordX) * fWeightX, (fTimeY + g_StartTexcoordY) * fWeightY);
    
    if (true == g_bTexcoordX)
        vTexcoord.x = lerp(0.5f, vTexcoord.x, (1.f - vTexcoord.y) * g_fTexcoordLerpX);
    if (true == g_bTexcoordY)
        vTexcoord.y = lerp(0.5f, vTexcoord.y, (1.f - vTexcoord.x) * g_fTexcoordLerpY);
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler_Clamp, vTexcoord) * fMask;
	
    if (vMtrlDiffuse.a < 0.1f)
        discard;
    
    vMtrlDiffuse *= vector(g_vRGB, 1.f);
    
    if (true == g_bGray)
    {
        float gray = dot(vMtrlDiffuse.rgb, float3(0.3f, 0.59f, 0.11f));
        vMtrlDiffuse.rgb = float3(gray, gray, gray);
    }
    
    Out.vGlow = vMtrlDiffuse * vector(g_vRGB, 1.f);
	
    return Out;
}

PS_OUT_GLOW PS_MAIN_GLOW_DISAPPEAR(PS_IN In)
{
    PS_OUT_GLOW Out = (PS_OUT_GLOW) 0;
    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    vector vNoise = 0.f;
    
    float2 vTexcoord = 0;
    
    vTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y);
    
    if (true == g_bUsing_Noise)
        vNoise = g_NoiseTexture.Sample(LinearSampler_Clamp, vTexcoord);
    else
        vNoise = g_DiffuseTexture.Sample(LinearSampler_Clamp, vTexcoord);
    
    float fWeightX = vNoise.r * g_fWeightX;
    float fWeightY = vNoise.r * g_fWeightY;
    
    vTexcoord = float2((g_TimeX + g_StartTexcoordX) * fWeightX, (g_TimeY + g_StartTexcoordY) * fWeightY);
    
    if (true == g_bTexcoordX)
        vTexcoord.x = lerp(0.5f, vTexcoord.x, (1.f - vTexcoord.y) * g_fTexcoordLerpX);
    if (true == g_bTexcoordY)
        vTexcoord.y = lerp(0.5f, vTexcoord.y, (1.f - vTexcoord.x) * g_fTexcoordLerpY);
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler_Clamp, vTexcoord) * fMask;
	
    if (vMtrlDiffuse.a < 0.1f)
        discard;
    
    float3 vRGB = float3(g_vRGB);
	
    vMtrlDiffuse *= vector(vRGB, 1.f - saturate(g_TimeX / g_fMaxTimer));
    
    if (vMtrlDiffuse.a < 0.1f)
        discard;
    
    Out.vGlow = vMtrlDiffuse;
    
    return Out;
}

PS_OUT_GLOW PS_MAIN_REVERSE(PS_IN In)
{
    PS_OUT_GLOW Out = (PS_OUT_GLOW) 0;
    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    vector vNoise = 0.f;
    
    float2 vTexcoord = 0;
    
    vTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y);
    
    if (true == g_bUsing_Noise)
        vNoise = g_NoiseTexture.Sample(LinearSampler_Clamp, vTexcoord);
    else
        vNoise = g_DiffuseTexture.Sample(LinearSampler_Clamp, vTexcoord);
    
    float fWeightX = vNoise.r * g_fWeightX;
    float fWeightY = vNoise.r * g_fWeightY;
    
    vTexcoord = float2((g_TimeX + g_StartTexcoordX) * fWeightX, (g_TimeY + g_StartTexcoordY) * fWeightY);
    
    if (true == g_bTexcoordX)
        vTexcoord.x = lerp(0.5f, vTexcoord.x, (1.f - vTexcoord.y) * g_fTexcoordLerpX);
    if (true == g_bTexcoordY)
        vTexcoord.y = lerp(0.5f, vTexcoord.y, (1.f - vTexcoord.x) * g_fTexcoordLerpY);
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler_Clamp, vTexcoord) * fMask;
	
    if (vMtrlDiffuse.a < 0.1f)
        discard;
    
    float3 vRGB = float3(g_vRGB);
	
    vMtrlDiffuse *= vector(vRGB, 1.f - saturate(g_TimeX / g_fMaxTimer));
    
    if (vMtrlDiffuse.a < 0.01f)
        discard;
    
    Out.vGlow = vMtrlDiffuse;
    
    return Out;
}


PS_OUT_GLOW PS_MAIN_TRAIL_DISAPPEAR(PS_IN In)
{
    PS_OUT_GLOW Out = (PS_OUT_GLOW) 0;
    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    vector vNoise = 0.f;
    
    float2 vTexcoord = 0;
    
    //vTexcoord = float2(In.vTexcoord.x * g_TimeX + g_StartTexcoordX, In.vTexcoord.y * g_TimeY + g_StartTexcoordY);
    vTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y);
    
    if (true == g_bUsing_Noise)
        vNoise = g_NoiseTexture.Sample(LinearSampler, vTexcoord);
    else
        vNoise = g_DiffuseTexture.Sample(LinearSampler, vTexcoord);
    
    float fWeightX = vNoise.r * g_fWeightX;
    float fWeightY = vNoise.r * g_fWeightY;
    
    //vTexcoord = float2(In.vTexcoord.x * fWeightX, In.vTexcoord.y * fWeightY);
    //vTexcoord = float2((In.vTexcoord.x * g_TimeX + g_StartTexcoordX) * fWeightX, (In.vTexcoord.y * g_TimeY + g_StartTexcoordY) * fWeightY);
    vTexcoord = float2((g_TimeX + g_StartTexcoordX) * fWeightX, (g_TimeY + g_StartTexcoordY) * fWeightY);
    
    if (true == g_bTexcoordX)
        vTexcoord.x = lerp(0.5f, vTexcoord.x, (1.f - vTexcoord.y) * g_fTexcoordLerpX);
    if (true == g_bTexcoordY)
        vTexcoord.y = lerp(0.5f, vTexcoord.y, (1.f - vTexcoord.x) * g_fTexcoordLerpY);
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, vTexcoord) * fMask;
	
    
    
    float3 vRGB = float3(g_vRGB);
	
    vMtrlDiffuse *= vector(vRGB, 1.f - saturate(g_TimeX / g_fMaxTimer));
    
    if (vMtrlDiffuse.a < 0.001f)
        discard;
    
    Out.vGlow = vMtrlDiffuse;
    
    
    //vector vLook = normalize(In.vWorldPos - g_vCamPosition);
    //
    //
    //
    //float fDegree_Look_Normal = dot(vLook, In.vNormal);
    //    
    //if (-0.4f < fDegree_Look_Normal && 0.4f > fDegree_Look_Normal)
    //    Out.vGlow.rgba = 1.f;
	
    return Out;
}

PS_OUT PS_MAIN_BLOOD_DISAPPEAR(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;
    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    vector vNoise = 0.f;
    
    float2 vTexcoord = 0;
    
    //vTexcoord = float2(In.vTexcoord.x * g_TimeX + g_StartTexcoordX, In.vTexcoord.y * g_TimeY + g_StartTexcoordY);
    vTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y);
    
    if (true == g_bUsing_Noise)
        vNoise = g_NoiseTexture.Sample(LinearSampler, vTexcoord);
    else
        vNoise = g_DiffuseTexture.Sample(LinearSampler, vTexcoord);
    
    float fWeightX = vNoise.r * g_fWeightX;
    float fWeightY = vNoise.r * g_fWeightY;
    
    //vTexcoord = float2(In.vTexcoord.x * fWeightX, In.vTexcoord.y * fWeightY);
    //vTexcoord = float2((In.vTexcoord.x * g_TimeX + g_StartTexcoordX) * fWeightX, (In.vTexcoord.y * g_TimeY + g_StartTexcoordY) * fWeightY);
    vTexcoord = float2((g_TimeX + g_StartTexcoordX) * fWeightX, (g_TimeY + g_StartTexcoordY) * fWeightY);
    
    if (true == g_bTexcoordX)
        vTexcoord.x = lerp(0.5f, vTexcoord.x, (1.f - vTexcoord.y) * g_fTexcoordLerpX);
    if (true == g_bTexcoordY)
        vTexcoord.y = lerp(0.5f, vTexcoord.y, (1.f - vTexcoord.x) * g_fTexcoordLerpY);
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, vTexcoord) * fMask;
	
    
    
    float3 vRGB = float3(g_vRGB);
	
    vMtrlDiffuse *= vector(vRGB, 1.f - saturate(g_TimeX / g_fMaxTimer));
    
    float fAlpha = pow((Out.vDiffuse.a - 0.1f), 3.f);
    
    if ((g_TimeX / g_fMaxTimer) >= fAlpha)
        discard;
    
    
    if (vMtrlDiffuse.a < 0.001f)
        discard;
    
    
    
    Out.vDiffuse = vMtrlDiffuse;
    
    float4 vNormalDesc = g_NoiseTexture.Sample(LinearSampler, In.vTexcoord);

    float3 vNormal = vNormalDesc.xyz * 2.f - 1.f;
	
    float3x3 WorldMatrix = float3x3(In.vTangent.xyz, In.vBinormal.xyz, In.vNormal.xyz);
    vNormal = normalize(mul(vNormal, WorldMatrix));

    Out.vNormal = vector(vNormal * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w, 0.f, 0.f);
    
    return Out;
}

//============================================ 추가 합니다. DESTRUCT 관련 부분 VS , GS , PS 는 요기서부터 쓸게요 (불만이라도?) ============================================ 


float g_fGravity = -9.8f;

// ============================================================= DESTRUCT 버텍스 쉐이더 부문 =============================================================
struct VS_IN_DESTRUCT
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float3 vTangent : TANGENT;
};

struct VS_OUT_DESTRUCT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
	
    float4 vTangent : TEXCOORD3;
    float4 vBinormal : TEXCOORD4;
};


VS_OUT_DESTRUCT VS_MAIN_DESTRUCT(VS_IN_DESTRUCT In)
{
    VS_OUT_DESTRUCT Out = (VS_OUT_DESTRUCT) 0;

    matrix matWV, matWVP;

    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);

    Out.vPosition = mul(vector(In.vPosition, 1.f), matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), matWV));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);
    Out.vProjPos = Out.vPosition;
	
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), g_WorldMatrix));
    Out.vBinormal = vector(normalize(cross(Out.vNormal.xyz, Out.vTangent.xyz)), 0.f);
	
    return Out;
}

// ============================================================= DESTRUCT 지오메트리 쉐이더 부문 =============================================================

struct GS_IN_DESTRUCT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
	
    float4 vTangent : TEXCOORD3;
    float4 vBinormal : TEXCOORD4;
};

struct GS_OUT_DESTRUCT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
    //float fDissolve : TEXCOORD3;                
	
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
};

#define PARTS_COUNT 12 
[maxvertexcount(3 * PARTS_COUNT)]
void GS_MAIN_MAGNITUDE(triangle GS_IN_DESTRUCT input[3], inout TriangleStream<GS_OUT_DESTRUCT> triStream)
{
    for (int iPartIndex = 0; iPartIndex < PARTS_COUNT; ++iPartIndex)
    {
        float fAngle = (2.0f * 3.141592f) * (iPartIndex / (float) PARTS_COUNT);
        
        float3 vMoveDir = normalize(float3(cos(fAngle), 1.0f, sin(fAngle)));
        
        float3 vMoveValue = vMoveDir * (g_fExplosionPower * 1.5f);

        float fFallingTime = g_fFallingTime;
        
        float fGravityPower = -9.8f;
        
        float fInitialPopVelocity = 1.2f;
        
        float fFallingValue = (fInitialPopVelocity * fFallingTime) + (0.4f * fGravityPower * fFallingTime * fFallingTime);
        vMoveValue.y += fFallingValue;

        float3 vRotationAxis = normalize(float3(cos(fAngle), 0.5f, sin(fAngle)));

        float fRotationSpeed = 30.0f;
        
        float fRotationAngle = g_fFallingTime * fRotationSpeed;
        float fCosAngle = cos(fRotationAngle);
        float fSinAngle = sin(fRotationAngle);

        float x = vRotationAxis.x;
        float y = vRotationAxis.y;
        float z = vRotationAxis.z;
        float3x3 matRotation =
        {
            fCosAngle + ((x * x) * (1 - fCosAngle)), (1 - fCosAngle) * x * y - fSinAngle * z, (1 - fCosAngle) * x * z + fSinAngle * y,
            (1 - fCosAngle) * x * y + fSinAngle * z, fCosAngle + ((y * y) * (1 - fCosAngle)), (1 - fCosAngle) * y * z - fSinAngle * x,
            (1 - fCosAngle) * x * z - fSinAngle * y, (1 - fCosAngle) * y * z + fSinAngle * x, fCosAngle + ((z * z) * (1 - fCosAngle))
        };
        
        for (int i = 0; i < 3; ++i)
        {
            GS_OUT_DESTRUCT Out = (GS_OUT_DESTRUCT) 0;

            Out.vTexcoord = input[i].vTexcoord;
            Out.vNormal = input[i].vNormal;
            Out.vTangent = input[i].vTangent;
            Out.vBinormal = input[i].vBinormal;
            
            float3 vLocalOffset = input[i].vWorldPos.xyz - g_ModelPosition.xyz;
            
            float3 vRotated = mul(vLocalOffset, matRotation);
            
            float3 vFinalPos = g_ModelPosition.xyz + vRotated + vMoveValue;

            Out.vWorldPos = float4(vFinalPos, 1);
            Out.vPosition = mul(Out.vWorldPos, mul(g_ViewMatrix, g_ProjMatrix));
            triStream.Append(Out);
        }
        
        triStream.RestartStrip();
    }
}


PS_OUT PS_MAIN_BLOODSUCKER(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;
    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    float2 vMtrlTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y);
    vMtrlTexcoord.y -= 1.0f * g_TimeY;
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, vMtrlTexcoord) * fMask;
    
    float2 noiseUV = In.vTexcoord + float2(g_TimeX * 0.1f, g_TimeY * 0.1f);
    
    float noiseValue = g_NoiseTexture.Sample(LinearSampler, noiseUV).r;
    
    float g_EdgeWidth = 0.3f;
    float edgeFactor = smoothstep(g_DissolveAmount - g_EdgeWidth, g_DissolveAmount, noiseValue);
    
    float4 g_EdgeColor = { 1.f, 1.f, 0.f, 1.f };
    float edgeStrength = 1.0 - edgeFactor; 
    float4 edgeBlend = lerp(vMtrlDiffuse, g_EdgeColor, edgeStrength);
    float4 finalColor = lerp(vMtrlDiffuse, edgeBlend, edgeStrength);
    
    if (noiseValue < g_DissolveAmount)
    {
        float alphaFactor = saturate((noiseValue - (g_DissolveAmount - g_EdgeWidth)) / g_EdgeWidth);
        finalColor.a *= alphaFactor;

        if (finalColor.a < 0.01f) 
            clip(-1);
    }
    
    float4 vNormalDesc = g_NormalTexture.Sample(LinearSampler, In.vTexcoord);
	
    float3 vNormal = vNormalDesc.xyz * 2.f - 1.f;

    float3x3 WorldMatrix = float3x3(In.vTangent.xyz, In.vBinormal.xyz, In.vNormal.xyz);
    
    vNormal = normalize(mul(vNormal, WorldMatrix));
    
    float blendFactor = 0.f; 
    
    float3 vRGB = float3(g_vRGB);

    vMtrlDiffuse.a *= (1.0f - saturate(g_TimeX / g_fMaxTimer));
    
    Out.vDiffuse = lerp(vMtrlDiffuse, finalColor, blendFactor) * vector(vRGB, 1.f);
    if (Out.vDiffuse.a < 0.01f || length(Out.vDiffuse.rgb) < 0.1f)
        discard;
    
    Out.vNormal = vector(vNormal * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w, 0.f, 0.f);
	
    return Out;
}


PS_OUT_GLOW PS_MAIN_GASBOOMBOOM(PS_IN In)
{
    PS_OUT_GLOW Out = (PS_OUT_GLOW) 0;
    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    float2 vMtrlTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y);
    vMtrlTexcoord.y -= 1.0f * g_TimeY;
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, vMtrlTexcoord) * fMask;
    
    float2 noiseUV = In.vTexcoord + float2(g_TimeX * 0.1f, g_TimeY * 0.1f);
    
    float noiseValue = g_NoiseTexture.Sample(LinearSampler, noiseUV).r;
    
    float g_EdgeWidth = 0.3f;
    float edgeFactor = smoothstep(g_DissolveAmount - g_EdgeWidth, g_DissolveAmount, noiseValue);
    
    float4 g_EdgeColor = { 1.f, 1.f, 0.f, 1.f };
    float edgeStrength = 1.0 - edgeFactor;
    float4 edgeBlend = lerp(vMtrlDiffuse, g_EdgeColor, edgeStrength);
    float4 finalColor = lerp(vMtrlDiffuse, edgeBlend, edgeStrength);
    
    if (noiseValue < g_DissolveAmount)
    {
        float alphaFactor = saturate((noiseValue - (g_DissolveAmount - g_EdgeWidth)) / g_EdgeWidth);
        finalColor.a *= alphaFactor;

        if (finalColor.a < 0.01f) 
            clip(-1);
    }
    
    float4 vNormalDesc = g_NormalTexture.Sample(LinearSampler, In.vTexcoord);
	
    float3 vNormal = vNormalDesc.xyz * 2.f - 1.f;

    float3x3 WorldMatrix = float3x3(In.vTangent.xyz, In.vBinormal.xyz, In.vNormal.xyz);
    
    vNormal = normalize(mul(vNormal, WorldMatrix));
    
    float blendFactor = 0.f;
    
    float3 vRGB = float3(g_vRGB);

    vRGB *= saturate((g_TimeX / g_fMaxTimer)) * 1.4f;
    
    vMtrlDiffuse.a *= (1.0f - saturate(g_TimeX / g_fMaxTimer));
    
    Out.vGlow = lerp(vMtrlDiffuse, finalColor, blendFactor) * vector(vRGB, 1.f);
    if (Out.vGlow.a < 0.01f || length(Out.vGlow.rgb) < 0.1f)
        discard;
    
	
    return Out;
}

PS_OUT_WEIGHTBLEND PS_MAIN_WEIGHTBLEND(PS_IN In) // 여기 값 조정 하고 Deferred 에서 제대로 받아서 누적 이란 느낌으로 해야함
{
    PS_OUT_WEIGHTBLEND Out = (PS_OUT_WEIGHTBLEND) 0;

    float2 vMaskTexcoord = float2(In.vTexcoord.x * g_MaskCountX, In.vTexcoord.y * g_MaskCountY);
    
    vector vMask = g_MaskTexture.Sample(LinearSampler, vMaskTexcoord);
    float fMask = vMask.r;
    
    float2 noiseUV = In.vTexcoord + float2(g_TimeX * 0.1f, g_TimeY * 0.1f);
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, vMaskTexcoord) * fMask;
    
    float fNoise = sin(g_NoiseTexture.Sample(LinearSampler, noiseUV * g_MaskCountX).r);
    
    
    float3 vRGB = float3(g_vRGB);

    vMtrlDiffuse.a *= (1.0f - saturate(g_TimeX / g_fMaxTimer));
    
    float fade_x = saturate(1.f - abs(In.vProjPos.x) / g_fWeightX) * fNoise;
    float fade_y = saturate(1.f - In.vProjPos.y / g_fWeightY);
    
    float fWeight = saturate(max(1e-2, In.vProjPos.w / g_DissolveAmount));
    Out.vDiffuse = vMtrlDiffuse * vector(vRGB, 1.f) * fWeight * fNoise * fade_y * fade_x;
    if (Out.vDiffuse.a < 0.01f || length(Out.vDiffuse.rgb) < 0.1f)
        discard;
    //Out.vDiffuse.a *= fade; //Weight Blend 는 a값이 필요가 없음 형식상 넣어둠
    
    return Out;
}


// ============================================================= DESTRUCT 픽셀 쉐이더 부문 =============================================================

struct PS_IN_DESTRUCT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
    float fDissolve : TEXCOORD3;               
	
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
};

struct PS_OUT_DESTRUCT
{
    float4 vDiffuse : SV_TARGET0;
    float4 vNormal : SV_TARGET1;
    float4 vDepth : SV_TARGET2;
    float fSpecular : SV_TARGET3;
};


PS_OUT_DESTRUCT PS_MAIN_DESTRUCT(PS_IN_DESTRUCT In)
{
    PS_OUT_DESTRUCT Out = (PS_OUT_DESTRUCT) 0;

    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
	
    if (vMtrlDiffuse.a < 0.1f)
        discard;
	
    float4 vNormalDesc = g_NormalTexture.Sample(LinearSampler, In.vTexcoord);
	
    float3 vNormal = vNormalDesc.xyz * 2.f - 1.f;
	
    float3x3 WorldMatrix = float3x3(In.vTangent.xyz, In.vBinormal.xyz, In.vNormal.xyz);
    vNormal = normalize(mul(vNormal, WorldMatrix));
	
    Out.vDiffuse = vMtrlDiffuse;
    Out.vNormal = vector(vNormal * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w, 0.f, 0.f);
    Out.fSpecular = 0.1f;
    
    return Out;
}















technique11 DefaultTechnique
{
    pass DefaultPass //0
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_SKip_Z, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    
    pass DissolvePass //1
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_SKip_Z, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_DISSOLVE();
    }

    pass Distortion //2
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_SKip_Z, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_DISTORTION();
    }

    pass Glow //3
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_SKip_Z, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_GLOW();
    }

    pass Blood //4
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_SKip_Z, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_BLOOD();
    }

    pass Round //5
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_WeightBlend, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_ROUND();
    }

    pass Varg_Roar //6 앞으로 왠지 응축되는 이펙트들 기반이 될듯
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_SKip_Z, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_ROAR();
    }

    pass Glow_Disapper //7 Glow 이지만 점점 사라지게끔 & Depth 비교함
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_GLOW_DISAPPEAR();
    }

    pass Reverse //8 Reverse
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_SKip_Z, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_REVERSE();
    }

    pass Glow_Disapper_Weapon //9 Glow_Weapon 이지만 점점 사라지게끔 & Depth 비교x
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_SKip_Z, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_GLOW_DISAPPEAR();
    }

    pass Trail_Disappear //10 Trail_Disappear
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_TRAIL_DISAPPEAR();
    }


    pass Blood_Disappear //11 
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_BLOOD_DISAPPEAR();
    }


    pass BLOOD_SUCKER   //12 
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_BLOODSUCKER();
    }

    pass GAS_BOOMBOOM  // 13
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_GASBOOMBOOM();
    }

    pass WeightBlend // 14 번 
    {
        SetRasterizerState(Rs_Cull_NONE);
        SetDepthStencilState(DSS_WeightBlend, 0);
        SetBlendState(BS_WeightBlend_Client, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_EXPLOSION();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN_WEIGHTBLEND();
    }

    //pass Pompeii // 10 
    //{
    //    SetRasterizerState(RS_Default);
    //    SetDepthStencilState(DSS_Default, 0);
    //    SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

    //    VertexShader = compile vs_5_0 VS_MAIN_DESTRUCT();
    //    GeometryShader = compile gs_5_0 GS_MAIN_MAGNITUDE();
    //    PixelShader = compile ps_5_0 PS_MAIN_DESTRUCT();
    //}
}
