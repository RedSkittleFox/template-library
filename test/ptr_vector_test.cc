#include <gtest/gtest.h>
#include <fox/ptr_vector.hpp>

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
}

template<class T>
class ptr_vector_test;

template<class T, class Allocator>
class ptr_vector_test<fox::ptr_vector<T, Allocator>> : public testing::Test
{
public:
	static inline thread_local std::mt19937 random_engine;
	using ptr_vector = fox::ptr_vector<T, Allocator>;
	using initializer_list = std::initializer_list<T>;

	std::vector<tracking_pointer<int>> tracker;

	T to_value(int v)
	{
		if constexpr(std::integral<T>)
		{
			return static_cast<T>(v);
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return std::to_string(v);
		}
		else if constexpr(std::is_same_v<T, tracking_pointer<int>>)
		{
			return tracker.emplace_back(std::make_shared<int>(v));
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
		for(auto& t : tracker) // Test RAII
		{
			EXPECT_EQ(t.ptr.use_count(), 1) << "Use count is " << t.ptr.use_count() << '\n';
		}
	}
};

using ptr_vector_test_types =
::testing::Types<
	fox::ptr_vector<std::int32_t>,
	fox::ptr_vector<std::string>,
	fox::ptr_vector<tracking_pointer<int>>
>;

TYPED_TEST_SUITE(ptr_vector_test, ptr_vector_test_types);

TYPED_TEST(ptr_vector_test, default_constructor)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector v;

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
	EXPECT_EQ(v.capacity(), 0);
}

TYPED_TEST(ptr_vector_test, copy_constructor)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector v{ this->to_value(0) };
	ptr_vector u(v);

	EXPECT_EQ(v, u);
}

TYPED_TEST(ptr_vector_test, move_constructor)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector v{ this->to_value(0) };
	ptr_vector u( std::move(v) );

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 1);

	EXPECT_EQ(u[0], this->to_value(0));
}

TYPED_TEST(ptr_vector_test, splat_constructor)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector v(3, this->to_value(1));

	EXPECT_EQ(v.size(), 3);

	for(std::size_t i = 0; i < 3; ++i)
	{
		EXPECT_EQ(v[i], this->to_value(1));
	}
}

TYPED_TEST(ptr_vector_test, initializer_list_constructor)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector v({ this->to_value(1), this->to_value(2), this->to_value(3) });

	EXPECT_EQ(v.size(), 3);

	for (std::size_t i = 0; i < 3; ++i)
	{
		EXPECT_EQ(v[i], this->to_value(static_cast<int>(i) + 1));
	}
}

TYPED_TEST(ptr_vector_test, iterator_pair_constructor)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	std::vector src{ this->to_value(1), this->to_value(2), this->to_value(3) };
	ptr_vector v(std::begin(src), std::end(src));

	EXPECT_EQ(v.size(), 3);

	for (std::size_t i = 0; i < 3; ++i)
	{
		EXPECT_EQ(v[i], this->to_value(static_cast<int>(i) + 1));
	}
}

TYPED_TEST(ptr_vector_test, range_constructor)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	std::vector src{ this->to_value(1), this->to_value(2), this->to_value(3) };
	ptr_vector v(std::from_range, src);

	EXPECT_EQ(v.size(), 3);

	for (std::size_t i = 0; i < 3; ++i)
	{
		EXPECT_EQ(v[i], this->to_value(static_cast<int>(i) + 1));
	}
}

TYPED_TEST(ptr_vector_test, copy_assign_operator)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector v{ this->to_value(0) };
	ptr_vector u{ this->to_value(1) };
	u = v;

	EXPECT_EQ(v, u);
}

TYPED_TEST(ptr_vector_test, move_assign_operator)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector v{ this->to_value(4), this->to_value(5) };
	ptr_vector u{ this->to_value(1) };
	u = std::move(v);

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 2);

	EXPECT_EQ(u[0], this->to_value(4));
	EXPECT_EQ(u[1], this->to_value(5));
}

TYPED_TEST(ptr_vector_test, initializer_list_assign_operator)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	typename TestFixture::initializer_list v{ this->to_value(4), this->to_value(5) };
	ptr_vector u{ this->to_value(1) };
	u = v;

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 2);

	EXPECT_EQ(u[0], this->to_value(4));
	EXPECT_EQ(u[1], this->to_value(5));
}

TYPED_TEST(ptr_vector_test, assign_splat)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	u.assign(2, this->to_value(4));

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 2);

	EXPECT_EQ(u[0], this->to_value(4));
	EXPECT_EQ(u[1], this->to_value(4));
}

