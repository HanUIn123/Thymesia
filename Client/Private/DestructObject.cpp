#include "pch.h"
#include "DestructObject.h"
#include "GameInstance.h"
#include "Player.h"

static _uint g_iCount_WhiteItem = 0;

CDestructObject::CDestructObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    :CGameObject{ pDevice, pContext }
{
}

CDestructObject::CDestructObject(const CDestructObject& Prototype)
    :CGameObject(Prototype)
{
}

HRESULT CDestructObject::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CDestructObject::Initialize(void* pArg)
{
    strcpy_s(m_szName, "DESTRUCT");

    DestructObject_Desc* pDesc = static_cast<DestructObject_Desc*>(pArg);

    strcpy_s(m_szIndivisualName, pDesc->ObjectName.c_str());

    m_fFrustumRadius = pDesc->fFrustumRadius;

    m_fModelPos = pDesc->fPosition;

    if (!strcmp(m_szIndivisualName, "barrel"))
        m_fModelHeightCenterY = m_fModelPos.y + 0.44f;
    else if (!strcmp(m_szIndivisualName, "Rock"))
        m_fModelHeightCenterY = m_fModelPos.y + 0.125f;
    else if (!strcmp(m_szIndivisualName, "Fence2"))
        m_fModelHeightCenterY = m_fModelPos.y + 0.41f;

    m_iCurrentLevel = pDesc->iCurLevel;


    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    m_pTransformCom->Rotation(pDesc->fRotation.x, pDesc->fRotation.y, pDesc->fRotation.z, pDesc->fRotation.w);

    if (!strcmp(m_szIndivisualName, "barrel") || !strcmp(m_szIndivisualName, "Rock"))
        m_pActor = m_pGameInstance->Create_Actor(COLLIDER_TYPE::COLLIDER_SPHERE, _float3{ 0.5f,0.5f, 0.1f }, _float3{ 0.f,0.f,1.f }, 90.f, this);
    else
        m_pActor = m_pGameInstance->Create_Actor(COLLIDER_TYPE::COLLIDER_BOX, _float3{ 0.5f,0.2f, 1.15f }, _float3{ 0.f,0.f,1.f }, 90.f, this);

    _uint iSettingColliderGroup = GROUP_TYPE::PLAYER_WEAPON | GROUP_TYPE::PLAYER;   
    m_pGameInstance->Set_GlobalPos(m_pActor, _fvector{ 0.f,20.f,0.f,1.f });
    m_pGameInstance->Set_CollisionGroup(m_pActor, GROUP_TYPE::DESTRUCT, iSettingColliderGroup);
    m_pGameInstance->Add_Actor_Scene(m_pActor);

    return S_OK;
}

void CDestructObject::Priority_Update(_float fTimeDelta)
{
}

void CDestructObject::Update(_float fTimeDelta)
{
    if (!m_bUpdating)
    {
        if (m_bHitted)
        {
            _float fY = XMVectorGetY(XMLoadFloat4(&m_fModelPos));
            _float4 vItemPos = {};
            XMStoreFloat4(&vItemPos, XMVectorSetY(XMLoadFloat4(&m_fModelPos), fY + 0.5f));

            if (!strcmp(m_szIndivisualName, "barrel") && !m_bDropped)
            {
                switch (g_iCount_WhiteItem)
                {
                case 0:
                    m_pGameInstance->Pop_Item(ITEM_TYPE::ITEM_BADZOKER, XMLoadFloat4(&vItemPos), this, 1);
                    g_iCount_WhiteItem++;
                    break;
                case 1:
                    m_pGameInstance->Pop_Item(ITEM_TYPE::ITEM_BEOMSEUNG, XMLoadFloat4(&vItemPos), this, 1);
                    g_iCount_WhiteItem++;
                    break;
                case 2:
                    m_pGameInstance->Pop_Item(ITEM_TYPE::ITEM_FAKER, XMLoadFloat4(&vItemPos), this, 1);
                    g_iCount_WhiteItem++;
                    break;
                case 3:
                    m_pGameInstance->Pop_Item(ITEM_TYPE::ITEM_UIN, XMLoadFloat4(&vItemPos), this, 1);
                    g_iCount_WhiteItem++;
                    break;
                case 4:
                    m_pGameInstance->Pop_Item(ITEM_TYPE::ITEM_PLAYERMASTER, XMLoadFloat4(&vItemPos), this, 1);
                    g_iCount_WhiteItem++;
                    break;
                case 5:
                    m_pGameInstance->Pop_Item(ITEM_TYPE::ITEM_YUBHIN, XMLoadFloat4(&vItemPos), this, 1);
                    g_iCount_WhiteItem++;
                    break;
                default:
                    m_pGameInstance->Drop_Item(ITEM_TYPE::ITEM_MEMORY, XMLoadFloat4(&vItemPos), this, 1);
                    break;
                }

                m_bDropped = true;

            }

            m_fLifeTime += fTimeDelta;
            m_pGameInstance->Sub_Actor_Scene(m_pActor);
            m_fExplosionPower += fTimeDelta * 3.0f;
            m_fFallingTime += fTimeDelta;

            if (m_fLifeTime >= 1.0f)
            {
                m_bUpdating = true;
            }
        }

        if (SUCCEEDED(m_pGameInstance->IsActorInScene(m_pActor)))
            m_pGameInstance->Update_Collider(m_pActor, XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrix_Ptr()), _vector{ 0.f, 30.f,0.f,1.f });
    }

    if (m_bUpdating)
    {
        m_fDissolveAmount += fTimeDelta * 1.2f;

        if (m_fDissolveAmount >= 1.0f)
        {
            m_pGameInstance->Add_DeadObject(TEXT("Layer_DestructObject"), this);
        }
    }
}

