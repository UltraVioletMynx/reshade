#include "Log.hpp"
#include "Hooks\DXGI.hpp"
#include "Runtimes\D3D11Runtime.hpp"

#include <sstream>
#include <assert.h>

#define EXPORT extern "C"

// ---------------------------------------------------------------------------------------------------

namespace
{
	std::string GetErrorString(HRESULT hr)
	{
		std::stringstream res;

		switch (hr)
		{
			case E_FAIL:
				res << "E_FAIL";
				break;
			case E_NOTIMPL:
				res << "E_NOTIMPL";
				break;
			case E_INVALIDARG:
				res << "E_INVALIDARG";
				break;
			case DXGI_ERROR_UNSUPPORTED:
				res << "DXGI_ERROR_UNSUPPORTED";
				break;
			default:
				res << std::showbase << std::hex << hr;
				break;
		}

		return res.str();
	}
}

// ID3D11DepthStencilView
ULONG STDMETHODCALLTYPE ID3D11DepthStencilView_Release(ID3D11DepthStencilView *pDepthStencilView)
{
	static const auto trampoline = ReShade::Hooks::Call(&ID3D11DepthStencilView_Release);

	D3D11Device *device = nullptr;
	UINT dataSize = sizeof(device);
	const bool succeeded = SUCCEEDED(pDepthStencilView->GetPrivateData(__uuidof(device), &dataSize, &device));

	const ULONG ref = trampoline(pDepthStencilView);

	if (succeeded && ref == 0)
	{
		for (auto runtime : device->mRuntimes)
		{
			runtime->OnDeleteDepthStencilView(pDepthStencilView);
		}

		device->Release();
	}

	return ref;
}

