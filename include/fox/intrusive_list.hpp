#pragma once

#include <type_traits>
#include <memory_resource>
#include <array>
#include <utility>
#include <bitset>
#include <cassert>
#include <algorithm>
#include <ranges>
#include <limits>

namespace fox
{
	template<class T>
	struct intrusive_list_node_traits
	{
		using sentinel = T;

		[[nodiscard]] static T* construct_sentinel(sentinel* memory)
		{
			return std::construct_at(memory);
		}

		static void destroy_sentinel(T* obj)
		{
			std::destroy_at(obj);
		}

		[[nodiscard]] static T* next(T* current)
		{
			return current->next;
		}

		[[nodiscard]] static T* previous(T* current)
		{
			return current->previous;
		}

		[[nodiscard]] static const T* next(const T* current)
		{
			return current->next;
		}

		[[nodiscard]] static const T* previous(const T* current)
		{
			return current->previous;
		}

		static void next(T* current, T* new_next)
		{
			current->next = new_next;
		}

		static void previous(T* current, T* new_previous)
		{
			current->previous = new_previous;
		}
	};

	template<
		class T,
		class Traits = intrusive_list_node_traits<T>,
		class Allocator = std::allocator<T>
	>
	class intrusive_list
	{
		using allocator_traits = std::allocator_traits<Allocator>;

	public:
		using value_type = T;
		using allocator_type = Allocator;
		using node_traits = Traits;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;

	private:
		template<class U>
		struct _iterator_implementation
		{
			U* node_;

			_iterator_implementation(U* node)
				: node_(node) {}

		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = U;
			using reference = U&;
			using pointer = U*;

		public:
			_iterator_implementation() = default;
			_iterator_implementation(const _iterator_implementation&) = default;
			_iterator_implementation(_iterator_implementation&&) noexcept = default;
			_iterator_implementation& operator=(const _iterator_implementation&) = default;
			_iterator_implementation& operator=(_iterator_implementation&&) noexcept = default;
			~_iterator_implementation() noexcept = default;

			template<class V>
			_iterator_implementation(const _iterator_implementation<V>& other)
				requires(std::is_convertible_v<V*, U*> && !std::is_same_v<U, V>)
				: node_(static_cast<U*>(other.node()))
			{}

		public:
			[[nodiscard]] value_type& operator*() const
			{
				return *node_;
			}

			[[nodiscard]] value_type* operator->() const
			{
				return node_;
			}

			_iterator_implementation& operator++()
			{
				node_ = node_traits::next(node_);

				return *this;
			}

			[[nodiscard]] _iterator_implementation operator++(int)
			{
				auto it = *this;
				++(*this);
				return it;
			}

			_iterator_implementation& operator--()
			{
				node_ = node_traits::previous(node_);

				return *this;
			}

			[[nodiscard]] _iterator_implementation operator--(int)
			{
				auto it = *this;
				--(*this);
				return it;
			}

			[[nodiscard]] U* node() const noexcept
			{
				return node_;
			}

		public:
			friend void iter_swap(_iterator_implementation& lhs, _iterator_implementation& rhs)
			{
				auto lhs_ptr = lhs.node_;
				auto rhs_ptr = rhs.node_;

				auto lp = node_traits::previous(lhs_ptr);
				auto ln = node_traits::next(lhs_ptr);

				auto rp = node_traits::previous(rhs_ptr);
				auto rn = node_traits::next(rhs_ptr);

				if(ln == rhs_ptr || rn == lhs_ptr)
				{
					node_traits::next(lp, rhs_ptr);
					node_traits::previous(rhs_ptr, lp);

					node_traits::next(rhs_ptr, lhs_ptr);
					node_traits::previous(lhs_ptr, rhs_ptr);

					node_traits::next(lhs_ptr, rn);
					node_traits::previous(rn, lhs_ptr);
				}
				else
				{
					node_traits::next(lp, rhs_ptr);
					node_traits::previous(ln, rhs_ptr);

					node_traits::next(rp, lhs_ptr);
					node_traits::previous(rn, lhs_ptr);

					node_traits::next(lhs_ptr, rn);
					node_traits::previous(lhs_ptr, rp);

					node_traits::next(rhs_ptr, ln);
					node_traits::previous(rhs_ptr, lp);
				}
			}

