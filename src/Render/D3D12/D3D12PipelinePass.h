#pragma once

#include "D3D12/D3D12CommandContext.h"
#include "D3D12/D3D12ResourceView.h"

//////////////////////////////////////////////////////////////////////////

class ComputePass
{
public:
	ComputePass(ComputeContext* context);

	void Dispatch();

public:
	struct
	{
		std::string mFile;
		const char* mEntry = nullptr;
	}					mRootSignatureDesc;

	std::string mCsFile;

public:
	ComputeContext* const					mContext = nullptr;
	std::map<int, IShaderResourceView*>		mTexParams;
	std::map<int, IUnorderedAccessView*>	mUavParams;
	std::map<int, Mat44f>		mCbParams;

	std::array<i32, 3>						mThreadCounts = {};

protected:
	ID3D12RootSignature* mRootSignature = nullptr;
	ComputePipelineState* mPso = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class GraphicsPass
{
public:
	GraphicsPass(GraphicsContext* context);

	void Draw();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC& PsoDesc() { return mPso->Descriptor(); }

	template <typename T>
	void AddCbVar(const std::string& name, const T& var);

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
	//class DSV* mDs = nullptr;

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

template <typename T>
void GraphicsPass::AddCbVar(const std::string& name, const T& var)
{
	Assert(mCbParams.find(name) == mCbParams.end());

	const int size = sizeof(T);

	std::vector<byte>& buf = mCbParams[name];
	buf.resize(size);
	memcpy_s(buf.data(), size, &var, size);
}

template <>
inline void GraphicsPass::AddCbVar(const std::string& name, const Vec3f& var)
{
	Assert(mCbParams.find(name) == mCbParams.end());

	const int size = sizeof(Vec3f) * 3;

	std::vector<byte>& buf = mCbParams[name];
	buf.resize(size);
	memcpy_s(buf.data(), size, &var, size);
}

template <>
inline void GraphicsPass::AddCbVar(const std::string& name, const Mat33f& var)
{
	Assert(mCbParams.find(name) == mCbParams.end());

	const int size = sizeof(float) * (4 + 4 + 3);

	std::vector<byte>& buf = mCbParams[name];
	buf.resize(size);
	memcpy_s(buf.data(), size, &var, size);
}
