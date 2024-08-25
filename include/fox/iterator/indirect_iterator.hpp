#pragma once

#include <iterator>

namespace fox::iterator
{
	template<std::forward_iterator Iterator>
		requires(std::indirectly_readable<std::iter_value_t<Iterator>>)
	class indirect_iterator
	{
	protected:
		Iterator current;

	public:
		using iterator_type = Iterator;
		using value_type = std::iter_value_t<std::iter_value_t<Iterator>>;
		using difference_type = std::iter_difference_t<Iterator>;
		using pointer = value_type*;
		using reference = value_type&;

		using iterator_category =
			std::conditional_t<
				std::derived_from<typename std::iterator_traits<Iterator>::iterator_category, std::random_access_iterator_tag>,
				std::random_access_iterator_tag,
				typename std::iterator_traits<Iterator>::iterator_category
			>;

	public:
		constexpr indirect_iterator() = default;

		constexpr explicit indirect_iterator(iterator_type x)
			: current(x) {}

		template<class U>
		constexpr indirect_iterator(const indirect_iterator<U>& other)
		requires(std::convertible_to<const U&, Iterator>)
		: current(other.base()) {}

		template<class U>
		constexpr indirect_iterator& operator=(const indirect_iterator<U>& other)
		requires (std::convertible_to<const U&, Iterator> && std::assignable_from<Iterator&, const U&>)
		{
			current = static_cast<indirect_iterator>(other.current);
			return *this;
		}

		[[nodiscard]] constexpr iterator_type base() const noexcept
		{
			return current;
		}

	public: // Input Iterator
		[[nodiscard]] constexpr reference operator*() const 
		{
			return *(*current);
		}

		[[nodiscard]] constexpr pointer operator->() const
			requires (std::is_pointer_v<Iterator> || requires (const Iterator i) { { *(i.operator->()) } -> std::convertible_to<pointer>; })
		{
			return *(current.operator->());
		}

		constexpr indirect_iterator& operator++()
		{
			++current;
			return *this;
		}

		[[nodiscard]] constexpr indirect_iterator operator++(int)
		{
			auto copy = current;
			++(*this);
			return indirect_iterator(copy);
		}

	public: // Bidirectional Iterator
		constexpr indirect_iterator& operator--()
			requires (std::bidirectional_iterator<iterator_type>)
		{
			--current;
			return *this;
		}

		[[nodiscard]] constexpr indirect_iterator operator--(int)
			requires (std::bidirectional_iterator<iterator_type>)
		{
			auto copy = current;
			--(*this);
			return indirect_iterator(copy);
		}

	public: // Random Access Iterator
		[[nodiscard]] constexpr reference operator[](difference_type n) const
			requires (std::random_access_iterator<iterator_type>)
		{
			return *(current.operator[](n));
		}

		[[nodiscard]] constexpr indirect_iterator operator+(difference_type n) const
			requires (std::random_access_iterator<iterator_type>)
		{
			return indirect_iterator(current + n);
		}

		[[nodiscard]] constexpr indirect_iterator operator-(difference_type n) const
			requires (std::random_access_iterator<iterator_type>)
		{
			return indirect_iterator(current - n);
		}

		constexpr indirect_iterator& operator+=(difference_type n)
			requires (std::random_access_iterator<iterator_type>)
		{
			current += n;
			return *this;
		}

		constexpr indirect_iterator& operator-=(difference_type n)
			requires (std::random_access_iterator<iterator_type>)
		{
			current -= n;
			return *this;
		}

		[[nodiscard]] constexpr difference_type operator-(indirect_iterator rhs) const
			requires (std::random_access_iterator<iterator_type>)
		{
			return current - rhs.current;
		}

	public:
		template<class Iterator2>
		[[nodiscard]] friend constexpr bool operator==(const indirect_iterator& lhs, const indirect_iterator<Iterator2>& rhs)
		{
			return lhs.base() == rhs.base();
		}

		template<class Iterator2>
		[[nodiscard]] friend constexpr bool operator!=(const indirect_iterator& lhs, const indirect_iterator<Iterator2>& rhs)
		{
			return lhs.base() != rhs.base();
		}

		template<class Iterator2>
		[[nodiscard]] friend constexpr bool operator<(const indirect_iterator& lhs, const indirect_iterator<Iterator2>& rhs)
			requires (requires { { lhs.base() < rhs.base() } -> std::convertible_to<bool>; })
		{
			return lhs.base() < rhs.base();
		}

		template<class Iterator2>
		[[nodiscard]] friend constexpr bool operator<=(const indirect_iterator& lhs, const indirect_iterator<Iterator2>& rhs)
			requires (requires { { lhs.base() <= rhs.base() } -> std::convertible_to<bool>; })
		{
			return lhs.base() <= rhs.base();
		}

		template<class Iterator2>
		[[nodiscard]] friend constexpr bool operator>(const indirect_iterator& lhs, const indirect_iterator<Iterator2>& rhs)
			requires (requires { { lhs.base() > rhs.base() } -> std::convertible_to<bool>; })
		{
			return lhs.base() > rhs.base();
		}

		template<class Iterator2>
		[[nodiscard]] friend constexpr bool operator>=(const indirect_iterator& lhs, const indirect_iterator<Iterator2>& rhs)
			requires (requires { { lhs.base() >= rhs.base()} -> std::convertible_to<bool>; })
		{
			return lhs.base() >= rhs.base();
		}

		template<class Iterator2>
		[[nodiscard]] friend constexpr std::compare_three_way_result_t<iterator_type, Iterator2>
			operator<=>(const indirect_iterator& lhs, const indirect_iterator<Iterator2>& rhs)
			requires (requires { { lhs.base() <=> rhs.base()} -> std::convertible_to<std::compare_three_way_result_t<iterator_type, Iterator2>>; })
		{
			return lhs.base() <=> rhs.base();
		}

		[[nodiscard]] friend constexpr indirect_iterator operator+(typename indirect_iterator::difference_type n, const indirect_iterator& it)
			requires (std::random_access_iterator<Iterator>)
		{
			return indirect_iterator(n + it.base());
		}
	};

	template<std::forward_iterator Iterator>
	[[nodiscard]] constexpr indirect_iterator<Iterator> make_indirect_iterator(Iterator i)
	{
		return indirect_iterator<Iterator>(i);
	}
}