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

class RenderPass
{
public:
	std::string Name;
	GraphicProgram Program;
	InputPrimitiveView InputPrimitive;
	std::map<std::string, ConstantBufferValue> ConstBuffers;
	std::map<std::string, ShaderResourceView> Srvs;
	std::vector<RenderTargetView> Rtvs;
	RHI::ViewPort ViewPort;
	RHI::Rect ScissorRect;
};

class RenderPassManager
{
public:
	RenderPassManager(RenderModule* renderModule) : mRenderModule(renderModule) {}

	void ParseAllPassses();

private:
	RenderModule* mRenderModule = nullptr;
	std::vector<RenderPass> mPasses;
};
