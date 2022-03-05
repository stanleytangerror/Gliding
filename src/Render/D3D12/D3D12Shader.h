#pragma once

#include "D3D12/D3D12Headers.h"
#include "Common/CommonTypes.h"

#include <vector>
#include <map>
#include <string>
#include <functional>

class NameTable
{
public:
	const char* AllocName(const char* name);

protected:
	std::vector<const char*> mTable;
};

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

struct InputSrvParam
{
	std::string mName;
	i32 mBindPoint = 0;
	i32 mBindCount = 0;
};

enum class ShaderType
{
	eVs, ePs, eNum
};

class ShaderPiece
{
public:
	ShaderPiece(const char* file, enum ShaderType type);

	const enum ShaderType				GetType() const { return mType; }
	ID3DBlob* GetShader() const { return mShader; }
	std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout() const { return mInputLayout; }
	std::map<std::string, InputCBufferParam>	GetCBufferBindings() const { return mCBufferBindings; }
	std::map<std::string, InputSrvParam>		GetSrvBindings() const { return mSrvBindings; }

protected:
	const enum ShaderType					mType;
	const std::string						mFile;
	ID3DBlob* mShader = nullptr;
	std::vector<D3D12_INPUT_ELEMENT_DESC>	mInputLayout;

	std::map<std::string, InputSrvParam>			mSrvBindings;
	std::map<std::string, InputCBufferParam>		mCBufferBindings;

	NameTable								mNameTable;
};

class D3D12ShaderLibrary
{
public:
	ShaderPiece* CreateVs(const char* file);
	ShaderPiece* CreatePs(const char* file);
	ID3DBlob* CreateCs(const char* file);

protected:
	std::map<std::string, ShaderPiece*> mVsCache;
	std::map<std::string, ShaderPiece*> mPsCache;
	std::map<std::string, ID3DBlob*> mCsCache;
};