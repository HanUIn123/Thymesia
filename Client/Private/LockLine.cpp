#include "pch.h"
#include "LockLine.h"
#include "GameInstance.h"

CLockLine::CLockLine(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{
}

CLockLine::CLockLine(const CLockLine& Prototype)
    : CGameObject(Prototype)
{
}

HRESULT CLockLine::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CLockLine::Initialize(void* pArg)
{
    CLockLine::LOCKLINE_DESC* pDesc = static_cast<CLockLine::LOCKLINE_DESC*>(pArg);

    strcpy_s(m_szName, "LOCKLINE");

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    m_iCurrentLevel = static_cast<LEVELID>(pDesc->iCurLevel);

    m_pPlayer = static_cast<CPlayer*>(m_pGameInstance->Get_GameObject_To_Layer(m_iCurrentLevel, TEXT("Layer_Player"), "PLAYER"));

    return S_OK;
}

void CLockLine::Priority_Update(_float fTimeDelta)
{
    _float4 vPos;
    XMStoreFloat4(&vPos, m_pPlayer->Get_Transfrom()->Get_State(CTransform::STATE_POSITION));

    m_vStartPos = vPos;
    m_vStartPos.y += 0.85f;

    if (nullptr == m_pTargetMonsterPtr && m_pPlayer->Get_Lockon())
    {
        m_pTargetMonsterPtr = dynamic_cast<CContainerObject*>(m_pPlayer->Get_TargetObjectPtr());
    }
    else if (!m_pPlayer->Get_Lockon())
    {
        m_pTargetMonsterPtr = nullptr;
        return;
    }

    m_pTransformCom->Set_State(CTransform::STATE_POSITION, XMLoadFloat4(&m_vStartPos));
}

void CLockLine::Update(_float fTimeDelta)
{
    if (nullptr == m_pTargetMonsterPtr)
        return;

    m_fTime += fTimeDelta;

    _float4x4 fMatrix = dynamic_cast<CContainerObject*>(m_pPlayer->Get_TargetObjectPtr())->Find_PartObject(TEXT("Part_Locked_On"))->Get_CombineWorldMatrix();

    _float4 vEndPosition = _float4(fMatrix._41, fMatrix._42, fMatrix._43, fMatrix._44);
    _vector vEndPos = XMLoadFloat4(&vEndPosition);

    vEndPos = XMVectorSetY(vEndPos, XMVectorGetY(vEndPos));
    _vector vDir = vEndPos - XMLoadFloat4(&m_vStartPos);
    _float fLength = XMVectorGetX(XMVector3Length(vDir));
    vDir = XMVector3Normalize(vDir);
    _vector vCenter = (XMLoadFloat4(&m_vStartPos) + vEndPos) * 0.5f;

    _vector vRight = vDir;
    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vLook = XMVector3Normalize(XMVector3Cross(vUp, vRight));
    vUp = XMVector3Normalize(XMVector3Cross(vRight, vLook));

    static XMVECTOR s_CurrentQuat = XMQuaternionIdentity();

    float fTargetAngle = 0.f;

    if (*(m_pPlayer->Get_State_Ptr()) == CPlayer::STATE_LOCK_ON_RUN_L ||
        *(m_pPlayer->Get_State_Ptr()) == CPlayer::STATE_LOCK_ON_RUN_BL ||
        *(m_pPlayer->Get_State_Ptr()) == CPlayer::STATE_LOCK_ON_RUN_FL)
    {
        fTargetAngle = -22.5f;
    }
    else if (*(m_pPlayer->Get_State_Ptr()) == CPlayer::STATE_LOCK_ON_RUN_R ||
        *(m_pPlayer->Get_State_Ptr()) == CPlayer::STATE_LOCK_ON_RUN_BR ||
        *(m_pPlayer->Get_State_Ptr()) == CPlayer::STATE_LOCK_ON_RUN_FR)
    {
        fTargetAngle = -157.5f;
    }
    else
    {
        fTargetAngle = -67.5f;
    }

    XMVECTOR vTargetQuat = XMQuaternionRotationAxis(vRight, XMConvertToRadians(fTargetAngle));

    float fLerpSpeed = 0.1f;
    s_CurrentQuat = XMQuaternionSlerp(s_CurrentQuat, vTargetQuat, fLerpSpeed);

    _matrix RotationMatrix = XMMatrixRotationQuaternion(s_CurrentQuat);


    // x축으로 -45도 만큼 돌려버려 그럼 좀 세워짐 ㅇㅅㅇ
    //_matrix RotationMatrix = XMMatrixRotationAxis(vRight, XMConvertToRadians(-45.f));

    vUp = XMVector3TransformNormal(vUp, RotationMatrix);
    vLook = XMVector3TransformNormal(vLook, RotationMatrix);

    m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
    m_pTransformCom->Set_State(CTransform::STATE_LOOK, vLook);
    m_pTransformCom->Set_State(CTransform::STATE_POSITION, vCenter);

    m_pTransformCom->Scaling(_float3(0.01f * fLength, 0.01f, 0.01f));

    m_fLineLength = fLength;
}

void CLockLine::Late_Update(_float fTimeDelta)
{
    if (nullptr == m_pTargetMonsterPtr)
        return;


    m_pGameInstance->Add_RenderGroup(CRenderer::RG_GLOW, this);
}

HRESULT CLockLine::Render()
{
    if (nullptr == m_pTargetMonsterPtr)
        return S_OK;

    if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    _float4x4   ViewMatrix, ProjMatrix;
    ViewMatrix = m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_VIEW);
    ProjMatrix = m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_PROJ);

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &ViewMatrix)))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &ProjMatrix)))
        return E_FAIL;

    _uint iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (_uint i = 0; i < iNumMeshes; ++i)
    {
        if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_DIFFUSE, "g_DiffuseTexture", 0)))
            return E_FAIL;

        m_pShaderCom->Begin(16);

        m_pModelCom->Render(i);
    }

    if (FAILED(m_pShaderCom->Bind_RawValue("g_fLineLength", &m_fLineLength, sizeof(_float))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_Time", &m_fTime, sizeof(_float))))
        return E_FAIL;

    return S_OK;
}

HRESULT CLockLine::Render_Glow()
{
    if (nullptr == m_pTargetMonsterPtr)
        return S_OK;

    if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    _float4x4   ViewMatrix, ProjMatrix;
    ViewMatrix = m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_VIEW);
    ProjMatrix = m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_PROJ);

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &ViewMatrix)))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &ProjMatrix)))
        return E_FAIL;

    _uint iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (_uint i = 0; i < iNumMeshes; ++i)
    {
        if (FAILED(m_pTextureCom->Bind_ShaderResource(m_pShaderCom, "g_LinePointTexture", 0)))
            return E_FAIL;

        m_pShaderCom->Begin(16);

        m_pModelCom->Render(i);
    }

    if (FAILED(m_pShaderCom->Bind_RawValue("g_fLineLength", &m_fLineLength, sizeof(_float))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_Time", &m_fTime, sizeof(_float))))
        return E_FAIL;

    return S_OK;
}

HRESULT CLockLine::Ready_Components()
{
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxMesh"), TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom))))
        return E_FAIL;

    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Model_LockLine"), TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
        return E_FAIL;

    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Texture_Line"), TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom))))
        return E_FAIL;


    return S_OK;
}

CLockLine* CLockLine::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CLockLine* pInstance = new CLockLine(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CLockLine");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CLockLine::Clone(void* pArg)
{
    CLockLine* pInstance = new CLockLine(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CLockLine");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CLockLine::Free()
{
    __super::Free();

    Safe_Release(m_pShaderCom);
    Safe_Release(m_pModelCom);
    Safe_Release(m_pTextureCom);
}