// ID3D11DeviceContext
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::QueryInterface(REFIID riid, void **ppvObj)
{
	if (ppvObj == nullptr)
	{
		return E_POINTER;
	}
	else if (
		riid == __uuidof(this) ||
		riid == __uuidof(IUnknown) ||
		riid == __uuidof(ID3D11DeviceChild) ||
		riid == __uuidof(ID3D11DeviceContext) ||
		riid == __uuidof(ID3D11DeviceContext1) ||
		riid == __uuidof(ID3D11DeviceContext2) ||
		riid == __uuidof(ID3D11DeviceContext3))
	{
		#pragma region Update to ID3D11DeviceContext1 interface
		if (riid == __uuidof(ID3D11DeviceContext1) && this->mInterfaceVersion < 1)
		{
			ID3D11DeviceContext1 *devicecontext1 = nullptr;

			if (FAILED(this->mOrig->QueryInterface(&devicecontext1)))
			{
				return E_NOINTERFACE;
			}

			this->mOrig->Release();

			LOG(TRACE) << "Upgraded 'ID3D11DeviceContext' object " << this << " to 'ID3D11DeviceContext1'.";

			this->mOrig = devicecontext1;
			this->mInterfaceVersion = 1;
		}
		#pragma endregion
		#pragma region Update to ID3D11DeviceContext2 interface
		if (riid == __uuidof(ID3D11DeviceContext2) && this->mInterfaceVersion < 2)
		{
			ID3D11DeviceContext2 *devicecontext2 = nullptr;

			if (FAILED(this->mOrig->QueryInterface(&devicecontext2)))
			{
				return E_NOINTERFACE;
			}

			this->mOrig->Release();

			LOG(TRACE) << "Upgraded 'ID3D11DeviceContext" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << " to 'ID3D11DeviceContext2'.";

			this->mOrig = devicecontext2;
			this->mInterfaceVersion = 2;
		}
		#pragma endregion
		#pragma region Update to ID3D11DeviceContext3 interface
		if (riid == __uuidof(ID3D11DeviceContext3) && this->mInterfaceVersion < 3)
		{
			ID3D11DeviceContext3 *devicecontext3 = nullptr;

			if (FAILED(this->mOrig->QueryInterface(&devicecontext3)))
			{
				return E_NOINTERFACE;
			}

			this->mOrig->Release();

			LOG(TRACE) << "Upgraded 'ID3D11DeviceContext" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << " to 'ID3D11DeviceContext3'.";

			this->mOrig = devicecontext3;
			this->mInterfaceVersion = 3;
		}
		#pragma endregion

		AddRef();

		*ppvObj = this;

		return S_OK;
	}

	return this->mOrig->QueryInterface(riid, ppvObj);
}
ULONG STDMETHODCALLTYPE D3D11DeviceContext::AddRef()
{
	this->mRef++;

	return this->mOrig->AddRef();
}
ULONG STDMETHODCALLTYPE D3D11DeviceContext::Release()
{
	ULONG ref = this->mOrig->Release();

	if (--this->mRef == 0 && ref != 0)
	{
		LOG(WARNING) << "Reference count for 'ID3D11DeviceContext" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << " is inconsistent: " << ref << ", but expected 0.";

		ref = 0;
	}

	if (ref == 0)
	{
		assert(this->mRef <= 0);

		LOG(TRACE) << "Destroyed 'ID3D11DeviceContext" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << ".";

		delete this;
	}

	return ref;
}
void STDMETHODCALLTYPE D3D11DeviceContext::GetDevice(ID3D11Device **ppDevice)
{
	if (ppDevice == nullptr)
	{
		return;
	}

	this->mDevice->AddRef();

	*ppDevice = this->mDevice;
}
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::GetPrivateData(REFGUID guid, UINT *pDataSize, void *pData)
{
	return this->mOrig->GetPrivateData(guid, pDataSize, pData);
}
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::SetPrivateData(REFGUID guid, UINT DataSize, const void *pData)
{
	return this->mOrig->SetPrivateData(guid, DataSize, pData);
}
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::SetPrivateDataInterface(REFGUID guid, const IUnknown *pData)
{
	return this->mOrig->SetPrivateDataInterface(guid, pData);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
{
	this->mOrig->VSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	this->mOrig->PSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSSetShader(ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
{
	this->mOrig->PSSetShader(pPixelShader, ppClassInstances, NumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
{
	this->mOrig->PSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSSetShader(ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
{
	this->mOrig->VSSetShader(pVertexShader, ppClassInstances, NumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	for (auto runtime : this->mDevice->mRuntimes)
	{
		runtime->OnDrawCall(this->mOrig, IndexCount);
	}

	this->mOrig->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}
void STDMETHODCALLTYPE D3D11DeviceContext::Draw(UINT VertexCount, UINT StartVertexLocation)
{
	for (auto runtime : this->mDevice->mRuntimes)
	{
		runtime->OnDrawCall(this->mOrig, VertexCount);
	}

	this->mOrig->Draw(VertexCount, StartVertexLocation);
}
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::Map(ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource)
{
	return this->mOrig->Map(pResource, Subresource, MapType, MapFlags, pMappedResource);
}
void STDMETHODCALLTYPE D3D11DeviceContext::Unmap(ID3D11Resource *pResource, UINT Subresource)
{
	this->mOrig->Unmap(pResource, Subresource);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
{
	this->mOrig->PSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::IASetInputLayout(ID3D11InputLayout *pInputLayout)
{
	this->mOrig->IASetInputLayout(pInputLayout);
}
void STDMETHODCALLTYPE D3D11DeviceContext::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets)
{
	this->mOrig->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}
void STDMETHODCALLTYPE D3D11DeviceContext::IASetIndexBuffer(ID3D11Buffer *pIndexBuffer, DXGI_FORMAT Format, UINT Offset)
{
	this->mOrig->IASetIndexBuffer(pIndexBuffer, Format, Offset);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
	for (auto runtime : this->mDevice->mRuntimes)
	{
		runtime->OnDrawCall(this->mOrig, IndexCountPerInstance * InstanceCount);
	}

	this->mOrig->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
	for (auto runtime : this->mDevice->mRuntimes)
	{
		runtime->OnDrawCall(this->mOrig, VertexCountPerInstance * InstanceCount);
	}

	this->mOrig->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
{
	this->mOrig->GSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSSetShader(ID3D11GeometryShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
{
	this->mOrig->GSSetShader(pShader, ppClassInstances, NumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology)
{
	this->mOrig->IASetPrimitiveTopology(Topology);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	this->mOrig->VSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
{
	this->mOrig->VSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::Begin(ID3D11Asynchronous *pAsync)
{
	this->mOrig->Begin(pAsync);
}
void STDMETHODCALLTYPE D3D11DeviceContext::End(ID3D11Asynchronous *pAsync)
{
	this->mOrig->End(pAsync);
}
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::GetData(ID3D11Asynchronous *pAsync, void *pData, UINT DataSize, UINT GetDataFlags)
{
	return this->mOrig->GetData(pAsync, pData, DataSize, GetDataFlags);
}
void STDMETHODCALLTYPE D3D11DeviceContext::SetPredication(ID3D11Predicate *pPredicate, BOOL PredicateValue)
{
	this->mOrig->SetPredication(pPredicate, PredicateValue);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	this->mOrig->GSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
{
	this->mOrig->GSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView)
{
	if (pDepthStencilView != nullptr)
	{
		for (auto runtime : this->mDevice->mRuntimes)
		{
			runtime->OnSetDepthStencilView(pDepthStencilView);
		}
	}

	this->mOrig->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
}
void STDMETHODCALLTYPE D3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts)
{
	if (pDepthStencilView != nullptr)
	{
		for (auto runtime : this->mDevice->mRuntimes)
		{
			runtime->OnSetDepthStencilView(pDepthStencilView);
		}
	}

	this->mOrig->OMSetRenderTargetsAndUnorderedAccessViews(NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}
void STDMETHODCALLTYPE D3D11DeviceContext::OMSetBlendState(ID3D11BlendState *pBlendState, const FLOAT BlendFactor[4], UINT SampleMask)
{
	this->mOrig->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
}
void STDMETHODCALLTYPE D3D11DeviceContext::OMSetDepthStencilState(ID3D11DepthStencilState *pDepthStencilState, UINT StencilRef)
{
	this->mOrig->OMSetDepthStencilState(pDepthStencilState, StencilRef);
}
void STDMETHODCALLTYPE D3D11DeviceContext::SOSetTargets(UINT NumBuffers, ID3D11Buffer *const *ppSOTargets, const UINT *pOffsets)
{
	this->mOrig->SOSetTargets(NumBuffers, ppSOTargets, pOffsets);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DrawAuto()
{
	this->mOrig->DrawAuto();
}
void STDMETHODCALLTYPE D3D11DeviceContext::DrawIndexedInstancedIndirect(ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
	this->mOrig->DrawIndexedInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DrawInstancedIndirect(ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
	this->mOrig->DrawInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
}
void STDMETHODCALLTYPE D3D11DeviceContext::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	this->mOrig->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DispatchIndirect(ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
	this->mOrig->DispatchIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
}
void STDMETHODCALLTYPE D3D11DeviceContext::RSSetState(ID3D11RasterizerState *pRasterizerState)
{
	this->mOrig->RSSetState(pRasterizerState);
}
void STDMETHODCALLTYPE D3D11DeviceContext::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT *pViewports)
{
	this->mOrig->RSSetViewports(NumViewports, pViewports);
}
void STDMETHODCALLTYPE D3D11DeviceContext::RSSetScissorRects(UINT NumRects, const D3D11_RECT *pRects)
{
	this->mOrig->RSSetScissorRects(NumRects, pRects);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CopySubresourceRegion(ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox)
{
	this->mOrig->CopySubresourceRegion(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CopyResource(ID3D11Resource *pDstResource, ID3D11Resource *pSrcResource)
{
	for (auto runtime : this->mDevice->mRuntimes)
	{
		runtime->OnCopyResource(pDstResource, pSrcResource);
	}

	this->mOrig->CopyResource(pDstResource, pSrcResource);
}
void STDMETHODCALLTYPE D3D11DeviceContext::UpdateSubresource(ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
	this->mOrig->UpdateSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CopyStructureCount(ID3D11Buffer *pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView *pSrcView)
{
	this->mOrig->CopyStructureCount(pDstBuffer, DstAlignedByteOffset, pSrcView);
}
void STDMETHODCALLTYPE D3D11DeviceContext::ClearRenderTargetView(ID3D11RenderTargetView *pRenderTargetView, const FLOAT ColorRGBA[4])
{
	this->mOrig->ClearRenderTargetView(pRenderTargetView, ColorRGBA);
}
void STDMETHODCALLTYPE D3D11DeviceContext::ClearUnorderedAccessViewUint(ID3D11UnorderedAccessView *pUnorderedAccessView, const UINT Values[4])
{
	this->mOrig->ClearUnorderedAccessViewUint(pUnorderedAccessView, Values);
}
void STDMETHODCALLTYPE D3D11DeviceContext::ClearUnorderedAccessViewFloat(ID3D11UnorderedAccessView *pUnorderedAccessView, const FLOAT Values[4])
{
	this->mOrig->ClearUnorderedAccessViewFloat(pUnorderedAccessView, Values);
}
void STDMETHODCALLTYPE D3D11DeviceContext::ClearDepthStencilView(ID3D11DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
	for (auto runtime : this->mDevice->mRuntimes)
	{
		runtime->OnClearDepthStencilView(pDepthStencilView);
	}

	this->mOrig->ClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GenerateMips(ID3D11ShaderResourceView *pShaderResourceView)
{
	this->mOrig->GenerateMips(pShaderResourceView);
}
void STDMETHODCALLTYPE D3D11DeviceContext::SetResourceMinLOD(ID3D11Resource *pResource, FLOAT MinLOD)
{
	this->mOrig->SetResourceMinLOD(pResource, MinLOD);
}
FLOAT STDMETHODCALLTYPE D3D11DeviceContext::GetResourceMinLOD(ID3D11Resource *pResource)
{
	return this->mOrig->GetResourceMinLOD(pResource);
}
void STDMETHODCALLTYPE D3D11DeviceContext::ResolveSubresource(ID3D11Resource *pDstResource, UINT DstSubresource, ID3D11Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
{
	this->mOrig->ResolveSubresource(pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
}
void STDMETHODCALLTYPE D3D11DeviceContext::ExecuteCommandList(ID3D11CommandList *pCommandList, BOOL RestoreContextState)
{
	this->mOrig->ExecuteCommandList(pCommandList, RestoreContextState);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	this->mOrig->HSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSSetShader(ID3D11HullShader *pHullShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
{
	this->mOrig->HSSetShader(pHullShader, ppClassInstances, NumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
{
	this->mOrig->HSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
{
	this->mOrig->HSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	this->mOrig->DSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSSetShader(ID3D11DomainShader *pDomainShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
{
	this->mOrig->DSSetShader(pDomainShader, ppClassInstances, NumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
{
	this->mOrig->DSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
{
	this->mOrig->DSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews)
{
	this->mOrig->CSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSSetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts)
{
	this->mOrig->CSSetUnorderedAccessViews(StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSSetShader(ID3D11ComputeShader *pComputeShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
{
	this->mOrig->CSSetShader(pComputeShader, ppClassInstances, NumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers)
{
	this->mOrig->CSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers)
{
	this->mOrig->CSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
{
	this->mOrig->VSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
{
	this->mOrig->PSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSGetShader(ID3D11PixelShader **ppPixelShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
{
	this->mOrig->PSGetShader(ppPixelShader, ppClassInstances, pNumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
{
	this->mOrig->PSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSGetShader(ID3D11VertexShader **ppVertexShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
{
	this->mOrig->VSGetShader(ppVertexShader, ppClassInstances, pNumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
{
	this->mOrig->PSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::IAGetInputLayout(ID3D11InputLayout **ppInputLayout)
{
	this->mOrig->IAGetInputLayout(ppInputLayout);
}
void STDMETHODCALLTYPE D3D11DeviceContext::IAGetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppVertexBuffers, UINT *pStrides, UINT *pOffsets)
{
	this->mOrig->IAGetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}
void STDMETHODCALLTYPE D3D11DeviceContext::IAGetIndexBuffer(ID3D11Buffer **pIndexBuffer, DXGI_FORMAT *Format, UINT *Offset)
{
	this->mOrig->IAGetIndexBuffer(pIndexBuffer, Format, Offset);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
{
	this->mOrig->GSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSGetShader(ID3D11GeometryShader **ppGeometryShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
{
	this->mOrig->GSGetShader(ppGeometryShader, ppClassInstances, pNumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY *pTopology)
{
	this->mOrig->IAGetPrimitiveTopology(pTopology);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
{
	this->mOrig->VSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
{
	this->mOrig->VSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GetPredication(ID3D11Predicate **ppPredicate, BOOL *pPredicateValue)
{
	this->mOrig->GetPredication(ppPredicate, pPredicateValue);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
{
	this->mOrig->GSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
{
	this->mOrig->GSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView)
{
	this->mOrig->OMGetRenderTargets(NumViews, ppRenderTargetViews, ppDepthStencilView);

	if (ppDepthStencilView != nullptr)
	{
		for (auto runtime : this->mDevice->mRuntimes)
		{
			runtime->OnGetDepthStencilView(*ppDepthStencilView);
		}
	}
}
void STDMETHODCALLTYPE D3D11DeviceContext::OMGetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews)
{
	this->mOrig->OMGetRenderTargetsAndUnorderedAccessViews(NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews);

	if (ppDepthStencilView != nullptr)
	{
		for (auto runtime : this->mDevice->mRuntimes)
		{
			runtime->OnGetDepthStencilView(*ppDepthStencilView);
		}
	}
}
void STDMETHODCALLTYPE D3D11DeviceContext::OMGetBlendState(ID3D11BlendState **ppBlendState, FLOAT BlendFactor[4], UINT *pSampleMask)
{
	this->mOrig->OMGetBlendState(ppBlendState, BlendFactor, pSampleMask);
}
void STDMETHODCALLTYPE D3D11DeviceContext::OMGetDepthStencilState(ID3D11DepthStencilState **ppDepthStencilState, UINT *pStencilRef)
{
	this->mOrig->OMGetDepthStencilState(ppDepthStencilState, pStencilRef);
}
void STDMETHODCALLTYPE D3D11DeviceContext::SOGetTargets(UINT NumBuffers, ID3D11Buffer **ppSOTargets)
{
	this->mOrig->SOGetTargets(NumBuffers, ppSOTargets);
}
void STDMETHODCALLTYPE D3D11DeviceContext::RSGetState(ID3D11RasterizerState **ppRasterizerState)
{
	this->mOrig->RSGetState(ppRasterizerState);
}
void STDMETHODCALLTYPE D3D11DeviceContext::RSGetViewports(UINT *pNumViewports, D3D11_VIEWPORT *pViewports)
{
	this->mOrig->RSGetViewports(pNumViewports, pViewports);
}
void STDMETHODCALLTYPE D3D11DeviceContext::RSGetScissorRects(UINT *pNumRects, D3D11_RECT *pRects)
{
	this->mOrig->RSGetScissorRects(pNumRects, pRects);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
{
	this->mOrig->HSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSGetShader(ID3D11HullShader **ppHullShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
{
	this->mOrig->HSGetShader(ppHullShader, ppClassInstances, pNumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
{
	this->mOrig->HSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
{
	this->mOrig->HSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
{
	this->mOrig->DSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSGetShader(ID3D11DomainShader **ppDomainShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
{
	this->mOrig->DSGetShader(ppDomainShader, ppClassInstances, pNumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
{
	this->mOrig->DSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
{
	this->mOrig->DSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews)
{
	this->mOrig->CSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSGetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews)
{
	this->mOrig->CSGetUnorderedAccessViews(StartSlot, NumUAVs, ppUnorderedAccessViews);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSGetShader(ID3D11ComputeShader **ppComputeShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances)
{
	this->mOrig->CSGetShader(ppComputeShader, ppClassInstances, pNumClassInstances);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers)
{
	this->mOrig->CSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers)
{
	this->mOrig->CSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D11DeviceContext::ClearState()
{
	this->mOrig->ClearState();
}
void STDMETHODCALLTYPE D3D11DeviceContext::Flush()
{
	this->mOrig->Flush();
}
UINT STDMETHODCALLTYPE D3D11DeviceContext::GetContextFlags()
{
	return this->mOrig->GetContextFlags();
}
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::FinishCommandList(BOOL RestoreDeferredContextState, ID3D11CommandList **ppCommandList)
{
	return this->mOrig->FinishCommandList(RestoreDeferredContextState, ppCommandList);
}
D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE D3D11DeviceContext::GetType()
{
	return this->mOrig->GetType();
}

// ID3D11DeviceContext1
void STDMETHODCALLTYPE D3D11DeviceContext::CopySubresourceRegion1(ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox, UINT CopyFlags)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->CopySubresourceRegion1(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox, CopyFlags);
}
void STDMETHODCALLTYPE D3D11DeviceContext::UpdateSubresource1(ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->UpdateSubresource1(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch, CopyFlags);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DiscardResource(ID3D11Resource *pResource)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->DiscardResource(pResource);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DiscardView(ID3D11View *pResourceView)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->DiscardView(pResourceView);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->VSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->HSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->DSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->GSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->PSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->CSSetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::VSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->VSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::HSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->HSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->DSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->GSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::PSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->PSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->CSGetConstantBuffers1(StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
}
void STDMETHODCALLTYPE D3D11DeviceContext::SwapDeviceContextState(ID3DDeviceContextState *pState, ID3DDeviceContextState **ppPreviousState)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->SwapDeviceContextState(pState, ppPreviousState);
}
void STDMETHODCALLTYPE D3D11DeviceContext::ClearView(ID3D11View *pView, const FLOAT Color[4], const D3D11_RECT *pRect, UINT NumRects)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->ClearView(pView, Color, pRect, NumRects);
}
void STDMETHODCALLTYPE D3D11DeviceContext::DiscardView1(ID3D11View *pResourceView, const D3D11_RECT *pRects, UINT NumRects)
{
	assert(this->mInterfaceVersion >= 1);

	static_cast<ID3D11DeviceContext1 *>(this->mOrig)->DiscardView1(pResourceView, pRects, NumRects);
}

// ID3D11DeviceContext2
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::UpdateTileMappings(ID3D11Resource *pTiledResource, UINT NumTiledResourceRegions, const D3D11_TILED_RESOURCE_COORDINATE *pTiledResourceRegionStartCoordinates, const D3D11_TILE_REGION_SIZE *pTiledResourceRegionSizes, ID3D11Buffer *pTilePool, UINT NumRanges, const UINT *pRangeFlags, const UINT *pTilePoolStartOffsets, const UINT *pRangeTileCounts, UINT Flags)
{
	assert(this->mInterfaceVersion >= 2);

	return static_cast<ID3D11DeviceContext2 *>(this->mOrig)->UpdateTileMappings(pTiledResource, NumTiledResourceRegions, pTiledResourceRegionStartCoordinates, pTiledResourceRegionSizes, pTilePool, NumRanges, pRangeFlags, pTilePoolStartOffsets, pRangeTileCounts, Flags);
}
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::CopyTileMappings(ID3D11Resource *pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE *pDestRegionStartCoordinate, ID3D11Resource *pSourceTiledResource, const D3D11_TILED_RESOURCE_COORDINATE *pSourceRegionStartCoordinate, const D3D11_TILE_REGION_SIZE *pTileRegionSize, UINT Flags)
{
	assert(this->mInterfaceVersion >= 2);

	return static_cast<ID3D11DeviceContext2 *>(this->mOrig)->CopyTileMappings(pDestTiledResource, pDestRegionStartCoordinate, pSourceTiledResource, pSourceRegionStartCoordinate, pTileRegionSize, Flags);
}
void STDMETHODCALLTYPE D3D11DeviceContext::CopyTiles(ID3D11Resource *pTiledResource, const D3D11_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE *pTileRegionSize, ID3D11Buffer *pBuffer, UINT64 BufferStartOffsetInBytes, UINT Flags)
{
	assert(this->mInterfaceVersion >= 2);

	static_cast<ID3D11DeviceContext2 *>(this->mOrig)->CopyTiles(pTiledResource, pTileRegionStartCoordinate, pTileRegionSize, pBuffer, BufferStartOffsetInBytes, Flags);
}
void STDMETHODCALLTYPE D3D11DeviceContext::UpdateTiles(ID3D11Resource *pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE *pDestTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE *pDestTileRegionSize, const void *pSourceTileData, UINT Flags)
{
	assert(this->mInterfaceVersion >= 2);

	static_cast<ID3D11DeviceContext2 *>(this->mOrig)->UpdateTiles(pDestTiledResource, pDestTileRegionStartCoordinate, pDestTileRegionSize, pSourceTileData, Flags);
}
HRESULT STDMETHODCALLTYPE D3D11DeviceContext::ResizeTilePool(ID3D11Buffer *pTilePool, UINT64 NewSizeInBytes)
{
	assert(this->mInterfaceVersion >= 2);

	return static_cast<ID3D11DeviceContext2 *>(this->mOrig)->ResizeTilePool(pTilePool, NewSizeInBytes);
}
void STDMETHODCALLTYPE D3D11DeviceContext::TiledResourceBarrier(ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier, ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier)
{
	assert(this->mInterfaceVersion >= 2);

	static_cast<ID3D11DeviceContext2 *>(this->mOrig)->TiledResourceBarrier(pTiledResourceOrViewAccessBeforeBarrier, pTiledResourceOrViewAccessAfterBarrier);
}
BOOL STDMETHODCALLTYPE D3D11DeviceContext::IsAnnotationEnabled()
{
	assert(this->mInterfaceVersion >= 2);

	return static_cast<ID3D11DeviceContext2 *>(this->mOrig)->IsAnnotationEnabled();
}
void STDMETHODCALLTYPE D3D11DeviceContext::SetMarkerInt(LPCWSTR pLabel, INT Data)
{
	assert(this->mInterfaceVersion >= 2);

	static_cast<ID3D11DeviceContext2 *>(this->mOrig)->SetMarkerInt(pLabel, Data);
}
void STDMETHODCALLTYPE D3D11DeviceContext::BeginEventInt(LPCWSTR pLabel, INT Data)
{
	assert(this->mInterfaceVersion >= 2);

	static_cast<ID3D11DeviceContext2 *>(this->mOrig)->BeginEventInt(pLabel, Data);
}
void STDMETHODCALLTYPE D3D11DeviceContext::EndEvent()
{
	assert(this->mInterfaceVersion >= 2);

	static_cast<ID3D11DeviceContext2 *>(this->mOrig)->EndEvent();
}

// ID3D11DeviceContext3
void STDMETHODCALLTYPE D3D11DeviceContext::Flush1(D3D11_CONTEXT_TYPE ContextType, HANDLE hEvent)
{
	assert(this->mInterfaceVersion >= 3);

	static_cast<ID3D11DeviceContext3 *>(this->mOrig)->Flush1(ContextType, hEvent);
}
void STDMETHODCALLTYPE D3D11DeviceContext::SetHardwareProtectionState(BOOL HwProtectionEnable)
{
	assert(this->mInterfaceVersion >= 3);

	static_cast<ID3D11DeviceContext3 *>(this->mOrig)->SetHardwareProtectionState(HwProtectionEnable);
}
void STDMETHODCALLTYPE D3D11DeviceContext::GetHardwareProtectionState(BOOL *pHwProtectionEnable)
{
	assert(this->mInterfaceVersion >= 3);

	static_cast<ID3D11DeviceContext3 *>(this->mOrig)->GetHardwareProtectionState(pHwProtectionEnable);
}

// ID3D11Device
HRESULT STDMETHODCALLTYPE D3D11Device::QueryInterface(REFIID riid, void **ppvObj)
{
	if (ppvObj == nullptr)
	{
		return E_POINTER;
	}
	else if (
		riid == __uuidof(this) ||
		riid == __uuidof(IUnknown) ||
		riid == __uuidof(ID3D11Device) ||
		riid == __uuidof(ID3D11Device1) ||
		riid == __uuidof(ID3D11Device2) ||
		riid == __uuidof(ID3D11Device3))
	{
		#pragma region Update to ID3D11Device1 interface
		if (riid == __uuidof(ID3D11Device1) && this->mInterfaceVersion < 1)
		{
			ID3D11Device1 *device1 = nullptr;
			ID3D11DeviceContext1 *devicecontext1 = nullptr;

			if (FAILED(this->mOrig->QueryInterface(&device1)))
			{
				return E_NOINTERFACE;
			}

			this->mOrig->Release();

			LOG(TRACE) << "Upgraded 'ID3D11Device' object " << this << " to 'ID3D11Device1'.";

			this->mOrig = device1;
			this->mInterfaceVersion = 1;

			this->mImmediateContext->QueryInterface(IID_PPV_ARGS(&devicecontext1));

			devicecontext1->Release();
		}
		#pragma endregion
		#pragma region Update to ID3D11Device2 interface
		if (riid == __uuidof(ID3D11Device2) && this->mInterfaceVersion < 2)
		{
			ID3D11Device2 *device2 = nullptr;
			ID3D11DeviceContext2 *devicecontext2 = nullptr;

			if (FAILED(this->mOrig->QueryInterface(&device2)))
			{
				return E_NOINTERFACE;
			}

			this->mOrig->Release();

			LOG(TRACE) << "Upgraded 'ID3D11Device" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << " to 'ID3D11Device2'.";

			this->mOrig = device2;
			this->mInterfaceVersion = 2;

			this->mImmediateContext->QueryInterface(IID_PPV_ARGS(&devicecontext2));

			devicecontext2->Release();
		}
		#pragma endregion
		#pragma region Update to ID3D11Device3 interface
		if (riid == __uuidof(ID3D11Device3) && this->mInterfaceVersion < 3)
		{
			ID3D11Device3 *device3 = nullptr;
			ID3D11DeviceContext3 *devicecontext3 = nullptr;

			if (FAILED(this->mOrig->QueryInterface(&device3)))
			{
				return E_NOINTERFACE;
			}

			this->mOrig->Release();

			LOG(TRACE) << "Upgraded 'ID3D11Device" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << " to 'ID3D11Device3'.";

			this->mOrig = device3;
			this->mInterfaceVersion = 3;

			this->mImmediateContext->QueryInterface(IID_PPV_ARGS(&devicecontext3));

			devicecontext3->Release();
		}
		#pragma endregion

		AddRef();

		*ppvObj = this;

		return S_OK;
	}
	else if (
		riid == __uuidof(IDXGIObject) ||
		riid == __uuidof(IDXGIDevice) ||
		riid == __uuidof(IDXGIDevice1) ||
		riid == __uuidof(IDXGIDevice2) ||
		riid == __uuidof(IDXGIDevice3))
	{
		assert(this->mDXGIDevice != nullptr);

		return this->mDXGIDevice->QueryInterface(riid, ppvObj);
	}

	return this->mOrig->QueryInterface(riid, ppvObj);
}
ULONG STDMETHODCALLTYPE D3D11Device::AddRef()
{
	this->mRef++;

	assert(this->mDXGIDevice != nullptr);
	assert(this->mImmediateContext != nullptr);

	static_cast<DXGIDevice *>(this->mDXGIDevice)->InternalAddRef();
	this->mImmediateContext->AddRef();

	return this->mOrig->AddRef();
}
ULONG STDMETHODCALLTYPE D3D11Device::Release()
{
	assert(this->mDXGIDevice != nullptr);
	assert(this->mImmediateContext != nullptr);

	static_cast<DXGIDevice *>(this->mDXGIDevice)->InternalRelease();
	this->mImmediateContext->Release();

	ULONG ref = this->mOrig->Release();

	if (--this->mRef == 0 && ref != 0)
	{
		LOG(WARNING) << "Reference count for 'ID3D11Device" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << " is inconsistent: " << ref << ", but expected 0.";

		ref = 0;
	}

	if (ref == 0)
	{
		assert(this->mRef <= 0);

		LOG(TRACE) << "Destroyed 'ID3D11Device" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << ".";

		delete this;
	}

	return ref;
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateBuffer(const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer)
{
	return this->mOrig->CreateBuffer(pDesc, pInitialData, ppBuffer);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateTexture1D(const D3D11_TEXTURE1D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture1D **ppTexture1D)
{
	return this->mOrig->CreateTexture1D(pDesc, pInitialData, ppTexture1D);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture2D)
{
	return this->mOrig->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateTexture3D(const D3D11_TEXTURE3D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture3D **ppTexture3D)
{
	return this->mOrig->CreateTexture3D(pDesc, pInitialData, ppTexture3D);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateShaderResourceView(ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView)
{
	return this->mOrig->CreateShaderResourceView(pResource, pDesc, ppSRView);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateUnorderedAccessView(ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView)
{
	return this->mOrig->CreateUnorderedAccessView(pResource, pDesc, ppUAView);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateRenderTargetView(ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView **ppRTView)
{
	return this->mOrig->CreateRenderTargetView(pResource, pDesc, ppRTView);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateDepthStencilView(ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView)
{
	if (ppDepthStencilView == nullptr)
	{
		return E_INVALIDARG;
	}

	const HRESULT hr = this->mOrig->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView);

	if (FAILED(hr))
	{
		return hr;
	}

	for (auto runtime : this->mRuntimes)
	{
		runtime->OnCreateDepthStencilView(pResource, *ppDepthStencilView);
	}

	D3D11Device *const device = this;
	ID3D11DepthStencilView *const depthstencil = *ppDepthStencilView;
	device->AddRef();
	depthstencil->SetPrivateData(__uuidof(device), sizeof(device), &device);

	ReShade::Hooks::Install(VTABLE(depthstencil), 2, reinterpret_cast<ReShade::Hook::Function>(&ID3D11DepthStencilView_Release));

	return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements, const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, ID3D11InputLayout **ppInputLayout)
{
	return this->mOrig->CreateInputLayout(pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11VertexShader **ppVertexShader)
{
	return this->mOrig->CreateVertexShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11GeometryShader **ppGeometryShader)
{
	return this->mOrig->CreateGeometryShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppGeometryShader);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const D3D11_SO_DECLARATION_ENTRY *pSODeclaration, UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ID3D11ClassLinkage *pClassLinkage, ID3D11GeometryShader **ppGeometryShader)
{
	return this->mOrig->CreateGeometryShaderWithStreamOutput(pShaderBytecode, BytecodeLength, pSODeclaration, NumEntries, pBufferStrides, NumStrides, RasterizedStream, pClassLinkage, ppGeometryShader);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11PixelShader **ppPixelShader)
{
	return this->mOrig->CreatePixelShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11HullShader **ppHullShader)
{
	return this->mOrig->CreateHullShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppHullShader);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11DomainShader **ppDomainShader)
{
	return this->mOrig->CreateDomainShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppDomainShader);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11ComputeShader **ppComputeShader)
{
	return this->mOrig->CreateComputeShader(pShaderBytecode, BytecodeLength, pClassLinkage, ppComputeShader);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateClassLinkage(ID3D11ClassLinkage **ppLinkage)
{
	return this->mOrig->CreateClassLinkage(ppLinkage);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateBlendState(const D3D11_BLEND_DESC *pBlendStateDesc, ID3D11BlendState **ppBlendState)
{
	return this->mOrig->CreateBlendState(pBlendStateDesc, ppBlendState);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc, ID3D11DepthStencilState **ppDepthStencilState)
{
	return this->mOrig->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateRasterizerState(const D3D11_RASTERIZER_DESC *pRasterizerDesc, ID3D11RasterizerState **ppRasterizerState)
{
	return this->mOrig->CreateRasterizerState(pRasterizerDesc, ppRasterizerState);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateSamplerState(const D3D11_SAMPLER_DESC *pSamplerDesc, ID3D11SamplerState **ppSamplerState)
{
	return this->mOrig->CreateSamplerState(pSamplerDesc, ppSamplerState);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateQuery(const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery)
{
	return this->mOrig->CreateQuery(pQueryDesc, ppQuery);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreatePredicate(const D3D11_QUERY_DESC *pPredicateDesc, ID3D11Predicate **ppPredicate)
{
	return this->mOrig->CreatePredicate(pPredicateDesc, ppPredicate);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateCounter(const D3D11_COUNTER_DESC *pCounterDesc, ID3D11Counter **ppCounter)
{
	return this->mOrig->CreateCounter(pCounterDesc, ppCounter);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateDeferredContext(UINT ContextFlags, ID3D11DeviceContext **ppDeferredContext)
{
	LOG(INFO) << "Redirecting '" << "ID3D11Device::CreateDeferredContext" << "(" << this << ", " << ContextFlags << ", " << ppDeferredContext << ")' ...";

	if (ppDeferredContext == nullptr)
	{
		return E_INVALIDARG;
	}

	const HRESULT hr = this->mOrig->CreateDeferredContext(ContextFlags, ppDeferredContext);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'ID3D11Device::CreateDeferredContext' failed with '" << GetErrorString(hr) << "'!";

		return hr;
	}

	*ppDeferredContext = new D3D11DeviceContext(this, *ppDeferredContext);

	return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D11Device::OpenSharedResource(HANDLE hResource, REFIID ReturnedInterface, void **ppResource)
{
	return this->mOrig->OpenSharedResource(hResource, ReturnedInterface, ppResource);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CheckFormatSupport(DXGI_FORMAT Format, UINT *pFormatSupport)
{
	return this->mOrig->CheckFormatSupport(Format, pFormatSupport);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CheckMultisampleQualityLevels(DXGI_FORMAT Format, UINT SampleCount, UINT *pNumQualityLevels)
{
	return this->mOrig->CheckMultisampleQualityLevels(Format, SampleCount, pNumQualityLevels);
}
void STDMETHODCALLTYPE D3D11Device::CheckCounterInfo(D3D11_COUNTER_INFO *pCounterInfo)
{
	this->mOrig->CheckCounterInfo(pCounterInfo);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CheckCounter(const D3D11_COUNTER_DESC *pDesc, D3D11_COUNTER_TYPE *pType, UINT *pActiveCounters, LPSTR szName, UINT *pNameLength, LPSTR szUnits, UINT *pUnitsLength, LPSTR szDescription, UINT *pDescriptionLength)
{
	return this->mOrig->CheckCounter(pDesc, pType, pActiveCounters, szName, pNameLength, szUnits, pUnitsLength, szDescription, pDescriptionLength);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CheckFeatureSupport(D3D11_FEATURE Feature, void *pFeatureSupportData, UINT FeatureSupportDataSize)
{
	return this->mOrig->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
}
HRESULT STDMETHODCALLTYPE D3D11Device::GetPrivateData(REFGUID guid, UINT *pDataSize, void *pData)
{
	return this->mOrig->GetPrivateData(guid, pDataSize, pData);
}
HRESULT STDMETHODCALLTYPE D3D11Device::SetPrivateData(REFGUID guid, UINT DataSize, const void *pData)
{
	return this->mOrig->SetPrivateData(guid, DataSize, pData);
}
HRESULT STDMETHODCALLTYPE D3D11Device::SetPrivateDataInterface(REFGUID guid, const IUnknown *pData)
{
	return this->mOrig->SetPrivateDataInterface(guid, pData);
}
UINT STDMETHODCALLTYPE D3D11Device::GetCreationFlags()
{
	return this->mOrig->GetCreationFlags();
}
HRESULT STDMETHODCALLTYPE D3D11Device::GetDeviceRemovedReason()
{
	return this->mOrig->GetDeviceRemovedReason();
}
void STDMETHODCALLTYPE D3D11Device::GetImmediateContext(ID3D11DeviceContext **ppImmediateContext)
{
	if (ppImmediateContext == nullptr)
	{
		return;
	}

	assert(this->mImmediateContext != nullptr);

	this->mImmediateContext->AddRef();

	*ppImmediateContext = this->mImmediateContext;
}
HRESULT STDMETHODCALLTYPE D3D11Device::SetExceptionMode(UINT RaiseFlags)
{
	return this->mOrig->SetExceptionMode(RaiseFlags);
}
UINT STDMETHODCALLTYPE D3D11Device::GetExceptionMode()
{
	return this->mOrig->GetExceptionMode();
}
D3D_FEATURE_LEVEL STDMETHODCALLTYPE D3D11Device::GetFeatureLevel()
{
	return this->mOrig->GetFeatureLevel();
}

// ID3D11Device1
void STDMETHODCALLTYPE D3D11Device::GetImmediateContext1(ID3D11DeviceContext1 **ppImmediateContext)
{
	if (ppImmediateContext == nullptr)
	{
		return;
	}

	assert(this->mInterfaceVersion >= 1);
	assert(this->mImmediateContext != nullptr);
	assert(this->mImmediateContext->mInterfaceVersion >= 1);

	this->mImmediateContext->AddRef();

	*ppImmediateContext = this->mImmediateContext;
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateDeferredContext1(UINT ContextFlags, ID3D11DeviceContext1 **ppDeferredContext)
{
	LOG(INFO) << "Redirecting '" << "ID3D11Device1::CreateDeferredContext1" << "(" << this << ", " << ContextFlags << ", " << ppDeferredContext << ")' ...";

	if (ppDeferredContext == nullptr)
	{
		return E_INVALIDARG;
	}

	assert(this->mInterfaceVersion >= 1);

	const HRESULT hr = static_cast<ID3D11Device1 *>(this->mOrig)->CreateDeferredContext1(ContextFlags, ppDeferredContext);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'ID3D11Device1::CreateDeferredContext1' failed with '" << GetErrorString(hr) << "'!";

		return hr;
	}

	*ppDeferredContext = new D3D11DeviceContext(this, *ppDeferredContext);

	return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateBlendState1(const D3D11_BLEND_DESC1 *pBlendStateDesc, ID3D11BlendState1 **ppBlendState)
{
	assert(this->mInterfaceVersion >= 1);

	return static_cast<ID3D11Device1 *>(this->mOrig)->CreateBlendState1(pBlendStateDesc, ppBlendState);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateRasterizerState1(const D3D11_RASTERIZER_DESC1 *pRasterizerDesc, ID3D11RasterizerState1 **ppRasterizerState)
{
	assert(this->mInterfaceVersion >= 1);

	return static_cast<ID3D11Device1 *>(this->mOrig)->CreateRasterizerState1(pRasterizerDesc, ppRasterizerState);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateDeviceContextState(UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, REFIID EmulatedInterface, D3D_FEATURE_LEVEL *pChosenFeatureLevel, ID3DDeviceContextState **ppContextState)
{
	assert(this->mInterfaceVersion >= 1);

	return static_cast<ID3D11Device1 *>(this->mOrig)->CreateDeviceContextState(Flags, pFeatureLevels, FeatureLevels, SDKVersion, EmulatedInterface, pChosenFeatureLevel, ppContextState);
}
HRESULT STDMETHODCALLTYPE D3D11Device::OpenSharedResource1(HANDLE hResource, REFIID returnedInterface, void **ppResource)
{
	assert(this->mInterfaceVersion >= 1);

	return static_cast<ID3D11Device1 *>(this->mOrig)->OpenSharedResource1(hResource, returnedInterface, ppResource);
}
HRESULT STDMETHODCALLTYPE D3D11Device::OpenSharedResourceByName(LPCWSTR lpName, DWORD dwDesiredAccess, REFIID returnedInterface, void **ppResource)
{
	assert(this->mInterfaceVersion >= 1);

	return static_cast<ID3D11Device1 *>(this->mOrig)->OpenSharedResourceByName(lpName, dwDesiredAccess, returnedInterface, ppResource);
}

// ID3D11Device2
void STDMETHODCALLTYPE D3D11Device::GetImmediateContext2(ID3D11DeviceContext2 **ppImmediateContext)
{
	assert(this->mInterfaceVersion >= 2);

	static_cast<ID3D11Device2 *>(this->mOrig)->GetImmediateContext2(ppImmediateContext);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateDeferredContext2(UINT ContextFlags, ID3D11DeviceContext2 **ppDeferredContext)
{
	assert(this->mInterfaceVersion >= 2);

	return static_cast<ID3D11Device2 *>(this->mOrig)->CreateDeferredContext2(ContextFlags, ppDeferredContext);
}
void STDMETHODCALLTYPE D3D11Device::GetResourceTiling(ID3D11Resource *pTiledResource, UINT *pNumTilesForEntireResource, D3D11_PACKED_MIP_DESC *pPackedMipDesc, D3D11_TILE_SHAPE *pStandardTileShapeForNonPackedMips, UINT *pNumSubresourceTilings, UINT FirstSubresourceTilingToGet, D3D11_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips)
{
	assert(this->mInterfaceVersion >= 2);

	static_cast<ID3D11Device2 *>(this->mOrig)->GetResourceTiling(pTiledResource, pNumTilesForEntireResource, pPackedMipDesc, pStandardTileShapeForNonPackedMips, pNumSubresourceTilings, FirstSubresourceTilingToGet, pSubresourceTilingsForNonPackedMips);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CheckMultisampleQualityLevels1(DXGI_FORMAT Format, UINT SampleCount, UINT Flags, UINT *pNumQualityLevels)
{
	assert(this->mInterfaceVersion >= 2);

	return static_cast<ID3D11Device2 *>(this->mOrig)->CheckMultisampleQualityLevels1(Format, SampleCount, Flags, pNumQualityLevels);
}

// ID3D11Device3
HRESULT STDMETHODCALLTYPE D3D11Device::CreateTexture2D1(const D3D11_TEXTURE2D_DESC1 *pDesc1, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D1 **ppTexture2D)
{
	assert(this->mInterfaceVersion >= 3);

	return static_cast<ID3D11Device3 *>(this->mOrig)->CreateTexture2D1(pDesc1, pInitialData, ppTexture2D);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateTexture3D1(const D3D11_TEXTURE3D_DESC1 *pDesc1, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture3D1 **ppTexture3D)
{
	assert(this->mInterfaceVersion >= 3);

	return static_cast<ID3D11Device3 *>(this->mOrig)->CreateTexture3D1(pDesc1, pInitialData, ppTexture3D);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateRasterizerState2(const D3D11_RASTERIZER_DESC2 *pRasterizerDesc, ID3D11RasterizerState2 **ppRasterizerState)
{
	assert(this->mInterfaceVersion >= 3);

	return static_cast<ID3D11Device3 *>(this->mOrig)->CreateRasterizerState2(pRasterizerDesc, ppRasterizerState);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateShaderResourceView1(ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1, ID3D11ShaderResourceView1 **ppSRView1)
{
	assert(this->mInterfaceVersion >= 3);

	return static_cast<ID3D11Device3 *>(this->mOrig)->CreateShaderResourceView1(pResource, pDesc1, ppSRView1);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateUnorderedAccessView1(ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc1, ID3D11UnorderedAccessView1 **ppUAView1)
{
	assert(this->mInterfaceVersion >= 3);

	return static_cast<ID3D11Device3 *>(this->mOrig)->CreateUnorderedAccessView1(pResource, pDesc1, ppUAView1);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateRenderTargetView1(ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc1, ID3D11RenderTargetView1 **ppRTView1)
{
	assert(this->mInterfaceVersion >= 3);

	return static_cast<ID3D11Device3 *>(this->mOrig)->CreateRenderTargetView1(pResource, pDesc1, ppRTView1);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateQuery1(const D3D11_QUERY_DESC1 *pQueryDesc1, ID3D11Query1 **ppQuery1)
{
	assert(this->mInterfaceVersion >= 3);

	return static_cast<ID3D11Device3 *>(this->mOrig)->CreateQuery1(pQueryDesc1, ppQuery1);
}
void STDMETHODCALLTYPE D3D11Device::GetImmediateContext3(ID3D11DeviceContext3 **ppImmediateContext)
{
	assert(this->mInterfaceVersion >= 3);

	static_cast<ID3D11Device3 *>(this->mOrig)->GetImmediateContext3(ppImmediateContext);
}
HRESULT STDMETHODCALLTYPE D3D11Device::CreateDeferredContext3(UINT ContextFlags, ID3D11DeviceContext3 **ppDeferredContext)
{
	assert(this->mInterfaceVersion >= 3);

	return static_cast<ID3D11Device3 *>(this->mOrig)->CreateDeferredContext3(ContextFlags, ppDeferredContext);
}
void STDMETHODCALLTYPE D3D11Device::WriteToSubresource(ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
	assert(this->mInterfaceVersion >= 3);

	static_cast<ID3D11Device3 *>(this->mOrig)->WriteToSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}
void STDMETHODCALLTYPE D3D11Device::ReadFromSubresource(void *pDstData, UINT DstRowPitch, UINT DstDepthPitch, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox)
{
	assert(this->mInterfaceVersion >= 3);

	static_cast<ID3D11Device3 *>(this->mOrig)->ReadFromSubresource(pDstData, DstRowPitch, DstDepthPitch, pSrcResource, SrcSubresource, pSrcBox);
}

// D3D11
EXPORT HRESULT WINAPI D3D11CreateDevice(IDXGIAdapter *pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, ID3D11Device **ppDevice, D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext)
{
	LOG(INFO) << "Redirecting '" << "D3D11CreateDevice" << "(" << pAdapter << ", " << DriverType << ", " << Software << ", " << std::showbase << std::hex << Flags << std::dec << std::noshowbase << ", " << pFeatureLevels << ", " << FeatureLevels << ", " << SDKVersion << ", " << ppDevice << ", " << pFeatureLevel << ", " << ppImmediateContext << ")' ...";
	LOG(INFO) << "> Passing on to 'D3D11CreateDeviceAndSwapChain':";

	return D3D11CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, nullptr, nullptr, ppDevice, pFeatureLevel, ppImmediateContext);
}
EXPORT HRESULT WINAPI D3D11CreateDeviceAndSwapChain(IDXGIAdapter *pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, IDXGISwapChain **ppSwapChain, ID3D11Device **ppDevice, D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext)
{
	LOG(INFO) << "Redirecting '" << "D3D11CreateDeviceAndSwapChain" << "(" << pAdapter << ", " << DriverType << ", " << Software << ", " << std::showbase << std::hex << Flags << std::dec << std::noshowbase << ", " << pFeatureLevels << ", " << FeatureLevels << ", " << SDKVersion << ", " << pSwapChainDesc << ", " << ppSwapChain << ", " << ppDevice << ", " << pFeatureLevel << ", " << ppImmediateContext << ")' ...";

#ifdef _DEBUG
	Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;

	HRESULT hr = ReShade::Hooks::Call(&D3D11CreateDeviceAndSwapChain)(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, nullptr, nullptr, ppDevice, &FeatureLevel, nullptr);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'D3D11CreateDeviceAndSwapChain' failed with '" << GetErrorString(hr) << "'!";

		return hr;
	}

	LOG(TRACE) << "> Using feature level " << std::showbase << std::hex << FeatureLevel << std::dec << std::noshowbase << ".";

	if (ppDevice != nullptr)
	{
		IDXGIDevice *dxgidevice = nullptr;
		ID3D11Device *const device = *ppDevice;
		ID3D11DeviceContext *devicecontext = nullptr;
		device->QueryInterface(&dxgidevice);
		device->GetImmediateContext(&devicecontext);

		assert(device != nullptr);
		assert(dxgidevice != nullptr);
		assert(devicecontext != nullptr);

		D3D11Device *const deviceProxy = new D3D11Device(device);
		D3D11DeviceContext *const devicecontextProxy = new D3D11DeviceContext(deviceProxy, devicecontext);
		deviceProxy->mDXGIDevice = new DXGIDevice(dxgidevice, deviceProxy);
		deviceProxy->mImmediateContext = devicecontextProxy;

		if (pSwapChainDesc != nullptr)
		{
			assert(ppSwapChain != nullptr);

			if (pAdapter != nullptr)
			{
				pAdapter->AddRef();
			}
			else
			{
				hr = deviceProxy->mDXGIDevice->GetAdapter(&pAdapter);

				assert(SUCCEEDED(hr));
			}

			IDXGIFactory *factory = nullptr;

			hr = pAdapter->GetParent(IID_PPV_ARGS(&factory));

			assert(SUCCEEDED(hr));

			hr = factory->CreateSwapChain(deviceProxy, const_cast<DXGI_SWAP_CHAIN_DESC *>(pSwapChainDesc), ppSwapChain);

			factory->Release();
			pAdapter->Release();
		}

		if (SUCCEEDED(hr))
		{
			*ppDevice = deviceProxy;

			if (ppImmediateContext != nullptr)
			{
				devicecontextProxy->AddRef();
				*ppImmediateContext = devicecontextProxy;
			}

			LOG(TRACE) << "> Returned device objects: " << deviceProxy << ", " << deviceProxy->mDXGIDevice;
		}
		else
		{
			deviceProxy->Release();
		}
	}

	if (pFeatureLevel != nullptr)
	{
		*pFeatureLevel = FeatureLevel;
	}

	return hr;
}