			[[nodiscard]] friend bool operator==(const _iterator_implementation& lhs, const _iterator_implementation& rhs)
			{
				return lhs.node_ == rhs.node_;
			}

			[[nodiscard]] friend bool operator!=(const _iterator_implementation& lhs, const _iterator_implementation& rhs)
			{
				return lhs.node_ != rhs.node_;
			}
		};

	public:
		using iterator = _iterator_implementation<T>;
		using const_iterator = _iterator_implementation<const T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	private:
#if __has_cpp_attribute(msvc::no_unique_address)
		[[msvc::no_unique_address]]
#elif __has_cpp_attribute(no_unique_address)
		[[no_unique_address]]
#endif
		allocator_type allocator_;
		T* sentinel_;

	public:
		intrusive_list()
			: sentinel_(_construct_sentinel()) {}

		explicit intrusive_list(const allocator_type& alloc)
			: allocator_(alloc), sentinel_(_construct_sentinel()) {}

		explicit intrusive_list(size_type count, const T& value, const allocator_type& alloc = allocator_type())
			: allocator_(alloc)
			, sentinel_(_construct_sentinel())
		{
			_insert_splat_value(this->begin(), count, value);
		}

		explicit intrusive_list(size_type count, const allocator_type& alloc = allocator_type())
			: intrusive_list(count, T(), alloc) {}

		template<std::input_iterator InputIt>
		intrusive_list(InputIt first, InputIt last, const allocator_type& alloc = allocator_type())
			: allocator_(alloc)
			, sentinel_(_construct_sentinel())
		{
			_insert_copy_range(this->begin(), first, last);
		}

		intrusive_list(const intrusive_list& other)
			: intrusive_list(other.begin(), other.end(), allocator_traits::select_on_container_copy_construction(other.allocator_))
		{}

		intrusive_list(const intrusive_list& other, const allocator_type& alloc)
			: intrusive_list(other.begin(), other.end(), alloc)
		{}

		intrusive_list(intrusive_list&& other)
			: allocator_(std::exchange(other.allocator_, allocator_type()))
			, sentinel_(std::exchange(other.sentinel_, other._construct_sentinel()))
		{}

		intrusive_list(intrusive_list&& other, const allocator_type& alloc)
			: allocator_(alloc)
			, sentinel_(_construct_sentinel())
		{
			_insert_move_range(this->begin(), other.begin(), other.end());
			other.clear();
		}

		intrusive_list(std::initializer_list<T> init, const allocator_type& allocator = allocator_type())
			: intrusive_list(std::begin(init), std::end(init), allocator)
		{}

		template<std::ranges::range R>
		intrusive_list(std::from_range_t, R&& rg, const allocator_type& allocator = allocator_type())
			: intrusive_list(std::begin(rg), std::end(rg), allocator)
		{}

		intrusive_list& operator=(const intrusive_list& other)
		{
			if (other.sentinel_ == sentinel_)
				return *this;

			this->clear();
			_insert_copy_range(this->begin(), other.begin(), other.end());
			return *this;
		}

		intrusive_list& operator=(intrusive_list&& other) noexcept
		{
			if (other.sentinel_ == sentinel_)
				return *this;

			this->clear();
			std::swap(allocator_, other.allocator_);
			std::swap(sentinel_, other.sentinel_);
			return *this;
		}

		intrusive_list& operator=(std::initializer_list<T> ilist) noexcept
		{
			this->clear();
			_insert_copy_range(this->begin(), std::begin(ilist), std::end(ilist));
			return *this;
		}

		~intrusive_list() noexcept
		{
			_destroy_all();
			node_traits::destroy_sentinel(sentinel_);
			typename std::allocator_traits<allocator_type>::template rebind_alloc<typename node_traits::sentinel> alloc(this->get_allocator());
			alloc.deallocate(sentinel_, 1);
		}

	public:
		void assign(size_type count, const T& value)
		{
			this->clear();
			_insert_splat_value(this->begin(), count, value);
		}

		template<std::input_iterator InputIt>
		void assign(InputIt first, InputIt last)
		{
			this->clear();
			_insert_copy_range(this->begin(), first, last);
		}

		void assign(std::initializer_list<T> ilist)
		{
			this->assign(std::begin(ilist), std::end(ilist));
		}

