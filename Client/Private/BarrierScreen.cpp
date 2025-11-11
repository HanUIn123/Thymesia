#include "pch.h"
#include "BarrierScreen.h"
#include "GameInstance.h"

CBarrierScreen::CBarrierScreen(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    :CGameObject{ pDevice, pContext }
{
}

CBarrierScreen::CBarrierScreen(const CBarrierScreen& Prototype)
    :CGameObject(Prototype)
{
}

HRESULT CBarrierScreen::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CBarrierScreen::Initialize(void* pArg)
{
    strcpy_s(m_szName, "BARRIERSCREEN");

    BARRIER_SCREEN_DESC* pDesc = static_cast<BARRIER_SCREEN_DESC*>(pArg);

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    m_iCurLevel = pDesc->iCurLevel;

    m_pTransformCom->Set_State(CTransform::STATE_POSITION, XMLoadFloat4(&pDesc->_fPosition));

    m_pTransformCom->Rotation(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(90.0f));

    if (m_iCurLevel == LEVELID::LEVEL_SEAOFTREES)
    {
        m_pActor = m_pGameInstance->Create_Actor(COLLIDER_TYPE::COLLIDER_BOX, _float3{ 5.2f, 0.4f, 5.5f }, _float3{ 0.f,0.f,1.f }, 90.f, this);
        m_pTransformCom->Scaling(_float3(0.001f, 0.0035f, 0.0065f));
    }
    else if (m_iCurLevel == LEVELID::LEVEL_TUTORIAL)
    {
        m_pActor = m_pGameInstance->Create_Actor(COLLIDER_TYPE::COLLIDER_BOX, _float3{ 5.2f, 0.4f, 5.5f }, _float3{ 0.f,0.f,1.f }, 90.f, this);
        m_pTransformCom->Scaling(_float3(0.001f, 0.0055f, 0.0055f));
    }
    else if(m_iCurLevel == LEVELID::LEVEL_ROYALGARDEN)
    {
        m_pActor = m_pGameInstance->Create_Actor(COLLIDER_TYPE::COLLIDER_BOX, _float3{ 5.2f, 0.4f, 5.5f }, _float3{ 0.f,0.f,1.f }, 90.f, this);
        m_pTransformCom->Scaling(_float3(0.001f, 0.0035f, 0.0065f));
    }

    _uint iSettingColliderGroup = GROUP_TYPE::PLAYER;
    m_pGameInstance->Set_GlobalPos(m_pActor, _fvector{ 0.f,20.f,0.f,1.f });
    m_pGameInstance->Set_CollisionGroup(m_pActor, GROUP_TYPE::ITEM, iSettingColliderGroup);

    return S_OK;
}

void CBarrierScreen::Priority_Update(_float fTimeDelta)
{
     if (m_pGameInstance->Get_Boss_Active())
     {
         m_bActivateBarrier = true;
     }
     else
     {
         m_bActivateBarrier = false;
     }
}

void CBarrierScreen::Update(_float fTimeDelta)
{
    if (m_bActivateBarrier)
    {
        m_fFlowTime += fTimeDelta;
        m_pGameInstance->Add_Actor_Scene(m_pActor);

        if (SUCCEEDED(m_pGameInstance->IsActorInScene(m_pActor)))
            m_pGameInstance->Update_Collider(m_pActor, XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrix_Ptr()), _vector{ 0.f, 0.f,0.f,1.f });
    }
    else
    {
        m_pGameInstance->Sub_Actor_Scene(m_pActor);
    }
}

void CBarrierScreen::Late_Update(_float fTimeDelta)
{
}

HRESULT CBarrierScreen::Render()
{
    return S_OK;
}

HRESULT CBarrierScreen::Render_Glow()
{
    if (!m_bActivateBarrier)
        return S_OK;

    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint			iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (_uint i = 0; i < iNumMeshes; i++)
    {
        if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_DIFFUSE, "g_DiffuseTexture", 0)))
            return E_FAIL;

        if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_NORMALS, "g_NormalTexture", 0)))
            return E_FAIL;

        m_pShaderCom->Begin(18);

        m_pModelCom->Render(i);
    }

    return S_OK;
}

HRESULT CBarrierScreen::Ready_Components()
{
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxMesh"), TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom))))
        return E_FAIL;

    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Texture_BarrierScreenTexture"), TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom))))
        return E_FAIL;

    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_GameObject_Model_BarrierScreen"), TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
        return E_FAIL;

    return S_OK;
}

HRESULT CBarrierScreen::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_VIEW))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_PROJ))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_Time", &m_fFlowTime, sizeof(_float))))
        return E_FAIL;

    if (FAILED(m_pTextureCom->Bind_ShaderResource(m_pShaderCom, "g_NoiseTexture", 0)))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_LevelID", &m_iCurLevel, sizeof(_uint))))
        return E_FAIL;

    return S_OK;
}

void CBarrierScreen::OnCollisionEnter(CGameObject* _pOther, PxContactPair _information)
{
}

void CBarrierScreen::OnCollision(CGameObject* _pOther, PxContactPair _information)
{
    m_pGameInstance->Add_RenderGroup(CRenderer::RG_GLOW, this);
}

void CBarrierScreen::OnCollisionExit(CGameObject* _pOther, PxContactPair _information)
{
}

CBarrierScreen* CBarrierScreen::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBarrierScreen* pInstance = new CBarrierScreen(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CBarrierScreen");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CBarrierScreen::Clone(void* pArg)
{
    CBarrierScreen* pInstance = new CBarrierScreen(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CBarrierScreen");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBarrierScreen::Free()
{
    __super::Free();

    m_pGameInstance->Sub_Actor_Scene(m_pActor);

    Safe_Release(m_pShaderCom);
    Safe_Release(m_pModelCom);
    Safe_Release(m_pTextureCom);
}
