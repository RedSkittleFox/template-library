#pragma once

#include <type_traits>
#include <memory_resource>
#include <array>
#include <utility>
#include <bitset>
#include <cassert>
#include <bit>

namespace fox
{
	template<class T, std::size_t Capacity>
	class inplace_free_list
	{
		// Type used to implement in place free list
		using offset_type = std::conditional_t<
			sizeof(T) == 1,
			std::uint8_t,
			std::uint16_t
		>;

		static constexpr offset_type offset_type_npos = std::numeric_limits<offset_type>::max();
		static_assert(Capacity < offset_type_npos);

		alignas(alignof(T)) std::array<std::uint8_t, Capacity * sizeof(T)> storage_;
		offset_type first_free_;
		std::size_t size_;

		// MSVC doesn't properly implement [[no_unique_address]]
		struct offset_accessor_a
		{
			offset_type offset;
		};

		struct offset_accessor_b
		{
			offset_type offset;
			std::array<std::uint8_t, sizeof(T) - sizeof(offset_type)> padding;
		};

		using offset_accessor = std::conditional_t<sizeof(T) == sizeof(offset_type), offset_accessor_a, offset_accessor_b>;

	public:
		using value_type = T;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

	public:
		inplace_free_list()
		{
			initialize_empty();
		}

		inplace_free_list(const inplace_free_list& other)
		{
			initialize_copy(other);
		}

		inplace_free_list(inplace_free_list&& other) noexcept
		{
			initialize_move(other);
			other.initialize_empty();
		}

		inplace_free_list& operator=(const inplace_free_list& other)
		{
			destroy_all();
			initialize_copy(other);
			return *this;
		}

		inplace_free_list& operator=(inplace_free_list&& other) noexcept
		{
			destroy_all();
			initialize_move(other);
			other.initialize_empty();
			return *this;
		}

		~inplace_free_list()
		{
			destroy_all();
		}

	public:
		[[nodiscard]] static constexpr size_type capacity() noexcept
		{
			return Capacity;
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return size_;
		}

		[[nodiscard]] offset_type first_free_offset() const noexcept
		{
			return first_free_;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return size() == 0;
		}

		[[nodiscard]] bool full() const noexcept
		{
			return first_free_ == offset_type_npos;
		}

		[[nodiscard]] bool is_sorted() const noexcept
		{
			offset_type current = first_free_;
			bool is_sorted_ = true;
			while (current != offset_type_npos && is_sorted_) 
			{
				offset_type next = reinterpret_cast<const offset_accessor*>(std::data(storage_))[current].offset;
				is_sorted_ = is_sorted_ && (next > current);
				current = next;
			}
			return is_sorted_;
		}

	public:
		void clear() noexcept
		{
			destroy_all();
			initialize_empty();
		}

		[[nodiscard]] std::bitset<Capacity> free_mask() const noexcept
		{
			std::bitset<Capacity> out;
			const offset_accessor* begin = reinterpret_cast<const offset_accessor*>(std::data(storage_));

			for(offset_type i = first_free_; i != offset_type_npos; i = begin[i].offset)
			{
				out.set(i);
			}

			return out;
		}

		template<std::invocable<pointer, pointer> Callback>
		void optimize(Callback&& cb)
		{
			optimize_at([&](offset_type from, offset_type to) { cb(this->operator[](from), this->operator[](to)); });
		}

