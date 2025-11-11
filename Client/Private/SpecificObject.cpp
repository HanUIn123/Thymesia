#include "pch.h"
#include "SpecificObject.h"
#include "GameInstance.h"

CSpecificObject::CSpecificObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    :CGameObject{ pDevice, pContext }
{
}

CSpecificObject::CSpecificObject(const CSpecificObject& Prototype)
    :CGameObject(Prototype)
{
}

HRESULT CSpecificObject::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CSpecificObject::Initialize(void* pArg)
{
    SpecificObject_Desc* pDesc = static_cast<SpecificObject_Desc*>(pArg);

    m_iCurrentLevel = pDesc->iCurLevel;
    m_fFrustumRadius = pDesc->fFrustumRadius;

    m_fPosition = pDesc->fPosition;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    strcpy_s(m_szName, pDesc->ObjectName.c_str());

    if (FAILED(Ready_Components()))
        return E_FAIL;

    m_pTransformCom->Rotation(pDesc->fRotation.x, pDesc->fRotation.y, pDesc->fRotation.z, pDesc->fRotation.w);
    m_pButtonGameObject = m_pGameInstance->Get_GameObject_To_Layer(pDesc->iCurLevel, TEXT("Layer_Button"), "BUTTON");
    m_pButton = static_cast<CButton*>(m_pButtonGameObject);

    return S_OK;
}

void CSpecificObject::Priority_Update(_float fTimeDelta)
{
}

void CSpecificObject::Update(_float fTimeDelta)
{
    if (m_bColliderOn)
    {
        m_pGameInstance->Add_Actor_Scene(m_pActor);

        if (SUCCEEDED(m_pGameInstance->IsActorInScene(m_pActor)))
            m_pGameInstance->Update_Collider(m_pActor, XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrix_Ptr()), _vector{ 0.f, 0.f,0.f,1.f });
    }
    else
    {
        m_pGameInstance->Sub_Actor_Scene(m_pActor);
    }
}

void CSpecificObject::Late_Update(_float fTimeDelta)
{
    if (m_bFadingIn)
    {
        m_fAlphaValue += fTimeDelta * 2.0f;
        if (m_fAlphaValue >= 1.0f)
        {
            m_fAlphaValue = 1.0f;
            m_bFadingIn = false;
        }
    }

    if (m_bFadingOut)
    {
        m_fAlphaValue -= fTimeDelta * 2.0f;
        if (m_fAlphaValue <= 0.0f)
        {
            m_fAlphaValue = 0.0f;
            m_bFadingOut = false;
        }
    }

    m_pGameInstance->Add_RenderGroup(CRenderer::RG_NONBLEND, this);
    m_pGameInstance->Add_RenderGroup(CRenderer::RG_GLOW, this);
    
}

HRESULT CSpecificObject::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint			iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (_uint i = 0; i < iNumMeshes; i++)
    {
        if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_DIFFUSE, "g_DiffuseTexture", 0)))
            return E_FAIL;

        if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_NORMALS, "g_NormalTexture", 0)))
            return E_FAIL;

        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }

    return S_OK;
}

HRESULT CSpecificObject::Render_Glow()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint			iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (_uint i = 0; i < iNumMeshes; i++)
    {
        if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_DIFFUSE, "g_DiffuseTexture", 0)))
            return E_FAIL;

        if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_NORMALS, "g_NormalTexture", 0)))
            return E_FAIL;

        if (FAILED(m_pShaderCom->Bind_RawValue("g_fObjectAlpha", &m_fAlphaValue, sizeof(_float))))
            return E_FAIL;

        m_pShaderCom->Begin(11);

        m_pModelCom->Render(i);
    }

    return S_OK;
}

HRESULT CSpecificObject::Ready_Components()
{
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxMesh"), TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom))))
        return E_FAIL;

    string strModelBaseName = m_szName;
    size_t pos = strModelBaseName.find_last_of('_');

    if (pos != string::npos)
        strModelBaseName = strModelBaseName.substr(0, pos);

    string strComponentName = "Prototype_Component_Model_" + strModelBaseName;

    _tchar szComponentName[MAX_PATH] = {};
    MultiByteToWideChar(CP_ACP, 0, strComponentName.c_str(), static_cast<_int>(strComponentName.length()), szComponentName, MAX_PATH);

    if (FAILED(__super::Add_Component(LEVEL_STATIC, szComponentName, TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
        return E_FAIL;

    return S_OK;
}

HRESULT CSpecificObject::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_VIEW))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_PROJ))))
        return E_FAIL;

    return S_OK;
}

void CSpecificObject::Free()
{
    __super::Free();

    Safe_Release(m_pShaderCom);
    Safe_Release(m_pModelCom);
}
