#pragma once

#include "D3D12Headers.h"
#include "D3D12DescriptorHeap.h"
#include "SuspendedRelease.h"
#include "D3D12DescriptorAllocator.h"
#include "D3D12PipelineState.h"
#include "D3D12Fence.h"
#include "D3D12Shader.h"
#include "D3D12RenderTarget.h"
#include <array>

class GraphicsContext;
using CommandAllocatorPool = SuspendedReleasePool<ID3D12CommandAllocator>;
using GraphicsContextPool = SuspendedReleasePool<GraphicsContext>;
class D3D12PipelineStateLibrary;
class D3D12ShaderLibrary;
class RuntimeDescriptorHeap;
class D3D12DescriptorAllocator;
class D3D12Fence;
class D3D12ConstantBuffer;
class SwapChainBuffers;

class D3D12Device
{
public:
	D3D12Device(HWND windowHandle);

	void	Present();

	ID3D12Device* GetDevice() const;

public:
	CommandAllocatorPool* GetCommandAllocatorPool() const;
	RuntimeDescriptorHeap* GetRuntimeDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
	D3D12DescriptorAllocator* GetDescAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
	D3D12ShaderLibrary* GetShaderLib() const { return mShaderLib; }
	D3D12PipelineStateLibrary* GetPipelineStateLib() const { return mPipelineStateLib; }
	CpuDescItem	GetNullSrvCpuDesc() const { return mNullSrvCpuDesc; }
	D3D12Fence* GetFence() const { return mFence; }
	GraphicsContextPool* GetGraphicContextPool() const { return mGraphicContextPool; }
	SwapChainBuffers* GetBackBuffer() const { return mBackBuffers; }

private:
	ID3D12Device* mDevice = nullptr;
	IDXGISwapChain3* mSwapChain = nullptr;

	SwapChainBuffers* mBackBuffers = nullptr;


	ID3D12CommandQueue* mCommandQueue = nullptr;

	CommandAllocatorPool* mCommandAllocatorPool = nullptr;
	
	D3D12PipelineStateLibrary* mPipelineStateLib = nullptr;
	D3D12ShaderLibrary* mShaderLib = nullptr;
	D3D12Fence*					mFence = nullptr;

	std::array<RuntimeDescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mRuntimeDescHeaps = {};
	std::array<D3D12DescriptorAllocator*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mDescAllocator = {};
	CpuDescItem	mNullSrvCpuDesc;

	GraphicsContextPool* mGraphicContextPool = nullptr;
};