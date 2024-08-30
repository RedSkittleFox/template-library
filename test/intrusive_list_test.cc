#include <gtest/gtest.h>
#include <fox/intrusive_list.hpp>

#include <random>
#include <memory>
#include <algorithm>
#include <map>
#include <vector>

namespace
{
	template<class T>
	struct tracking_pointer
	{
		std::shared_ptr<T> ptr;

		[[nodiscard]] friend bool operator==(const tracking_pointer& lhs, const tracking_pointer& rhs)
		{
			return *lhs.ptr.get() == *rhs.ptr.get();
		}

		[[nodiscard]] friend auto operator<=>(const tracking_pointer& lhs, const tracking_pointer& rhs)
		{
			return *lhs.ptr.get() <=> *rhs.ptr.get();
		}
	};

	template<class T>
	[[nodiscard]] bool operator==(const tracking_pointer<T>& lhs, const tracking_pointer<T>& rhs)
	{
		return *lhs.ptr == *rhs.ptr;
	}

	template<class T>
	[[nodiscard]] bool operator!=(const tracking_pointer<T>& lhs, const tracking_pointer<T>& rhs)
	{
		return *lhs.ptr != *rhs.ptr;
	}

	template<class T>
	struct node
	{
		using value_type = T;

		T value;

		node* next = nullptr;
		node* previous = nullptr;

		[[nodiscard]] friend bool operator==(const node& lhs, const node& rhs)
		{
			return lhs.value == rhs.value;
		}

		[[nodiscard]] friend bool operator!=(const node& lhs, const node& rhs)
		{
			return lhs.value != rhs.value;
		}

		[[nodiscard]] friend auto operator<=>(const node& lhs, const node& rhs)
		{
			return lhs.value <=> rhs.value;
		}
	};
}

template<class T>
class intrusive_list_test;

template<class T, class Traits, class Allocator>
class intrusive_list_test<fox::intrusive_list<T, Traits, Allocator>> : public testing::Test
{
public:
	static inline thread_local std::mt19937 random_engine;
	using intrusive_list = fox::intrusive_list<T, Traits, Allocator>;
	using initializer_list = std::initializer_list<T>;

	std::vector<tracking_pointer<int>> tracker;

	T to_value(int v)
	{
		if constexpr (std::integral<typename T::value_type>)
		{
			return static_cast<T>(v);
		}
		else if constexpr (std::is_same_v<typename T::value_type, std::string>)
		{
			return static_cast<T>(std::to_string(v));
		}
		else if constexpr (std::is_same_v<typename T::value_type, tracking_pointer<int>>)
		{
			return static_cast<T>(tracker.emplace_back(std::make_shared<int>(v)));
		}
		else
		{
			return {};
		}
	}

protected:
	void SetUp() override
	{
		tracker.clear();
	}

	void TearDown() override
	{
		for (auto& t : tracker) // Test RAII
		{
			EXPECT_EQ(t.ptr.use_count(), 1) << "Use count is " << t.ptr.use_count() << '\n';
		}
	}
};

using intrusive_list_types =
::testing::Types<
	fox::intrusive_list<node<std::int32_t>>,
	fox::intrusive_list<node<std::string>>,
	fox::intrusive_list<node<tracking_pointer<int>>>
>;

TYPED_TEST_SUITE(intrusive_list_test, intrusive_list_types);

TYPED_TEST(intrusive_list_test, default_constructor)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v;

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
}

TYPED_TEST(intrusive_list_test, splat_value_constructor)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v(3, this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, iterator_pair_constructor)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v(std::begin(u), std::end(u));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, copy_constructor)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	intrusive_list v(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, move_constructor)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	intrusive_list v(std::move(u));

	EXPECT_TRUE(u.empty());
	EXPECT_EQ(u.size(), 0);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, initializer_list_constructor)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, from_range_constructor)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v(std::from_range, u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, copy_assignment_operator)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	intrusive_list v{this->to_value(4) };

	v = u;

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(v.size(), 3);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, move_assignment_operator)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	intrusive_list v{ this->to_value(4) };

	v = std::move(u);

	EXPECT_TRUE(u.empty());
	EXPECT_EQ(u.size(), 0);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, initializer_list_assignment_operator)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v{ this->to_value(4) };
	v = u;

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}