TYPED_TEST(ptr_vector_test, assign_iterator_pair)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	std::vector v{ this->to_value(4), this->to_value(5) };
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	u.assign(std::begin(v), std::end(v));

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 2);

	EXPECT_EQ(u[0], this->to_value(4));
	EXPECT_EQ(u[1], this->to_value(5));
}

TYPED_TEST(ptr_vector_test, assign_initializer_list)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	u.assign({ this->to_value(4), this->to_value(5) });

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 2);

	EXPECT_EQ(u[0], this->to_value(4));
	EXPECT_EQ(u[1], this->to_value(5));
}

TYPED_TEST(ptr_vector_test, assign_range)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	using initializer_list = typename TestFixture::initializer_list;

	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	u.assign_range(initializer_list{ this->to_value(4), this->to_value(5) });

	EXPECT_FALSE(u.empty());
	EXPECT_EQ(u.size(), 2);

	EXPECT_EQ(u[0], this->to_value(4));
	EXPECT_EQ(u[1], this->to_value(5));
}

TYPED_TEST(ptr_vector_test, at)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(2));
	EXPECT_EQ(u.at(2), this->to_value(3));
	EXPECT_THROW((void)u.at(3), std::out_of_range);
}

TYPED_TEST(ptr_vector_test, const_at)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	const ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(2));
	EXPECT_EQ(u.at(2), this->to_value(3));
	EXPECT_THROW((void)u.at(3), std::out_of_range);
}

TYPED_TEST(ptr_vector_test, subscript_operator)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	EXPECT_EQ(u[0], this->to_value(1));
	EXPECT_EQ(u[1], this->to_value(2));
	EXPECT_EQ(u[2], this->to_value(3));
}

TYPED_TEST(ptr_vector_test, const_subscript_operator)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	const ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	EXPECT_EQ(u[0], this->to_value(1));
	EXPECT_EQ(u[1], this->to_value(2));
	EXPECT_EQ(u[2], this->to_value(3));
}

TYPED_TEST(ptr_vector_test, front)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	EXPECT_EQ(u.front(), this->to_value(1));
}

TYPED_TEST(ptr_vector_test, const_front)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	const ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	EXPECT_EQ(u.front(), this->to_value(1));
}

TYPED_TEST(ptr_vector_test, back)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	EXPECT_EQ(u.back(), this->to_value(3));
}

TYPED_TEST(ptr_vector_test, const_back)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	const ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	EXPECT_EQ(u.back(), this->to_value(3));
}

TYPED_TEST(ptr_vector_test, empty)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	const ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	const ptr_vector v;

	EXPECT_FALSE(u.empty());
	EXPECT_TRUE(v.empty());
}

TYPED_TEST(ptr_vector_test, size)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	const ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	const ptr_vector v;

	EXPECT_EQ(u.size(), 3);
	EXPECT_EQ(v.size(), 0);
}

TYPED_TEST(ptr_vector_test, reserve_capacity)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	u.reserve(100);

	EXPECT_EQ(static_cast<const ptr_vector&>(u).capacity(), 100);
}

TYPED_TEST(ptr_vector_test, shrink_to_fit)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	u.reserve(100);

	EXPECT_EQ(static_cast<const ptr_vector&>(u).capacity(), 100);
	u.shrink_to_fit();
	EXPECT_EQ(static_cast<const ptr_vector&>(u).capacity(), 3);
}

TYPED_TEST(ptr_vector_test, clear)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	u.clear();
	EXPECT_EQ(u.size(), 0);
}

TYPED_TEST(ptr_vector_test, insert_const_reference)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	auto v = this->to_value(4);
	auto it = u.insert(std::begin(u) + 1, v);

	EXPECT_EQ(u.size(), 4);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(4));
	EXPECT_EQ(u.at(2), this->to_value(2));
	EXPECT_EQ(u.at(3), this->to_value(3));

	EXPECT_EQ(*it, this->to_value(4));
}

TYPED_TEST(ptr_vector_test, insert_rvalue_reference)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	auto v = this->to_value(4);
	auto it = u.insert(std::begin(u) + 1, std::move(v));

	EXPECT_EQ(u.size(), 4);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(4));
	EXPECT_EQ(u.at(2), this->to_value(2));
	EXPECT_EQ(u.at(3), this->to_value(3));

	EXPECT_EQ(*it, this->to_value(4));
}

