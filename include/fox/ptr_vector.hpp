#pragma once

#include <fox/iterator/indirect_iterator.hpp>

#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <vector>

namespace fox
{
	template<class T, class Allocator = std::allocator<T>>
	class ptr_vector
	{
		using storage_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<T*>;
		std::vector<T*, storage_allocator> storage_;
		using storage_iterator = typename decltype(storage_)::iterator;

	public:
		using value_type = T;
		using allocator_type = Allocator;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;

		using iterator = ::fox::iterator::indirect_iterator<typename decltype(storage_)::iterator>;
		using const_iterator = ::fox::iterator::indirect_iterator<typename decltype(storage_)::const_iterator>;
		using reverse_iterator = ::fox::iterator::indirect_iterator<typename decltype(storage_)::reverse_iterator>;
		using const_reverse_iterator = ::fox::iterator::indirect_iterator<typename decltype(storage_)::const_reverse_iterator>;

	public:
		constexpr ptr_vector() noexcept(noexcept(allocator_type())) = default;

		constexpr explicit ptr_vector(const allocator_type& alloc) noexcept
			: storage_(static_cast<storage_allocator>(alloc)) {}

		constexpr ptr_vector(size_type count, const T& value, const allocator_type& alloc = allocator_type())
		requires (std::is_copy_constructible_v<T>)
			: storage_(count, nullptr, static_cast<storage_allocator>(alloc))
		{
			_construct_splat(std::begin(storage_), std::end(storage_), value);
		}

		constexpr explicit ptr_vector(size_type count, const allocator_type& alloc = allocator_type())
			: ptr_vector(count, T(), alloc)
		{}

		template<std::input_iterator InputIt>
		constexpr ptr_vector(InputIt first, InputIt last, const allocator_type& alloc = allocator_type())
			: storage_(std::distance(first, last), nullptr, static_cast<storage_allocator>(alloc))
		{
			_construct_from_range(std::begin(storage_), std::end(storage_), first);
		}

		constexpr ptr_vector(const ptr_vector& other)
			: storage_(other.storage_)
		{
			_construct_from_range(std::begin(storage_), std::end(storage_), std::begin(other));
		}

		constexpr ptr_vector(const ptr_vector& other, const allocator_type& alloc)
			: ptr_vector(std::begin(other), std::end(other), alloc) {}

		constexpr ptr_vector(ptr_vector&& other) noexcept(std::is_nothrow_move_constructible_v<allocator_type>)
			: storage_(std::move(other.storage_))
		{

		}

		constexpr ptr_vector(ptr_vector&& other, const allocator_type& alloc) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
			: storage_(std::move(other.storage_), alloc)
		{

		}

		constexpr ptr_vector(std::initializer_list<T> ilist, const allocator_type& alloc = allocator_type())
			: ptr_vector(std::begin(ilist), std::end(ilist), alloc)
		{}