TYPED_TEST(intrusive_list_test, assign_splat)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{this->to_value(1)};
	v.assign(3, this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, assign_iterator_pair)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v{ this->to_value(4) };
	v.assign(std::begin(u), std::end(u));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, assign_initializer_list)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v{ this->to_value(4) };
	v.assign(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, assign_range)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v{ this->to_value(4) };
	v.assign_range(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, front)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	EXPECT_EQ(v.front(), this->to_value(1));
}

TYPED_TEST(intrusive_list_test, front_const)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	const intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	EXPECT_EQ(v.front(), this->to_value(1));
}

TYPED_TEST(intrusive_list_test, back)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	EXPECT_EQ(v.back(), this->to_value(3));
}

TYPED_TEST(intrusive_list_test, back_const)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	const intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	EXPECT_EQ(v.back(), this->to_value(3));
}

TYPED_TEST(intrusive_list_test, begin_end)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v(u);

	EXPECT_TRUE(std::equal(std::begin(u), std::end(u), v.begin(), v.end()));
}

TYPED_TEST(intrusive_list_test, const_begin_end)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	const intrusive_list v(u);

	EXPECT_TRUE(std::equal(std::begin(u), std::end(u), v.begin(), v.end()));
}

TYPED_TEST(intrusive_list_test, cbegin_cend)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	const intrusive_list v(u);

	EXPECT_TRUE(std::equal(std::begin(u), std::end(u), v.cbegin(), v.cend()));
}

TYPED_TEST(intrusive_list_test, rbegin_rend)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	std::vector u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v(std::from_range, u);

	EXPECT_TRUE(std::equal(std::rbegin(u), std::rend(u), v.rbegin(), v.rend()));
}

TYPED_TEST(intrusive_list_test, const_rbegin_rend)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	std::vector u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	const intrusive_list v(std::from_range, u);

	EXPECT_TRUE(std::equal(std::rbegin(u), std::rend(u), v.rbegin(), v.rend()));
}

TYPED_TEST(intrusive_list_test, crbegin_crend)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	std::vector u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	const intrusive_list v(std::from_range, u);

	EXPECT_TRUE(std::equal(std::rbegin(u), std::rend(u), v.crbegin(), v.crend()));
}

TYPED_TEST(intrusive_list_test, empty)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	const intrusive_list v;
	const intrusive_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	EXPECT_TRUE(v.empty());
	EXPECT_FALSE(u.empty());
}

TYPED_TEST(intrusive_list_test, size)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	const intrusive_list v;
	const intrusive_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	EXPECT_EQ(v.size(), 0);
	EXPECT_EQ(u.size(), 3);
}

TYPED_TEST(intrusive_list_test, clear)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	EXPECT_EQ(u.size(), 3);
	u.clear();

	EXPECT_EQ(u.size(), 0);
	EXPECT_TRUE(u.empty());
}

