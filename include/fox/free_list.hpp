#pragma once

#include <fox/ptr_vector.hpp>
#include <fox/inplace_free_list.hpp>

#include <type_traits>
#include <memory_resource>
#include <set>

namespace fox
{
	template<class T, std::size_t ChunkCapacity, class Allocator = std::allocator<T>>
	class free_list
	{
	public:
		using value_type = T;
		using chunk_type = inplace_free_list<T, ChunkCapacity>;
		using allocator_type = Allocator;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;

	private:
		using chunk_allocator = typename std::allocator_traits<allocator_type>::template rebind_alloc<chunk_type>;

	private:
		struct chunk_less
		{
			using is_transparent = void;

			[[nodiscard]] bool operator()(const chunk_type& lhs, const chunk_type& rhs) const noexcept
			{
				return lhs.data() < rhs.data();
			}

			[[nodiscard]] bool operator()(const chunk_type& lhs, const_pointer rhs) const noexcept
			{
				return lhs.data() < rhs;
			}

			[[nodiscard]] bool operator()(const_pointer lhs, const chunk_type& rhs) const noexcept
			{
				return lhs < rhs.data();
			}
		};

		fox::ptr_vector<inplace_free_list<T, ChunkCapacity>, chunk_allocator> chunks_;

	public:
		free_list() = default;

		free_list(const allocator_type& allocator)
			: chunks_(static_cast<chunk_allocator>(allocator)) {}

		free_list(const free_list& other) = default;
		free_list(free_list&& other) noexcept = default;
		free_list& operator=(const free_list& other) = default;
		free_list& operator=(free_list&& other) noexcept = default;
		~free_list() noexcept = default;

	public:
		[[nodiscard]] allocator_type get_allocator() const
		{
			return static_cast<allocator_type>(chunks_.get_allocator());
		}

	public:
		[[nodiscard]] static constexpr size_type chunk_capacity() noexcept
		{
			return ChunkCapacity;
		}

		[[nodiscard]] size_type capacity() const noexcept
		{
			return std::size(chunks_) * chunk_capacity();
		}

		[[nodiscard]] size_type size() const noexcept
		{
			size_type out{};
			for (const auto& c : chunks_)
				out += c.size();

			return out;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return size() == static_cast<size_type>(0);
		}

	public:

		void clear()
		{
			chunks_.clear();
		}

		template<std::invocable<pointer, pointer> Callback>
		void optimize(Callback&& cb)
		{
			optimize_at([&](size_type from, size_type to) { 
				cb(this->operator[](from), this->operator[](to)); 
			});
		}

		template<std::invocable<size_type, size_type> Callback>
		void optimize_at(Callback&& cb)
		{
			std::uint16_t chunk_i = 0;
			std::uint16_t chunk_j = static_cast<std::uint16_t>(chunks_.size() - 1);
			while (chunk_i <= chunk_j && chunk_j > 0) 
			{
				if (chunks_[chunk_i].size() == ChunkCapacity) 
				{
					chunk_i++;
					continue;
				}
				
				chunks_[chunk_i].sort();

				if (chunk_i == chunk_j) 
				{
					chunks_[chunk_i].optimize_at([&](std::uint16_t from, std::uint16_t to) {
						cb(pack_index(chunk_i, from), pack_index(chunk_i, to)); 
					});
					break;
				}
				else
				{
					for (std::uint16_t idx = 0; idx < ChunkCapacity; idx++) 
					{
						std::uint16_t rev_idx = ChunkCapacity - idx - 1;

						if (chunks_[chunk_j].holds_value_at(rev_idx) && !chunks_[chunk_i].full()) 
						{
							
							const T* source = chunks_[chunk_j].at(rev_idx);
							size_type packed_from = pack_index(chunk_j, rev_idx);

							const T* dest = chunks_[chunk_i].emplace(*source);
							std::uint16_t dest_idx = static_cast<std::uint16_t>(chunks_[chunk_i].as_index(dest));
							size_type packed_to = pack_index(chunk_i, dest_idx);

							chunks_[chunk_j].erase(source);

							cb(packed_from, packed_to);
						}
					}

					if (chunks_[chunk_i].full())
						chunk_i++;

					if (chunks_[chunk_j].empty())
						chunk_j--;

				}
			}
			shrink();
		}

		void shrink() {
			while (chunks_.back().empty()) {
				chunks_.pop_back();
			}
			chunks_.shrink_to_fit();
		}

		void sort() {
			for (auto& chunk : chunks_) 
			{
				chunk.sort();
			}
		}

