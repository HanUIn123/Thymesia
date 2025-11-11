#include "../../../EngineSDK/hlsl/Engine_Shader_Defines.hlsli"

float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix, g_OldViewMatrix, g_OldWorldMatrix;
Texture2D g_DiffuseTexture;
Texture2D g_NormalTexture;
Texture2D g_EmissiveTexture;

Texture2D g_DissolveTexture;

vector g_vCamPosition;

matrix g_LightViewMatrix[3];
matrix g_LightProjMatrix[3];

float g_DissolveAmount;


float4 g_ModelPosition;
float g_fExplosionPower;
float g_fFallingTime;
float g_fModelHeightCenterY;


float g_WigglingTime;
float g_InitialJumpPower = 0.3f;
float g_ExplosionPowerOffset = 0.8f;



// ============================================================= 버텍스 쉐이더 부문 =============================================================
struct VS_IN
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
    float fDissolve : TEXCOORD3;
	
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
};

VS_OUT_DESTRUCT VS_MAIN(VS_IN In)
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
    Out.vBinormal = vector(normalize(cross(Out.vNormal.xyz, Out.vTangent.xyz)), 0.f); // 외적 순서 중요하다 왜냐하면 순서바뀌면 binormal이 - 축으로 설정되니깐 
	
    return Out;
}


VS_OUT_DESTRUCT VS_MAIN_DESTRUCT(VS_IN In)
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

// ============================================================= 지오메트리 쉐이더 부문 =============================================================

struct GS_IN_DESTRUCT
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

struct GS_OUT_DESTRUCT
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


float3 HashDirection(float3 seed)
{
    // 들어오는 정점의 [0]과 내적을 해서 값을 만들고 
    // sin * 45758.5453 등등 해서 그걸 수도 유사 난수 로 생성해버리장. (펠린 노이즈 )
    // https://m.blog.naver.com/jysa000/222062364456 (참고)
    float hashX = frac(sin(dot(seed, float3(12.9898, 78.233, 45.543))) * 43758.5453);
    float hashY = frac(sin(dot(seed, float3(93.989, 67.345, 12.345))) * 12458.3721);
    float hashZ = frac(sin(dot(seed, float3(45.123, 12.345, 78.234))) * 32513.6521);

    // 0 ~ 1 을 -1 ~ 1 로 만듬. 그리고 방향벡터로 쓰기위해 정규화작업.
    return normalize(float3(hashX * 2 - 1, hashY * 2 - 1, hashZ * 2 - 1));
}