TYPED_TEST(intrusive_list_test, insert_const_reference)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	const auto x = this->to_value(4);
	v.insert(++v.begin(), x);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, insert_rvalue_reference)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.insert(v.begin(), this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, insert_splat)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.insert(++(++v.begin()), 3, this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 6);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, insert_iterator_pair)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(4), this->to_value(5) };
	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(3) };

	v.insert(++v.begin(), std::begin(u), std::end(u));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, insert_initializer_list)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(4), this->to_value(5) };
	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(3) };

	v.insert(v.end(), std::begin(u), std::end(u));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, insert_range)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(4), this->to_value(5) };
	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(3) };

	v.insert_range(v.end(), u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, emplace)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.emplace(v.begin(), this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, erase_first)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.erase(v.begin());

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 2);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, erase_last)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.erase(--v.end());

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 2);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, erase_range_end)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.erase(++v.begin(), v.end());

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, erase_range_begin)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.erase(v.begin(), --v.end());

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, erase_range_begin_end)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.erase(v.begin(), v.end());

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);

	auto it = v.begin();
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, push_back_const_reference)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	const auto x = this->to_value(4);
	v.push_back(x);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, push_back_rvalue)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.push_back(this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, emplace_back)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	auto& x = v.emplace_back(this->to_value(4));
	EXPECT_EQ(x, this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, append_range)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(4), this->to_value(5) };
	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(3) };

	v.append_range(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, pop_back)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.pop_back();

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 2);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, push_front_const_reference)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	const auto x = this->to_value(4);
	v.push_front(x);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, push_front_rvalue)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.push_front(this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, emplace_front)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	auto& x = v.emplace_front(this->to_value(4));
	EXPECT_EQ(x, this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, prepend_range)
{
	using intrusive_list = typename TestFixture::intrusive_list;
	using initializer_list = typename TestFixture::initializer_list;

	initializer_list u{ this->to_value(4), this->to_value(5) };
	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(3) };

	v.prepend_range(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, pop_front)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.pop_front();

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 2);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, resize_bigger)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.resize(5, this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, resize_smaller)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.resize(2, this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 2);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, resize_equal)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.resize(3, this->to_value(4));

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, resize_zero)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2) , this->to_value(3) };

	v.resize(0, this->to_value(4));

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);

	auto it = v.begin();
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, swap)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(1), this->to_value(2) , this->to_value(3) };
	intrusive_list v{ this->to_value(4), this->to_value(5) };

	v.swap(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(it, v.end());

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 2);

	it = u.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, u.end());
}

TYPED_TEST(intrusive_list_test, merge_reference)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(1), this->to_value(3) , this->to_value(5) };
	intrusive_list v{ this->to_value(2), this->to_value(4) };

	v.merge(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, v.end());

	EXPECT_TRUE(u.empty());
	EXPECT_EQ(u.size(), 0);
}

TYPED_TEST(intrusive_list_test, merge_empty_rhs)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u;
	intrusive_list v{ this->to_value(2), this->to_value(4) };

	v.merge(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 2);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(it, v.end());

	EXPECT_TRUE(u.empty());
	EXPECT_EQ(u.size(), 0);
}

TYPED_TEST(intrusive_list_test, merge_empty_lhs)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(2), this->to_value(4) };
	intrusive_list v;

	v.merge(u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 2);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(it, v.end());

	EXPECT_TRUE(u.empty());
	EXPECT_EQ(u.size(), 0);
}

TYPED_TEST(intrusive_list_test, merge_empty_both)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u;
	intrusive_list v;

	v.merge(u);

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);

	EXPECT_TRUE(u.empty());
	EXPECT_EQ(u.size(), 0);
}

TYPED_TEST(intrusive_list_test, merge_rvalue_reference)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(2), this->to_value(4) };

	v.merge(intrusive_list{ this->to_value(1), this->to_value(1) , this->to_value(1) });

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, merge_reference_comparator)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(4), this->to_value(2) };

	intrusive_list u{ this->to_value(5), this->to_value(3) , this->to_value(1) };
	v.merge(u, std::greater<>{});

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(it, v.end());

	EXPECT_TRUE(u.empty());
	EXPECT_EQ(u.size(), 0);
}

TYPED_TEST(intrusive_list_test, merge_rvalue_reference_comparator)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(4), this->to_value(2) };

	v.merge(intrusive_list{ this->to_value(5), this->to_value(5) , this->to_value(1) }, std::greater<>{});

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, splice_reference_whole)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(5), this->to_value(5) , this->to_value(1) };
	intrusive_list v{ this->to_value(4), this->to_value(2) };

	v.splice(v.begin(), u);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(it, v.end());

	EXPECT_TRUE(u.empty());
	EXPECT_EQ(u.size(), 0);
}

