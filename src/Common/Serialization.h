#pragma once

#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <ranges>

#include "Stream.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#define EXPAND(x) x
#define FOR_EACH_1(what, x, ...) what(x)
#define FOR_EACH_2(what, x, ...) what(x) EXPAND(FOR_EACH_1(what, __VA_ARGS__))
#define FOR_EACH_3(what, x, ...) what(x) EXPAND(FOR_EACH_2(what, __VA_ARGS__))
#define FOR_EACH_4(what, x, ...) what(x) EXPAND(FOR_EACH_3(what, __VA_ARGS__))
#define FOR_EACH_5(what, x, ...) what(x) EXPAND(FOR_EACH_4(what, __VA_ARGS__))
#define FOR_EACH_6(what, x, ...) what(x) EXPAND(FOR_EACH_5(what, __VA_ARGS__))
#define FOR_EACH_7(what, x, ...) what(x) EXPAND(FOR_EACH_6(what, __VA_ARGS__))
#define FOR_EACH_8(what, x, ...) what(x) EXPAND(FOR_EACH_7(what, __VA_ARGS__))
#define FOR_EACH_9(what, x, ...) what(x) EXPAND(FOR_EACH_8(what, __VA_ARGS__))
#define FOR_EACH_10(what, x, ...) what(x) EXPAND(FOR_EACH_9(what, __VA_ARGS__))

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME
#define FOR_EACH(action, ...) \
  EXPAND(GET_MACRO(__VA_ARGS__, FOR_EACH_10, FOR_EACH_9, FOR_EACH_8, FOR_EACH_7, FOR_EACH_6, FOR_EACH_5, FOR_EACH_4, FOR_EACH_3, FOR_EACH_2, FOR_EACH_1)(action, __VA_ARGS__))


#pragma region Declaration

template <typename T, typename Enable = void>
struct _SerializeTextImpl
{
	void Serialize(const T& in, std::ostringstream& oss);
	T Deserialize(std::istringstream& iss);
};

template <typename T, typename Enable = void>
struct _SerializeBytesImpl;

template <typename T, typename Enable = void>
struct _SerializeJsonImpl;


template <typename T>
std::string SerializeToString(const T& in)
{
	std::ostringstream oss;
	_SerializeTextImpl<std::decay_t<T>>().Serialize(in, oss);
	return oss.str();
}

template <typename T>
T DeserializeFromString(const std::string& str)
{
	std::istringstream iss(str);
	return _SerializeTextImpl<std::decay_t<T>>().Deserialize(iss);
}


template <typename T>
std::vector<std::byte> SerializeToBytes(const T& in)
{
	obytestream oss;
	_SerializeBytesImpl<std::decay_t<T>>().Serialize(in, oss);
	return oss.to_bytes();
}

template <typename T>
T DeserializeFromBytes(const std::vector<std::byte>& bytes)
{
	ibytestream iss(bytes.data(), bytes.size());
	return _SerializeBytesImpl<std::decay_t<T>>().Deserialize(iss);
}

template <typename T>
json SerializeToJson(const T& in)
{
	return _SerializeJsonImpl<std::decay_t<T>>().Serialize(in);
}

template <typename T>
T DeserializeFromJson(const json& json)
{
	return _SerializeJsonImpl<std::decay_t<T>>().Deserialize(json);
}

#pragma endregion

#pragma region Implementation

template <typename T>
struct _SerializeTextImpl<T, std::enable_if_t<std::is_enum_v<T>>>
{
	void Serialize(const T& in, std::ostringstream& oss)
	{
		using UnderlyingType = std::underlying_type_t<T>;
		_SerializeTextImpl<UnderlyingType>().Serialize(static_cast<UnderlyingType>(in), oss);
	}

	T Deserialize(std::istringstream& iss)
	{
		using UnderlyingType = std::underlying_type_t<T>;
		auto underlyingValue = _SerializeTextImpl<UnderlyingType>().Deserialize(iss);
		return static_cast<T>(underlyingValue);
	}
};

template <typename T>
struct _SerializeBytesImpl<T, std::enable_if_t<std::is_enum_v<T>>>
{
	void Serialize(const T& in, obytestream& oss)
	{
		using UnderlyingType = std::underlying_type_t<T>;
		_SerializeBytesImpl<UnderlyingType>().Serialize(static_cast<UnderlyingType>(in), oss);
	}

	T Deserialize(ibytestream& iss)
	{
		using UnderlyingType = std::underlying_type_t<T>;
		auto underlyingValue = _SerializeBytesImpl<UnderlyingType>().Deserialize(iss);
		return static_cast<T>(underlyingValue);
	}
};