[maxvertexcount(3)]
void GS_MAIN_DESTRUCT(triangle GS_IN_DESTRUCT input[3], inout TriangleStream<GS_OUT_DESTRUCT> triStream)
{
    float fVerticesWorldPosX = input[0].vWorldPos.x;
    float fVerticesWorldPosY = input[0].vWorldPos.y;

    // 모델 정점의 월드 좌표에서, 모델의 중심 좌표 뺀 값.( 정점이 모델 중심에서 얼마나 떨어져잇는가 )
    float fVerticesPosX = fVerticesWorldPosX - g_ModelPosition.x;       // x는 모델 피킹 배치된 그 x중심 
    float fVerticesPosY = fVerticesWorldPosY - g_fModelHeightCenterY;   // y는 블렌더에서 + 오프셋한 값 해준걸로

    float fHorizonQuaterValue = 0.25f;

    int iHorizontalPart = 0;
    
    if (fVerticesPosX < -fHorizonQuaterValue * 1.5f)            // -0.375
        iHorizontalPart = 0;
    else if (fVerticesPosX < -fHorizonQuaterValue * 0.5f)       // -0.125
        iHorizontalPart = 1;
    else if (fVerticesPosX < fHorizonQuaterValue * 0.5f)        // 0.125
        iHorizontalPart = 2;
    else
        iHorizontalPart = 3;
    
    int iVerticalPart = (fVerticesPosY >= 0.0f) ? 1 : 0;
    int iPartIndex = iHorizontalPart + (iVerticalPart * 4); // ( 0 ~ 3 ) * 0 / ( 0 ~ 3 ) * 1 에다가 * 4  => 0 ~ 8
    
    float3 vMoveDir = float3(0, 1, 0);
    
    switch (iPartIndex)
    {
        case 0:
            vMoveDir = float3(-1, -1, -1); // 대각 1
            break;
        case 1:
            vMoveDir = float3(-1, -1, 0); // 옆 1
            break;
        case 2:
            vMoveDir = float3(1, -1, 0); // 옆 2
            break;
        case 3:
            vMoveDir = float3(1, -1, -1); // 대각 2
            break;
        case 4:
            vMoveDir = float3(-1, 1, 1); // 대각 3
            break;
        case 5:
            vMoveDir = float3(-1, 1, 0); // 옆 3
            break;
        case 6:
            vMoveDir = float3(1, 1, 0); // 옆 4
            break;
        case 7:
            vMoveDir = float3(1, 1, 1); // 대각 4
            break;
    }
    vMoveDir = normalize(vMoveDir);

    // 모델의 정점들의 이동량
    float3 vMoveValue = vMoveDir * (g_fExplosionPower * 0.8f) * g_ExplosionPowerOffset;

    float fFallingTime = g_fFallingTime;
    float fGravityPower = -9.8f;
    
    // 초기 튀어오르는(점프느낌) 속도 
    float fInitialPopVelocity = g_InitialJumpPower;
    //  V0 * t  + (0.5) * g * t^2
    float fFallingValue = (fInitialPopVelocity * fFallingTime) + (0.4f) * (fGravityPower * fFallingTime * fFallingTime);
    vMoveValue.y += fFallingValue;

    // 0.25 * ( 0 - 1.5 )
    // 오프셋 
    float fHorizonOffset = fHorizonQuaterValue * (iHorizontalPart - 1.5f); // -0.375 ~ +0.375
    float fVerticalOffset = (iVerticalPart == 0) ? -0.25f : 0.25f;
    
    // 파편의 중심(오프셋 적용)
    float3 vPartCenter = float3(g_ModelPosition.x + fHorizonOffset, g_fModelHeightCenterY + fVerticalOffset, g_ModelPosition.z);

    // 회전 축( 각각 따로 ) 
    float3 vRotationAxis = float3(1, 0, 0);
    switch (iPartIndex)
    {
        case 0:
            vRotationAxis = float3(1, 0, 0);
            break; // X
        case 1:
            vRotationAxis = float3(0, 1, 0);
            break; // Y
        case 2:
            vRotationAxis = float3(0, 0, 1);
            break; // Z
        case 3:
            vRotationAxis = normalize(float3(1, 1, 0));
            break; // XY
        case 4:
            vRotationAxis = normalize(float3(1, 0, 1));
            break; // XZ
        case 5:
            vRotationAxis = normalize(float3(0, 1, 1));
            break; // YZ
        case 6:
            vRotationAxis = normalize(float3(-1, 1, 0));
            break; // -X,Y
        case 7:
            vRotationAxis = normalize(float3(1, -1, 0));
            break; // X,-Y
    }

    float fRotationSpeed = 3.0f;
    float fAngle = g_fFallingTime * fRotationSpeed;

    // 로드리게스 회전
    // https://mathworld.wolfram.com/RodriguesRotationFormula.html
    float fCosAngle = cos(fAngle);
    float fSinAngle = sin(fAngle);

    float fRotationX = vRotationAxis.x;
    float fRotationY = vRotationAxis.y;
    float fRotationZ = vRotationAxis.z;

    float3x3 matRotation =
    {
        fCosAngle + ((fRotationX * fRotationX) * (1 - fCosAngle)),
        ((1 - fCosAngle) * fRotationX * fRotationY) - (fSinAngle * fRotationZ),
        ((1 - fCosAngle) * fRotationX * fRotationZ) + (fSinAngle * fRotationY),
        
        
        ((1 - fCosAngle) * fRotationX * fRotationY) + (fSinAngle * fRotationZ),
        fCosAngle + (fRotationY * fRotationY * (1 - fCosAngle)),
        (-fRotationX * fSinAngle) + (fRotationY * fRotationZ * (1 - fCosAngle)),
        
        
        ((1 - fCosAngle) * fRotationX * fRotationZ) - (fRotationY * fSinAngle),
        (fRotationX * fSinAngle) + (fRotationY * fRotationZ * (1 - fCosAngle)),
        fCosAngle + ((fRotationZ * fRotationZ) * (1 - fCosAngle))
    };

    for (int i = 0; i < 3; ++i)
    {
        GS_OUT_DESTRUCT Out = (GS_OUT_DESTRUCT) 0;

        Out.vTexcoord = input[i].vTexcoord;
        Out.vNormal = input[i].vNormal;
        Out.vTangent = input[i].vTangent;
        Out.vBinormal = input[i].vBinormal;
        Out.vProjPos = input[i].vProjPos;

        float fVerticesYPos = input[0].vWorldPos.y + vMoveValue.y;
        float3 vResultMoveValue = vMoveValue;

        if (fVerticesYPos <= g_ModelPosition.y)
        {
            float fCurrentVerticesYpos = input[0].vWorldPos.y;
            vResultMoveValue.y = g_ModelPosition.y - fCurrentVerticesYpos;
        }

        bool bIsDissolve = ((input[i].vWorldPos.y + vResultMoveValue.y) <= g_ModelPosition.y);
        Out.fDissolve = bIsDissolve ? 1.0f : 0.0f;

        float3 vVerticesPosOffset = input[i].vWorldPos.xyz - vPartCenter;
        float3 vRotationOffset = mul(vVerticesPosOffset, matRotation);
        float3 vResultVerticesPos = vPartCenter + vRotationOffset + vResultMoveValue;

        Out.vWorldPos = float4(vResultVerticesPos, 1.0f);
        Out.vPosition = mul(mul(Out.vWorldPos, g_ViewMatrix), g_ProjMatrix);
        

        triStream.Append(Out);
    }
    
    triStream.RestartStrip();
}

