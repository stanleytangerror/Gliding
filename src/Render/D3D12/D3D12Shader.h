#pragma once

#include "D3D12/D3D12Headers.h"
#include "Common/CommonTypes.h"

#include <vector>
#include <map>
#include <string>
#include <functional>

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

struct ShaderMacro
{
	std::string mName;
	std::string mDefinition;

	bool operator<(const ShaderMacro& other) const;
};

class ShaderPiece
{
public:
	ShaderPiece(const char* file, enum ShaderType type, const std::vector<ShaderMacro>& macros);

	const enum ShaderType						GetType() const { return mType; }
	ID3DBlob*									GetShader() const { return mShader; }
	std::vector<D3D12_INPUT_ELEMENT_DESC>		GetInputLayout() const { return mInputLayout; }
	std::map<std::string, InputCBufferParam>	GetCBufferBindings() const { return mCBufferBindings; }
	std::map<std::string, InputSrvParam>		GetSrvBindings() const { return mSrvBindings; }
	std::map<std::string, InputUavParam>		GetUavBindings() const { return mUavBindings; }
	std::map<std::string, InputSamplerParam>	GetSamplerBindings() const { return mSamplerBindings; }

protected:
	const enum ShaderType					mType;
	const std::string						mFile;
	const std::vector<ShaderMacro>			mMacros;

	ID3DBlob* mShader = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC>	mInputLayout;

	std::map<std::string, InputSrvParam>			mSrvBindings;
	std::map<std::string, InputCBufferParam>		mCBufferBindings;
	std::map<std::string, InputUavParam>			mUavBindings;
	std::map<std::string, InputSamplerParam>		mSamplerBindings;
};

class D3D12ShaderLibrary
{
public:
	ShaderPiece* CreateVs(const char* file, const std::vector<ShaderMacro>& macros);
	ShaderPiece* CreatePs(const char* file, const std::vector<ShaderMacro>& macros);
	ShaderPiece* CreateCs(const char* file, const std::vector<ShaderMacro>& macros);

protected:
	struct Entry
	{
		std::string mName;
		std::vector<ShaderMacro> mMacros;

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