#pragma once

#include "D3D12Headers.h"
#include "Common/CommonTypes.h"

#include <vector>
#include <map>
#include <string>
#include <functional>

namespace D3D12Backend
{
	struct InputCBufferParam
	{
		struct CBufferVar
		{
			std::string mName;
			int	mStartOffset = 0;
			int mSize = 0;
		};

		std::string mName;
		i32 mBindPoint = 0;
		i32 mBindCount = 0;

		i32 mVarNum = 0;
		i32 mSize = 0;
		std::map<std::string, CBufferVar> mVariables;
	};

	struct InputSamplerParam
	{
		std::string mName;
		i32 mBindPoint = 0;
		i32 mBindCount = 0;
	};

	struct InputSrvParam
	{
		std::string mName;
		i32 mBindPoint = 0;
		i32 mBindCount = 0;
	};

	struct InputUavParam
	{
		std::string mName;
		i32 mBindPoint = 0;
		i32 mBindCount = 0;
	};

	enum class ShaderType
	{
		eVs, ePs, eCs, eNum
	};

	class ShaderPiece
	{
	public:
		ShaderPiece(const char* file, enum ShaderType type, const std::vector<GI::ShaderMacro>& macros);

		const enum ShaderType						GetType() const { return mType; }
		ID3DBlob* GetShader() const { return mShader; }
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputLayout() const { return mInputLayout; }
		const std::vector<InputCBufferParam>& GetCBufferBindings() const { return mCBufferBindings; }
		const std::map<std::string, InputSrvParam>& GetSrvBindings() const { return mSrvBindings; }
		const std::map<std::string, InputUavParam>& GetUavBindings() const { return mUavBindings; }
		const std::map<std::string, InputSamplerParam>& GetSamplerBindings() const { return mSamplerBindings; }

	protected:
		const enum ShaderType					mType;
		const std::string						mFile;
		const std::vector<GI::ShaderMacro>		mMacros;

		ID3DBlob* mShader = nullptr;
		std::vector<D3D12_INPUT_ELEMENT_DESC>	mInputLayout;

		std::map<std::string, InputSrvParam>			mSrvBindings;
		std::vector<InputCBufferParam>					mCBufferBindings;
		std::map<std::string, InputUavParam>			mUavBindings;
		std::map<std::string, InputSamplerParam>		mSamplerBindings;
	};

	class D3D12ShaderLibrary
	{
	public:
		ShaderPiece* CreateVs(const char* file, const std::vector<GI::ShaderMacro>& macros);
		ShaderPiece* CreatePs(const char* file, const std::vector<GI::ShaderMacro>& macros);
		ShaderPiece* CreateCs(const char* file, const std::vector<GI::ShaderMacro>& macros);

	protected:
		struct Entry
		{
			std::string mName;
			std::vector<GI::ShaderMacro> mMacros;

			class Less
			{
			public:
				bool operator()(Entry const& o0, Entry const& o1) const;
			};
		};

		std::map<Entry, ShaderPiece*, Entry::Less> mVsCache;
		std::map<Entry, ShaderPiece*, Entry::Less> mPsCache;
		std::map<Entry, ShaderPiece*, Entry::Less> mCsCache;
	};
}