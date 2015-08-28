#include "Log.hpp"
#include "Hooks\DXGI.hpp"
#include "Runtimes\D3D10Runtime.hpp"

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

// ID3D10DepthStencilView
ULONG STDMETHODCALLTYPE ID3D10DepthStencilView_Release(ID3D10DepthStencilView *pDepthStencilView)
{
	static const auto trampoline = ReShade::Hooks::Call(&ID3D10DepthStencilView_Release);

	D3D10Device *device = nullptr;
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

// ID3D10Device
HRESULT STDMETHODCALLTYPE D3D10Device::QueryInterface(REFIID riid, void **ppvObj)
{
	if (ppvObj == nullptr)
	{
		return E_POINTER;
	}
	else if (
		riid == __uuidof(this) ||
		riid == __uuidof(IUnknown) ||
		riid == __uuidof(ID3D10Device) ||
		riid == __uuidof(ID3D10Device1))
	{
		#pragma region Update to ID3D10Device1 interface
		if (riid == __uuidof(ID3D10Device1) && this->mInterfaceVersion < 1)
		{
			ID3D10Device1 *device1 = nullptr;

			if (FAILED(this->mOrig->QueryInterface(&device1)))
			{
				return E_NOINTERFACE;
			}

			this->mOrig->Release();

			LOG(TRACE) << "Upgraded 'ID3D10Device' object " << this << " to 'ID3D10Device1'.";

			this->mOrig = device1;
			this->mInterfaceVersion = 1;
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
ULONG STDMETHODCALLTYPE D3D10Device::AddRef()
{
	this->mRef++;

	assert(this->mDXGIDevice != nullptr);

	static_cast<DXGIDevice *>(this->mDXGIDevice)->InternalAddRef();

	return this->mOrig->AddRef();
}
ULONG STDMETHODCALLTYPE D3D10Device::Release()
{
	assert(this->mDXGIDevice != nullptr);

	static_cast<DXGIDevice *>(this->mDXGIDevice)->InternalRelease();

	ULONG ref = this->mOrig->Release();

	if (--this->mRef == 0 && ref != 0)
	{
		LOG(WARNING) << "Reference count for 'ID3D10Device" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << " is inconsistent: " << ref << ", but expected 0.";

		ref = 0;
	}

	if (ref == 0)
	{
		assert(this->mRef <= 0);

		LOG(TRACE) << "Destroyed 'ID3D10Device" << (this->mInterfaceVersion > 0 ? std::to_string(this->mInterfaceVersion) : "") << "' object " << this << ".";

		delete this;
	}

	return ref;
}
void STDMETHODCALLTYPE D3D10Device::VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D10Buffer *const *ppConstantBuffers)
{
	this->mOrig->VSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D10Device::PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D10ShaderResourceView *const *ppShaderResourceViews)
{
	this->mOrig->PSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D10Device::PSSetShader(ID3D10PixelShader *pPixelShader)
{
	this->mOrig->PSSetShader(pPixelShader);
}
void STDMETHODCALLTYPE D3D10Device::PSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D10SamplerState *const *ppSamplers)
{
	this->mOrig->PSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D10Device::VSSetShader(ID3D10VertexShader *pVertexShader)
{
	this->mOrig->VSSetShader(pVertexShader);
}
void STDMETHODCALLTYPE D3D10Device::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	for (auto runtime : this->mRuntimes)
	{
		runtime->OnDrawCall(IndexCount);
	}

	this->mOrig->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}
void STDMETHODCALLTYPE D3D10Device::Draw(UINT VertexCount, UINT StartVertexLocation)
{
	for (auto runtime : this->mRuntimes)
	{
		runtime->OnDrawCall(VertexCount);
	}

	this->mOrig->Draw(VertexCount, StartVertexLocation);
}
void STDMETHODCALLTYPE D3D10Device::PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D10Buffer *const *ppConstantBuffers)
{
	this->mOrig->PSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D10Device::IASetInputLayout(ID3D10InputLayout *pInputLayout)
{
	this->mOrig->IASetInputLayout(pInputLayout);
}
void STDMETHODCALLTYPE D3D10Device::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D10Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets)
{
	this->mOrig->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}
void STDMETHODCALLTYPE D3D10Device::IASetIndexBuffer(ID3D10Buffer *pIndexBuffer, DXGI_FORMAT Format, UINT Offset)
{
	this->mOrig->IASetIndexBuffer(pIndexBuffer, Format, Offset);
}
void STDMETHODCALLTYPE D3D10Device::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
	for (auto runtime : this->mRuntimes)
	{
		runtime->OnDrawCall(IndexCountPerInstance * InstanceCount);
	}

	this->mOrig->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}
void STDMETHODCALLTYPE D3D10Device::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
	for (auto runtime : this->mRuntimes)
	{
		runtime->OnDrawCall(VertexCountPerInstance * InstanceCount);
	}

	this->mOrig->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}