template <typename T>
struct _SerializeJsonImpl<T, std::enable_if_t<std::is_enum_v<T>>>
{
	json Serialize(const T& in)
	{
		using UnderlyingType = std::underlying_type_t<T>;
		return json(static_cast<UnderlyingType>(in));
	}

	T Deserialize(const json& json)
	{
		using UnderlyingType = std::underlying_type_t<T>;
		return static_cast<T>(json.get<UnderlyingType>());
	}
};


#define SERIALIZE_TEXT_IMPL_SCALAR(SCALAR, STR_TO_SCALAR) \
	template <> \
	struct _SerializeTextImpl<SCALAR> \
	{ \
		void Serialize(const SCALAR& in, std::ostringstream& oss) \
		{ \
			oss << in << ' '; \
		} \
		SCALAR Deserialize(std::istringstream& iss) \
		{ \
			std::string result; \
			iss >> result; \
			return (STR_TO_SCALAR)(result); \
		} \
	};

SERIALIZE_TEXT_IMPL_SCALAR(float, std::stof);
SERIALIZE_TEXT_IMPL_SCALAR(double, std::stod);
SERIALIZE_TEXT_IMPL_SCALAR(int64_t, std::stoll);
SERIALIZE_TEXT_IMPL_SCALAR(uint64_t, std::stoull);
SERIALIZE_TEXT_IMPL_SCALAR(int32_t, std::stol);
SERIALIZE_TEXT_IMPL_SCALAR(uint32_t, std::stoul);

template <typename T>
struct _SerializeBytesImpl<T, std::enable_if_t<!std::is_enum_v<T> && std::is_trivially_copyable_v<T>>>
{
	void Serialize(const T& in, obytestream& oss)
	{
		oss << in;
	}

	T Deserialize(ibytestream& iss)
	{
		T result;
		iss >> result;
		return result;
	}
};

template <typename T>
struct _SerializeJsonImpl<T, std::enable_if_t<std::is_arithmetic_v<T>>>
{
	json Serialize(const T& in)
	{
		return json(in);
	}
	T Deserialize(const json& json)
	{
		return json.template get<T>();
	}
};

template <>
struct _SerializeTextImpl<std::string>
{
	void Serialize(const std::string& in, std::ostringstream& oss)
	{
		oss << in << ' ';
	}

	std::string Deserialize(std::istringstream& iss)
	{
		std::string result;
		iss >> result;
		return result;
	}
};

template <>
struct _SerializeBytesImpl<std::string>
{
	void Serialize(const std::string& in, obytestream& oss)
	{
		_SerializeBytesImpl<std::string::size_type>().Serialize(in.size(), oss);
		oss.write(reinterpret_cast<const std::byte*>(in.data()), in.size());
	}

	std::string Deserialize(ibytestream& iss)
	{
		auto size = _SerializeBytesImpl<std::string::size_type>().Deserialize(iss);
		
		std::string result(size + 1, '\0');
		iss.read(reinterpret_cast<std::byte*>(const_cast<char*>(result.data())), size);

		return result;
	}
};

template <>
struct _SerializeJsonImpl<std::string>
{
	json Serialize(const std::string& in)
	{
		return json(in);
	}
	std::string Deserialize(const json& json)
	{
		return json.template get<std::string>();
	}
};


template <typename S, typename T>
struct _SerializeTextImpl<std::pair<S, T>>
{
	void Serialize(const std::pair<S, T>& in, std::ostringstream& oss)
	{
		_SerializeTextImpl<S>().Serialize(in.first, oss);
		_SerializeTextImpl<T>().Serialize(in.second, oss);
	}

	std::pair<S, T> Deserialize(std::istringstream& iss)
	{
		auto&& first = _SerializeTextImpl<S>().Deserialize(iss);
		auto&& second = _SerializeTextImpl<T>().Deserialize(iss);
		return std::make_pair(std::move(first), std::move(second));
	}
};

template <typename S, typename T>
struct _SerializeBytesImpl<std::pair<S, T>>
{
	void Serialize(const std::pair<S, T>& in, obytestream& oss)
	{
		_SerializeBytesImpl<std::decay_t<S>>().Serialize(in.first, oss);
		_SerializeBytesImpl<std::decay_t<T>>().Serialize(in.second, oss);
	}

	std::pair<S, T> Deserialize(ibytestream& iss)
	{
		auto&& first = _SerializeBytesImpl<std::decay_t<S>>().Deserialize(iss);
		auto&& second = _SerializeBytesImpl<std::decay_t<T>>().Deserialize(iss);
		return std::make_pair(std::move(first), std::move(second));
	}
};