		template<std::invocable<offset_type, offset_type> Callback>
		void optimize_at(Callback&& cb)
		{
			std::bitset<Capacity> mask = free_mask();
			offset_type left = 0;
			offset_type right = static_cast<offset_type>(Capacity - 1);
			while (left < right) 
			{
				if (mask.test(left)) 
				{
					if (mask.test(right)) 
					{
						reinterpret_cast<offset_accessor*>(std::data(storage_))[right].offset = right + 1;
						right--;
					}
					else
					{
						T* source = reinterpret_cast<T*>(std::data(storage_)) + right;
						T* dest = reinterpret_cast<T*>(std::data(storage_)) + left;
						std::construct_at(dest, std::move(*source));
						std::construct_at(reinterpret_cast<offset_accessor*>(std::data(storage_)) + right, static_cast<offset_type>(right + 1));

						cb(right, left);
						
						mask.reset(left);
						mask.set(right);						
						first_free_ = right;
					}

				}
				else 
					left++;
			}

			if (right >= 0 && mask.test(right)) 
			{
				first_free_ = right;
				reinterpret_cast<offset_accessor*>(std::data(storage_))[right].offset = right + 1;
			}

			if (mask.test(Capacity - 1))
			{
				reinterpret_cast<offset_accessor*>(std::data(storage_))[Capacity - 1].offset = offset_type_npos;
			}
		}
		
		void sort() {
			std::bitset<Capacity> mask = free_mask();
			offset_type next_offset = offset_type_npos;
			for (offset_type idx = 0; idx < Capacity; idx++) 
			{
				offset_type rev_idx = Capacity - idx - 1;
				if (mask.test(rev_idx)) 
				{
					reinterpret_cast<offset_accessor*>(std::data(storage_))[rev_idx].offset = next_offset;
					next_offset = static_cast<offset_type>(rev_idx);
				}
			}
			first_free_ = next_offset;
		}
	public:
		template<class... Args>
		[[nodiscard]] T* emplace(Args&&... args) requires (std::constructible_from<T, Args...>)
		{
			if (first_free_ == offset_type_npos)
				return nullptr;

			T* out = reinterpret_cast<T*>(std::data(storage_)) + first_free_;
			first_free_ = reinterpret_cast<offset_accessor*>(std::data(storage_))[first_free_].offset;

			std::construct_at(out, std::forward<Args>(args)...);
			size_ = size_ + 1;
			return out;
		}

		[[nodiscard]] T* insert(const T& value) requires (std::is_copy_constructible_v<T>)
		{
			return emplace(value);
		}

		[[nodiscard]] T* insert(T&& value) requires (std::is_copy_constructible_v<T>)
		{
			return emplace(std::forward<T&&>(value));
		}

	public:
		void erase(const T* ptr) noexcept
		{
			assert_holds_value(ptr);

			std::destroy_at(const_cast<T*>(ptr));
			offset_accessor* p_offset_accessor = std::bit_cast<offset_accessor*>(ptr);
			std::construct_at(p_offset_accessor, first_free_);
			first_free_ = static_cast<offset_type>(std::bit_cast<std::ptrdiff_t>(p_offset_accessor - reinterpret_cast<offset_accessor*>(std::data(storage_))));
			size_ = size_ - 1;
		}

	public:
		[[nodiscard]] T* data() noexcept
		{
			return reinterpret_cast<T*>(std::data(storage_));
		}

		[[nodiscard]] const T* data() const noexcept
		{
			return reinterpret_cast<const T*>(std::data(storage_));
		}

		[[nodiscard]] bool owns(const T* ptr) const noexcept
		{
			return
				std::data(storage_) <= reinterpret_cast<const std::uint8_t*>(ptr) &&
				reinterpret_cast<const std::uint8_t*>(ptr) < std::data(storage_) + std::size(storage_);
		}

		[[nodiscard]] bool holds_value(const T* ptr) const noexcept
		{
			return !free_mask().test(as_index(ptr));
		}

		[[nodiscard]] size_type as_index(const T* ptr) const noexcept
		{
			assert_own(ptr);

			return static_cast<std::size_t>(ptr - data());
		}

	public:
		[[nodiscard]] const T* operator[](size_type idx) const noexcept
		{
			auto ptr = this->data() + idx;
			assert_holds_value(ptr);
			return ptr;
		}

		[[nodiscard]] T* operator[](size_type idx) noexcept
		{
			auto ptr = this->data() + idx;
			//assert_holds_value(ptr);
			return ptr;
		}