[maxvertexcount(3)]
void GS_MAIN_PARTICLE(triangle GS_IN_DESTRUCT input[3], inout TriangleStream<GS_OUT_DESTRUCT> triStream)
{
    float3 vMoveDir = HashDirection(input[0].vWorldPos.xyz);

    float fRandomPerlinNoiseSeedNum = frac(sin(dot(input[0].vWorldPos.xyz, float3(12.9898, 78.233, 45.164))) * 43758.5453);
    float fMoveScale = lerp(0.8f, 1.5f, fRandomPerlinNoiseSeedNum) * 2.5f;

    float3 vMoveValue = vMoveDir * (g_fExplosionPower * fMoveScale);

    float fFallingTime = g_fFallingTime;
    float fGravityPower = -9.8f;
    
    float fInitialPopVelocity = 0.45f;

    float fFallingValue = (fInitialPopVelocity * fFallingTime) + (0.4f) * (fGravityPower * fFallingTime * fFallingTime);
    vMoveValue.y += fFallingValue;
    
    float fAngle = fFallingTime * lerp(3.0f, 6.0f, fRandomPerlinNoiseSeedNum);
    float fSinAngle = sin(fAngle);
    float fCosAngle = cos(fAngle);

    float3x3 matRotation = float3x3(
    
    fCosAngle, -fSinAngle, 0,
    fSinAngle, fCosAngle, 0,
    0, 0, 1
    );

    float3 vVerticesCenterPos = (
        input[0].vWorldPos.xyz +
        input[1].vWorldPos.xyz +
        input[2].vWorldPos.xyz
    ) / 3.0f;

    for (int i = 0; i < 3; ++i)
    {
        GS_OUT_DESTRUCT Out = (GS_OUT_DESTRUCT) 0;

        Out.vTexcoord = input[i].vTexcoord;
        Out.vNormal = input[i].vNormal;
        Out.vTangent = input[i].vTangent;
        Out.vBinormal = input[i].vBinormal;
        Out.vProjPos = input[i].vProjPos;

        float3 vPos = input[i].vWorldPos.xyz - vVerticesCenterPos;
        float3 vRotatedPos = mul(vPos, matRotation);
        float3 vResultPos = vVerticesCenterPos + vRotatedPos;

        float3 vResultMoveValue = vMoveValue;

        float fNewPosY = input[i].vWorldPos.y + vMoveValue.y;
        if (fNewPosY <= g_ModelPosition.y)
        {
            float fCurrentVerticesY = input[i].vWorldPos.y;
            vResultMoveValue.y = g_ModelPosition.y - fCurrentVerticesY;
        }
      
        bool bShouldDissolve = ((input[i].vWorldPos.y + vResultMoveValue.y) <= g_ModelPosition.y);
        Out.fDissolve = bShouldDissolve ? 1.0f : 0.0f;

        Out.vWorldPos = float4(vResultPos + vResultMoveValue, 1.0f);
        Out.vPosition = mul(Out.vWorldPos, mul(g_ViewMatrix, g_ProjMatrix));

        triStream.Append(Out);
    }
    triStream.RestartStrip();
}


