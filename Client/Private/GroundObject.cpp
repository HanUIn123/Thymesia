#include "pch.h"
#include "GroundObject.h"
#include "GameInstance.h"

CGroundObject::CGroundObject(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
    :CEnvironmentObject{ _pDevice, _pContext }
{
}

CGroundObject::CGroundObject(const CGroundObject& _Prototype)
    :CEnvironmentObject(_Prototype)
{
}

HRESULT CGroundObject::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CGroundObject::Initialize(void* _pArg)
{
    GROUND_OBJECCT_DESC* pDesc = static_cast<GROUND_OBJECCT_DESC*>(_pArg);

    if (FAILED(__super::Initialize(_pArg)))
        return E_FAIL;

    if (pDesc->vecInstancePosition.empty())
        return E_FAIL;

    strcpy_s(m_EnvironmentMeshName, pDesc->ObjectName.c_str());

    m_bModeSelected = pDesc->isBasicMode;
    m_vecBoxSize = pDesc->vecBoxSize;
    m_bCullingObject = pDesc->isCullingObject;

    if (!m_bModeSelected)
    {
        m_vecInstancePosition = pDesc->vecInstancePosition;
        m_vecInstanceScale = pDesc->vecInstanceScale;
        m_vecInstanceRotation = pDesc->vecInstanceRotation;
        m_iNumInstance = static_cast<_uint>(pDesc->vecInstancePosition.size());

    }
    else
    {
        m_vecInstancePosition = pDesc->vecInstancePosition;
        m_vecInstanceScale = pDesc->vecInstanceScale;
        m_vecInstanceRotation = pDesc->vecInstanceRotation;
        m_iNumInstance = static_cast<_uint>(pDesc->fInstanceCount);
    }

    m_vecInstanceData.clear();
    m_vecInstanceData.reserve(m_iNumInstance);

    for (UINT i = 0; i < m_iNumInstance; ++i)
    {
        VTX_MODEL_INSTANCE instance;

        if (!m_bModeSelected)
        {
            XMVECTOR Quaternion = XMLoadFloat4(&m_vecInstanceRotation[i]);
            XMMATRIX matRotation = XMMatrixRotationQuaternion(Quaternion);

            XMFLOAT3 fTerrainPos = pDesc->vecInstancePosition[i];
            XMMATRIX matScale = XMMatrixScaling(
                XMVectorGetX(XMLoadFloat3(&pDesc->vecInstanceScale[i])),
                XMVectorGetY(XMLoadFloat3(&pDesc->vecInstanceScale[i])),
                XMVectorGetZ(XMLoadFloat3(&pDesc->vecInstanceScale[i]))
            );

            XMMATRIX matPosition = XMMatrixTranslation(fTerrainPos.x, fTerrainPos.y, fTerrainPos.z);
            XMMATRIX matWorld = matScale * matRotation * matPosition;

            XMFLOAT4X4 tempMatrix;
            XMStoreFloat4x4(&tempMatrix, matWorld);

            instance.InstanceMatrix[0] = XMFLOAT4(tempMatrix._11, tempMatrix._12, tempMatrix._13, tempMatrix._14);
            instance.InstanceMatrix[1] = XMFLOAT4(tempMatrix._21, tempMatrix._22, tempMatrix._23, tempMatrix._24);
            instance.InstanceMatrix[2] = XMFLOAT4(tempMatrix._31, tempMatrix._32, tempMatrix._33, tempMatrix._34);
            instance.InstanceMatrix[3] = XMFLOAT4(tempMatrix._41, tempMatrix._42, tempMatrix._43, tempMatrix._44);

            m_vecInstanceData.push_back(instance);
        }
        else
        {
            XMVECTOR Quaternion = XMLoadFloat4(&m_vecInstanceRotation[i]);
            XMMATRIX matRotation = XMMatrixRotationQuaternion(Quaternion);

            XMFLOAT4 fTerrainPos = pDesc->fPosition;
            XMMATRIX matScale = XMMatrixScaling(
                XMVectorGetX(XMLoadFloat3(&pDesc->vecInstanceScale[i])),
                XMVectorGetY(XMLoadFloat3(&pDesc->vecInstanceScale[i])),
                XMVectorGetZ(XMLoadFloat3(&pDesc->vecInstanceScale[i]))
            );

            XMMATRIX matPosition = XMMatrixTranslation(fTerrainPos.x, fTerrainPos.y, fTerrainPos.z);
            XMMATRIX matWorld = matScale * matRotation * matPosition;

            XMFLOAT4X4 tempMatrix;
            XMStoreFloat4x4(&tempMatrix, matWorld);

            instance.InstanceMatrix[0] = XMFLOAT4(tempMatrix._11, tempMatrix._12, tempMatrix._13, tempMatrix._14);
            instance.InstanceMatrix[1] = XMFLOAT4(tempMatrix._21, tempMatrix._22, tempMatrix._23, tempMatrix._24);
            instance.InstanceMatrix[2] = XMFLOAT4(tempMatrix._31, tempMatrix._32, tempMatrix._33, tempMatrix._34);
            instance.InstanceMatrix[3] = XMFLOAT4(tempMatrix._41, tempMatrix._42, tempMatrix._43, tempMatrix._44);

            m_vecInstanceData.push_back(instance);
        }
    }

    if (m_vecInstanceData.empty())
        return E_FAIL;

    if (FAILED(m_pModelCom->Create_InstanceBuffer(m_iNumInstance, m_vecInstanceData.data())))
    {
        return E_FAIL;
    }

    m_vecVisible.resize(m_iNumInstance);


    m_pDitheringOnOff = m_pGameInstance->Get_Dithering_Ptr();


    return S_OK;
}

void CGroundObject::Priority_Update(_float fTimeDelta)
{
}

void CGroundObject::Update(_float fTimeDelta)
{
}

void CGroundObject::Late_Update(_float _fTimeDelta)
{
    if (m_iNumInstance > 0)
    {
        m_pGameInstance->Add_RenderGroup(CRenderer::RG_NONBLEND, this);
    }
}

HRESULT CGroundObject::Render()
{
    if (m_iNumInstance == 0)
        return S_OK;

    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint			iNumMeshes = m_pModelCom->Get_NumMeshes();

    if (*m_pDitheringOnOff)
    {

        for (_uint i = 0; i < iNumMeshes; i++)
        {
            if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_DIFFUSE, "g_DiffuseTexture", 0)))
                return E_FAIL;
            if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_NORMALS, "g_NormalTexture", 0)))
                return E_FAIL;
            m_pShaderCom->Begin(0);
            m_pModelCom->Render_Instance(i, m_iNumInstance);
        }
    }

    else
    {
        for (_uint i = 0; i < iNumMeshes; i++)
        {
            if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_DIFFUSE, "g_DiffuseTexture", 0)))
                return E_FAIL;
            if (FAILED(m_pModelCom->Bind_Material(m_pShaderCom, i, aiTextureType_NORMALS, "g_NormalTexture", 0)))
                return E_FAIL;
            m_pShaderCom->Begin(2);
            m_pModelCom->Render_Instance(i, m_iNumInstance);
        }
    }

    return S_OK;
}