void STDMETHODCALLTYPE D3D10Device::GSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D10Buffer *const *ppConstantBuffers)
{
	this->mOrig->GSSetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D10Device::GSSetShader(ID3D10GeometryShader *pShader)
{
	this->mOrig->GSSetShader(pShader);
}
void STDMETHODCALLTYPE D3D10Device::IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY Topology)
{
	this->mOrig->IASetPrimitiveTopology(Topology);
}
void STDMETHODCALLTYPE D3D10Device::VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D10ShaderResourceView *const *ppShaderResourceViews)
{
	this->mOrig->VSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D10Device::VSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D10SamplerState *const *ppSamplers)
{
	this->mOrig->VSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D10Device::SetPredication(ID3D10Predicate *pPredicate, BOOL PredicateValue)
{
	this->mOrig->SetPredication(pPredicate, PredicateValue);
}
void STDMETHODCALLTYPE D3D10Device::GSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D10ShaderResourceView *const *ppShaderResourceViews)
{
	this->mOrig->GSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D10Device::GSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D10SamplerState *const *ppSamplers)
{
	this->mOrig->GSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D10Device::OMSetRenderTargets(UINT NumViews, ID3D10RenderTargetView *const *ppRenderTargetViews, ID3D10DepthStencilView *pDepthStencilView)
{
	if (pDepthStencilView != nullptr)
	{
		for (auto runtime : this->mRuntimes)
		{
			runtime->OnSetDepthStencilView(pDepthStencilView);
		}
	}

	this->mOrig->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
}
void STDMETHODCALLTYPE D3D10Device::OMSetBlendState(ID3D10BlendState *pBlendState, const FLOAT BlendFactor[4], UINT SampleMask)
{
	this->mOrig->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
}
void STDMETHODCALLTYPE D3D10Device::OMSetDepthStencilState(ID3D10DepthStencilState *pDepthStencilState, UINT StencilRef)
{
	this->mOrig->OMSetDepthStencilState(pDepthStencilState, StencilRef);
}
void STDMETHODCALLTYPE D3D10Device::SOSetTargets(UINT NumBuffers, ID3D10Buffer *const *ppSOTargets, const UINT *pOffsets)
{
	this->mOrig->SOSetTargets(NumBuffers, ppSOTargets, pOffsets);
}
void STDMETHODCALLTYPE D3D10Device::DrawAuto()
{
	this->mOrig->DrawAuto();
}
void STDMETHODCALLTYPE D3D10Device::RSSetState(ID3D10RasterizerState *pRasterizerState)
{
	this->mOrig->RSSetState(pRasterizerState);
}
void STDMETHODCALLTYPE D3D10Device::RSSetViewports(UINT NumViewports, const D3D10_VIEWPORT *pViewports)
{
	this->mOrig->RSSetViewports(NumViewports, pViewports);
}
void STDMETHODCALLTYPE D3D10Device::RSSetScissorRects(UINT NumRects, const D3D10_RECT *pRects)
{
	this->mOrig->RSSetScissorRects(NumRects, pRects);
}
void STDMETHODCALLTYPE D3D10Device::CopySubresourceRegion(ID3D10Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D10Resource *pSrcResource, UINT SrcSubresource, const D3D10_BOX *pSrcBox)
{
	this->mOrig->CopySubresourceRegion(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
}
void STDMETHODCALLTYPE D3D10Device::CopyResource(ID3D10Resource *pDstResource, ID3D10Resource *pSrcResource)
{
	for (auto runtime : this->mRuntimes)
	{
		runtime->OnCopyResource(pDstResource, pSrcResource);
	}

	this->mOrig->CopyResource(pDstResource, pSrcResource);
}
void STDMETHODCALLTYPE D3D10Device::UpdateSubresource(ID3D10Resource *pDstResource, UINT DstSubresource, const D3D10_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
	this->mOrig->UpdateSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}
void STDMETHODCALLTYPE D3D10Device::ClearRenderTargetView(ID3D10RenderTargetView *pRenderTargetView, const FLOAT ColorRGBA[4])
{
	this->mOrig->ClearRenderTargetView(pRenderTargetView, ColorRGBA);
}
void STDMETHODCALLTYPE D3D10Device::ClearDepthStencilView(ID3D10DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
	for (auto runtime : this->mRuntimes)
	{
		runtime->OnClearDepthStencilView(pDepthStencilView);
	}

	this->mOrig->ClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil);
}
void STDMETHODCALLTYPE D3D10Device::GenerateMips(ID3D10ShaderResourceView *pShaderResourceView)
{
	this->mOrig->GenerateMips(pShaderResourceView);
}
void STDMETHODCALLTYPE D3D10Device::ResolveSubresource(ID3D10Resource *pDstResource, UINT DstSubresource, ID3D10Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
{
	this->mOrig->ResolveSubresource(pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
}
void STDMETHODCALLTYPE D3D10Device::VSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D10Buffer **ppConstantBuffers)
{
	this->mOrig->VSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D10Device::PSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D10ShaderResourceView **ppShaderResourceViews)
{
	this->mOrig->PSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D10Device::PSGetShader(ID3D10PixelShader **ppPixelShader)
{
	this->mOrig->PSGetShader(ppPixelShader);
}
void STDMETHODCALLTYPE D3D10Device::PSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D10SamplerState **ppSamplers)
{
	this->mOrig->PSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D10Device::VSGetShader(ID3D10VertexShader **ppVertexShader)
{
	this->mOrig->VSGetShader(ppVertexShader);
}
void STDMETHODCALLTYPE D3D10Device::PSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D10Buffer **ppConstantBuffers)
{
	this->mOrig->PSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D10Device::IAGetInputLayout(ID3D10InputLayout **ppInputLayout)
{
	this->mOrig->IAGetInputLayout(ppInputLayout);
}
void STDMETHODCALLTYPE D3D10Device::IAGetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D10Buffer **ppVertexBuffers, UINT *pStrides, UINT *pOffsets)
{
	this->mOrig->IAGetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}
void STDMETHODCALLTYPE D3D10Device::IAGetIndexBuffer(ID3D10Buffer **pIndexBuffer, DXGI_FORMAT *Format, UINT *Offset)
{
	this->mOrig->IAGetIndexBuffer(pIndexBuffer, Format, Offset);
}
void STDMETHODCALLTYPE D3D10Device::GSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D10Buffer **ppConstantBuffers)
{
	this->mOrig->GSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
}
void STDMETHODCALLTYPE D3D10Device::GSGetShader(ID3D10GeometryShader **ppGeometryShader)
{
	this->mOrig->GSGetShader(ppGeometryShader);
}
void STDMETHODCALLTYPE D3D10Device::IAGetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY *pTopology)
{
	this->mOrig->IAGetPrimitiveTopology(pTopology);
}
void STDMETHODCALLTYPE D3D10Device::VSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D10ShaderResourceView **ppShaderResourceViews)
{
	this->mOrig->VSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D10Device::VSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D10SamplerState **ppSamplers)
{
	this->mOrig->VSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D10Device::GetPredication(ID3D10Predicate **ppPredicate, BOOL *pPredicateValue)
{
	this->mOrig->GetPredication(ppPredicate, pPredicateValue);
}
void STDMETHODCALLTYPE D3D10Device::GSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D10ShaderResourceView **ppShaderResourceViews)
{
	this->mOrig->GSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
}
void STDMETHODCALLTYPE D3D10Device::GSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D10SamplerState **ppSamplers)
{
	this->mOrig->GSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}
