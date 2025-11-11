#include "pch.h"
#include "TriggerObject.h"
#include "GameInstance.h"

CTriggerObject::CTriggerObject(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
    :CGameObject{ _pDevice, _pContext }
{
}

CTriggerObject::CTriggerObject(const CTriggerObject& _Prototype)
    :CGameObject(_Prototype)
{
}

HRESULT CTriggerObject::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CTriggerObject::Initialize(void* _pArg)
{
    TC_DESC* pDesc = static_cast<TC_DESC*>(_pArg);

    m_tagInfoTempCollider.fPosition = pDesc->fPosition;
    m_tagInfoTempCollider.fRotation = pDesc->fRotation;
    m_tagInfoTempCollider.fScale = pDesc->fScale;

    m_pActor = m_pGameInstance->Create_Actor(COLLIDER_TYPE::COLLIDER_BOX, _float3{ m_tagInfoTempCollider.fScale.y, m_tagInfoTempCollider.fScale.x, m_tagInfoTempCollider.fScale.z }, _float3{ 0.f,0.f,1.f }, 90.f, this);
    m_pGameInstance->Set_GlobalPos(m_pActor, _fvector{ 0.f,0.f,0.f,1.f });
    _uint settingColliderGroup = GROUP_TYPE::PLAYER;
    m_pGameInstance->Set_CollisionGroup(m_pActor, GROUP_TYPE::OBJECT, settingColliderGroup);
    m_pGameInstance->Add_Actor_Scene(m_pActor);

    if (FAILED(__super::Initialize(_pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    m_pTransformCom->Set_State(CTransform::STATE_POSITION, XMVectorSet(m_tagInfoTempCollider.fPosition.x, m_tagInfoTempCollider.fPosition.y, m_tagInfoTempCollider.fPosition.z, m_tagInfoTempCollider.fPosition.w));

    return S_OK;
}

void CTriggerObject::Priority_Update(_float _fTimeDelta)
{
}

void CTriggerObject::Update(_float _fTimeDelta)
{
    if (SUCCEEDED(m_pGameInstance->IsActorInScene(m_pActor)))
        m_pGameInstance->Update_Collider(m_pActor, XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrix_Ptr()), _vector{ 0.f, 0.5f,0.f,1.f });
}

void CTriggerObject::Late_Update(_float _fTimeDelta)
{
}

HRESULT CTriggerObject::Render()
{
    return S_OK;
}

HRESULT CTriggerObject::Ready_Components()
{
    return S_OK;
}

void CTriggerObject::OnCollisionEnter(CGameObject* _pOther, PxContactPair _information)
{
    m_pGameInstance->Set_bInWater();
}

void CTriggerObject::OnCollision(CGameObject* _pOther, PxContactPair _information)
{
}

void CTriggerObject::OnCollisionExit(CGameObject* _pOther, PxContactPair _information)
{
}

CTriggerObject* CTriggerObject::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
{
    CTriggerObject* pInstance = new CTriggerObject(_pDevice, _pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CTriggerObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CTriggerObject::Clone(void* _pArg)
{
    CTriggerObject* pInstance = new CTriggerObject(*this);

    if (FAILED(pInstance->Initialize(_pArg)))
    {
        MSG_BOX("Failed To Cloned : CTriggerObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTriggerObject::Free()
{
    __super::Free();
    m_pGameInstance->Sub_Actor_Scene(m_pActor);
}