		template<std::ranges::range R>
		constexpr ptr_vector(std::from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
			requires (std::assignable_from<reference, std::ranges::range_reference_t<R>>)
		: ptr_vector(std::begin(std::forward<R>(rg)), std::end(std::forward<R>(rg)), alloc)
		{}

		constexpr ptr_vector& operator=(const ptr_vector& other)
		{
			_destroy_all();
			storage_ = other.storage_;
			_construct_from_range(std::begin(storage_), std::end(storage_), std::begin(other));
			return *this;
		}

		constexpr ptr_vector& operator=(ptr_vector&& other)
			noexcept(std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value || std::allocator_traits<Allocator>::is_always_equal::value)
		{
			_destroy_all();
			storage_ = std::move(other.storage_);
			return *this;
		}

		constexpr ptr_vector& operator=(std::initializer_list<T> ilist)
		{
			_destroy_all();
			storage_.resize(std::size(ilist));
			_construct_from_range(std::begin(storage_), std::end(storage_), std::begin(ilist));
			return *this;
		}

		constexpr ~ptr_vector() noexcept
		{
			_destroy_all();
		}

		constexpr void assign(size_type count, const T& value)
		{
			_destroy_all();
			storage_.resize(count);
			_construct_splat(std::begin(storage_), std::end(storage_), value);
		}

		template<std::input_iterator InputIt>
		constexpr void assign(InputIt first, InputIt last)
		{
			_destroy_all();
			storage_.resize(std::distance(first, last));
			_construct_from_range(std::begin(storage_), std::end(storage_), first);
		}

		constexpr void assign(std::initializer_list<T> ilist)
		{
			_destroy_all();
			storage_.resize(std::size(ilist));
			_construct_from_range(std::begin(storage_), std::end(storage_), std::begin(ilist));
		}

		template<std::ranges::range R>
		constexpr void assign_range(R&& rg)
			requires (std::constructible_from<value_type, std::ranges::range_reference_t<R>>)
		{
			this->assign(std::begin(rg), std::end(rg));
		}

		[[nodiscard]] constexpr allocator_type get_allocator() const noexcept
		{
			return static_cast<allocator_type>(storage_.get_allocator());
		}

	public:
		[[nodiscard]] constexpr reference at(size_type pos)
		{
			return *storage_.at(pos);
		}

		[[nodiscard]] constexpr const_reference at(size_type pos) const
		{
			return *storage_.at(pos);
		}

		[[nodiscard]] constexpr reference operator[](size_type pos) noexcept
		{
			return *storage_.operator[](pos);
		}

		[[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept
		{
			return *storage_.operator[](pos);
		}

		[[nodiscard]] constexpr reference front() noexcept
		{
			return *storage_.front();
		}

		[[nodiscard]] constexpr const_reference front() const noexcept
		{
			return *storage_.front();
		}

		[[nodiscard]] constexpr reference back() noexcept
		{
			return *storage_.back();
		}

		[[nodiscard]] constexpr const_reference back() const noexcept
		{
			return *storage_.back();
		}

		[[nodiscard]] constexpr T** data() noexcept
		{
			return storage_.data();
		}

		[[nodiscard]] constexpr T const*const* data() const noexcept
		{
			return storage_.data();
		}

	public:
		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return static_cast<iterator>(storage_.begin());
		}

		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return static_cast<const_iterator>(storage_.begin());
		}

		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return static_cast<const_iterator>(storage_.cbegin());
		}