void CDestructObject::Late_Update(_float fTimeDelta)
{
    m_pGameInstance->Add_RenderGroup(CRenderer::RG_NONBLEND, this);
    //m_pGameInstance->Add_RenderGroup(CRenderer::RG_OCCULUSION, this);
}

HRESULT CDestructObject::Render()
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

        if (!strcmp(m_szIndivisualName, "barrel") || (!strcmp(m_szIndivisualName, "Fence2")))
            m_pShaderCom->Begin(0);
        else
            m_pShaderCom->Begin(1);

        m_pModelCom->Render(i);
    }

    return S_OK;
}

HRESULT CDestructObject::Ready_Components()
{
    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxDestructMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom))))
        return E_FAIL;

    string strComponentName = "Prototype_Component_Model_";
    strComponentName += m_szIndivisualName;
    _tchar		szComponentName[MAX_PATH] = {};

    MultiByteToWideChar(CP_ACP, 0, strComponentName.c_str(), static_cast<int>(strlen(strComponentName.c_str())), szComponentName, MAX_PATH);

    if (FAILED(__super::Add_Component(LEVEL_STATIC, szComponentName, TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
        return E_FAIL;


    if (FAILED(__super::Add_Component(LEVEL_STATIC, TEXT("Prototype_Component_Texture_TestFenceNoiseTexture"), TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom))))
        return E_FAIL;

    return S_OK;
}

HRESULT CDestructObject::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_VIEW))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_PROJ))))
        return E_FAIL;

    // =========================== 지메쉐로 드가자아잇 ===========================

    // 얼마나 세게
    if (FAILED(m_pShaderCom->Bind_RawValue("g_fExplosionPower", &m_fExplosionPower, sizeof(_float))))
        return E_FAIL;

    // 얼마나 오래 떨어질꺼?
    if (FAILED(m_pShaderCom->Bind_RawValue("g_fFallingTime", &m_fFallingTime, sizeof(_float))))
        return E_FAIL;

    // 모델의 위치점 (처음 피킹 배치 될 때 그 x y z 를 지메쉐에서 나눠서 쓸 꺼임
    if (FAILED(m_pShaderCom->Bind_RawValue("g_ModelPosition", &m_fModelPos, sizeof(_float4))))
        return E_FAIL;

    // 모델의 y 중심점 (등분 할 때 쓸꺼임) 
    if (FAILED(m_pShaderCom->Bind_RawValue("g_fModelHeightCenterY", &m_fModelHeightCenterY, sizeof(_float))))
        return E_FAIL;

    // 디졸브 맥일꺼임 ㅋㅋ
    if (FAILED(m_pTextureCom->Bind_ShaderResource(m_pShaderCom, "g_DissolveTexture", 0)))
        return E_FAIL;

    // 디졸브 변화량 값임 ㅋㅋ
    if (FAILED(m_pShaderCom->Bind_RawValue("g_DissolveAmount", &m_fDissolveAmount, sizeof(_float))))
        return E_FAIL;

    return S_OK;
}

void CDestructObject::OnCollisionEnter(CGameObject* _pOther, PxContactPair _information)
{
    if (!strcmp("PLAYER_WEAPON", _pOther->Get_Name()) || !strcmp("PLAYER_PLAGUE_WEAPON", _pOther->Get_Name()))
    {
        if (!strcmp(m_szIndivisualName, "barrel"))
        {
            m_pGameInstance->Play_Sound(TEXT("WoodBreak_01.ogg"), CHANNELID::SOUND_STRUCT, 0.4f);
        }
        else if (!strcmp(m_szIndivisualName, "Fence2"))
        {
            m_pGameInstance->Play_Sound(TEXT("WoodBreak_02.ogg"), CHANNELID::SOUND_STRUCT, 0.4f);
        }
        else if (!strcmp(m_szIndivisualName, "Rock"))
            m_pGameInstance->Play_Sound(TEXT("Rock_Hit_01.ogg"), CHANNELID::SOUND_STRUCT, 0.4f);

        m_bHitted = true;
    }
    if (!strcmp("PLAYER", _pOther->Get_Name()))
    {
        if (static_cast<CPlayer*>(_pOther)->Get_PhaseState() & CPlayer::PHASE_DASH)
        {
            if (!strcmp(m_szIndivisualName, "barrel"))
            {
                m_pGameInstance->Play_Sound(TEXT("WoodBreak_01.ogg"), CHANNELID::SOUND_STRUCT, 0.4f);
            }
            else if (!strcmp(m_szIndivisualName, "Fence2"))
            {
                m_pGameInstance->Play_Sound(TEXT("WoodBreak_02.ogg"), CHANNELID::SOUND_STRUCT, 0.4f);
            }
            else if (!strcmp(m_szIndivisualName, "Rock"))
                m_pGameInstance->Play_Sound(TEXT("Rock_Hit_01.ogg"), CHANNELID::SOUND_STRUCT, 0.4f);

            m_bHitted = true;
        }
    }
}

void CDestructObject::OnCollision(CGameObject* _pOther, PxContactPair _information)
{
}

void CDestructObject::OnCollisionExit(CGameObject* _pOther, PxContactPair _information)
{
}

CDestructObject* CDestructObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CDestructObject* pInstance = new CDestructObject(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CDestructObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CDestructObject::Clone(void* pArg)
{
    CDestructObject* pInstance = new CDestructObject(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Created : CDestructObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CDestructObject::Free()
{
    __super::Free();

    m_pGameInstance->Sub_Actor_Scene(m_pActor);

    Safe_Release(m_pShaderCom);
    Safe_Release(m_pModelCom);
    Safe_Release(m_pTextureCom);
}



