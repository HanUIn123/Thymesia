#include "..\Public\Shader_Compute_Deferred.h"

CShader_Compute_Deferred::CShader_Compute_Deferred(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CShader{ pDevice, pContext }
{

}

CShader_Compute_Deferred::CShader_Compute_Deferred(const CShader_Compute_Deferred& Prototype)
	: CShader(Prototype)
	, m_pComputeShader(Prototype.m_pComputeShader)
	, m_pSampleParmsBuffer(Prototype.m_pSampleParmsBuffer)
	, m_pComputeShaderSampler(Prototype.m_pComputeShaderSampler)
{
	Safe_AddRef(m_pComputeShader);
	Safe_AddRef(m_pSampleParmsBuffer);
	Safe_AddRef(m_pComputeShaderSampler);
}

HRESULT CShader_Compute_Deferred::Initialize_Prototype(const _tchar* pShaderFilePath, const _char* _pFunctionName, const D3D11_INPUT_ELEMENT_DESC* pVertexElements, _uint iNumElements, void* pArg, Compute_Deferred_Type eType)
{
	_uint			iHlslFlag = {};

#ifdef _DEBUG
	iHlslFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	iHlslFlag = D3DCOMPILE_OPTIMIZATION_LEVEL1;
#endif

	ID3DBlob* pErrMsg = { nullptr };

	// shader 만드는 것 마냥 쉐이더파일 찾아서 컴파일하는거고 
	if (FAILED(D3DX11CompileEffectFromFile(pShaderFilePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, iHlslFlag, 0, m_pDevice, &m_pEffect, &pErrMsg)))
		return E_FAIL;

	ID3DX11EffectTechnique* pTechnique = m_pEffect->GetTechniqueByIndex(0);
	if (nullptr == pTechnique)
		return E_FAIL;

	D3DX11_TECHNIQUE_DESC		TechniqueDesc{};

	pTechnique->GetDesc(&TechniqueDesc);

	m_iNumPasses = TechniqueDesc.Passes;

	if (pVertexElements != nullptr && iNumElements > 0)
	{
		for (_uint i = 0; i < m_iNumPasses; i++)
		{
			ID3DX11EffectPass* pPass = pTechnique->GetPassByIndex(i);
			ID3D11InputLayout* pInputLayout = { nullptr };

			D3DX11_PASS_DESC		PassDesc{};

			pPass->GetDesc(&PassDesc);



			if (FAILED(m_pDevice->CreateInputLayout(pVertexElements, iNumElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pInputLayout)))
				return E_FAIL;

			m_InputLayout.push_back(pInputLayout);

		}
	}

	Create_ComputeShader(pShaderFilePath, _pFunctionName, m_pDevice);


	if (eType == DEFERRED_TYPE_LIGHTSHAFT)
	{
		LIGHTSHAFTPARAMS* Desc = static_cast<LIGHTSHAFTPARAMS*>(pArg);

		Create_ConstantBuffer_LightShaft(Desc);
	}
	else if (eType == DEFERRED_TYPE_FOG)
	{
		FOGPARAMS* Desc = static_cast<FOGPARAMS*>(pArg);

		Create_Buffer_Fog(Desc);
	}
	return S_OK;
}

HRESULT CShader_Compute_Deferred::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT CShader_Compute_Deferred::Create_ComputeShader(const _tchar* _pShaderFilePath, const _char* _pFunctionName, ID3D11Device* _pDevice)
{
	if (_pDevice == nullptr)
		return E_FAIL;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	//dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

	const D3D_SHADER_MACRO defines[] =
	{
		"USE_STRUCTURED_BUFFERS", "1",
		//#ifdef USE_STRUCTURED_BUFFERS
		//		"USE_STRUCTURED_BUFFERS", "1",
		//#endif
				nullptr, nullptr
	};

	_char* pProfile = (m_pDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) ? "cs_5_0" : "cs_4_0";

	ID3DBlob* pError = { nullptr };
	ID3DBlob* pBlob = { nullptr };
	if (FAILED(D3DCompileFromFile(_pShaderFilePath, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, _pFunctionName, pProfile, dwShaderFlags, 0, &pBlob, &pError)))
	{
		if (pError != nullptr)
		{
			OutputDebugStringA((_char*)pError->GetBufferPointer());
			_char* asd = (_char*)pError->GetBufferPointer();
			int a = 10;

		}


		Safe_Release(pError);
		Safe_Release(pBlob);

		return E_FAIL;
	}

	if (FAILED(m_pDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &m_pComputeShader)))
		return E_FAIL;

	Safe_Release(pError);
	Safe_Release(pBlob);


#ifdef _DEBUG
	(m_pComputeShader)->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(_pFunctionName), _pFunctionName);
#endif // _DEBUG


	return S_OK;
}

HRESULT CShader_Compute_Deferred::Compute_Shader_LightShaft(_uint _iThreadCountX, _uint _iThreadCountY, _uint _iThreadCountZ, ID3D11ShaderResourceView* pSRV, ID3D11UnorderedAccessView* pUAV, void* pArg)
{

	LIGHTSHAFTPARAMS* Desc = static_cast<LIGHTSHAFTPARAMS*>(pArg);

	float clearColor[4] = { 0, 0, 0, 0 };
	m_pContext->ClearUnorderedAccessViewFloat(pUAV, clearColor);

	m_pContext->CSSetShaderResources(0, 1, &pSRV);
	m_pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, nullptr);

	D3D11_MAPPED_SUBRESOURCE	SubResource{};

	m_pContext->Map(m_pSampleParmsBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &SubResource);

	LIGHTSHAFTPARAMS* pDest = static_cast<LIGHTSHAFTPARAMS*>(SubResource.pData);

	/*pDest->g_LightShaftValue = Desc->g_LightShaftValue;
	pDest->g_ScreenLightPos = Desc->g_ScreenLightPos;*/

	memcpy(pDest, Desc, sizeof(LIGHTSHAFTPARAMS));

	m_pContext->Unmap(m_pSampleParmsBuffer, 0);

	m_pContext->CSSetConstantBuffers(0, 1, &m_pSampleParmsBuffer);
	m_pContext->CSSetShader(m_pComputeShader, nullptr, 0);
	m_pContext->Dispatch(_iThreadCountX / 16, _iThreadCountY / 16, 1);

	ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	m_pContext->CSSetShaderResources(0, 1, nullSRV);
	m_pContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
	m_pContext->CSSetShader(nullptr, nullptr, 0);

	return S_OK;
}