HRESULT CGroundObject::Render_Motion_Blur()
{
    if (m_iNumInstance == 0)
        return S_OK;

    if (FAILED(Bind_Motion_Blur_ShaderResources()))
        return E_FAIL;

    _uint			iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (_uint i = 0; i < iNumMeshes; i++)
    {
        vector<VTX_MODEL_INSTANCE> vecVTXInstance;
        for (_uint j = 0; j < m_iNumInstance; ++j)
        {
            if (m_vecVisible[j])
                vecVTXInstance.push_back(m_vecInstanceData[j]);
        }

        _uint iVisibleCount = static_cast<_uint>(vecVTXInstance.size());

        if (iVisibleCount == 0)
            continue;

        m_pModelCom->Update_InstanceBuffer(iVisibleCount, vecVTXInstance.data());

        if (m_iNumInstance > 0)
        {
            m_pShaderCom->Begin(1);
            m_pModelCom->Render_Instance(i, iVisibleCount);
        }
    }

    return S_OK;
}

void CGroundObject::Update_InstanceBuffer(_uint _iInstanceIndex, const XMFLOAT3& _vPosition, const XMFLOAT3& _vScale, const XMFLOAT4& _vRotation)
{
    if (_iInstanceIndex >= m_iNumInstance)
        return;

    if (!m_vecVisible[_iInstanceIndex])
        return;

    m_vecInstancePosition[_iInstanceIndex] = _vPosition;
    m_vecInstanceScale[_iInstanceIndex] = _vScale;
    m_vecInstanceRotation[_iInstanceIndex] = _vRotation;

    XMMATRIX matPosition = XMMatrixTranslation(_vPosition.x, _vPosition.y, _vPosition.z);
    XMMATRIX matScale = XMMatrixScaling(_vScale.x, _vScale.y, _vScale.z);
    XMVECTOR Quaternion = XMLoadFloat4(&_vRotation);
    XMMATRIX matRotation = XMMatrixRotationQuaternion(Quaternion);
    XMMATRIX matWorld = matScale * matRotation * matPosition;

    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&m_vecInstanceData[_iInstanceIndex]), matWorld);

    m_pModelCom->Update_InstanceBuffer(1, &m_vecInstanceData[_iInstanceIndex]);
}