template <typename S, typename T>
struct _SerializeJsonImpl<std::pair<S, T>>
{
	json Serialize(const std::pair<S, T>& in)
	{
		json result;
		result["first"] = _SerializeJsonImpl<std::decay_t<S>>().Serialize(in.first);
		result["second"] = _SerializeJsonImpl<std::decay_t<T>>().Serialize(in.second);
		return result;
	}

	std::pair<S, T> Deserialize(const json& json)
	{
		std::pair<S, T> result;
		result.first = _SerializeJsonImpl<std::decay_t<S>>().Deserialize(json["first"]);
		result.second = _SerializeJsonImpl<std::decay_t<T>>().Deserialize(json["second"]);
		return result;
	}
};


template <typename T, size_t Size>
struct _SerializeTextImpl<std::array<T, Size>>
{
	void Serialize(const std::array<T, Size>& in, std::ostringstream& oss)
	{
		for (auto&& e : in)
		{
			_SerializeTextImpl<T>().Serialize(e, oss);
		}
	}

	std::array<T, Size> Deserialize(std::istringstream& iss)
	{
		std::array<T, Size> result;
		auto view = std::ranges::views::iota(0, Size)
			| std::ranges::views::transform([&iss](auto) { return _SerializeTextImpl<T>().Deserialize(iss); });
		std::ranges::copy(view, result.begin());
		return result;
	}
};

template <typename T, size_t Size>
struct _SerializeBytesImpl<std::array<T, Size>, std::enable_if_t<!std::is_trivially_copyable_v<T>>>
{
	void Serialize(const std::array<T, Size>& in, obytestream& oss)
	{
		for (auto&& e : in)
		{
			_SerializeBytesImpl<std::decay_t<T>>().Serialize(e, oss);
		}
	}

	std::array<T, Size> Deserialize(ibytestream& iss)
	{
		std::array<T, Size> result;
		auto view = std::ranges::views::iota(0, Size)
			| std::ranges::views::transform([&iss](auto) { return _SerializeTextImpl<std::decay_t<T>>().Deserialize(iss); });
		std::ranges::copy(view, result.begin());
		return result;
	}
};

template <typename T, size_t Size>
struct _SerializeJsonImpl<std::array<T, Size>>
{
	json Serialize(const std::array<T, Size>& in)
	{
		json result;
		for (auto i = 0; i < in.size(); ++i)
		{
			result[i] = _SerializeJsonImpl<std::decay_t<T>>().Serialize(in[i]);
		}
		return result;
	}

	std::array<T, Size> Deserialize(const json& json)
	{
		std::array<T, Size> result;
		int i = 0;
		for (const auto& e : json)
		{
			result[i++] = _SerializeJsonImpl<std::decay_t<T>>().Deserialize(e);
		}
		return result;
	}
};


template <typename T>
struct _SerializeTextImpl<std::vector<T>>
{
	void Serialize(const std::vector<T>& in, std::ostringstream& oss)
	{
		_SerializeTextImpl<std::vector<T>::size_type>().Serialize(in.size(), oss);
		for (const auto& e : in)
		{
			_SerializeTextImpl<T>().Serialize(e, oss);
		}
	}

	std::vector<T> Deserialize(std::istringstream& iss)
	{
		std::vector<T> result;
		auto size = _SerializeTextImpl<std::vector<T>::size_type>().Deserialize(iss);
		for (auto i = 0; i < size; ++i)
		{
			result.push_back(_SerializeTextImpl<T>().Deserialize(iss));
		}
		return result;
	}
};

template <typename T>
struct _SerializeBytesImpl<std::vector<T>, std::enable_if_t<std::is_trivially_copyable_v<T>>>
{
	void Serialize(const std::vector<T>& in, obytestream& oss)
	{
		_SerializeBytesImpl<std::vector<T>::size_type>().Serialize(in.size(), oss);
		oss.write(reinterpret_cast<const std::byte*>(in.data()), in.size() * sizeof(T));
	}

	std::vector<T> Deserialize(ibytestream& iss)
	{
		std::vector<T> result;
		auto size = _SerializeBytesImpl<std::vector<T>::size_type>().Deserialize(iss);
		result.resize(size);
		iss.read(reinterpret_cast<std::byte*>(result.data()), size * sizeof(T));
		return result;
	}
};