		[[nodiscard]] bool is_sorted() const noexcept 
		{
			bool is_sorted_ = true;
			for (auto& chunk : chunks_) 
			{
				is_sorted_ = is_sorted_ && chunk.is_sorted();
				if (!is_sorted_) break;
			}
			return is_sorted_;
		}

		template<class... Args>
		[[nodiscard]] T* emplace(Args&&... args) requires(std::is_constructible_v<value_type, Args...>)
		{
			chunk_type* allocation_chunk;

			// Find first free
			if (auto r = std::find_if(std::begin(chunks_), std::end(chunks_), [](const auto& v) { return v.full() == false; });
				r != std::end(chunks_))
			{
				allocation_chunk = std::addressof(*r);
			}
			else
			{
				allocation_chunk = std::addressof(chunks_.emplace_back());
			}

			return allocation_chunk->emplace(std::forward<Args>(args)...);
		}

		[[nodiscard]] T* insert(const T& value) requires(std::is_copy_constructible_v<T>)
		{
			return this->emplace(value);
		}

		[[nodiscard]] T* insert(T&& value) requires(std::is_move_constructible_v<T>)
		{
			return this->emplace(std::forward<T&&>(value));
		}

		[[nodiscard]] size_type first_free_index()
		{
			size_type out = std::numeric_limits<size_type>::max();
			// Find first free
			if (auto r = std::find_if(std::begin(chunks_), std::end(chunks_), [](const auto& v) { return v.full() == false; });
				r != std::end(chunks_))
			{
				std::uint16_t position = (*r).first_free_offset();
				out = (static_cast<size_type>(std::distance(std::begin(chunks_), r)) << (sizeof(position) * 8)) | static_cast<size_type>(position);
			}
			
			return out;
		}

	public:
		void erase(const T* ptr)
		{
			auto chunk = owning_chunk(ptr);
			chunk->erase(ptr);

			auto last_chunk_it = --chunks_.end();
			if(chunk->empty() && std::addressof(*last_chunk_it) == chunk)
			{
				chunks_.pop_back();
			}
		}

	public:
		[[nodiscard]] bool owns(const T* ptr) const noexcept
		{
			if (std::empty(chunks_))
				return false;

			auto r = std::find_if(std::begin(chunks_), std::end(chunks_), [=](const auto& c) { return c.owns(ptr); });

			return r != std::end(chunks_);
		}

		[[nodiscard]] bool holds_value(const T* ptr) const noexcept
		{
			auto chunk = owning_chunk(ptr);
			return chunk->holds_value(ptr);
		}

		[[nodiscard]] size_t as_index(const T* ptr) const noexcept
		{
			assert(!std::empty(chunks_) && "free_list<T> doesn't own this pointer.");

			auto chunk_it = std::find_if(std::begin(chunks_), std::end(chunks_), [=](const auto& c) { return c.owns(ptr); });
			assert(chunk_it != std::end(chunks_) && "free_list<T> doesn't own this pointer.");

			auto chunk = std::distance(std::begin(chunks_), chunk_it);
			auto index = chunk_it->as_index(ptr);

			return pack_index(static_cast<std::uint16_t>(chunk), static_cast<std::uint16_t>(index));
		}

	public:
		[[nodiscard]] const T* operator[](size_type idx) const noexcept
		{
			auto [chunk, index] = unpack_index(idx);

			auto chunk_ptr = std::data(chunks_)[chunk];
			return chunk_ptr->operator[](index);
		}

		[[nodiscard]] T* operator[](size_type idx) noexcept
		{
			auto [chunk, index] = unpack_index(idx);

			auto chunk_ptr = std::data(chunks_)[chunk];
			return chunk_ptr->operator[](index);
		}

		[[nodiscard]] bool holds_value_at(size_type idx) const noexcept
		{
			auto [chunk, index] = unpack_index(idx);

			assert(chunk < std::size(chunks_) && "free_list<T> doesn't own this index.");

			auto chunk_ptr = std::data(chunks_)[chunk];
			return chunk_ptr->holds_value_at(index);
		}

		[[nodiscard]] const T* at(size_type idx) const
		{
			auto [chunk, index] = unpack_index(idx);

			auto chunk_ptr = std::data(chunks_)[chunk];
			return chunk_ptr->at(index);
		}

		[[nodiscard]] T* at(size_type idx)
		{
			auto [chunk, index] = unpack_index(idx);

			auto chunk_ptr = std::data(chunks_)[chunk];
			return chunk_ptr->at(index);
		}

