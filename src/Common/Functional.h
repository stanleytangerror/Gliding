#pragma once


/* https://blog.rink.nu/2024/02/24/implementing-a-stdfunction-like-wrapper-in-c-part-1-type-erasing/
 */

template <typename Ret, typename... Args>
struct IMyFunc
{
	virtual Ret operator() (Args... args) = 0;

	virtual void MoveTo(void* p) = 0;
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

	void MoveTo(void* p) override
	{
		auto c = new (p) MyFunc(std::move(callable));
	}
};

template <typename T>
struct MoveOnlyFunction;

template <typename Ret, typename... Args>
struct MoveOnlyFunction<Ret(Args...)>
{
	static inline constexpr size_t StorageSize = 64;
	static inline constexpr size_t SmallObjectSize = StorageSize - sizeof(void*);

	union Storage
	{
		std::max_align_t dummy1;
		struct
		{
			std::array<std::byte, SmallObjectSize> buffer;
			IMyFunc<Ret, Args...>* ptr;
		};
	} storage;

	template <typename Callable>
	MoveOnlyFunction(Callable c)
	{
		storage.buffer.fill({});

		using T = MyFunc<Callable, Ret, Args...>;
		if constexpr (sizeof(Callable) <= SmallObjectSize)
		{
			new (storage.buffer.data()) T(std::move(c));
			storage.ptr = reinterpret_cast<T*>(storage.buffer.data());
		}
		else
		{
			storage.ptr = new T(std::move(c));
		}
	}

	MoveOnlyFunction()
	{
		storage.buffer.fill({});
		storage.ptr = nullptr;
	}
	
	MoveOnlyFunction(MoveOnlyFunction&& other)
		: MoveOnlyFunction()
	{
		if (other.IsLocal())
		{
			other.storage.ptr->MoveTo(this->storage.buffer.data());
			storage.ptr = reinterpret_cast<IMyFunc<Ret, Args...>*>(storage.buffer.data());
		}
		else
		{
			std::swap(storage.ptr, other.storage.ptr);
		}
	}

	MoveOnlyFunction& operator=(MoveOnlyFunction&& other)
	{
		if (other.IsLocal())
		{
			other.storage.ptr->MoveTo(this->storage.buffer.data());
		}
		else
		{
			std::swap(storage.ptr, other.storage.ptr);
		}
		return *this;
	}

	~MoveOnlyFunction()
	{
		if (IsLocal())
		{
			std::destroy_at(storage.ptr);
		}
		else
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
	bool IsLocal() const noexcept
	{
		return storage.ptr == reinterpret_cast<const IMyFunc<Ret, Args...>*>(storage.buffer.data());
	}
};