		template<std::ranges::range R>
		void assign_range(R&& r)
		{
			this->assign(std::begin(r), std::end(r));
		}

		[[nodiscard]] allocator_type get_allocator() const noexcept
		{
			return this->allocator_;
		}

	public:
		[[nodiscard]] value_type& front() noexcept
		{
			assert(begin() != end() && "Element is out of range.");
			return *begin();
		}

		[[nodiscard]] const value_type& front() const noexcept
		{
			assert(begin() != end() && "Element is out of range.");
			return *begin();
		}

		[[nodiscard]] value_type& back() noexcept
		{
			assert(begin() != end() && "Element is out of range.");
			return *(--end());
		}

		[[nodiscard]] const value_type& back() const noexcept
		{
			assert(begin() != end() && "Element is out of range.");
			return *(--end());
		}

	public:
		[[nodiscard]] iterator begin() noexcept
		{
			return iterator(node_traits::next(sentinel_));
		}

		[[nodiscard]] const_iterator begin() const noexcept
		{
			return const_iterator(node_traits::next(sentinel_));
		}

		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return const_iterator(node_traits::next(sentinel_));
		}

		[[nodiscard]] iterator end() noexcept
		{
			return iterator(sentinel_);
		}

		[[nodiscard]] const_iterator end() const noexcept
		{
			return const_iterator(sentinel_);
		}

		[[nodiscard]] const_iterator cend() const noexcept
		{
			return const_iterator(sentinel_);
		}

		[[nodiscard]] reverse_iterator rbegin() noexcept
		{
			return reverse_iterator(this->end());
		}

		[[nodiscard]] const_reverse_iterator rbegin() const noexcept
		{
			return const_reverse_iterator(this->end());
		}

		[[nodiscard]] const_reverse_iterator crbegin() const noexcept
		{
			return const_reverse_iterator(this->cend());
		}

		[[nodiscard]] reverse_iterator rend() noexcept
		{
			return reverse_iterator(this->begin());
		}

		[[nodiscard]] const_reverse_iterator rend() const noexcept
		{
			return const_reverse_iterator(this->begin());
		}

		[[nodiscard]] const_reverse_iterator crend() const noexcept
		{
			return const_reverse_iterator(this->cbegin());
		}

	public:
		[[nodiscard]] bool empty() const noexcept
		{
			return this->begin() == this->end();
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return std::distance(this->begin(), this->end());
		}

		[[nodiscard]] size_type max_size() const noexcept
		{
			return std::numeric_limits<size_type>::max();
		}

	public:
		void clear()
		{
			_destroy_all();
			_sentinel_reset();
		}

		iterator insert(const_iterator pos, const T& value)
		{
			return this->emplace(pos, value);
		}

		iterator insert(const_iterator pos, T&& value)
		{
			return this->emplace(pos, std::move(value));
		}

		iterator insert(const_iterator pos, size_type count, const T& value)
		{
			return _insert_splat_value(pos, count, value);
		}

		template<std::input_iterator InputIt>
		iterator insert(const_iterator pos, InputIt first, InputIt last)
		{
			return _insert_copy_range(pos, first, last);
		}

		iterator insert(const_iterator pos, std::initializer_list<T> ilist)
		{
			return this->insert(pos, std::begin(ilist), std::end(ilist));
		}

		template<std::ranges::range R>
		iterator insert_range(const_iterator pos, R&& rg)
		{
			return _insert_copy_range(pos, std::begin(rg), std::end(rg));
		}

		template<class... Args>
		iterator emplace(const_iterator pos, Args&&... args) requires(std::constructible_from<T, Args...>)
		{
			return _insert_emplace(pos, std::forward<Args>(args)...);
		}

		iterator erase(const_iterator pos)
		{
			_assert_valid_iterator(pos);

			pointer ptr = const_cast<pointer>(pos.node_);

			pointer next = node_traits::next(ptr);
			pointer prev = node_traits::previous(ptr);

			node_traits::next(prev, next);
			node_traits::previous(next, prev);

			std::destroy_at(ptr);
			this->get_allocator().deallocate(ptr, 1);

			return iterator(next);
		}

