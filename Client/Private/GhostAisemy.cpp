#include "pch.h"
#include "GhostAisemy.h"
#include "GameInstance.h"

CGhostAisemy::CGhostAisemy(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
    : CContainerObject{ _pDevice , _pContext }
{
}

CGhostAisemy::CGhostAisemy(const CGhostAisemy& _Prototype)
    :CContainerObject(_Prototype)
{
}

HRESULT CGhostAisemy::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CGhostAisemy::Initialize(void* pArg)
{
    strcpy_s(m_szName, "GHOSEMY");

    GHOST_SEMY_DESC* pDesc = static_cast<GHOST_SEMY_DESC*>(pArg);

    m_eMyLevel = static_cast<LEVELID>(pDesc->iCurLevel);

    if (FAILED(__super::Initialize(&pDesc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    if (FAILED(Ready_PartObjects()))
        return E_FAIL;

    m_iState = STATE_GOSEMY_APPROACH;

    m_pTransformCom->Scaling(_float3{ 0.002f, 0.002f, 0.002f });

    m_pButtonGameObject = m_pGameInstance->Get_GameObject_To_Layer(m_eMyLevel, TEXT("Layer_Button"), "BUTTON");
    m_pButton = static_cast<CButton*>(m_pButtonGameObject);

    m_pActor = m_pGameInstance->Create_Actor(COLLIDER_TYPE::COLLIDER_CAPSULE, _float3{ 0.3f,0.3f,0.1f }, _float3{ 0.f,0.f,1.f }, 90.f, this);

    _uint settingColliderGroup = GROUP_TYPE::PLAYER;

    m_pGameInstance->Set_CollisionGroup(m_pActor, GROUP_TYPE::OBJECT, settingColliderGroup);

    m_pGameInstance->Set_GlobalPos(m_pActor, _fvector{ 0.f,20.f,0.f,1.f });

    return S_OK;
}

void CGhostAisemy::Priority_Update(_float _fTimeDelta)
{
    Culling();
    if (m_bCulling)
        return;

    __super::Priority_Update(_fTimeDelta);
}

void CGhostAisemy::Update(_float _fTimeDelta)
{
    if (m_pBody_GhoSemy->IsActivate())
    {
        _vector vGhoSemyPosition = m_pTransformCom->Get_State(CTransform::STATE_POSITION);

        if (m_bColliderOn)
        {
            if (SUCCEEDED(m_pGameInstance->IsActorInScene(m_pActor)))
                m_pGameInstance->Update_Collider(m_pActor, XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrix_Ptr()), _vector{ -150.f, 250.f,200.f,1.f });
        }

        switch (m_iState)
        {
        case STATE_GOSEMY_APPROACH:
            Setting_Approach(_fTimeDelta);
            break;
        case STATE_GOSEMY_LIGHT_OFF:
            Setting_LightOff(_fTimeDelta);
            break;
        case STATE_GOSEMY_LIGHT_ON:
        {
            if (m_pBody_GhoSemy->IsAnimationFinish())
                m_iState = STATE_GOSEMY_LIGHT_LOOP;
        }
        break;
        case STATE_GOSEMY_LIGHT_LOOP:
            Setting_LightLoop(_fTimeDelta);
            break;
        }
    }


    if (m_pGameInstance->Get_Boss_Dead())
    {
        _float4 vPos = {};
        if (m_eMyLevel == LEVEL_TUTORIAL)
            vPos = { 112.0f, 15.6f, -30.0f ,1.0f };
        else if (m_eMyLevel == LEVEL_SEAOFTREES)
            vPos = { -42.4256f, 100.8f, -95.475f, 1.0f };
        else if (m_eMyLevel == LEVEL_ROYALGARDEN)
            vPos = { 12.1903f, 13.6913f, -19.276f,1.0f };
        else
            vPos = { 14.913f, 8.50871f, 12.7244f,1.0f };

        Spawn_Conversation_Gosemy(vPos, true);
        return;
    }

    __super::Update(_fTimeDelta);
}

void CGhostAisemy::Late_Update(_float _fTimeDelta)
{
    if (m_bCulling)
        return;

    __super::Late_Update(_fTimeDelta);
}

HRESULT CGhostAisemy::Render()
{
    return S_OK;
}

void CGhostAisemy::Spawn_Gosemy(_float4 _vPos, _bool _bFirstAppear)
{
    if (_bFirstAppear)
    {
        m_pBody_GhoSemy->BindOff();
        m_iState = STATE_GOSEMY_APPROACH;
    }
    else
        m_iState = STATE_GOSEMY_LIGHT_LOOP;

    m_pTransformCom->Set_State(CTransform::STATE_POSITION, XMLoadFloat4(&_vPos));
}

void CGhostAisemy::Spawn_Conversation_Gosemy(_float4 _vPos, _bool _bColliderOn)
{
    m_pGameInstance->Add_Actor_Scene(m_pActor);

    m_pBody_GhoSemy->Activate_SemyBody(true, 1);

    m_iState = STATE_GOSEMY_LIGHT_OFF;
    m_pTransformCom->Set_State(CTransform::STATE_POSITION, XMLoadFloat4(&_vPos));
}

void CGhostAisemy::Set_AnimState(_uint _iAnimStateNum)
{
    m_iState = _iAnimStateNum;
}

HRESULT CGhostAisemy::Ready_Components()
{
    return S_OK;
}

HRESULT CGhostAisemy::Ready_PartObjects()
{
    CBody_GhostSemy::BODY_Ghost_Aisemy_DESC BodyDesc{};
    BodyDesc.pParentState = &m_iState;
    BodyDesc.pParentWorldMatrix = m_pTransformCom->Get_WorldMatrix_Ptr();
    BodyDesc.fSpeedPerSec = 0.f;
    BodyDesc.fRotationPerSec = 0.f;

    if (FAILED(__super::Add_PartObject(TEXT("Part_Body_GhoSemy"), LEVEL_STATIC, TEXT("Prototype_GameObject_GhoSemy_Body"), &BodyDesc)))
        return E_FAIL;

    m_pBody_GhoSemy = static_cast<CBody_GhostSemy*>(Find_PartObject(TEXT("Part_Body_GhoSemy")));
    if (nullptr == m_pBody_GhoSemy)
        return E_FAIL;

#pragma region LAMP
    CLamp::LAMP_DESC        LampDesc = {};
    CModel* pBodyModelCom = dynamic_cast<CModel*>(__super::Find_PartObject_Component(TEXT("Part_Body_GhoSemy"), TEXT("Com_Model")));
    if (nullptr == pBodyModelCom)
        return E_FAIL;

    LampDesc.pSocketMatrix = pBodyModelCom->Get_BoneMatrix("ik_hand_l");
    LampDesc.pParentWorldMatrix = m_pTransformCom->Get_WorldMatrix_Ptr();

    if (FAILED(__super::Add_PartObject(TEXT("Part_Lamp"), LEVEL_STATIC, TEXT("Prototype_GameObject_Lamp"), &LampDesc)))
        return E_FAIL;

    m_pLamp = static_cast<CLamp*>(Find_PartObject(TEXT("Part_Lamp")));
    if (m_pLamp == nullptr)
        return E_FAIL;
#pragma endregion

    return S_OK;
}

void CGhostAisemy::Culling()
{
    if (!m_bActive)
    {
        if (m_pGameInstance->isIn_Frustum_WorldSpace(m_pTransformCom->Get_State(CTransform::STATE_POSITION), 0.1f, FRUSTUM_TYPE::FRUSTUM_MONSTER))
        {
            m_bCulling = false;
        }
        else
        {
            m_bCulling = true;
        }
    }
}

void CGhostAisemy::OnCollisionEnter(CGameObject* _pOther, PxContactPair _information)
{
    _vector vGhosemyPos = m_pTransformCom->Get_State(CTransform::STATE_POSITION);
    vGhosemyPos = XMVectorSetY(vGhosemyPos, XMVectorGetY(vGhosemyPos) + 1.0f);

    _float4 vGhosemyPosition;
    XMStoreFloat4(&vGhosemyPosition, vGhosemyPos);

    m_pButton->Set_WorldPosition(vGhosemyPosition);
    m_pButton->Set_ButtonText(TEXT("E"), TEXT("대화"));
    m_pButton->Activate_Button(true);
}

void CGhostAisemy::OnCollision(CGameObject* _pOther, PxContactPair _information)
{
    _uint iRandVoice = {};

    if (m_pGameInstance->isKeyEnter(DIK_E))
    {
        m_pButton->Activate_Button(false);

        if (m_eMyLevel == LEVEL_OCEAN)
        {
            if (!m_pGameInstance->Get_Scene_Render_State(m_pGameInstance->Find_UIScene(UISCENE_DIALOGUE, L"UIScene_AIsemyeEnding"))
                && !m_pGameInstance->Get_Scene_Render_State(m_pGameInstance->Find_UIScene(UISCENE_DIALOGUE, L"UIScene_AIsemyeEnding_Image")))
            {
                m_pGameInstance->UIGroup_Render_OnOff(LEVEL_STATIC, TEXT("Layer_Mouse"), true); // 마우스 이미지 켜기
                m_pGameInstance->UIGroup_Render_OnOff(m_eMyLevel, TEXT("Layer_Dialogue"), true);
                m_pGameInstance->UIScene_UIObject_Render_OnOff((m_pGameInstance->Find_UIScene(UISCENE_DIALOGUE, L"UIScene_AIsemyeEnding")), true);

            }

        }
        else
        {
            if (!m_pGameInstance->Get_Scene_Render_State(m_pGameInstance->Find_UIScene(UISCENE_DIALOGUE, L"UIScene_AIsemy_1"))
                && !m_pGameInstance->Get_Scene_Render_State(m_pGameInstance->Find_UIScene(UISCENE_DIALOGUE, L"UIScene_AIsemyPop_1")))
            {
                m_pGameInstance->UIGroup_Render_OnOff(LEVEL_STATIC, TEXT("Layer_Mouse"), true); // 마우스 이미지 켜기

                m_pGameInstance->UIGroup_Render_OnOff(m_eMyLevel, TEXT("Layer_Dialogue"), true);
                m_pGameInstance->UIScene_UIObject_Render_OnOff((m_pGameInstance->Find_UIScene(UISCENE_DIALOGUE, L"UIScene_AIsemy_1")), true);
            }

        }

        iRandVoice = rand() % 5 + 1;
        switch (iRandVoice)
        {
        case 1:
            m_pGameInstance->Play_Sound(TEXT("Aisemy_Talk09.ogg"), CHANNELID::SOUND_NPC, 0.3f);
            break;
        case 2:
            m_pGameInstance->Play_Sound(TEXT("Aisemy_Talk08.ogg"), CHANNELID::SOUND_NPC, 0.3f);
            break;
        case 3:
            m_pGameInstance->Play_Sound(TEXT("Aisemy_Talk06.ogg"), CHANNELID::SOUND_NPC, 0.3f);
            break;
        case 4:
            m_pGameInstance->Play_Sound(TEXT("Aisemy_Talk04.ogg"), CHANNELID::SOUND_NPC, 0.3f);
            break;
        case 5:
            m_pGameInstance->Play_Sound(TEXT("Aisemy_Talk01.ogg"), CHANNELID::SOUND_NPC, 0.3f);
            break;
        }
    }

}

void CGhostAisemy::OnCollisionExit(CGameObject* _pOther, PxContactPair _information)
{
    m_pButton->Activate_Button(false);
}

void CGhostAisemy::Setting_Approach(_float _fTimeDelta)
{
    m_pLamp->LightUp_Lamp(false);

    if (m_pBody_GhoSemy->IsAnimationFinish())
        m_iState = STATE_GOSEMY_LIGHT_ON;
}

void CGhostAisemy::Setting_LightOn(_float _fTimeDelta)
{
    m_iState = STATE_GOSEMY_LIGHT_LOOP;
}

void CGhostAisemy::Setting_LightOff(_float _fTimeDelta)
{
    m_bColliderOn = true;
    //m_pGameInstance->Add_Actor_Scene(m_pActor);

    m_pLamp->LightUp_Lamp(false);
}

void CGhostAisemy::Setting_LightLoop(_float _fTimeDelta)
{
    m_bColliderOn = false;
    m_pGameInstance->Sub_Actor_Scene(m_pActor);
    m_iState = STATE_GOSEMY_LIGHT_LOOP;
    m_pLamp->LightUp_Lamp(true);
}

CGhostAisemy* CGhostAisemy::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
{
    CGhostAisemy* pInstance = new CGhostAisemy(_pDevice, _pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CGhostAisemy");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CGhostAisemy::Clone(void* _pArg)
{
    CGhostAisemy* pInstance = new CGhostAisemy(*this);

    if (FAILED(pInstance->Initialize(_pArg)))
    {
        MSG_BOX("Failed To Cloned : CGhostAisemy");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CGhostAisemy::Free()
{
    __super::Free();

    m_pGameInstance->Sub_Actor_Scene(m_pActor);
}