TYPED_TEST(intrusive_list_test, splice_rvalue_reference_whole)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(4), this->to_value(2) };

	v.splice(v.begin(), intrusive_list{ this->to_value(5), this->to_value(5) , this->to_value(1) });

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, splice_reference_at)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(5), this->to_value(6) , this->to_value(1) };
	intrusive_list v{ this->to_value(4), this->to_value(2) };

	v.splice(++v.begin(), u, ++u.begin());

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(6));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(it, v.end());

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 2);

	it = u.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(it, u.end());
}

TYPED_TEST(intrusive_list_test, splice_rvalue_reference_at)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(5), this->to_value(6) , this->to_value(1) };
	intrusive_list v{ this->to_value(4), this->to_value(2) };

	v.splice(++v.begin(), std::move(u), ++u.begin());

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(6));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(it, v.end());

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 2);

	it = u.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(it, u.end());
}

TYPED_TEST(intrusive_list_test, splice_reference_range)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(5), this->to_value(6) , this->to_value(1) };
	intrusive_list v{ this->to_value(4), this->to_value(2) };

	v.splice(++v.begin(), u, ++u.begin(), u.end());

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(6));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(it, v.end());

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 1);

	it = u.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, u.end());
}

TYPED_TEST(intrusive_list_test, splice_rvalue_reference_range)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list u{ this->to_value(5), this->to_value(6) , this->to_value(1) };
	intrusive_list v{ this->to_value(4), this->to_value(2) };

	v.splice(++v.begin(), std::move(u), ++u.begin(), u.end());

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(6));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(it, v.end());

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 1);

	it = u.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, u.end());
}

TYPED_TEST(intrusive_list_test, remove)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(5), this->to_value(1), this->to_value(5) };

	auto u = v.remove(this->to_value(5));
	EXPECT_EQ(u, 2);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, remove_if)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(5), this->to_value(1), this->to_value(5) };

	auto u = v.remove_if([this](auto v) { return v == this->to_value(2); });
	EXPECT_EQ(u, 1);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, reverse)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(3), this->to_value(4), this->to_value(5) };

	v.reverse();

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, sort)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(4), this->to_value(2), this->to_value(1), this->to_value(3), this->to_value(5) };

	v.sort();

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, sort_predicate)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(4), this->to_value(2), this->to_value(1), this->to_value(3), this->to_value(5) };

	v.sort(std::greater{});

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 5);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(4));
	EXPECT_EQ(*(it++), this->to_value(3));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, equals)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	const intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(3), this->to_value(4), this->to_value(5) };
	const intrusive_list u{ this->to_value(1), this->to_value(2), this->to_value(3), this->to_value(4), this->to_value(5) };

	EXPECT_TRUE(v == u);
	EXPECT_TRUE(u == v);

	const intrusive_list z{ this->to_value(1), this->to_value(6), this->to_value(3), this->to_value(4), this->to_value(5) };
	EXPECT_TRUE(u != z);
	EXPECT_TRUE(z != u);
}

TYPED_TEST(intrusive_list_test, three_way)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	const intrusive_list v{ this->to_value(4), this->to_value(2), this->to_value(1), this->to_value(3), this->to_value(5) };
	const intrusive_list u{ this->to_value(1), this->to_value(2), this->to_value(3), this->to_value(4), this->to_value(5) };

	EXPECT_EQ(v <=> u, std::lexicographical_compare_three_way(v.begin(), v.end(), u.begin(), u.end()));
}

TYPED_TEST(intrusive_list_test, erase)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(5), this->to_value(1), this->to_value(5) };

	auto u = fox::erase(v, this->to_value(5));
	EXPECT_EQ(u, 2);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 3);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(2));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(it, v.end());
}

TYPED_TEST(intrusive_list_test, erase_if)
{
	using intrusive_list = typename TestFixture::intrusive_list;

	intrusive_list v{ this->to_value(1), this->to_value(2), this->to_value(5), this->to_value(1), this->to_value(5) };

	auto u = fox::erase_if(v, [this](auto v) { return v == this->to_value(2); });
	EXPECT_EQ(u, 1);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 4);

	auto it = v.begin();
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(*(it++), this->to_value(1));
	EXPECT_EQ(*(it++), this->to_value(5));
	EXPECT_EQ(it, v.end());
}