		iterator erase(const_iterator first, const_iterator last)
		{
			_assert_valid_iterator_range(first, last);

			last = --last;

			pointer first_ptr = const_cast<pointer>(first.node_);
			pointer last_ptr = const_cast<pointer>(last.node_);

			pointer prev = node_traits::previous(first_ptr);
			pointer next = node_traits::next(last_ptr);

			node_traits::next(prev, next);
			node_traits::previous(next, prev);

			this->_destroy_range_inclusive(first_ptr, last_ptr);

			return static_cast<iterator>(node_traits::next(prev));
		}

		void push_back(const T& value)
		{
			this->emplace_back(value);
		}

		void push_back(T&& value)
		{
			this->emplace_back(std::move(value));
		}

		template<class... Args>
		reference emplace_back(Args&&... args) requires(std::constructible_from<T, Args...>)
		{
			return *_insert_emplace(this->end(), std::forward<Args>(args)...);
		}

		template<std::ranges::range R>
		void append_range(R&& rg)
		{
			this->insert_range(this->end(), std::forward<R>(rg));
		}

		void pop_back()
		{
			assert(!empty() && "Element is out of range");
			this->erase(--this->end());
		}

		void push_front(const T& value)
		{
			this->emplace_front(value);
		}

		void push_front(T&& value)
		{
			this->emplace_front(std::move(value));
		}

		template<class... Args>
		reference emplace_front(Args&&... args) requires(std::constructible_from<T, Args...>)
		{
			return *_insert_emplace(this->begin(), std::forward<Args>(args)...);
		}

		template<std::ranges::range R>
		void prepend_range(R&& rg)
		{
			this->insert_range(this->begin(), std::forward<R>(rg));
		}

		void pop_front()
		{
			assert(!empty() && "Element is out of range");
			this->erase(this->begin());
		}

		void resize(size_type count) requires (std::is_default_constructible_v<T> && std::is_copy_constructible_v<T>)
		{
			return this->resize(count, T());
		}

		void resize(size_type count, const value_type& value) requires(std::is_copy_constructible_v<T>)
		{
			size_type c = 0;
			auto it = begin();

			for( auto e = end(); it != e && c < count; ++it)
			{
				c = c + 1;
			}

			if(it == end())
			{
				this->insert(this->end(), count - c, value);
			}
			else
			{
				this->erase(it, this->end());
			}
		}

		void swap(intrusive_list& other) noexcept(std::allocator_traits<Allocator>::is_always_equal::value)
		{
			std::swap(sentinel_, other.sentinel_);

			if constexpr(noexcept(std::allocator_traits<Allocator>::is_always_equal::value) == false)
			{
				std::swap(allocator_, other.allocator_);
			}
		}

	public:
		void merge(intrusive_list& other)
		{
			this->merge(other, std::less<value_type>{});
		}

		void merge(intrusive_list&& other)
		{
			this->merge(static_cast<intrusive_list&>(other));
		}

		template<class Compare>
		void merge(intrusive_list& other, Compare comp)
		{
			if (other.sentinel_ == this->sentinel_)
				return;

			assert(this->allocator_ == other.allocator_ && "Allocators are not equivalent.");

			if(this->empty())
			{
				this->swap(other);
				return;
			}

			if(other.empty())
			{
				return;
			}

			auto this_it = this->begin();
			auto this_end = this->end();

			auto [other_it, other_end] = other._extract_nodes(other.begin(), other.end());
			auto past_end_inclusive = other.end().node_;

			while(this_it != this_end && other_it != past_end_inclusive)
			{
				if(comp(*other_it, *this_it))
				{
					auto old_v = other_it;
					other_it = node_traits::next(other_it);
					_insert_node(this_it, old_v);
				}
				else
				{
					++this_it;
				}
			}

			while(other_it != past_end_inclusive)
			{
				auto old_v = other_it;
				other_it = node_traits::next(other_it);
				_insert_node(this_it, old_v);
			}

			other._sentinel_reset();
		}

		template<class Compare>
		void merge(intrusive_list&& other, Compare comp)
		{
			this->merge(static_cast<intrusive_list&>(other), std::move(comp));
		}

		void splice(const_iterator pos, intrusive_list& other)
		{
			this->splice(pos, other, other.begin(), other.end());
		}

		void splice(const_iterator pos, intrusive_list&& other)
		{
			this->splice(pos, static_cast<intrusive_list&>(other));
		}