HRESULT CShader_Compute_Deferred::Compute_Shader_Fog(_uint _iThreadCountX, _uint _iThreadCountY, _uint _iThreadCountZ, ID3D11ShaderResourceView* pSRVDepth, ID3D11ShaderResourceView* pNoiseSRV, ID3D11ShaderResourceView* pSRVGodRay, ID3D11ShaderResourceView* pSRVFinal, ID3D11ShaderResourceView* pSRVRangeFogFinal, ID3D11UnorderedAccessView* pUAV, void* pArg)
{
	FOGPARAMS* Desc = static_cast<FOGPARAMS*>(pArg);

	float clearColor[4] = { 0, 0, 0, 0 };
	m_pContext->ClearUnorderedAccessViewFloat(pUAV, clearColor);

	m_pContext->CSSetShaderResources(0, 1, &pSRVDepth);

	if (pNoiseSRV != nullptr)
		m_pContext->CSSetShaderResources(1, 1, &pNoiseSRV);
	m_pContext->CSSetShaderResources(2, 1, &pSRVGodRay);
	m_pContext->CSSetShaderResources(3, 1, &pSRVFinal);
	m_pContext->CSSetShaderResources(4, 1, &pSRVRangeFogFinal);
	m_pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, nullptr);
	m_pContext->CSSetSamplers(0, 1, &m_pComputeShaderSampler);

	D3D11_MAPPED_SUBRESOURCE	SubResource{};

	m_pContext->Map(m_pSampleParmsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubResource);

	FOGPARAMS* pDest = static_cast<FOGPARAMS*>(SubResource.pData);

	pDest->fFogFactor = Desc->fFogFactor;
	pDest->fFogStartDistance = Desc->fFogStartDistance;
	pDest->fHeightNoiseFactor = Desc->fHeightNoiseFactor;
	pDest->g_FogColor = Desc->g_FogColor;
	pDest->g_fTime = Desc->g_fTime;
	pDest->g_ProjMatrixInv = Desc->g_ProjMatrixInv;
	pDest->g_ViewMatrixInv = Desc->g_ViewMatrixInv;
	pDest->g_vCamPosition = Desc->g_vCamPosition;

	m_pContext->Unmap(m_pSampleParmsBuffer, 0);

	m_pContext->CSSetConstantBuffers(0, 1, &m_pSampleParmsBuffer);
	m_pContext->CSSetShader(m_pComputeShader, nullptr, 0);
	m_pContext->Dispatch(_iThreadCountX / 16, _iThreadCountY / 16, 1);

	ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	m_pContext->CSSetShaderResources(0, 1, nullSRV);
	m_pContext->CSSetShaderResources(1, 1, nullSRV);
	m_pContext->CSSetShaderResources(2, 1, nullSRV);
	m_pContext->CSSetShaderResources(3, 1, nullSRV);
	m_pContext->CSSetShaderResources(4, 1, nullSRV);
	m_pContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
	m_pContext->CSSetShader(nullptr, nullptr, 0);

	return S_OK;
}

HRESULT CShader_Compute_Deferred::Create_ConstantBuffer_LightShaft(LightShaftParams* ParamDesc)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(LightShaftParams);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.MiscFlags = 0;

	m_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_pSampleParmsBuffer);

	if (m_pSampleParmsBuffer == nullptr)
		return E_FAIL;

	return S_OK;
}

HRESULT CShader_Compute_Deferred::Create_Buffer_Fog(FogParams* ParamDesc)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(FogParams);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.MiscFlags = 0;

	m_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_pSampleParmsBuffer);

	if (m_pSampleParmsBuffer == nullptr)
		return E_FAIL;

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	if (FAILED(m_pDevice->CreateSamplerState(&sampDesc, &m_pComputeShaderSampler)))
		return E_FAIL;

	return S_OK;
}



CShader_Compute_Deferred* CShader_Compute_Deferred::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pShaderFilePath, const _char* _pFunctionName, const D3D11_INPUT_ELEMENT_DESC* pVertexElements, _uint iNumElements, void* pArg, Compute_Deferred_Type eType)
{
	CShader_Compute_Deferred* pInstance = new CShader_Compute_Deferred(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(pShaderFilePath, _pFunctionName, pVertexElements, iNumElements, pArg, eType)))
	{
		MSG_BOX("Failed To Created : CShader_Compute_Deferred");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CComponent* CShader_Compute_Deferred::Clone(void* pArg)
{
	CShader_Compute_Deferred* pInstance = new CShader_Compute_Deferred(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Created : CShader_Compute_Deferred");
		Safe_Release(pInstance);
	}

	return pInstance;
}
void CShader_Compute_Deferred::Free()
{
	__super::Free();

	Safe_Release(m_pComputeShader);
	Safe_Release(m_pSampleParmsBuffer);
	Safe_Release(m_pComputeShaderSampler);
}
