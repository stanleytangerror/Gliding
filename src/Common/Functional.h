#pragma once


/* https://blog.rink.nu/2024/02/24/implementing-a-stdfunction-like-wrapper-in-c-part-1-type-erasing/
 */

#define MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE 0

template <typename Ret, typename... Args>
struct IMyFunc
{
	virtual Ret operator() (Args... args) = 0;

#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
	virtual void MoveTo(void* p) = 0;
#endif
};

template <typename Callable, typename Ret, typename... Args>
struct MyFunc : IMyFunc<Ret, Args...>
{
	Callable callable;
	MyFunc(Callable c) : callable(std::move(c)) {}

	Ret operator() (Args... args) override
	{
		return callable(args...);
	}

#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
	void MoveTo(void* p) override
	{
		new (p) MyFunc(std::move(callable));
	}
#endif
};

template <typename T>
struct MoveOnlyFunction;

template <typename Ret, typename... Args>
struct MoveOnlyFunction<Ret(Args...)>
{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
	static inline constexpr size_t StorageSize = 64;
	static inline constexpr size_t SmallObjectSize = StorageSize - sizeof(void*);
#endif

	union Storage
	{
		std::max_align_t dummy1;
		struct
		{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
			std::array<std::byte, SmallObjectSize> buffer;
#endif
			IMyFunc<Ret, Args...>* ptr;
		};
	} storage;

	template <typename Callable>
	MoveOnlyFunction(Callable c)
	{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		storage.buffer.fill({});
#endif

		using T = MyFunc<Callable, Ret, Args...>;
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		if constexpr (sizeof(Callable) <= SmallObjectSize)
		{
			new (storage.buffer.data()) T(std::move(c));
			storage.ptr = reinterpret_cast<T*>(storage.buffer.data());
		}
		else
#endif
		{
			storage.ptr = new T(std::move(c));
		}
	}

	MoveOnlyFunction()
	{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		storage.buffer.fill({});
#endif
		storage.ptr = nullptr;
	}
	
	MoveOnlyFunction(MoveOnlyFunction&& other)
		: MoveOnlyFunction()
	{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		if (other.IsLocal())
		{
			other.storage.ptr->MoveTo(this->storage.buffer.data());
			storage.ptr = reinterpret_cast<IMyFunc<Ret, Args...>*>(storage.buffer.data());
		}
		else
#endif
		{
			std::swap(storage.ptr, other.storage.ptr);
		}
	}

	MoveOnlyFunction& operator=(MoveOnlyFunction&& other)
	{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		if (other.IsLocal())
		{
			other.storage.ptr->MoveTo(this->storage.buffer.data());
			storage.ptr = reinterpret_cast<IMyFunc<Ret, Args...>*>(storage.buffer.data());
		}
		else
#endif
		{
			std::swap(storage.ptr, other.storage.ptr);
		}
		return *this;
	}

	~MoveOnlyFunction()
	{
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
		if (IsLocal())
		{
			std::destroy_at(storage.ptr);
		}
		else
#endif
		{
			if (storage.ptr != nullptr)
			{
				delete storage.ptr;
			}
		}
	}

	Ret operator() (Args... args) const
	{
		return storage.ptr->operator()(args...);
	}

private:
#if MOVE_ONLY_FUNCTION_ENABLE_LOCAL_STORAGE
	bool IsLocal() const noexcept
	{
		return storage.ptr == reinterpret_cast<const IMyFunc<Ret, Args...>*>(storage.buffer.data());
	}
#endif
};