TYPED_TEST(ptr_vector_test, insert_splat)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	auto it = u.insert(std::begin(u) + 1, 2, this->to_value(4));

	EXPECT_EQ(u.size(), 5);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(4));
	EXPECT_EQ(u.at(2), this->to_value(4));
	EXPECT_EQ(u.at(3), this->to_value(2));
	EXPECT_EQ(u.at(4), this->to_value(3));

	EXPECT_EQ(*it, this->to_value(4));
}

TYPED_TEST(ptr_vector_test, insert_iterator_pair)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	using initializer_list = typename TestFixture::initializer_list;

	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	initializer_list il{ this->to_value(4), this->to_value(5) };
	auto it = u.insert(std::begin(u) + 1, std::begin(il), std::end(il));

	EXPECT_EQ(u.size(), 5);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(4));
	EXPECT_EQ(u.at(2), this->to_value(5));
	EXPECT_EQ(u.at(3), this->to_value(2));
	EXPECT_EQ(u.at(4), this->to_value(3));

	EXPECT_EQ(*it, this->to_value(4));
}

TYPED_TEST(ptr_vector_test, insert_initializer_list)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	using initializer_list = typename TestFixture::initializer_list;

	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	initializer_list il{ this->to_value(4), this->to_value(5) };
	auto it = u.insert(std::begin(u) + 1, il);

	EXPECT_EQ(u.size(), 5);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(4));
	EXPECT_EQ(u.at(2), this->to_value(5));
	EXPECT_EQ(u.at(3), this->to_value(2));
	EXPECT_EQ(u.at(4), this->to_value(3));

	EXPECT_EQ(*it, this->to_value(4));
}

TYPED_TEST(ptr_vector_test, insert_range)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	using initializer_list = typename TestFixture::initializer_list;

	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	initializer_list il{ this->to_value(4), this->to_value(5) };
	auto it = u.insert_range(std::begin(u) + 1, il);

	EXPECT_EQ(u.size(), 5);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(4));
	EXPECT_EQ(u.at(2), this->to_value(5));
	EXPECT_EQ(u.at(3), this->to_value(2));
	EXPECT_EQ(u.at(4), this->to_value(3));

	EXPECT_EQ(*it, this->to_value(4));
}

TYPED_TEST(ptr_vector_test, emplace)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	auto v = this->to_value(4);
	auto it = u.emplace(std::begin(u) + 1, std::move(v));

	EXPECT_EQ(u.size(), 4);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(4));
	EXPECT_EQ(u.at(2), this->to_value(2));
	EXPECT_EQ(u.at(3), this->to_value(3));

	EXPECT_EQ(*it, this->to_value(4));
}

TYPED_TEST(ptr_vector_test, erase)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	auto it = u.erase(std::begin(u) + 1);

	EXPECT_EQ(u.size(), 2);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(3));
	

	EXPECT_EQ(*it, this->to_value(3));
}

TYPED_TEST(ptr_vector_test, erase_range)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	auto it = u.erase(std::begin(u) + 1, std::end(u));

	EXPECT_EQ(u.size(), 1);
	EXPECT_EQ(u.at(0), this->to_value(1));


	EXPECT_EQ(it, std::end(u));
}

TYPED_TEST(ptr_vector_test, push_back_const_reference)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	auto v = this->to_value(4);
	u.push_back(v);

	EXPECT_EQ(u.size(), 4);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(2));
	EXPECT_EQ(u.at(2), this->to_value(3));
	EXPECT_EQ(u.at(3), this->to_value(4));
}

TYPED_TEST(ptr_vector_test, push_back_rvalue_reference)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	auto v = this->to_value(4);
	u.push_back(std::move(v));

	EXPECT_EQ(u.size(), 4);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(2));
	EXPECT_EQ(u.at(2), this->to_value(3));
	EXPECT_EQ(u.at(3), this->to_value(4));
}

TYPED_TEST(ptr_vector_test, emplace_back)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	auto v = this->to_value(4);
	auto& x = u.emplace_back(std::move(v));

	EXPECT_EQ(x, this->to_value(4));

	EXPECT_EQ(u.size(), 4);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(2));
	EXPECT_EQ(u.at(2), this->to_value(3));
	EXPECT_EQ(u.at(3), this->to_value(4));
}

TYPED_TEST(ptr_vector_test, append_range)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	using initializer_list = typename TestFixture::initializer_list;

	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	initializer_list il{ this->to_value(4), this->to_value(5) };
	u.append_range(il);

	EXPECT_EQ(u.size(), 5);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(2));
	EXPECT_EQ(u.at(2), this->to_value(3));
	EXPECT_EQ(u.at(3), this->to_value(4));
	EXPECT_EQ(u.at(4), this->to_value(5));
}

