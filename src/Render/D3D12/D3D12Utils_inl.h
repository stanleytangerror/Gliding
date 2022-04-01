#pragma once

#include "D3D12Utils.h"

template <typename T>
inline std::vector<byte> D3D12Utils::ToD3DConstBufferParamData(const T& var)
{
	std::vector<byte> result(sizeof(T), 0);
	memcpy(result.data(), &var, sizeof(T));
	return result;
}

template <>
inline std::vector<byte> D3D12Utils::ToD3DConstBufferParamData(const Mat33f& var)
{
	std::vector<byte> result(sizeof(f32) * (4 + 4 + 3), 0);
	Assert(false);
	return result;
}


