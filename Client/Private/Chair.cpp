#include "pch.h"
#include "Chair.h"
#include "GameInstance.h"

CChair::CChair(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    :CSpecificObject{ pDevice, pContext }
{
}

CChair::CChair(const CChair& Prototype)
    :CSpecificObject(Prototype)
{
}

HRESULT CChair::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CChair::Initialize(void* pArg)
{
    CHAIR_DESC* pDesc = static_cast<CHAIR_DESC*>(pArg);

    string chairName = "P_Archive_Chair01_" + to_string(pDesc->iPairNum);

    m_iCurrentLevel = pDesc->iCurLevel;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    _vector vPosition = m_pTransformCom->Get_State(CTransform::STATE_POSITION);

    LIGHT_DESC LightDesc{};
    LightDesc.eType = LIGHT_DESC::TYPE_POINT;
    LightDesc.vDiffuse = _float4(0.f, 1.f, 0.f, 1.f);
    LightDesc.vAmbient = _float4(0.9f, 0.9f, 0.9f, 1.f);
    LightDesc.vSpecular = _float4(0.5f, 0.5f, 0.5f, 1.f);
    LightDesc.vPosition = _float4(XMVectorGetX(vPosition), XMVectorGetY(vPosition), XMVectorGetZ(vPosition), 1.f);
    LightDesc.fRange = 2.f;

    if (FAILED(m_pGameInstance->Add_Light(LightDesc)))
        return E_FAIL;

    m_pActor = m_pGameInstance->Create_Actor(COLLIDER_TYPE::COLLIDER_SPHERE, _float3{ 0.6f, 0.6f, 0.1f }, _float3{ 0.f,0.f,1.f }, 90.f, this);
    _uint iSettingColliderGroup = GROUP_TYPE::PLAYER;
    m_pGameInstance->Set_GlobalPos(m_pActor, _fvector{ 0.f,20.f,0.f,1.f });
    m_pGameInstance->Set_CollisionGroup(m_pActor, GROUP_TYPE::OBJECT, iSettingColliderGroup);


    return S_OK;
}

void CChair::Priority_Update(_float fTimeDelta)
{
    __super::Priority_Update(fTimeDelta);
}

void CChair::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);
}

void CChair::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
}

HRESULT CChair::Render()
{
    if (FAILED(__super::Render()))
        return E_FAIL;

    return S_OK;
}

HRESULT CChair::Render_Glow()
{
    if (FAILED(__super::Render_Glow()))
        return E_FAIL;

    return S_OK;
}

void CChair::OnCollisionEnter(CGameObject* _pOther, PxContactPair _information)
{
    m_iColliderCount++;

    if (!strncmp(m_szName, "P_Archive_Chair01", 17))
    {
        _vector vChairPos = m_pTransformCom->Get_State(CTransform::STATE_POSITION);
        vChairPos = XMVectorSetY(vChairPos, XMVectorGetY(vChairPos) + 1.0f);

        _float4 vChairPosition;
        XMStoreFloat4(&vChairPosition, vChairPos);

        m_pButton->Set_WorldPosition(vChairPosition);
        m_pButton->Set_ButtonText(TEXT("E"), TEXT("¾É±â"));
        m_pButton->Activate_Button(true);
        m_bInteractOn = true;
        m_bFadingIn = true;
        m_bFadingOut = false;

        m_pGosemy = dynamic_cast<CGhostAisemy*>(m_pGameInstance->Get_GameObject_To_Layer(m_iCurrentLevel, TEXT("Layer_NPC"), "GHOSEMY"));
        m_pBodyGosemy = m_pGosemy->Get_SemyBody();

        if (nullptr != m_pBodyGosemy)
            m_pBodyGosemy->Activate_SemyBody(true);

        _float4 vGosemyPos = {};
        _float fY = XMVectorGetY(vChairPos);
        vChairPos = XMVectorSetY(vChairPos, fY - 1.0f);
        XMStoreFloat4(&vGosemyPos, vChairPos);

        if (m_iColliderCount <= 1)
            m_pGosemy->Spawn_Gosemy(vGosemyPos, true);
        else
            m_pGosemy->Spawn_Gosemy(vGosemyPos, false);

        _vector vChairLook = m_pTransformCom->Get_State(CTransform::STATE_LOOK);
        _vector vChairUp = m_pTransformCom->Get_State(CTransform::STATE_UP);
        _vector vChairRight = m_pTransformCom->Get_State(CTransform::STATE_RIGHT);

        m_pGosemy->Get_Transfrom()->Set_State(CTransform::STATE_LOOK, vChairLook);
        m_pGosemy->Get_Transfrom()->Set_State(CTransform::STATE_UP, vChairUp);
        m_pGosemy->Get_Transfrom()->Set_State(CTransform::STATE_RIGHT, vChairRight);

        m_pGosemy->Get_Transfrom()->Scaling(_float3(0.002f, 0.002f, 0.002f));
    }
}

void CChair::OnCollision(CGameObject* _pOther, PxContactPair _information)
{
    if (m_pGameInstance->isKeyEnter(DIK_E) && !strncmp(m_szName, "P_Archive_Chair01", 17))
    {
        m_pGameInstance->UIGroup_Render_OnOff(LEVEL_STATIC, TEXT("Layer_Mouse"), true);
        m_pGameInstance->UIGroup_Render_OnOff(m_iCurrentLevel, TEXT("Layer_PlayerScreen"), false);
        m_pGameInstance->UIScene_UIObject_Render_OnOff((m_pGameInstance->Find_UIScene(UISCENE_PLAYERSCREEN, L"UIScene_PlayerScreen")), false);
        m_pGameInstance->UIGroup_Render_OnOff(m_iCurrentLevel, TEXT("Layer_PlayerMenu"), true);
        m_pGameInstance->UIScene_UIObject_Render_OnOff((m_pGameInstance->Find_UIScene(UISCENE_MENU, L"UIScene_PlayerMenu")), true);
        m_pButton->Activate_Button(false);
    }
}

void CChair::OnCollisionExit(CGameObject* _pOther, PxContactPair _information)
{
    m_pButton->Activate_Button(false);

    m_pBodyGosemy->IsAppear(false);
    m_pGosemy->Set_AnimState(2);
    m_pGameInstance->Stop_Effect(EFFECT_NAME::EFFECT_PARTCLE_GHOSEMY_DUST);

    m_bInteractOn = false;
    m_bFadingIn = false;
    m_bFadingOut = true;
}

HRESULT CChair::Ready_Components()
{
    if (FAILED(__super::Ready_Components()))
        return E_FAIL;

    return S_OK;
}

HRESULT CChair::Bind_ShaderResources()
{
    if (FAILED(__super::Bind_ShaderResources()))
        return E_FAIL;

    return S_OK;
}

CChair* CChair::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CChair* pInstance = new CChair(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CChair");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CChair::Clone(void* pArg)
{
    CChair* pInstance = new CChair(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Created : CChair");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CChair::Free()
{
    __super::Free();

    m_pGameInstance->Stop_Effect(EFFECT_NAME::EFFECT_PARTICLE_CHAIR);
    m_pGameInstance->Sub_Actor_Scene(m_pActor);
}