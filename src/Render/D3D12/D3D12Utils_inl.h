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


template <>
inline std::vector<byte> D3D12Utils::ToD3DConstBufferParamData(const std::vector<f32>& var)
{
	const auto size = var.size() * sizeof(f32);

	std::vector<byte> result(size, 0);
	memcpy_s(result.data(), size, var.data(), size);
	return result;
}
