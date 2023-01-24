#pragma once

#include "RenderTypes.h"
#include "Common/Math.h"
#include "RenderResource.h"
#include <string>
#include <map>

struct GraphicProgram
{
	std::wstring RsName;
	std::wstring RsEntry;
	std::wstring VsName;
	std::wstring PsName;
};

struct VertexBufferView
{
	ResourceId ResourceId;
	RHI::VertexBufferViewDesc Desc;
};


class IndexBufferView
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

	static RenderPass GenerateTestRenderPass(class RenderResourceManager* resMgr);
};
