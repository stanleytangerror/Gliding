#pragma once

#include "Common/GraphicsInfrastructure.h"
#include "D3D12Headers.h"

#define CONSTRUCTOR_WITH_RESOURCE_ID(Type, Base) \
	public:     Type() {} \
	            Type(GI::IGraphicMemoryResource* resource) : mResourceId(resource->GetResourceId()) {} \
	            Type(GI::CommittedResourceId id, const Base& base) : Base(base), mResourceId(id) {} \
	            Type(const Type& other) : Base(other), mResourceId(other.mResourceId) {} \
	            Type(const std::unique_ptr<GI::IGraphicMemoryResource>& resource) : mResourceId(resource->GetResourceId()) {} \
                GI::CommittedResourceId GetResourceId() const { return mResourceId; } \
	private:    GI::CommittedResourceId mResourceId = {};

namespace D3D12Backend
{
	class GD_D3D12BACKEND_API D3D12GraphicsInfra : public GI::IGraphicsInfra
	{
	public:
		D3D12GraphicsInfra();

		std::unique_ptr<GI::IGraphicMemoryResource> CreateMemoryResource(const GI::MemoryResourceDesc& desc) override;
		std::unique_ptr<GI::IGraphicMemoryResource> CreateMemoryResource(const GI::IImage& image) override;

		void CopyToUploadBufferResource(GI::IGraphicMemoryResource* resource, const std::vector<b8>& data) override;

		std::unique_ptr<GI::IImage> CreateFromImageMemory(const TextureFileExt::Enum& ext, const std::vector<b8>& content) const override;
		std::unique_ptr<GI::IImage> CreateFromScratch(GI::Format::Enum format, const std::vector<b8>& content, const Vec3i& size, i32 mipLevel, const char* name) const override;

		void                        AdaptToWindow(u8 windowId, const WindowRuntimeInfo& windowInfo) override;
		void                        ResizeWindow(u8 windowId, const Vec2u& windowSize) override;
		GI::IGraphicMemoryResource* GetWindowBackBuffer(u8 windowId) override;

		void                        StartFrame() override;
		void                        EndFrame() override;
		void                        Present() override;

		void						StartRecording() override;
		void						EndRecording(bool dropAllCommands) override;
		class GI::IGraphicsRecorder* GetRecorder() const override;

		GI::DevicePtr               GetNativeDevicePtr() const override;

	private:
		class D3D12Device*	mDevice = nullptr;
		class D3D12GraphicsRecorder* mCurrentRecorder = nullptr;
		bool						mSkipFrameCommands = false;
	};

	class GD_D3D12BACKEND_API D3D12GraphicsRecorder : public GI::IGraphicsRecorder
	{
	public:
		using Command = std::function<void()>;

		D3D12GraphicsRecorder(D3D12CommandContext* context);

		void AddClearOperation(const GI::RtvUsage& rtv, const Vec4f& value) override;
		void AddClearOperation(const GI::DsvUsage& dsv, bool clearDepth, float depth, bool clearStencil, u32 stencil) override;
		void AddCopyOperation(GI::IGraphicMemoryResource* dest, GI::IGraphicMemoryResource* src) override;
		void AddGraphicsPass(const GI::GraphicsPass& pass) override;
		void AddComputePass(const GI::ComputePass& pass) override;
		void AddPreparePresent(GI::IGraphicMemoryResource* res) override;
		void AddBeginEvent(const char* mark) override;
		void AddEndEvent() override;

		void Finalize(bool dropAllCommands);

		// TODO remove this
		D3D12CommandContext* GetContext() const { return mContext; }

	private:
		D3D12CommandContext*	mContext = nullptr;
		std::queue<Command>		mCommands;
	};

	struct GD_COMMON_API SrvUsageImpl : public GI::SrvDesc
	{
		CONSTRUCTOR_WITH_RESOURCE_ID(SrvUsageImpl, GI::SrvDesc);
	};
	struct GD_COMMON_API RtvUsageImpl : public GI::RtvDesc
	{
		CONSTRUCTOR_WITH_RESOURCE_ID(RtvUsageImpl, GI::RtvDesc);
	};
	struct GD_COMMON_API UavUsageImpl : public GI::UavDesc
	{
		CONSTRUCTOR_WITH_RESOURCE_ID(UavUsageImpl, GI::UavDesc);
	};
	struct GD_COMMON_API DsvUsageImpl : public GI::DsvDesc
	{
		CONSTRUCTOR_WITH_RESOURCE_ID(DsvUsageImpl, GI::DsvDesc);
	};
	struct GD_COMMON_API VbvUsageImpl : public GI::VbvDesc
	{
		CONSTRUCTOR_WITH_RESOURCE_ID(VbvUsageImpl, GI::VbvDesc);
	};
	struct GD_COMMON_API IbvUsageImpl : public GI::IbvDesc
	{
		CONSTRUCTOR_WITH_RESOURCE_ID(IbvUsageImpl, GI::IbvDesc);
	};

	class GraphicsPass
	{
		friend D3D12GraphicsRecorder;
	public:
		GI::RootSignatureDesc			            mRootSignatureDesc;

		std::string                                 mVsFile;
		std::string                                 mPsFile;
		std::vector<GI::ShaderMacro>	                mShaderMacros;

	public:
		std::array<RtvUsageImpl, 8>               	mRtvs;
		DsvUsageImpl                                mDsv;

		std::vector<GI::InputElementDesc>               mInputLayout;

		GI::RasterizerDesc                              mRasterizerDesc;
		GI::DepthStencilDesc                            mDepthStencilDesc;
		GI::BlendDesc                                   mBlendDesc;

		std::vector<VbvUsageImpl>	                    mVbvs;
		IbvUsageImpl                 					mIbv;
		i32 										mVertexStartLocation = 0;
		i32 										mIndexStartLocation = 0;
		i32 										mIndexCount = 0;
		i32 										mInstanceCount = 1;

		GI::Viewport        							mViewPort = {};
		Math::Rect  								mScissorRect = {};
		u32     									mStencilRef = 0;

		//protected:
		std::map<std::string, std::vector<b8>>      mCbParams;
		std::map<std::string, SrvUsageImpl>	            mSrvParams;
		std::map<std::string, GI::SamplerDesc>	        mSamplerParams;
	};

	class ComputePass
	{
		friend D3D12GraphicsRecorder;
	public:
		GI::RootSignatureDesc			            mRootSignatureDesc;

		std::string mCsFile;
		std::vector<GI::ShaderMacro>	mShaderMacros;

	public:
		std::map<std::string, GI::SamplerDesc>		mSamplerParams;
		std::map<std::string, SrvUsageImpl>  		mSrvParams;
		std::map<std::string, UavUsageImpl>	        mUavParams;
		std::map<std::string, std::vector<b8>>	    mCbParams;

		std::array<u32, 3>							mThreadGroupCounts = {};

	protected:
		//bool                                        mReady = true;
	};

}