[maxvertexcount(3)]
void GS_MAIN_SPORE(triangle GS_IN_DESTRUCT input[3], inout TriangleStream<GS_OUT_DESTRUCT> triStream)
{
    float fWiggleTime = g_WigglingTime;
    
    for (int i = 0; i < 3; ++i)
    {
        GS_OUT_DESTRUCT Out = (GS_OUT_DESTRUCT) 0;
        
        Out.vTexcoord = input[i].vTexcoord;
        
        Out.vNormal = input[i].vNormal;
        Out.vTangent = input[i].vTangent;
        Out.vBinormal = input[i].vBinormal;
        
        float3 vWorldPos = input[i].vWorldPos.xyz;
        float3 vLocalOffset = vWorldPos - g_ModelPosition.xyz;
        
        float fFarOffsetValue = (vLocalOffset.x + vLocalOffset.z) * 2.0f;
        float fWiggleValue = sin(fWiggleTime * 10.0f + fFarOffsetValue) * 0.02f;
        
        vLocalOffset += input[i].vNormal.xyz * fWiggleValue;
        
        float3 vFinalPos = g_ModelPosition.xyz + vLocalOffset;
        
        Out.vWorldPos = float4(vFinalPos, 1);
        Out.vPosition = mul(Out.vWorldPos, mul(g_ViewMatrix, g_ProjMatrix));
        Out.vProjPos = input[i].vProjPos;
        triStream.Append(Out);
    }
    triStream.RestartStrip();
}

// ============================================================= 픽셀 쉐이더 부문 =============================================================

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

PS_OUT_DESTRUCT PS_MAIN(PS_IN_DESTRUCT In)
{
    PS_OUT_DESTRUCT Out = (PS_OUT_DESTRUCT) 0;

    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
	
    float vDissolveAlpha = g_DissolveTexture.Sample(LinearSampler, In.vTexcoord).r;
    
    if (vDissolveAlpha < g_DissolveAmount)
    {
        clip(-1);
    }
    
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


PS_OUT_DESTRUCT PS_MAIN_DESTRUCT(PS_IN_DESTRUCT In)
{
    PS_OUT_DESTRUCT Out = (PS_OUT_DESTRUCT) 0;

    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
	
    if (In.fDissolve > 0.1f)
    {
        float vDissolveAlpha = g_DissolveTexture.Sample(LinearSampler, In.vTexcoord).r;

        if (vDissolveAlpha < g_DissolveAmount)
        {
            clip(-1);
        }
    }
    
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

PS_OUT_DESTRUCT PS_MAIN_DESTRUCT_SPIKES(PS_IN_DESTRUCT In)
{
    PS_OUT_DESTRUCT Out = (PS_OUT_DESTRUCT) 0;

    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
	
    float4 vNormalDesc = g_NormalTexture.Sample(LinearSampler, In.vTexcoord);
    float fPower = 3.f;
 
    
    float2 vTexcoord = float2(In.vTexcoord.x, In.vTexcoord.y + 0.08);

    float fEmissionStrength = g_EmissiveTexture.Sample(LinearSampler, vTexcoord).a;
    
    float3 vEmissionColor = float3(1.0f, 0.07f, 0.07f);
    float3 vEmission = vEmissionColor * fEmissionStrength * fPower;

    float3 finalColor = vMtrlDiffuse.rgb + vEmission;

    float3 vNormal = vNormalDesc.xyz * 2.f - 1.f;

    float3x3 WorldMatrix = float3x3(In.vTangent.xyz, In.vBinormal.xyz, In.vNormal.xyz);
    vNormal = normalize(mul(vNormal, WorldMatrix));
	
    
    Out.vDiffuse.rgb = finalColor;
    Out.vDiffuse.a = vMtrlDiffuse.a;
    Out.vNormal = vector(vNormal * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w, 0.f, 0.f);
    Out.fSpecular = 1.f;

    return Out;
}


technique11 DefaultTechnique
{
    pass BarrelFencePass // 0 
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN_DESTRUCT();
        GeometryShader = compile gs_5_0 GS_MAIN_DESTRUCT();
        PixelShader = compile ps_5_0 PS_MAIN_DESTRUCT();
    }

    pass RockPass // 1
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN_DESTRUCT();
        GeometryShader = compile gs_5_0 GS_MAIN_PARTICLE();
        PixelShader = compile ps_5_0 PS_MAIN_DESTRUCT();
    }

    pass SporePass // 2
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN_DESTRUCT();
        GeometryShader = compile gs_5_0 GS_MAIN_SPORE();
        PixelShader = compile ps_5_0 PS_MAIN_DESTRUCT();
    }

    pass PartSporePass // 3
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass BatSpikeType2Pass // 4
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN_DESTRUCT();
        GeometryShader = compile gs_5_0 GS_MAIN_PARTICLE();
        PixelShader = compile ps_5_0 PS_MAIN_DESTRUCT_SPIKES();
    }
}
