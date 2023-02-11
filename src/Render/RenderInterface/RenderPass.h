#pragma once

#include "RenderTypes.h"
#include "Common/Math.h"
#include "RenderResource.h"
#include <string>
#include <map>
#include <vector>

class RenderModule;

struct GraphicProgram
{
	std::string RsName;
	std::string RsEntry;
	std::string VsName;
	std::string PsName;
	std::vector<RHI::ProgramMacro> Macros;
};

struct VertexBufferView
{
	ResourceId ResourceId;
	RHI::VertexBufferViewDesc Desc;
};


struct IndexBufferView
{
	ResourceId ResourceId;
	RHI::IndexBufferViewDesc Desc;
};

struct TransientResource
{
	ResourceId ResourceId;
};


struct FileResource
{
	std::wstring File;
	ResourceId ResourceId;
};


struct FromDataResource
{
	ResourceId ResourceId;
	std::vector<b8> Data;
};


struct SwapChainResource
{
	ResourceId ResourceId;
};

struct ConstantBufferValue
{
	Vec4f Value;
};

struct ShaderResourceView
{
	ResourceId ResourceId;
	RHI::ShaderResourceViewDesc SrvDesc;
};


struct RenderTargetView
{
	ResourceId ResourceId;
	RHI::RenderTargetViewDesc RtvDesc;
};


struct InputPrimitiveView
{
	std::vector<VertexBufferView> Vbvs;
	std::vector<RHI::InputElementDesc> InputElements;
	IndexBufferView Ibv;
	RHI::IndexedInstancedParam Param;
};

struct SamplerView
{
	SamplerId SamplerId;
};

class RenderPass
{
public:
	std::string Name;
	GraphicProgram Program;
	InputPrimitiveView InputPrimitive;
	std::map<std::string, ConstantBufferValue> ConstBuffers;
	std::map<std::string, ShaderResourceView> Srvs;
	std::map<std::string, SamplerView> Samplers;
	std::vector<RenderTargetView> Rtvs;
	RHI::ViewPort ViewPort;
	RHI::Rect ScissorRect;
};

namespace D3D12Backend
{
	class GraphicsContext;
}

class RenderPassManager
{
public:
	RenderPassManager(RenderModule* renderModule) : mRenderModule(renderModule) {}

	void AddPass(const RenderPass& pass);

	void ParseAllPassses(D3D12Backend::GraphicsContext* context);

private:
	bool CheckRequiredResourceReady(const RenderPass& pass);
	void ParseRenderPass(const RenderPass& pass, D3D12Backend::GraphicsContext* context);

private:
	RenderModule* mRenderModule = nullptr;
	std::vector<RenderPass> mPasses;
};