TYPED_TEST(ptr_vector_test, pop_back)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	u.pop_back();

	EXPECT_EQ(u.size(), 2);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(2));
}

TYPED_TEST(ptr_vector_test, resize_bigger)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	u.resize(5, this->to_value(4));

	EXPECT_EQ(u.size(), 5);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(2));
	EXPECT_EQ(u.at(2), this->to_value(3));
	EXPECT_EQ(u.at(3), this->to_value(4));
	EXPECT_EQ(u.at(4), this->to_value(4));
}

TYPED_TEST(ptr_vector_test, resize_smaller)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	u.resize(1, this->to_value(4));

	EXPECT_EQ(u.size(), 1);
	EXPECT_EQ(u.at(0), this->to_value(1));
}

TYPED_TEST(ptr_vector_test, resize_equal)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	u.resize(3, this->to_value(4));

	EXPECT_EQ(u.size(), 3);
	EXPECT_EQ(u.at(0), this->to_value(1));
	EXPECT_EQ(u.at(1), this->to_value(2));
	EXPECT_EQ(u.at(2), this->to_value(3));
}

TYPED_TEST(ptr_vector_test, resize_zero)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };

	u.resize(0, this->to_value(4));

	EXPECT_EQ(u.size(), 0);
	EXPECT_TRUE(u.empty());
}

TYPED_TEST(ptr_vector_test, swap)
{
	using ptr_vector = typename TestFixture::ptr_vector;
	ptr_vector u{ this->to_value(1), this->to_value(2), this->to_value(3) };
	ptr_vector v{ this->to_value(4) };

	u.swap(v);

	EXPECT_EQ(v.size(), 3);
	EXPECT_EQ(v.at(0), this->to_value(1));
	EXPECT_EQ(v.at(1), this->to_value(2));
	EXPECT_EQ(v.at(2), this->to_value(3));

	EXPECT_EQ(u.size(), 1);
	EXPECT_EQ(u.at(0), this->to_value(4));
}

TYPED_TEST(ptr_vector_test, begin_end)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	std::vector v{ this->to_value(1), this->to_value(2), this->to_value(3) };
	ptr_vector u(std::from_range, v);

	auto j = std::begin(v);
	for(auto i = u.begin(); i != u.end(); ++i, ++j)
	{
		EXPECT_EQ(*i, *j);
	}
}

TYPED_TEST(ptr_vector_test, const_begin_end)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	std::vector v{ this->to_value(1), this->to_value(2), this->to_value(3) };
	const ptr_vector u(std::from_range, v);

	auto j = std::cbegin(v);
	for (auto i = u.begin(); i != u.end(); ++i, ++j)
	{
		EXPECT_EQ(*i, *j);
	}
}

TYPED_TEST(ptr_vector_test, cbegin_cend)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	std::vector v{ this->to_value(1), this->to_value(2), this->to_value(3) };
	const ptr_vector u(std::from_range, v);

	auto j = std::cbegin(v);
	for (auto i = u.cbegin(); i != u.cend(); ++i, ++j)
	{
		EXPECT_EQ(*i, *j);
	}
}

TYPED_TEST(ptr_vector_test, rbegin_rend)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	std::vector v{ this->to_value(1), this->to_value(2), this->to_value(3) };
	ptr_vector u(std::from_range, v);

	auto j = std::rbegin(v);
	for (auto i = u.rbegin(); i != u.rend(); ++i, ++j)
	{
		EXPECT_EQ(*i, *j);
	}
}

TYPED_TEST(ptr_vector_test, const_rbegin_rend)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	std::vector v{ this->to_value(1), this->to_value(2), this->to_value(3) };
	const ptr_vector u(std::from_range, v);

	auto j = std::crbegin(v);
	for (auto i = u.rbegin(); i != u.rend(); ++i, ++j)
	{
		EXPECT_EQ(*i, *j);
	}
}

TYPED_TEST(ptr_vector_test, crbegin_crend)
{
	using ptr_vector = typename TestFixture::ptr_vector;

	std::vector v{ this->to_value(1), this->to_value(2), this->to_value(3) };
	const ptr_vector u(std::from_range, v);

	auto j = std::crbegin(v);
	for (auto i = u.crbegin(); i != u.crend(); ++i, ++j)
	{
		EXPECT_EQ(*i, *j);
	}
}