template <typename T>
struct _SerializeBytesImpl<std::vector<T>, std::enable_if_t<!std::is_trivially_copyable_v<T>>>
{
	void Serialize(const std::vector<T>& in, obytestream& oss)
	{
		_SerializeBytesImpl<std::vector<T>::size_type>().Serialize(in.size(), oss);
		for (const auto& e : in)
		{
			_SerializeBytesImpl<std::decay_t<T>>().Serialize(e, oss);
		}
	}

	std::vector<T> Deserialize(ibytestream& iss)
	{
		std::vector<T> result;
		auto size = _SerializeBytesImpl<std::vector<T>::size_type>().Deserialize(iss);
		for (auto i = 0; i < size; ++i)
		{
			result.push_back(_SerializeBytesImpl<std::decay_t<T>>().Deserialize(iss));
		}
		return result;
	}
};

template <typename T>
struct _SerializeJsonImpl<std::vector<T>>
{
	json Serialize(const std::vector<T>& in)
	{
		json result;
		for (auto i = 0; i < in.size(); ++i)
		{
			result[i] = _SerializeJsonImpl<std::decay_t<T>>().Serialize(in[i]);
		}
		return result;
	}

	std::vector<T> Deserialize(const json& json)
	{
		std::vector<T> result;
		for (const auto& e : json)
		{
			result.push_back(_SerializeJsonImpl<std::decay_t<T>>().Deserialize(e));
		}
		return result;
	}
};

template <typename T, int DIM>
struct _SerializeTextImpl<Eigen::Matrix<T, DIM, 1>>
{
	void Serialize(const Eigen::Matrix<T, DIM, 1>& in, std::ostringstream& oss)
	{
		for (int i = 0; i < DIM; ++i)
		{
			_SerializeTextImpl<std::decay_t<T>>().Serialize(in(i, 0), oss);
		}
	}

	Eigen::Matrix<T, DIM, 1> Deserialize(std::istringstream& iss)
	{
		Eigen::Matrix<T, DIM, 1> result;
		for (int i = 0; i < DIM; ++i)
		{
			result(i, 0) = _SerializeTextImpl<std::decay_t<T>>().Deserialize(iss);
		}
		return result;
	}
};

template <typename T, int DIM>
struct _SerializeBytesImpl<Eigen::Matrix<T, DIM, 1>>
{
	void Serialize(const Eigen::Matrix<T, DIM, 1>& in, obytestream& oss)
	{
		for (auto i = 0; i < DIM; ++i)
		{
			_SerializeBytesImpl<std::decay_t<T>>().Serialize(in(i, 0), oss);
		}
	}

	Eigen::Matrix<T, DIM, 1> Deserialize(ibytestream& iss)
	{
		Eigen::Matrix<T, DIM, 1> result;
		for (auto i = 0; i < DIM; ++i)
		{
			result(i, 0) = _SerializeBytesImpl<std::decay_t<T>>().Deserialize(iss);
		}
		return result;
	}
};

template <typename T, int DIM>
struct _SerializeJsonImpl<Eigen::Matrix<T, DIM, 1>>
{
	json Serialize(const Eigen::Matrix<T, DIM, 1>& in)
	{
		json result;
		for (auto i = 0; i < DIM; ++i)
		{
			result[i] = _SerializeJsonImpl<std::decay_t<T>>().Serialize(in(i, 0));
		}
		return result;
	}

	Eigen::Matrix<T, DIM, 1> Deserialize(const json& json)
	{
		Eigen::Matrix<T, DIM, 1> result;
		for (int i = 0; i < DIM; ++i)
		{
			result(i, 0) = _SerializeJsonImpl<std::decay_t<T>>().Deserialize(json[i]);
		}
		return result;
	}
};

#pragma endregion


#define MEMBER_SERIALIZE_TEXT(ARG) \
	_SerializeTextImpl<decltype(SerializeType::ARG)>().Serialize(in.ARG, oss);
#define MEMBER_DESERIALIZE_TEXT(ARG) \
	result.ARG = _SerializeTextImpl<decltype(SerializeType::ARG)>().Deserialize(iss);

#define CLASS_SERIALIZE_TEXT(CLASS_NAME, ...) \
	template <> \
	struct _SerializeTextImpl<CLASS_NAME> \
	{ \
		using SerializeType = CLASS_NAME; \
		void Serialize(const CLASS_NAME& in, std::ostringstream& oss) \
		{ \
			FOR_EACH(MEMBER_SERIALIZE_TEXT, __VA_ARGS__) \
		} \
		CLASS_NAME Deserialize(std::istringstream& iss) \
		{ \
			CLASS_NAME result; \
			FOR_EACH(MEMBER_DESERIALIZE_TEXT, __VA_ARGS__) \
			return result; \
		} \
	};