void CGroundObject::Update_InstanceBuffer_ForCulling()
{
    if (m_iNumInstance == 0)
        return;

    for (_uint i = 0; i < m_vecInstanceData.size(); ++i)
    {
        XMFLOAT3 fMin =
        {
            (m_vecInstancePosition[i].x - ((m_vecInstanceScale[i].x * 0.5f))),
            (m_vecInstancePosition[i].y - ((m_vecInstanceScale[i].y * 0.5f))),
            (m_vecInstancePosition[i].z - ((m_vecInstanceScale[i].z * 0.5f)))
        };

        XMFLOAT3 fMax =
        {
            (m_vecInstancePosition[i].x + ((m_vecInstanceScale[i].x * 0.5f))),
            (m_vecInstancePosition[i].y + ((m_vecInstanceScale[i].y * 0.5f))),
            (m_vecInstancePosition[i].z + ((m_vecInstanceScale[i].z * 0.5f)))
        };

        if (m_pGameInstance->isAABB_InFrustum(fMin, fMax))
        {
            m_vecVisible[i] = true;
        }
        else
        {
            m_vecVisible[i] = false;
        }
    }

    if (m_iNumInstance > 0)
        m_pModelCom->Update_InstanceBuffer(m_iNumInstance, m_vecInstanceData.data());
}

HRESULT CGroundObject::Ready_Components()
{
    if (FAILED(__super::Ready_Components()))
        return E_FAIL;

    string strComponentName = "Prototype_Component_Model_";
    strComponentName += m_EnvironmentMeshName;

    _tchar		szComponentName[MAX_PATH] = {};

    MultiByteToWideChar(CP_ACP, 0, strComponentName.c_str(), static_cast<_int>(strlen(strComponentName.c_str())), szComponentName, MAX_PATH);

    if (FAILED(__super::Add_Component(m_eCurrentLevel, szComponentName, TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom))))
        return E_FAIL;

    return S_OK;
}

HRESULT CGroundObject::Bind_ShaderResources()
{

    if (m_pGameInstance->Get_Current_Level_Index() >= 3)
    {
        m_pCamera = m_pGameInstance->Get_GameObject_To_Layer(m_pGameInstance->Get_Current_Level_Index(), TEXT("Layer_Camera"), "Camera_Free");

        if (FAILED(m_pShaderCom->Bind_RawValue("g_WorldCamPos", &m_pCamera->Get_Transfrom()->Get_State(CTransform::STATE_POSITION), sizeof(_float4))))
            return E_FAIL;
    }


    if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_VIEW))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_PROJ))))
        return E_FAIL;

    return S_OK;
}

HRESULT CGroundObject::Bind_Motion_Blur_ShaderResources()
{
    /* 물체는 멈춰있기 때문에 월드 매트릭스를 바인딩 안해도 되겠다. */
    if (FAILED(m_pShaderCom->Bind_Matrix("g_PreViewMatrix", &m_pGameInstance->Get_PreTransform_Float4x4(CPipeLine::D3DTS_VIEW))))
        return E_FAIL;

    if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_VIEW))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_pGameInstance->Get_Transform_Float4x4(CPipeLine::D3DTS_PROJ))))
        return E_FAIL;

    return S_OK;
}

CGroundObject* CGroundObject::Create(ID3D11Device* _pDevice, ID3D11DeviceContext* _pContext)
{
    CGroundObject* pInstance = new CGroundObject(_pDevice, _pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CGroundObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CGroundObject::Clone(void* pArg)
{
    CGroundObject* pInstance = new CGroundObject(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CGroundObject");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CGroundObject::Free()
{
    __super::Free();
}

