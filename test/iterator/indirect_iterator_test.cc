#include <gtest/gtest.h>
#include <fox/iterator/indirect_iterator.hpp>

#include <memory>
#include <forward_list>
#include <list>
#include <vector>
#include <ranges>

template<class T>
class indirect_iterator_test : public ::testing::Test
{
protected:
	void SetUp() override
	{
		container.clear();
		static_assert(std::is_same_v<typename T::value_type, std::shared_ptr<int>>, "Use std::unique_ptr<int> as value type");
		container.assign(4, nullptr);

		for(auto&& [i, e] : container | std::views::enumerate)
		{
			e = std::make_shared<int>(static_cast<int>(i + 1));
		}
	}

	void TearDown() override
	{
		container.clear();
	}

public:
	T container;

	using container_type = T;
	using iterator = fox::iterator::indirect_iterator<typename container_type::iterator>;
public:
	auto begin_indirect()
	{
		return fox::iterator::make_indirect_iterator(std::begin(container));
	}

	auto end_indirect()
	{
		return fox::iterator::make_indirect_iterator(std::end(container));
	}

	auto begin()
	{
		return std::begin(container);
	}

	auto end()
	{
		return std::end(container);
	}
};


using indirect_iterator_test_types =
::testing::Types<
	// std::forward_list<std::shared_ptr<int>>,
	std::list<std::shared_ptr<int>>,
	std::vector<std::shared_ptr<int>>
>;

TYPED_TEST_SUITE(indirect_iterator_test, indirect_iterator_test_types);

TYPED_TEST(indirect_iterator_test, default_constructor)
{
	using indirect_it_t = fox::iterator::indirect_iterator<typename TestFixture::container_type::iterator>;

	[[maybe_unused]] indirect_it_t it;
}

TYPED_TEST(indirect_iterator_test, init_constructor)
{
	using indirect_it_t = fox::iterator::indirect_iterator<typename TestFixture::container_type::iterator>;

	indirect_it_t it(this->begin());
	EXPECT_EQ(it.base(), this->begin());
}

TYPED_TEST(indirect_iterator_test, copy_constructor)
{
	using indirect_it_t = fox::iterator::indirect_iterator<typename TestFixture::container_type::iterator>;

	indirect_it_t other(this->begin());
	indirect_it_t it(other);
	EXPECT_EQ(it.base(), this->begin());
}

TYPED_TEST(indirect_iterator_test, copy_assignment_operator)
{
	using indirect_it_t = fox::iterator::indirect_iterator<typename TestFixture::container_type::iterator>;

	indirect_it_t other(this->begin());
	indirect_it_t it(this->end());
	it = other;
	EXPECT_EQ(it.base(), this->begin());
}

namespace
{
	struct test_input_iterator
	{
		using difference_type = std::ptrdiff_t;
		using value_type = int;

		int operator*() const { return 0; }

		test_input_iterator& operator++() { return *this; }
		void operator++(int) { ++*this; }
	};

	template<class T>
	concept test_input_iterator_fails = requires { fox::iterator::indirect_iterator<T>(); };
}

TYPED_TEST(indirect_iterator_test, input_iterator)
{
	static_assert(test_input_iterator_fails<test_input_iterator> == false);
}

TYPED_TEST(indirect_iterator_test, forward_iterator_assert)
{
	static_assert(std::forward_iterator<typename TestFixture::container_type::iterator>);
	static_assert(std::forward_iterator<typename TestFixture::iterator>);
}

TYPED_TEST(indirect_iterator_test, forward_iterator_equivalence)
{
	EXPECT_TRUE(this->begin_indirect() != this->end_indirect());
	EXPECT_FALSE(this->begin_indirect() == this->end_indirect());

	EXPECT_TRUE(this->begin_indirect() == this->begin_indirect());
	EXPECT_FALSE(this->begin_indirect() != this->begin_indirect());
}

TYPED_TEST(indirect_iterator_test, forward_iterator_dereference)
{
	EXPECT_EQ(*this->begin_indirect(), 1);
	EXPECT_EQ(*this->begin_indirect() = 1, 1);
}