		void splice(const_iterator pos, intrusive_list& other, const_iterator it)
		{
			auto first = it++;
			this->splice(pos, other, first, it);
		}

		void splice(const_iterator pos, intrusive_list&& other, const_iterator it)
		{
			this->splice(pos, static_cast<intrusive_list&>(other), it);
		}

		void splice(const_iterator pos, intrusive_list& other, const_iterator first, const_iterator last)
		{
			if (other.sentinel_ == this->sentinel_)
				return;

			assert(this->allocator_ == other.allocator_ && "Allocators are not equivalent.");

			if (first == last)
				return;

			auto [first_ptr, end_ptr] = other._extract_nodes(
				const_cast<pointer>(first.node_), 
				const_cast<pointer>(last.node_)
			);

			auto ptr = const_cast<T*>(pos.node_);
			auto prev = node_traits::previous(ptr);

			node_traits::next(prev, first_ptr);
			node_traits::previous(first_ptr, prev);

			node_traits::previous(ptr, end_ptr);
			node_traits::next(end_ptr, ptr);
		}

		void splice(const_iterator pos, intrusive_list&& other, const_iterator first, const_iterator last)
		{
			this->splice(pos, static_cast<intrusive_list&>(other), first, last);
		}

		size_type remove(const T& value) requires (std::equality_comparable<T>)
		{
			size_type i{};
			for(auto it = this->begin(), end = this->end(); it != end; )
			{
				if (*it != value)
				{
					++it;
				}
				else
				{
					it = this->erase(it);
					i = i + 1;
				}
			}

			return i;
		}

		template<std::predicate<const T&> UnaryPredicate>
		size_type remove_if(UnaryPredicate p)
		{
			size_type i{};
			for (auto it = this->begin(), end = this->end(); it != end; )
			{
				if (!p(*it))
				{
					++it;
				}
				else
				{
					it = this->erase(it);
					i = i + 1;
				}
			}

			return i;
		}

		void reverse()
		{
			for (auto it = this->begin(), end = this->end(); it != end; )
			{
				auto current = it++;

				auto ptr = current.node_;

				auto next = node_traits::next(ptr);
				node_traits::next(ptr, node_traits::previous(ptr));
				node_traits::previous(ptr, next);
			}

			auto next = node_traits::next(sentinel_);
			node_traits::next(sentinel_, node_traits::previous(sentinel_));
			node_traits::previous(sentinel_, next);
		}

		void sort() 
		{
			this->sort(std::less<value_type>{});
		}

		template<std::predicate<const T&, const T&> Compare>
		void sort(Compare comp)
		{
			const auto e = this->end();
			for(auto i = this->begin(); i != e; ++i)
			{
				for(auto j = ++iterator(i); j != e; ++j)
				{
					if(comp(*j, *i))
					{
						iter_swap(i, j);
						std::swap(i, j);
					}
				}
			}
		}

	public:
		[[nodiscard]] friend bool operator==(const intrusive_list& lhs, const intrusive_list& rhs) noexcept
		{
			return std::ranges::equal(lhs, rhs);
		}

		[[nodiscard]] friend auto operator<=>(const intrusive_list& lhs, const intrusive_list& rhs) noexcept
		{
			return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}

	private:
		std::pair<T*, T*> _extract_nodes(iterator first, iterator last)
		{
			pointer first_ptr = const_cast<pointer>(first.node_);

			pointer last_ptr = const_cast<pointer>(last.node_);
			pointer last_prev = const_cast<pointer>((--last).node_);

			node_traits::next(node_traits::previous(first_ptr), last_ptr);
			node_traits::previous(last_ptr, node_traits::previous(first_ptr));

			return std::make_pair(first_ptr, last_prev);
		}

		T* _insert_node(iterator it, T* node)
		{
			node_traits::next(node, it.node_);
			node_traits::previous(node, node_traits::previous(it.node_));

			node_traits::next(node_traits::previous(it.node_), node);
			node_traits::previous(it.node_, node);

			return node;
		}

