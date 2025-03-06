#pragma once


/* https://blog.rink.nu/2024/02/24/implementing-a-stdfunction-like-wrapper-in-c-part-1-type-erasing/
 */

#if 1

#define MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE 1

namespace details
{
	template <typename Ret, typename... Args>
	struct IMyFunc
	{
		virtual Ret operator() (Args... args) = 0;
		virtual ~IMyFunc() {}

#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		virtual void MoveTo(IMyFunc<Ret, Args...>* p) = 0;
#endif
	};

	template <typename Callable, typename Ret, typename... Args>
	struct MyFunc : IMyFunc<Ret, Args...>
	{
		Callable callable;
		MyFunc(Callable&& c) : callable(std::move(c)) {}

		Ret operator() (Args... args) override
		{
			return callable(args...);
		}

#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		void MoveTo(IMyFunc<Ret, Args...>* p) override
		{
			std::construct_at(reinterpret_cast<MyFunc*>(p), std::move(callable));
		}
#endif
	};
}

template <typename T>
struct MoveOnlyFunction;

template <typename Ret, typename... Args>
struct MoveOnlyFunction<Ret(Args...)>
{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
	static inline constexpr size_t StorageSize = 64;
	static inline constexpr size_t SmallObjectSize = StorageSize - sizeof(void*);
#endif

	using Basic = details::IMyFunc<Ret, Args...>;

#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
	alignas(alignof(std::max_align_t)) std::array<std::byte, SmallObjectSize> buffer;
#endif

	Basic* pimpl = nullptr;

	template <typename Callable>
	MoveOnlyFunction(Callable&& c)
	{
		using Impl = details::MyFunc<Callable, Ret, Args...>;

#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		if constexpr (sizeof(Impl) <= SmallObjectSize)
		{
			pimpl = std::construct_at(reinterpret_cast<Impl*>(buffer.data()), std::move(c));
		}
		else
#endif
		{
			pimpl = new Impl(std::move(c));
		}
	}

	MoveOnlyFunction() {}

	MoveOnlyFunction(MoveOnlyFunction&& other)
	{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		if (other.IsLocal())
		{
			pimpl = reinterpret_cast<Basic*>(buffer.data());

			other.pimpl->MoveTo(pimpl);
			other.pimpl = nullptr;
		}
		else
#endif
		{
			std::swap(pimpl, other.pimpl);
		}
	}

	MoveOnlyFunction& operator=(MoveOnlyFunction&& other)
	{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		if (other.IsLocal())
		{
			pimpl = reinterpret_cast<Basic*>(buffer.data());
			
			other.pimpl->MoveTo(pimpl);
			other.pimpl = nullptr;
		}
		else
#endif
		{
			std::swap(pimpl, other.pimpl);
		}
		return *this;
	}

	operator bool() const
	{
		return pimpl != nullptr;
	}

	~MoveOnlyFunction()
	{
		if (pimpl != nullptr)
		{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
			if (IsLocal())
			{
				std::destroy_at(reinterpret_cast<Basic*>(pimpl));
			}
			else
#endif
			{
				delete pimpl;
			}
		}
	}

	Ret operator() (Args... args) const
	{
		return (*pimpl)(args...);
	}

private:
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
	bool IsLocal() const noexcept
	{
		return pimpl == reinterpret_cast<const Basic*>(buffer.data());
	}
#endif
};

#else

template <typename T>
using MoveOnlyFunction = std::move_only_function<T>;

#endif