#define MEMBER_SERIALIZE_BYTES(ARG) \
	_SerializeBytesImpl<decltype(SerializeType::ARG)>().Serialize(in.ARG, oss);
#define MEMBER_DESERIALIZE_BYTES(ARG) \
	result.ARG = _SerializeBytesImpl<decltype(SerializeType::ARG)>().Deserialize(iss);

#define CLASS_SERIALIZE_BYTES(CLASS_NAME, ...) \
	template <> \
	struct _SerializeBytesImpl<CLASS_NAME> \
	{ \
		using SerializeType = CLASS_NAME; \
		void Serialize(const CLASS_NAME& in, obytestream& oss) \
		{ \
			FOR_EACH(MEMBER_SERIALIZE_BYTES, __VA_ARGS__) \
		} \
		CLASS_NAME Deserialize(ibytestream& iss) \
		{ \
			CLASS_NAME result; \
			FOR_EACH(MEMBER_DESERIALIZE_BYTES, __VA_ARGS__) \
			return result; \
		} \
	};

#define MEMBER_SERIALIZE_JSON(ARG) \
	result[#ARG] = _SerializeJsonImpl<decltype(SerializeType::ARG)>().Serialize(in.ARG);
#define MEMBER_DESERIALIZE_JSON(ARG) \
	result.ARG = _SerializeJsonImpl<decltype(SerializeType::ARG)>().Deserialize(json[#ARG]);

#define CLASS_SERIALIZE_JSON(CLASS_NAME, ...) \
	template <> \
	struct _SerializeJsonImpl<CLASS_NAME> \
	{ \
		using SerializeType = CLASS_NAME; \
		json Serialize(const CLASS_NAME& in) \
		{ \
			json result; \
			FOR_EACH(MEMBER_SERIALIZE_JSON, __VA_ARGS__) \
			return result; \
		} \
		CLASS_NAME Deserialize(const json& json) \
		{ \
			CLASS_NAME result; \
			FOR_EACH(MEMBER_DESERIALIZE_JSON, __VA_ARGS__) \
			return result; \
		} \
	};

#define CLASS_REFLECTION(CLASS_NAME, ...) \
	CLASS_SERIALIZE_TEXT(CLASS_NAME, __VA_ARGS__) \
	CLASS_SERIALIZE_BYTES(CLASS_NAME, __VA_ARGS__) \
	CLASS_SERIALIZE_JSON(CLASS_NAME, __VA_ARGS__)



//int main()
//{
//	ChannelParam param;
//	param.Name = "sdfasfsda";
//	param.TexturePath = "dummy-path";
//	param.TexCoord = 1;
//	param.ScalarParams = { { "param1", 1.0f }, { "param2", 2.0f } };
//	param.Vector2Params = { { "param1", { { 1.0f, 2.0f } } }, { "param2", { { 3.0f, 4.0f } } } };
//	param.Vector3Params = { { "param1", { { 1.0f, 2.0f, 3.0f } } }, { "param2", { { 4.0f, 5.0f, 6.0f } } } };
//	param.Vector4Params = { { "param1", { { 1.0f, 2.0f, 3.0f, 4.0f } } }, { "param2", { { 5.0f, 6.0f, 7.0f, 8.0f } } } };
//
//	Material mat;
//	mat.Name = "mat01";
//	mat.Channels.push_back(Channel{ "channel01", std::vector<ChannelParam>{ param, param } });
//
//	auto str1 = SerializeToString(mat);
//	auto o1 = DeserializeFromString<Material>(str1);
//
//	auto bs = SerializeToBytes(mat);
//	auto o2 = DeserializeFromBytes<Material>(bs);
//
//	auto json1 = SerializeToJson(mat);
//	std::cout << json1 << std::endl;
//	auto o3 = DeserializeFromJson<Material>(json1);
//
//	using Type = std::vector<std::pair<int, std::string>>;
//	Type p = {
//		{ 1, "hello" },
//		{ 2, "world" }
//	};
//	json j = SerializeToJson(p);
//	std::cout << j << std::endl;
//	auto p2 = DeserializeFromJson<Type>(j);
//
//	std::vector<int> a = { 1,2 ,3, 4 };
//	auto bv = SerializeToBytes(a);
//	auto a2 = DeserializeFromBytes<std::vector<int>>(bv);
//
//	std::vector<std::string> s = { "1" ,"2" ,"3", "4" };
//	auto bss = SerializeToBytes(s);
//	auto a3 = DeserializeFromBytes<std::vector<std::string>>(bss);
//}