		T* _construct_sentinel()
		{
			typename std::allocator_traits<allocator_type>::template rebind_alloc<typename node_traits::sentinel> alloc(this->get_allocator());
			typename node_traits::sentinel* sentinel = alloc.allocate(1);
			sentinel = node_traits::construct_sentinel(sentinel);

			_sentinel_reset(sentinel);
			return reinterpret_cast<T*>(sentinel);
		}

		void _sentinel_reset(pointer sentinel)
		{
			node_traits::next(sentinel, sentinel);
			node_traits::previous(sentinel, sentinel);
		}

		void _sentinel_reset()
		{
			this->_sentinel_reset(sentinel_);
		}

		void _destroy_range_inclusive(pointer first, pointer last)
		{
			assert(first != nullptr);

			if (first == sentinel_ || last == sentinel_)
				return;

			while(true)
			{
				auto next = node_traits::next(first);
				std::destroy_at(first);
				this->get_allocator().deallocate(first, 1);

				if (first == last)
					break;

				first = next;
			} 
		}

		void _assert_valid_iterator([[maybe_unused]] const_iterator it)
		{
			assert(it != end() && "Iterator is out of range.");
		}

		void _assert_valid_iterator_range([[maybe_unused]] const_iterator first, [[maybe_unused]] const_iterator last)
		{
			assert(first != end() && "Iterator is out of range.");
		}

		void _destroy_all() noexcept
		{
			_destroy_range_inclusive(node_traits::next(sentinel_), node_traits::previous(sentinel_));
		}

		iterator _insert_splat_value(const_iterator it, size_t count, const T& value)
		{
			auto after = it;
			auto before = --it;

			pointer previous = const_cast<pointer>(before.node_);

			for(; count != 0; --count)
			{
				auto ptr = this->get_allocator().allocate(1);
				ptr = std::construct_at(ptr, value);

				node_traits::next(previous, ptr);
				node_traits::previous(ptr, previous);
				previous = ptr;
			}

			pointer after_ptr = const_cast<pointer>(after.node_);
			node_traits::previous(after_ptr, previous);
			node_traits::next(previous, after_ptr);

			return iterator(const_cast<pointer>(++before.node_));
		}

		template<class It>
		iterator _insert_copy_range(const_iterator it, It first, It last)
		{
			auto after = it;
			auto before = --it;

			pointer previous = const_cast<pointer>(before.node_);

			for(; first != last; ++first)
			{
				auto ptr = this->get_allocator().allocate(1);
				ptr = std::construct_at(ptr, *first);

				node_traits::next(previous, ptr);
				node_traits::previous(ptr, previous);
				previous = ptr;
			}

			pointer after_ptr = const_cast<pointer>(after.node_);
			node_traits::previous(after_ptr, previous);
			node_traits::next(previous, after_ptr);

			return iterator(const_cast<pointer>(++before.node_));
		}

		template<class It>
		iterator _insert_move_range(const_iterator it, It first, It last)
		{
			auto after = it;
			auto before = --it;

			pointer previous = const_cast<pointer>(before.node_);

			for (; first != last; ++first)
			{
				auto ptr = this->get_allocator().allocate(1);
				ptr = std::construct_at(ptr, std::move(*first));

				node_traits::next(previous, ptr);
				node_traits::previous(ptr, previous);
				previous = ptr;
			}

			pointer after_ptr = const_cast<pointer>(after.node_);
			node_traits::previous(after_ptr, previous);
			node_traits::next(previous, after_ptr);

			return ++before;
		}

		template<class... Args>
		iterator _insert_emplace(const_iterator it, Args&&... args)
		{
			auto ptr = this->get_allocator().allocate(1);
			ptr = std::construct_at(ptr, std::forward<Args>(args)...);
			iterator mutable_it(const_cast<pointer>(it.node_));
			return iterator(_insert_node(mutable_it, ptr));
		}
	};

	template<class T, class Trait, class Allocator, class U>
	typename intrusive_list<T, Trait, Allocator>::size_type erase(intrusive_list<T, Trait, Allocator>& c, const U& value)
	{
		return c.remove_if([&](auto& v) { return v == value; });
	}

	template<class T, class Trait, class Allocator, std::predicate<const T&> UnaryPredicate>
	typename intrusive_list<T, Trait, Allocator>::size_type erase_if(intrusive_list<T, Trait, Allocator>& c, UnaryPredicate pred)
	{
		return c.remove_if(pred);
	}
}