	public:
		[[nodiscard]] chunk_type* owning_chunk(const T* ptr) noexcept
		{
			const auto chunk = owning_chunk_no_assert(ptr);
			assert(chunk != nullptr && "free_list<T> doesn't own this pointer.");
			return chunk;
		}

		[[nodiscard]] const chunk_type* owning_chunk(const T* ptr) const noexcept
		{
			const auto chunk = owning_chunk_no_assert(ptr);
			assert(chunk != nullptr && "free_list<T> doesn't own this pointer.");
			return chunk;
		}

		[[nodiscard]] chunk_type* owning_chunk(size_type idx) noexcept
		{
			auto [chunk, index] = unpack_index(idx);

			assert(chunk < std::size(chunks_) && "free_list<T> doesn't own this pointer.");

			auto chunk_it = std::begin(chunks_) + chunk;

			return std::addressof(*chunk_it);
		}

		[[nodiscard]] const chunk_type* owning_chunk(size_type idx) const noexcept
		{
			auto [chunk, index] = unpack_index(idx);

			assert(chunk < std::size(chunks_) && "free_list<T> doesn't own this pointer.");

			auto chunk_it = std::begin(chunks_) + chunk;

			return std::addressof(*chunk_it);
		}

		[[nodiscard]] auto chunks_begin() noexcept { return std::begin(chunks_); }
		[[nodiscard]] auto chunks_begin() const noexcept { return std::begin(chunks_); }
		[[nodiscard]] auto chunks_cbegin() const noexcept { return std::cbegin(chunks_); }
		[[nodiscard]] auto chunks_end() noexcept { return std::end(chunks_); }
		[[nodiscard]] auto chunks_end() const noexcept { return std::end(chunks_); }
		[[nodiscard]] auto chunks_cend() const noexcept { return std::cend(chunks_); }

		[[nodiscard]] auto chunks_rbegin() noexcept { return std::rbegin(chunks_); }
		[[nodiscard]] auto chunks_rbegin() const noexcept { return std::rbegin(chunks_); }
		[[nodiscard]] auto chunks_crbegin() const noexcept { return std::crbegin(chunks_); }
		[[nodiscard]] auto chunks_rend() noexcept { return std::rend(chunks_); }
		[[nodiscard]] auto chunks_rend() const noexcept { return std::rend(chunks_); }
		[[nodiscard]] auto chunks_crend() const noexcept { return std::crend(chunks_); }

	private:
		void assert_owns(const T* ptr) const
		{
			assert(this->owns(ptr) == true && "free_list<T> doesn't own this pointer.");
		}

		void assert_chunks_not_empty() const
		{
			assert(!std::empty(chunks_));
		}

		[[nodiscard]] chunk_type* owning_chunk_no_assert(const T* ptr) noexcept
		{
			if (std::empty(chunks_))
				return nullptr;

			auto r = std::find_if(std::begin(chunks_), std::end(chunks_), [=](const auto& c) { return c.owns(ptr); });
			if (r == std::end(chunks_))
				return nullptr;

			return std::addressof(*r);
		}

		[[nodiscard]] const chunk_type* owning_chunk_no_assert(const T* ptr) const noexcept
		{
			if (std::empty(chunks_))
				return nullptr;

			auto r = std::find_if(std::begin(chunks_), std::end(chunks_), [=](const auto& c) { return c.owns(ptr); });
			if (r == std::end(chunks_))
				return nullptr;

			return std::addressof(*r);
		}

		[[nodiscard]] size_type pack_index(std::uint16_t chunk, std::uint16_t position) const noexcept
		{
			static_assert(ChunkCapacity < std::numeric_limits<std::uint16_t>::max());
			assert(std::size(chunks_) < std::numeric_limits<std::uint16_t>::max());
			static_assert(sizeof(size_type) >= sizeof(std::uint16_t) * 2);

			const size_type out = (static_cast<size_type>(chunk) << (sizeof(position) * 8)) | static_cast<size_type>(position);
			return out;
		}

		[[nodiscard]] static std::pair<std::uint16_t, std::uint16_t> unpack_index(size_type index) noexcept
		{
			std::uint16_t position = static_cast<std::uint16_t>(index & static_cast<size_type>(0xFFFFu));
			std::uint16_t chunk = static_cast<std::uint16_t>((index >> (sizeof(position) * 8)) & static_cast<size_type>(0xFFFFu));

			return std::make_pair(chunk, position);
		}
	};

	namespace pmr
	{
		template<class T, std::size_t ChunkCapacity>
		using free_list = ::fox::free_list<T, ChunkCapacity, std::pmr::polymorphic_allocator<T>>;
	}
}