#pragma once

#include "D3D12/D3D12CommandContext.h"
#include "D3D12/D3D12ResourceView.h"

//////////////////////////////////////////////////////////////////////////

template <typename T>
inline void AddCbVar(std::map<std::string, std::vector<byte>>& cbParams, const std::string& name, const T& var)
{
	Assert(cbParams.find(name) == cbParams.end());

	const int size = sizeof(T);

	std::vector<byte>& buf = cbParams[name];
	buf.resize(size);
	memcpy_s(buf.data(), size, &var, size);
}

template <>
inline void AddCbVar(std::map<std::string, std::vector<byte>>& cbParams, const std::string& name, const Mat33f& var)
{
	Assert(cbParams.find(name) == cbParams.end());

	const int size = sizeof(float) * (4 + 4 + 3);

	std::vector<byte>& buf = cbParams[name];
	buf.resize(size);
	memcpy_s(buf.data(), size, &var, size);
}

//////////////////////////////////////////////////////////////////////////

class ComputePass
{
public:
	ComputePass(GraphicsContext* context);

	void Dispatch();

	template <typename T>
	void AddCbVar(const std::string& name, const T& var) 
	{ 
		Assert(mCbParams.find(name) == mCbParams.end());
		mCbParams[name] = D3D12Utils::ToD3DConstBufferParamData(var);
	}

	void AddSrv(const std::string& name, IShaderResourceView* srv);
	void AddUav(const std::string& name, IUnorderedAccessView* uav);
	
public:
	struct
	{
		std::string mFile;
		const char* mEntry = nullptr;
	}					mRootSignatureDesc;

	std::string mCsFile;

public:
	GraphicsContext* const					mContext = nullptr;
	std::map<std::string, IShaderResourceView*>		mSrvParams;
	std::map<std::string, IUnorderedAccessView*>	mUavParams;
	std::map<std::string, std::vector<byte>>		mCbParams;

	std::array<u32, 3>								mThreadGroupCounts = {};

protected:
	ID3D12RootSignature* mRootSignature = nullptr;
	std::unique_ptr<ComputePipelineState>			mPso;
};

//////////////////////////////////////////////////////////////////////////

class GraphicsPass
{
public:
	GraphicsPass(GraphicsContext* context);

	void Draw();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& PsoDesc() { return mPso->Descriptor(); }

	template <typename T>
	void AddCbVar(const std::string& name, const T& var)
	{
		Assert(mCbParams.find(name) == mCbParams.end());
		mCbParams[name] = D3D12Utils::ToD3DConstBufferParamData(var);
	}

	void AddSrv(const std::string& name, IShaderResourceView* srv);

public:
	struct
	{
		std::string	mFile;
		const char* mEntry = nullptr;
	}					mRootSignatureDesc;

	std::string mVsFile;
	std::string mPsFile;

public:
	GraphicsContext* const					mContext = nullptr;
	std::map<int, IRenderTargetView*>		mRts;
	DSV*									mDs = nullptr;

	std::vector<D3D12_VERTEX_BUFFER_VIEW>	mVbvs;
	D3D12_INDEX_BUFFER_VIEW					mIbv = {};
	int										mIndexCount = 0;
	int										mInstanceCount = 1;

	D3D12_VIEWPORT							mViewPort = {};
	D3D12_RECT								mScissorRect = {};
	UINT									mStencilRef = 0;

protected:
	std::map<std::string, std::vector<byte>>	mCbParams;
	std::map<std::string, IShaderResourceView*>	mSrvParams;

	ID3D12RootSignature* mRootSignature = nullptr;
	std::unique_ptr<GraphicsPipelineState> mPso;
};

