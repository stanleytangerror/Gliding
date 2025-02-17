#pragma once


/* https://blog.rink.nu/2024/02/24/implementing-a-stdfunction-like-wrapper-in-c-part-1-type-erasing/
 */

template <typename R, typename... Args>
struct IMyFunc
{
	virtual R operator() (Args... args) = 0;
};

template <typename F, typename R, typename... Args>
struct MyFunc : IMyFunc<R, Args...>
{
	F f;
	MyFunc(F f) : f(std::move(f)) {}
	R operator() (Args... args) override
	{
		return f(args...);
	}
};

template <typename T>
struct MoveOnlyFunction;

template <typename R, typename... Args>
struct MoveOnlyFunction<R(Args...)>
{
	static inline constexpr size_t StorageSize = 64;
	static inline constexpr size_t SmallObjectSize = StorageSize - sizeof(void*);

	union Storage
	{
		std::max_align_t dummy1;
		struct
		{
			std::array<std::byte, SmallObjectSize> buf;
			IMyFunc<R, Args...>* ptr;
		};
	} s;

	template <typename F>
	MoveOnlyFunction(F f)
	{
		if constexpr (sizeof(F) <= SmallObjectSize)
		{
			std::construct_at(
				reinterpret_cast<MyFunc<F, R, Args...>*>(s.buf.data()),
				std::move(f));

			s.ptr = reinterpret_cast<MyFunc<F, R, Args...>*>(s.buf.data());
		}
		else
		{
			s.ptr = new MyFunc<F, R, Args...>(std::move(f));
		}
	}

	~MoveOnlyFunction()
	{
		if (IsLocal())
		{
			std::destroy_at(s.ptr);
		}
		else
		{
			if (s.ptr != nullptr)
			{
				delete s.ptr;
			}
		}
	}

	R operator() (Args... args)
	{
		return s.ptr->operator()(args...);
	}

private:
	bool IsLocal() const noexcept
	{
		return s.ptr == reinterpret_cast<const IMyFunc<R, Args...>*>(s.buf.data());
	}
};