TYPED_TEST(indirect_iterator_test, forward_iterator_prefix_incrementable)
{
	auto it = this->begin_indirect();
	EXPECT_EQ(*it, 1);
	EXPECT_EQ(std::addressof(++it), std::addressof(it));
	EXPECT_EQ(*it, 2);
}

TYPED_TEST(indirect_iterator_test, forward_iterator_postfix_incrementable)
{
	auto it = this->begin_indirect();
	EXPECT_EQ(*it, 1);
	EXPECT_EQ(*(it++) = 2, 2);
	EXPECT_EQ(*it, 2);
}

TYPED_TEST(indirect_iterator_test, bidirectional_iterator_assert)
{
	static_assert((std::bidirectional_iterator<typename TestFixture::container_type::iterator> ? std::bidirectional_iterator<typename TestFixture::iterator> : true));
}

TYPED_TEST(indirect_iterator_test, bidirectional_iterator_prefix_decrementable)
{
	if constexpr (std::bidirectional_iterator<typename TestFixture::container_type::iterator>)
	{
		auto it = --this->end_indirect();
		EXPECT_EQ(*it, 4);
		EXPECT_EQ(std::addressof(--it), std::addressof(it));
		EXPECT_EQ(*it, 3);
	}
}

TYPED_TEST(indirect_iterator_test, bidirectional_iterator_postfix_decrementable)
{
	if constexpr (std::bidirectional_iterator<typename TestFixture::container_type::iterator>)
	{
		auto it = --this->end_indirect();
		EXPECT_EQ(*it, 4);
		EXPECT_EQ(*(it--) = 3, 3);
		EXPECT_EQ(*it, 3);
	}
}


TYPED_TEST(indirect_iterator_test, random_access_iterator_assert)
{
	static_assert((std::random_access_iterator<typename TestFixture::container_type::iterator> ? std::random_access_iterator<typename TestFixture::iterator> : true));
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_add_assign)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		auto it = this->begin_indirect();
		EXPECT_EQ(*it, 1);
		EXPECT_EQ(std::addressof(it += 1), std::addressof(it));
		EXPECT_EQ(*it, 2);
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_add_lhs)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		auto it = this->begin_indirect();
		EXPECT_EQ(*it, 1);
		EXPECT_EQ(*(it + 1) = 2, 2);
		EXPECT_EQ(*it, 1);
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_add_rhs)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		auto it = this->begin_indirect();
		EXPECT_EQ(*it, 1);
		EXPECT_EQ(*(1 + it) = 2, 2);
		EXPECT_EQ(*it, 1);
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_subtact_assign)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		auto it = --this->end_indirect();
		EXPECT_EQ(*it, 4);
		EXPECT_EQ(std::addressof(it -= 1), std::addressof(it));
		EXPECT_EQ(*it, 3);
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_subtract_lhs)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		auto it = --this->end_indirect();
		EXPECT_EQ(*it, 4);
		EXPECT_EQ(*(it - 1) = 3, 3);
		EXPECT_EQ(*it, 4);
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_iterator_difference)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		EXPECT_EQ(
			(this->end_indirect() - this->begin_indirect()), 
			(this->end() - this->begin())
		);
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_subscript_operator)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		auto it = this->begin_indirect();
		EXPECT_EQ(it[1], 2);
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_compare_less)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		bool lhs = this->begin() < this->end();
		bool rhs = this->begin_indirect() < this->end_indirect();
		EXPECT_EQ(lhs, rhs);
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_compare_less_equals)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		EXPECT_EQ(this->begin() <= this->end(), this->begin_indirect() <= this->end_indirect());
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_compare_greater)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		EXPECT_EQ(this->begin() > this->end(), this->begin_indirect() > this->end_indirect());
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_compare_greater_equal)
{
	if constexpr (std::random_access_iterator<typename TestFixture::container_type::iterator>)
	{
		EXPECT_EQ(this->begin() >= this->end(), this->begin_indirect() >= this->end_indirect());
	}
}

TYPED_TEST(indirect_iterator_test, random_access_iterator_compare_three_way)
{
	if constexpr (std::three_way_comparable<typename TestFixture::container_type::iterator>)
	{
		EXPECT_EQ(this->begin() <=> this->end(), this->begin_indirect() <=> this->end_indirect());
	}
}