void STDMETHODCALLTYPE D3D10Device::OMGetRenderTargets(UINT NumViews, ID3D10RenderTargetView **ppRenderTargetViews, ID3D10DepthStencilView **ppDepthStencilView)
{
	this->mOrig->OMGetRenderTargets(NumViews, ppRenderTargetViews, ppDepthStencilView);

	if (ppDepthStencilView != nullptr)
	{
		for (auto runtime : this->mRuntimes)
		{
			runtime->OnGetDepthStencilView(*ppDepthStencilView);
		}
	}
}
void STDMETHODCALLTYPE D3D10Device::OMGetBlendState(ID3D10BlendState **ppBlendState, FLOAT BlendFactor[4], UINT *pSampleMask)
{
	this->mOrig->OMGetBlendState(ppBlendState, BlendFactor, pSampleMask);
}
void STDMETHODCALLTYPE D3D10Device::OMGetDepthStencilState(ID3D10DepthStencilState **ppDepthStencilState, UINT *pStencilRef)
{
	this->mOrig->OMGetDepthStencilState(ppDepthStencilState, pStencilRef);
}
void STDMETHODCALLTYPE D3D10Device::SOGetTargets(UINT NumBuffers, ID3D10Buffer **ppSOTargets, UINT *pOffsets)
{
	this->mOrig->SOGetTargets(NumBuffers, ppSOTargets, pOffsets);
}
void STDMETHODCALLTYPE D3D10Device::RSGetState(ID3D10RasterizerState **ppRasterizerState)
{
	this->mOrig->RSGetState(ppRasterizerState);
}
void STDMETHODCALLTYPE D3D10Device::RSGetViewports(UINT *NumViewports, D3D10_VIEWPORT *pViewports)
{
	this->mOrig->RSGetViewports(NumViewports, pViewports);
}
void STDMETHODCALLTYPE D3D10Device::RSGetScissorRects(UINT *NumRects, D3D10_RECT *pRects)
{
	this->mOrig->RSGetScissorRects(NumRects, pRects);
}
HRESULT STDMETHODCALLTYPE D3D10Device::GetDeviceRemovedReason()
{
	return this->mOrig->GetDeviceRemovedReason();
}
HRESULT STDMETHODCALLTYPE D3D10Device::SetExceptionMode(UINT RaiseFlags)
{
	return this->mOrig->SetExceptionMode(RaiseFlags);
}
UINT STDMETHODCALLTYPE D3D10Device::GetExceptionMode()
{
	return this->mOrig->GetExceptionMode();
}
HRESULT STDMETHODCALLTYPE D3D10Device::GetPrivateData(REFGUID guid, UINT *pDataSize, void *pData)
{
	return this->mOrig->GetPrivateData(guid, pDataSize, pData);
}
HRESULT STDMETHODCALLTYPE D3D10Device::SetPrivateData(REFGUID guid, UINT DataSize, const void *pData)
{
	return this->mOrig->SetPrivateData(guid, DataSize, pData);
}
HRESULT STDMETHODCALLTYPE D3D10Device::SetPrivateDataInterface(REFGUID guid, const IUnknown *pData)
{
	return this->mOrig->SetPrivateDataInterface(guid, pData);
}
void STDMETHODCALLTYPE D3D10Device::ClearState()
{
	this->mOrig->ClearState();
}
void STDMETHODCALLTYPE D3D10Device::Flush()
{
	this->mOrig->Flush();
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateBuffer(const D3D10_BUFFER_DESC *pDesc, const D3D10_SUBRESOURCE_DATA *pInitialData, ID3D10Buffer **ppBuffer)
{
	return this->mOrig->CreateBuffer(pDesc, pInitialData, ppBuffer);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateTexture1D(const D3D10_TEXTURE1D_DESC *pDesc, const D3D10_SUBRESOURCE_DATA *pInitialData, ID3D10Texture1D **ppTexture1D)
{
	return this->mOrig->CreateTexture1D(pDesc, pInitialData, ppTexture1D);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateTexture2D(const D3D10_TEXTURE2D_DESC *pDesc, const D3D10_SUBRESOURCE_DATA *pInitialData, ID3D10Texture2D **ppTexture2D)
{
	return this->mOrig->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateTexture3D(const D3D10_TEXTURE3D_DESC *pDesc, const D3D10_SUBRESOURCE_DATA *pInitialData, ID3D10Texture3D **ppTexture3D)
{
	return this->mOrig->CreateTexture3D(pDesc, pInitialData, ppTexture3D);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateShaderResourceView(ID3D10Resource *pResource, const D3D10_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D10ShaderResourceView **ppSRView)
{
	return this->mOrig->CreateShaderResourceView(pResource, pDesc, ppSRView);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateRenderTargetView(ID3D10Resource *pResource, const D3D10_RENDER_TARGET_VIEW_DESC *pDesc, ID3D10RenderTargetView **ppRTView)
{
	return this->mOrig->CreateRenderTargetView(pResource, pDesc, ppRTView);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateDepthStencilView(ID3D10Resource *pResource, const D3D10_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D10DepthStencilView **ppDepthStencilView)
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

	D3D10Device *const device = this;
	ID3D10DepthStencilView *const depthstencil = *ppDepthStencilView;
	device->AddRef();
	depthstencil->SetPrivateData(__uuidof(device), sizeof(device), &device);

	ReShade::Hooks::Install(VTABLE(depthstencil), 2, reinterpret_cast<ReShade::Hook::Function>(&ID3D10DepthStencilView_Release));

	return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateInputLayout(const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements, const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, ID3D10InputLayout **ppInputLayout)
{
	return this->mOrig->CreateInputLayout(pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D10VertexShader **ppVertexShader)
{
	return this->mOrig->CreateVertexShader(pShaderBytecode, BytecodeLength, ppVertexShader);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D10GeometryShader **ppGeometryShader)
{
	return this->mOrig->CreateGeometryShader(pShaderBytecode, BytecodeLength, ppGeometryShader);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const D3D10_SO_DECLARATION_ENTRY *pSODeclaration, UINT NumEntries, UINT OutputStreamStride, ID3D10GeometryShader **ppGeometryShader)
{
	return this->mOrig->CreateGeometryShaderWithStreamOutput(pShaderBytecode, BytecodeLength, pSODeclaration, NumEntries, OutputStreamStride, ppGeometryShader);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D10PixelShader **ppPixelShader)
{
	return this->mOrig->CreatePixelShader(pShaderBytecode, BytecodeLength, ppPixelShader);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateBlendState(const D3D10_BLEND_DESC *pBlendStateDesc, ID3D10BlendState **ppBlendState)
{
	return this->mOrig->CreateBlendState(pBlendStateDesc, ppBlendState);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateDepthStencilState(const D3D10_DEPTH_STENCIL_DESC *pDepthStencilDesc, ID3D10DepthStencilState **ppDepthStencilState)
{
	return this->mOrig->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateRasterizerState(const D3D10_RASTERIZER_DESC *pRasterizerDesc, ID3D10RasterizerState **ppRasterizerState)
{
	return this->mOrig->CreateRasterizerState(pRasterizerDesc, ppRasterizerState);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateSamplerState(const D3D10_SAMPLER_DESC *pSamplerDesc, ID3D10SamplerState **ppSamplerState)
{
	return this->mOrig->CreateSamplerState(pSamplerDesc, ppSamplerState);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateQuery(const D3D10_QUERY_DESC *pQueryDesc, ID3D10Query **ppQuery)
{
	return this->mOrig->CreateQuery(pQueryDesc, ppQuery);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreatePredicate(const D3D10_QUERY_DESC *pPredicateDesc, ID3D10Predicate **ppPredicate)
{
	return this->mOrig->CreatePredicate(pPredicateDesc, ppPredicate);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateCounter(const D3D10_COUNTER_DESC *pCounterDesc, ID3D10Counter **ppCounter)
{
	return this->mOrig->CreateCounter(pCounterDesc, ppCounter);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CheckFormatSupport(DXGI_FORMAT Format, UINT *pFormatSupport)
{
	return this->mOrig->CheckFormatSupport(Format, pFormatSupport);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CheckMultisampleQualityLevels(DXGI_FORMAT Format, UINT SampleCount, UINT *pNumQualityLevels)
{
	return this->mOrig->CheckMultisampleQualityLevels(Format, SampleCount, pNumQualityLevels);
}
void STDMETHODCALLTYPE D3D10Device::CheckCounterInfo(D3D10_COUNTER_INFO *pCounterInfo)
{
	this->mOrig->CheckCounterInfo(pCounterInfo);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CheckCounter(const D3D10_COUNTER_DESC *pDesc, D3D10_COUNTER_TYPE *pType, UINT *pActiveCounters, LPSTR szName, UINT *pNameLength, LPSTR szUnits, UINT *pUnitsLength, LPSTR szDescription, UINT *pDescriptionLength)
{
	return this->mOrig->CheckCounter(pDesc, pType, pActiveCounters, szName, pNameLength, szUnits, pUnitsLength, szDescription, pDescriptionLength);
}
UINT STDMETHODCALLTYPE D3D10Device::GetCreationFlags()
{
	return this->mOrig->GetCreationFlags();
}
HRESULT STDMETHODCALLTYPE D3D10Device::OpenSharedResource(HANDLE hResource, REFIID ReturnedInterface, void **ppResource)
{
	return this->mOrig->OpenSharedResource(hResource, ReturnedInterface, ppResource);
}
void STDMETHODCALLTYPE D3D10Device::SetTextFilterSize(UINT Width, UINT Height)
{
	this->mOrig->SetTextFilterSize(Width, Height);
}
void STDMETHODCALLTYPE D3D10Device::GetTextFilterSize(UINT *pWidth, UINT *pHeight)
{
	this->mOrig->GetTextFilterSize(pWidth, pHeight);
}

// ID3D10Device1
HRESULT STDMETHODCALLTYPE D3D10Device::CreateShaderResourceView1(ID3D10Resource *pResource, const D3D10_SHADER_RESOURCE_VIEW_DESC1 *pDesc, ID3D10ShaderResourceView1 **ppSRView)
{
	assert(this->mInterfaceVersion >= 1);

	return static_cast<ID3D10Device1 *>(this->mOrig)->CreateShaderResourceView1(pResource, pDesc, ppSRView);
}
HRESULT STDMETHODCALLTYPE D3D10Device::CreateBlendState1(const D3D10_BLEND_DESC1 *pBlendStateDesc, ID3D10BlendState1 **ppBlendState)
{
	assert(this->mInterfaceVersion >= 1);

	return static_cast<ID3D10Device1 *>(this->mOrig)->CreateBlendState1(pBlendStateDesc, ppBlendState);
}
D3D10_FEATURE_LEVEL1 STDMETHODCALLTYPE D3D10Device::GetFeatureLevel()
{
	assert(this->mInterfaceVersion >= 1);

	return static_cast<ID3D10Device1 *>(this->mOrig)->GetFeatureLevel();
}

// D3D10
EXPORT HRESULT WINAPI D3D10CreateDevice(IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, UINT SDKVersion, ID3D10Device **ppDevice)
{
	LOG(INFO) << "Redirecting '" << "D3D10CreateDevice" << "(" << pAdapter << ", " << DriverType << ", " << Software << ", " << std::showbase << std::hex << Flags << std::dec << std::noshowbase << ", " << SDKVersion << ", " << ppDevice << ")' ...";
	LOG(INFO) << "> Passing on to 'D3D10CreateDeviceAndSwapChain':";

	return D3D10CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags, SDKVersion, nullptr, nullptr, ppDevice);
}
EXPORT HRESULT WINAPI D3D10CreateDevice1(IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, D3D10_FEATURE_LEVEL1 HardwareLevel, UINT SDKVersion, ID3D10Device1 **ppDevice)
{
	LOG(INFO) << "Redirecting '" << "D3D10CreateDevice1" << "(" << pAdapter << ", " << DriverType << ", " << Software << ", " << std::showbase << std::hex << Flags << std::dec << std::noshowbase << ", " << HardwareLevel << ", " << SDKVersion << ", " << ppDevice << ")' ...";
	LOG(INFO) << "> Passing on to 'D3D10CreateDeviceAndSwapChain1':";

	return D3D10CreateDeviceAndSwapChain1(pAdapter, DriverType, Software, Flags, HardwareLevel, SDKVersion, nullptr, nullptr, ppDevice);
}
EXPORT HRESULT WINAPI D3D10CreateDeviceAndSwapChain(IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, UINT SDKVersion, DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, IDXGISwapChain **ppSwapChain, ID3D10Device **ppDevice)
{
	LOG(INFO) << "Redirecting '" << "D3D10CreateDeviceAndSwapChain" << "(" << pAdapter << ", " << DriverType << ", " << Software << ", " << std::showbase << std::hex << Flags << std::dec << std::noshowbase << ", " << SDKVersion << ", " << pSwapChainDesc << ", " << ppSwapChain << ", " << ppDevice << ")' ...";

#ifdef _DEBUG
	Flags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr = ReShade::Hooks::Call(&D3D10CreateDeviceAndSwapChain)(pAdapter, DriverType, Software, Flags, SDKVersion, nullptr, nullptr, ppDevice);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'D3D10CreateDeviceAndSwapChain' failed with '" << GetErrorString(hr) << "'!";

		return hr;
	}

	if (ppDevice != nullptr)
	{
		IDXGIDevice *dxgidevice = nullptr;
		ID3D10Device *const device = *ppDevice;
		device->QueryInterface(&dxgidevice);

		assert(device != nullptr);
		assert(dxgidevice != nullptr);

		D3D10Device *const deviceProxy = new D3D10Device(device);
		deviceProxy->mDXGIDevice = new DXGIDevice(dxgidevice, deviceProxy);

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

			hr = factory->CreateSwapChain(deviceProxy, pSwapChainDesc, ppSwapChain);

			factory->Release();
			pAdapter->Release();
		}

		if (SUCCEEDED(hr))
		{
			*ppDevice = deviceProxy;

			LOG(TRACE) << "> Returned device objects: " << deviceProxy << ", " << deviceProxy->mDXGIDevice;
		}
		else
		{
			deviceProxy->Release();
		}
	}

	return hr;
}
EXPORT HRESULT WINAPI D3D10CreateDeviceAndSwapChain1(IDXGIAdapter *pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, D3D10_FEATURE_LEVEL1 HardwareLevel, UINT SDKVersion, DXGI_SWAP_CHAIN_DESC *pSwapChainDesc, IDXGISwapChain **ppSwapChain, ID3D10Device1 **ppDevice)
{
	LOG(INFO) << "Redirecting '" << "D3D10CreateDeviceAndSwapChain1" << "(" << pAdapter << ", " << DriverType << ", " << Software << ", " << std::showbase << std::hex << Flags << ", " << HardwareLevel << std::dec << std::noshowbase << ", " << SDKVersion << ", " << pSwapChainDesc << ", " << ppSwapChain << ", " << ppDevice << ")' ...";

#ifdef _DEBUG
	Flags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr = ReShade::Hooks::Call(&D3D10CreateDeviceAndSwapChain1)(pAdapter, DriverType, Software, Flags, HardwareLevel, SDKVersion, nullptr, nullptr, ppDevice);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'D3D10CreateDeviceAndSwapChain1' failed with '" << GetErrorString(hr) << "'!";

		return hr;
	}

	if (ppDevice != nullptr)
	{
		IDXGIDevice *dxgidevice = nullptr;
		ID3D10Device1 *const device = *ppDevice;
		device->QueryInterface(&dxgidevice);

		assert(device != nullptr);
		assert(dxgidevice != nullptr);

		D3D10Device *const deviceProxy = new D3D10Device(device);
		deviceProxy->mDXGIDevice = new DXGIDevice(dxgidevice, deviceProxy);

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

			hr = factory->CreateSwapChain(deviceProxy, pSwapChainDesc, ppSwapChain);

			factory->Release();
			pAdapter->Release();
		}

		if (SUCCEEDED(hr))
		{
			*ppDevice = deviceProxy;

			LOG(TRACE) << "> Returned device objects: " << deviceProxy << ", " << deviceProxy->mDXGIDevice;
		}
		else
		{
			deviceProxy->Release();
		}
	}

	return hr;
}