		[[nodiscard]] constexpr iterator end() noexcept
		{
			return static_cast<iterator>(storage_.end());
		}

		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return static_cast<const_iterator>(storage_.end());
		}

		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return static_cast<const_iterator>(storage_.cend());
		}

		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept
		{
			return static_cast<reverse_iterator>(storage_.rbegin());
		}

		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
		{
			return static_cast<const_reverse_iterator>(storage_.rbegin());
		}

		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
		{
			return static_cast<const_reverse_iterator>(storage_.crbegin());
		}

		[[nodiscard]] constexpr reverse_iterator rend() noexcept
		{
			return static_cast<reverse_iterator>(storage_.rend());
		}

		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
		{
			return static_cast<const_reverse_iterator>(storage_.rend());
		}

		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
		{
			return static_cast<const_reverse_iterator>(storage_.crend());
		}

	public:
		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return storage_.empty();
		}

		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return storage_.size();
		}

		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			return storage_.max_size();
		}

		constexpr void reserve(size_type new_capacity) 
		{
			return storage_.reserve(new_capacity);
		}

		[[nodiscard]] constexpr size_type capacity() const noexcept
		{
			return storage_.capacity();
		}

		constexpr void shrink_to_fit() 
		{
			return storage_.shrink_to_fit();
		}

	public:
		constexpr void clear() noexcept
		{
			_destroy_all();
			storage_.clear();
		}

		constexpr iterator insert(const_iterator pos, const T& value)
		{
			return this->emplace(pos, value);
		}

		constexpr iterator insert(const_iterator pos, T&& value)
		{
			return this->emplace(pos, std::move(value));
		}

		constexpr iterator insert(const_iterator pos, size_type count, const T& value)
		{
			const std::size_t original_pos = std::distance(std::cbegin(storage_), pos.base());
			storage_.insert(pos.base(), count, nullptr);

			try
			{
				_construct_splat(
					std::begin(storage_) + original_pos,
					std::begin(storage_) + original_pos + count,
					value
				);

				return this->begin() + original_pos;
			}
			catch(...)
			{
				// Undo the change
				storage_.erase(std::begin(storage_) + original_pos, std::begin(storage_) + original_pos + count);
				std::rethrow_exception(std::current_exception());
			}
		}

		template<std::input_iterator InputIt>
		constexpr iterator insert(const_iterator pos, InputIt first, InputIt last)
		{
			const std::size_t original_pos = std::distance(std::cbegin(storage_), pos.base());
			const auto count = std::distance(first, last);
			storage_.insert(pos.base(), count, nullptr);

			try
			{
				_construct_from_range(
					std::begin(storage_) + original_pos,
					std::begin(storage_) + original_pos + count,
					first
				);

				return this->begin() + original_pos;
			}
			catch (...)
			{
				// Undo the change
				storage_.erase(std::begin(storage_) + original_pos, std::begin(storage_) + original_pos + count);
				std::rethrow_exception(std::current_exception());
			}
		}

		constexpr iterator insert(const_iterator pos, std::initializer_list<T> ilist)
		{
			return insert(pos, std::begin(ilist), std::end(ilist));
		}

		template<std::ranges::range R>
		constexpr iterator insert_range(const_iterator pos, R&& rg)
			requires (std::constructible_from<value_type, std::ranges::range_reference_t<R>>)
		{
			return insert(pos, std::begin(rg), std::end(rg));
		}

		template<class... Args>
		constexpr iterator emplace(const_iterator pos, Args&&... args)
			requires(std::constructible_from<T, Args...>)
		{
			auto ptr = _construct_object(std::forward<Args>(args)...);
			auto out = static_cast<iterator>(storage_.insert(pos.base(), ptr.get()));;
			ptr.release();
			return out;
		}

		constexpr iterator erase(const_iterator pos)
		{
			pointer ptr = *(pos.base());
			auto out = static_cast<iterator>(storage_.erase(pos.base()));
			std::destroy_at(ptr);
			return out;
		}

		constexpr iterator erase(const_iterator first, const_iterator last)
		{
			for(auto it = first.base(); it != last.base(); ++it)
			{
				std::destroy_at(*it);
				this->get_allocator().deallocate(*it, 1);
			}

			return static_cast<iterator>(storage_.erase(first.base(), last.base()));
		}

		constexpr void push_back(const T& value)
		{
			this->emplace_back(value);
		}

		constexpr void push_back(T&& value)
		{
			this->emplace_back(std::move(value));
		}

		template<class... Args>
		constexpr reference emplace_back(Args&&... args)
			requires(std::constructible_from<T, Args...>)
		{
			auto ptr = _construct_object(std::forward<Args>(args)...);
			auto out = storage_.emplace_back(ptr.get());
			(void)ptr.release();
			return *out;
		}

		template<std::ranges::range R>
		constexpr void append_range(R&& rg)
			requires (std::constructible_from<value_type, std::ranges::range_reference_t<R>>)
		{
			this->insert_range(std::end(*this), std::forward<R>(rg));
		}

		constexpr void pop_back()
		{
			auto ptr = storage_.back();
			storage_.pop_back();
			std::destroy_at(ptr);
			this->get_allocator().deallocate(ptr, 1);
		}

		constexpr void resize(size_type count)
		{
			this->resize(count, T());
		}

		constexpr void resize(size_type count, const T& value)
		{
			if(count == 0)
			{
				this->clear();
			}
			if(count < std::size(storage_))
			{
				this->erase(std::begin(*this) + std::size(storage_) - count - 1, std::end(*this));
			}
			else if(count > std::size(storage_))
			{
				this->insert(std::end(*this), count - std::size(storage_), value);
			}
		}

		constexpr void swap(ptr_vector& other) noexcept(noexcept(storage_.swap(std::declval<decltype(storage_)&>())))
		{
			storage_.swap(other.storage_);
		}

	private:
		constexpr void _undo_partial_construction(storage_iterator storage_begin, storage_iterator storage_end) noexcept
		{
			for (; storage_begin != storage_end; )
			{
				std::destroy_at(*storage_begin);
				get_allocator().deallocate(*storage_begin, 1);
			}
		}

		template<class It>
		constexpr void _construct_from_range(
			storage_iterator storage_begin, storage_iterator storage_end,
			It first
		)
		{
			// Strong exception guarantee
			// Make sure we don't leak memory or objects when this throws 
			pointer last_allocation = nullptr;
			auto original_begin = storage_begin; // Keep original in case we throw and we need to destroy objects
			try
			{
				for(; storage_begin != storage_end; ++storage_begin, ++first)
				{
					last_allocation = this->get_allocator().allocate(1);
					last_allocation = std::construct_at(last_allocation, *first);
					*storage_begin = last_allocation;
				}
			}
			catch (...)
			{
				// Won't be null when object constructor throws
				this->get_allocator().deallocate(last_allocation, 1);

				_undo_partial_construction(original_begin, storage_begin - 1);
				std::rethrow_exception(std::current_exception());
			}
		}

		constexpr void _construct_splat(
			storage_iterator storage_begin, storage_iterator storage_end,
			const value_type& value)
		{
			pointer last_allocation = nullptr;
			auto original_begin = storage_begin; // Keep original in case we throw and we need to destroy objects
			try
			{
				for (; storage_begin != storage_end; ++storage_begin)
				{
					last_allocation = this->get_allocator().allocate(1);
					last_allocation = std::construct_at(last_allocation, value);
					*storage_begin = last_allocation;
				}
			}
			catch (...)
			{
				// Won't be null when object constructor throws
				this->get_allocator().deallocate(last_allocation, 1);

				_undo_partial_construction(original_begin, storage_begin - 1);
				std::rethrow_exception(std::current_exception());
			}
		}

		template<class... Args>
		constexpr auto _construct_object(Args&&... args)
		{
			// Strong exception guarantee
			// Make sure we don't leak memory or objects when this throws 
			pointer last_allocation = nullptr;
			try
			{
				last_allocation = this->get_allocator().allocate(1);
				last_allocation = std::construct_at(last_allocation, std::forward<Args>(args)...);
				auto deleter = [u = this](pointer p) { u->get_allocator().deallocate(p, 1); };
				return std::unique_ptr<value_type, decltype(deleter)>(last_allocation, std::move(deleter));
			}
			catch(...)
			{
				this->get_allocator().deallocate(last_allocation, 1);

				std::rethrow_exception(std::current_exception());
			}
		}

		constexpr void _destroy_all() noexcept
		{
			for (auto e : storage_)
			{
				std::destroy_at(e);
				get_allocator().deallocate(e, 1);
			}
		}

		[[nodiscard]] constexpr allocator_type _propagate_on_copy(const auto& other)
		{
			if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value)
			{
				return static_cast<allocator_type>(other.get_allocator());
			}
			else
			{
				return allocator_type();
			}
		}

		[[nodiscard]] constexpr allocator_type _propagate_on_move(const auto& other)
		{
			if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value)
			{
				return static_cast<allocator_type>(other.get_allocator());
			}
			else
			{
				return allocator_type();
			}
		}
	};

	template<class T, class Allocator>
	[[nodiscard]] constexpr bool operator==(const ptr_vector<T, Allocator>& lhs, const ptr_vector<T, Allocator>& rhs)
	{
		return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
	}

	template<class T, class Allocator>
	[[nodiscard]] constexpr auto operator<=>(const ptr_vector<T, Allocator>& lhs, const ptr_vector<T, Allocator>& rhs)
	{
		return std::lexicographical_compare_three_way(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
	}

	template<class T, class Allocator, std::equality_comparable_with<T> U = T>
	constexpr typename std::vector<T, Allocator>::size_type erase(ptr_vector<T, Allocator>& c, const U& value)
	{
		auto it = std::remove(c.begin(), c.end(), value);
		auto r = std::end(c) - it;
		c.erase(it, std::end(c));
		return r;
	}

	template<class T, class Allocator, std::predicate<T&> Predicate>
	constexpr typename std::vector<T, Allocator>::size_type erase_if(ptr_vector<T, Allocator>& c, Predicate predicate)
	{
		auto it = std::remove_if(std::begin(c), std::end(c), predicate);
		auto r = std::end(c) - it;
		c.erase(it, std::end(c));
		return r;
	}
}