		[[nodiscard]] bool holds_value_at(size_type idx) const noexcept
		{
			assert_own(this->data() + idx);
			return !free_mask().test(idx);
		}

		[[nodiscard]] const T* at(size_type idx) const
		{
			auto ptr = this->data() + idx;

			if (!owns(ptr))
				throw std::out_of_range("Index is out of range.");

			if (!holds_value(ptr))
				throw std::out_of_range("Index doesn't hold value.");

			return ptr;
		}

		[[nodiscard]] T* at(size_type idx)
		{
			auto ptr = this->data() + idx;

			if (!owns(ptr))
				throw std::out_of_range("Index is out of range.");

			if (!holds_value(ptr))
				throw std::out_of_range("Index doesn't hold value.");

			return ptr;
		}

	private:
		void assert_own([[maybe_unused]] const T* ptr) const
		{
			assert(this->owns(ptr) == true && "inplace_free_list<T> doesn't own this pointer.");
		}

		void assert_holds_value([[maybe_unused]] const T* ptr) const noexcept
		{
			assert(this->holds_value(ptr) == true && "inplace_free_list<T> ptr doesn't hold value.");
		}

		void destroy_all()
		{
			auto mask = free_mask();
			T* begin = reinterpret_cast<T*>(std::data(storage_));
			for (std::size_t i{}; i < std::size(mask); ++i)
			{
				if(mask.test(i) == false)
					std::destroy_at(begin + i);
			}
		}

		void initialize_copy(const inplace_free_list& other)
		{
			first_free_ = other.first_free_;
			size_ = other.size_;

			if constexpr (std::is_trivially_copy_constructible_v<T>)
			{
				storage_ = other.storage_;
			}
			else
			{
				std::bitset<Capacity> mask;
				const offset_accessor* other_begin = reinterpret_cast<const offset_accessor*>(std::data(other.storage_));
				offset_accessor* begin = reinterpret_cast<offset_accessor*>(std::data(storage_));

				for (offset_type i = first_free_; i != offset_type_npos; i = other_begin[i].offset)
				{
					mask.set(i);
					std::construct_at(begin + i, other_begin[i]);
				}

				for (std::size_t i{}; i < std::size(mask); ++i)
				{
					if (mask.test(i) == false)
					{
						std::construct_at(
							reinterpret_cast<T*>(begin + i),
							*reinterpret_cast<const T*>(other_begin + i)
						);
					}
				}
			}
		}

		void initialize_move(inplace_free_list& other) noexcept
		{
			first_free_ = other.first_free_;
			size_ = other.size_;

			if constexpr (std::is_trivially_copy_constructible_v<T>)
			{
				storage_ = std::move(other.storage_);
			}
			else
			{
				std::bitset<Capacity> mask;
				offset_accessor* other_begin = reinterpret_cast<offset_accessor*>(std::data(other.storage_));
				offset_accessor* begin = reinterpret_cast<offset_accessor*>(std::data(storage_));

				for (offset_type i = first_free_; i != offset_type_npos; i = other_begin[i].offset)
				{
					mask.set(i);
					std::construct_at(begin + i, other_begin[i]);
				}

				for (std::size_t i{}; i < std::size(mask); ++i)
				{
					if (mask.test(i) == false)
					{
						std::construct_at(
							reinterpret_cast<T*>(begin + i),
							std::move(*reinterpret_cast<T*>(other_begin + i))
						);
					}
				}
			}
		}

		void initialize_empty()
		{
			size_ = {};
			first_free_ = {};
			offset_type i = {};
			for (offset_accessor*
				begin = reinterpret_cast<offset_accessor*>(std::data(storage_)),
				*end = begin + Capacity;
				begin != end;
				++begin
				)
			{
				std::construct_at(begin, ++i);
			}

			reinterpret_cast<offset_accessor*>(std::data(storage_))[Capacity - 1].offset = offset_type_npos;
		}
	};
}