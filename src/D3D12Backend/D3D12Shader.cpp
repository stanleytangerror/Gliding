#include "D3D12BackendPch.h"
#include "D3D12Shader.h"
#include "D3D12Utils.h"

namespace D3D12Backend
{
	namespace
	{
		DXGI_FORMAT GetInputFormat(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc, u32& byteOffset)
		{
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

			// determine DXGI format
			if (paramDesc.Mask == 1)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32_FLOAT;
				byteOffset += 4;
			}
			else if (paramDesc.Mask <= 3)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32G32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32G32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32_FLOAT;
				byteOffset += 8;
			}
			else if (paramDesc.Mask <= 7)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32G32B32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32G32B32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32B32_FLOAT;
				byteOffset += 12;
			}
			else if (paramDesc.Mask <= 15)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				byteOffset += 16;
			}

			return format;
		}
	}

	ShaderPiece::ShaderPiece(const char* file, enum ShaderType type, const std::vector<GI::ShaderMacro>& macros)
		: mType(type)
		, mFile(file)
		, mMacros(macros)
	{
		std::vector<D3D_SHADER_MACRO> d3dMacros(mMacros.size() + 1, D3D_SHADER_MACRO{}); // a terminator
		std::transform(mMacros.begin(), mMacros.end(), d3dMacros.begin(),
			[](const GI::ShaderMacro& m) { return D3D_SHADER_MACRO{ m.mName.c_str(), m.mDefinition.c_str() }; });

		mShader =
			type == ShaderType::eVs ? D3D12Utils::CompileBlobFromFile(file, "VSMain", "vs_5_0", d3dMacros) :
			type == ShaderType::ePs ? D3D12Utils::CompileBlobFromFile(file, "PSMain", "ps_5_0", d3dMacros) :
			D3D12Utils::CompileBlobFromFile(file, "CSMain", "cs_5_0", d3dMacros);

		Assert(mShader);

		ID3D12ShaderReflection* reflection = nullptr;
		AssertHResultOk(D3DReflect(mShader->GetBufferPointer(), mShader->GetBufferSize(), IID_PPV_ARGS(&reflection)));

		// input layout
		D3D12_SHADER_DESC shaderDesc = {};
		reflection->GetDesc(&shaderDesc);

		u32 byteOffset = 0;
		for (i32 i = 0; i < static_cast<i32>(shaderDesc.InputParameters); ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC paramDesc = {};
			reflection->GetInputParameterDesc(i, &paramDesc);

			if (paramDesc.SystemValueType == D3D_NAME_INSTANCE_ID)
			{
				continue;
			}

			D3D12_INPUT_ELEMENT_DESC elementDesc = {};
			{
				elementDesc.SemanticName = paramDesc.SemanticName;
				elementDesc.SemanticIndex = paramDesc.SemanticIndex;
				elementDesc.InputSlot = 0;
				elementDesc.AlignedByteOffset = byteOffset;
				elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
				elementDesc.InstanceDataStepRate = 0;
				elementDesc.Format = GetInputFormat(paramDesc, byteOffset);
			}

			mInputLayout.push_back(elementDesc);
		}

		for (i32 i = 0; i < static_cast<i32>(shaderDesc.BoundResources); ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
			reflection->GetResourceBindingDesc(i, &bindDesc);

			switch (bindDesc.Type)
			{
			case D3D_SIT_CBUFFER:
			{
				// const buffer
				Assert(mCBufferBindings.end() == std::find_if(mCBufferBindings.begin(), mCBufferBindings.end(),
					[&bindDesc](const InputCBufferParam& p) { return p.mName == bindDesc.Name; }));
				InputCBufferParam param = {};
				{
					param.mName = bindDesc.Name;
					param.mBindPoint = bindDesc.BindPoint;
					param.mBindCount = bindDesc.BindCount;
				}
				mCBufferBindings.push_back(param);
				break;
			}
			case D3D_SIT_SAMPLER:
			{
				InputSamplerParam param = {};
				{
					param.mName = bindDesc.Name;
					param.mBindPoint = bindDesc.BindPoint;
					param.mBindCount = bindDesc.BindCount;
				}
				mSamplerBindings[bindDesc.Name] = param;
				break;
			}
			case D3D_SIT_TEXTURE:
			case D3D_SIT_STRUCTURED:
			case D3D_SIT_BYTEADDRESS:
			{
				// texture, structured buffer
				Assert(mSrvBindings.find(bindDesc.Name) == mSrvBindings.end());
				InputSrvParam param = {};
				{
					param.mName = bindDesc.Name;
					param.mBindPoint = bindDesc.BindPoint;
					param.mBindCount = bindDesc.BindCount;
				}
				mSrvBindings[bindDesc.Name] = param;
				break;
			}
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_RWBYTEADDRESS:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_UAV_CONSUME_STRUCTURED:
			case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			{
				Assert(mUavBindings.find(bindDesc.Name) == mUavBindings.end());
				InputUavParam param = {};
				{
					param.mName = bindDesc.Name;
					param.mBindPoint = bindDesc.BindPoint;
					param.mBindCount = bindDesc.BindCount;
				}
				mUavBindings[bindDesc.Name] = param;
				break;
			}
			default:
				break;
			}
		}

		// retrieve const buffer variables
		for (i32 i = 0; i < static_cast<i32>(shaderDesc.ConstantBuffers); ++i)
		{
			ID3D12ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			cb->GetDesc(&cbDesc);

			auto it = std::find_if(mCBufferBindings.begin(), mCBufferBindings.end(),
				[&cbDesc](const auto& p) { return p.mName == cbDesc.Name; });
			if (it != mCBufferBindings.end())
			{
				InputCBufferParam& param = *it;
				{
					param.mSize = cbDesc.Size;
					param.mVarNum = cbDesc.Variables;
				}

				for (i32 j = 0; j < static_cast<i32>(cbDesc.Variables); ++j)
				{
					ID3D12ShaderReflectionVariable* variable = cb->GetVariableByIndex(j);
					D3D12_SHADER_VARIABLE_DESC varDesc;
					variable->GetDesc(&varDesc);

					InputCBufferParam::CBufferVar var = {};
					{
						var.mName = varDesc.Name;
						var.mStartOffset = varDesc.StartOffset;
						var.mSize = varDesc.Size;
					}
					param.mVariables[varDesc.Name] = var;
				}
			}
		}
	}

	ShaderPiece* D3D12ShaderLibrary::CreateVs(const char* file, const std::vector<GI::ShaderMacro>& macros)
	{
		const Entry entry = { file, macros };
		if (mVsCache.find(entry) == mVsCache.end())
		{
			ShaderPiece* vs = new ShaderPiece(file, ShaderType::eVs, macros);
			mVsCache[entry] = vs;
		}

		return mVsCache[entry];
	}

	ShaderPiece* D3D12ShaderLibrary::CreatePs(const char* file, const std::vector<GI::ShaderMacro>& macros)
	{
		const Entry entry = { file, macros };
		if (mPsCache.find(entry) == mPsCache.end())
		{
			ShaderPiece* vs = new ShaderPiece(file, ShaderType::ePs, macros);
			mPsCache[entry] = vs;
		}

		return mPsCache[entry];
	}

	ShaderPiece* D3D12ShaderLibrary::CreateCs(const char* file, const std::vector<GI::ShaderMacro>& macros)
	{
		const Entry entry = { file, macros };
		if (mCsCache.find(entry) == mCsCache.end())
		{
			ShaderPiece* cs = new ShaderPiece(file, ShaderType::eCs, macros);
			mCsCache[entry] = cs;
		}

		return mCsCache[entry];
	}

	bool D3D12ShaderLibrary::Entry::Less::operator()(Entry const& o0, Entry const& o1) const
	{
		return
			o0.mName < o1.mName ? true :
			o0.mName > o1.mName ? false :
			o0.mMacros < o1.mMacros;
	}

}