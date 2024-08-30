#include <fox/inplace_free_list.hpp>

#include <gtest/gtest.h>
#include <random>
#include <memory>
#include <algorithm>
#include <map>
#include <vector>

template<class T>
class inplace_free_list_test;

template<class T, std::size_t Capacity>
class inplace_free_list_test<fox::inplace_free_list<T, Capacity>> : public testing::Test
{
public:
	static inline thread_local std::mt19937 random_engine;
	using inplace_free_list = fox::inplace_free_list<T, Capacity>;

	[[nodiscard]] T random_value()
	{
		std::uniform_int_distribution<std::int32_t> dist(-127, 127);

		std::int32_t v = dist(random_engine);

		if constexpr (std::is_same_v<T, std::string>)
		{
			return std::to_string(v);
		}
		else
		{
			return static_cast<T>(v);
		}
	}

	void insert_helper(std::map<const T*, T>& expected, inplace_free_list& actual)
	{
		auto v = random_value();
		auto ptr = actual.emplace(v);

		EXPECT_TRUE(ptr != nullptr);

		EXPECT_FALSE(expected.contains(ptr)) << "Object inserted into same place twice.\n";

		expected[ptr] = v;
	}

	void insert_helper(std::map<std::size_t, T>& expected, inplace_free_list& actual)
	{
		auto v = random_value();
		auto ptr = actual.emplace(v);

		EXPECT_TRUE(ptr != nullptr);

		auto idx = actual.as_index(ptr);
		EXPECT_FALSE(expected.contains(idx)) << "Object inserted into same place twice.\n";

		expected[actual.as_index(ptr)] = v;
	}

	void erase_helper(std::map<const T*, T>& expected, inplace_free_list& actual, const T* ptr)
	{
		EXPECT_TRUE(expected.contains(ptr));
		expected.erase(ptr);
		actual.erase(ptr);
	}

	void erase_helper(std::map<std::size_t, T>& expected, inplace_free_list& actual, const T* ptr)
	{
		auto idx = actual.as_index(ptr);
		EXPECT_TRUE(expected.contains(idx));
		expected.erase(idx);
		actual.erase(ptr);
	}

	void fill_random_diffuse(std::map<const T*, T>& expected, inplace_free_list& actual)
	{
		while (std::size(expected) < actual.capacity())
		{
			insert_helper(expected, actual);
		}

		// Pick random to delete
		auto v = std::vector<std::pair<const T*, T>>(std::begin(expected), std::end(expected));
		std::shuffle(std::begin(v), std::end(v), random_engine);

		for (std::size_t i = {}; i < v.size() / 2; ++i)
		{
			erase_helper(expected, actual, v[i].first);
		}
	}

	void fill_random_diffuse(std::map<std::size_t, T>& expected, inplace_free_list& actual)
	{
		while(std::size(expected) < actual.capacity())
		{
			insert_helper(expected, actual);
		}

		// Pick random to delete
		auto v = std::vector<std::pair<std::size_t, T>>(std::begin(expected), std::end(expected));
		std::shuffle(std::begin(v), std::end(v), random_engine);

		for(std::size_t i = {}; i < v.size() / 2; ++i)
		{
			erase_helper(expected, actual, actual.at(v[i].first));
		}
	}

	void fill_shared_ptr_diffuse(std::map<std::size_t, std::shared_ptr<T>>& expected, fox::inplace_free_list<std::shared_ptr<T>, Capacity>& actual, std::shared_ptr<T> value)
	{
		std::uniform_int_distribution<std::int32_t> erase_dist(0, static_cast<std::int32_t>(actual.capacity()));

		while (std::size(expected) < actual.capacity())
		{
			auto ptr = actual.emplace(value);

			auto idx = actual.as_index(ptr);
			EXPECT_FALSE(expected.contains(idx)) << "Object inserted into same place twice.\n";

			expected[actual.as_index(ptr)] = value;
		}

		// Pick random to delete
		auto v = std::vector<std::pair<std::size_t, std::shared_ptr<T>>>(std::begin(expected), std::end(expected));
		std::shuffle(std::begin(v), std::end(v), random_engine);

		for (std::size_t i = {}; i < v.capacity() / 2; ++i)
		{
			std::size_t idx = v[i].first;
			EXPECT_TRUE(expected.contains(idx));
			expected.erase(idx);
			actual.erase(actual.at(idx));
		}
	}
};

using inplace_free_list_test_types =
::testing::Types<
	fox::inplace_free_list<std::int32_t, 32>,
	fox::inplace_free_list<std::int32_t, 64>,
	fox::inplace_free_list<std::string, 64>
>;

TYPED_TEST_SUITE(inplace_free_list_test, inplace_free_list_test_types);

TYPED_TEST(inplace_free_list_test, default_constructor)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
	EXPECT_TRUE(v.free_mask().all());
}

TYPED_TEST(inplace_free_list_test, copy_constructor)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list from;
	std::map<std::size_t, typename inplace_free_list::value_type> expected;

	TestFixture::fill_random_diffuse(expected, from);

	inplace_free_list to(from);

	EXPECT_EQ(from.size(), from.size());

	for(const auto& e : expected)
	{
		EXPECT_TRUE(to.holds_value_at(e.first));
		EXPECT_EQ(e.second, *to.at(e.first));
	}
}

TYPED_TEST(inplace_free_list_test, move_constructor)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list from;
	std::map<std::size_t, typename inplace_free_list::value_type> expected;

	TestFixture::fill_random_diffuse(expected, from);

	inplace_free_list to(std::move(from));

	EXPECT_TRUE(from.empty());
	EXPECT_EQ(from.size(), 0);
	EXPECT_TRUE(from.free_mask().all());

	for (const auto& e : expected)
	{
		EXPECT_TRUE(to.holds_value_at(e.first));
		EXPECT_EQ(e.second, *to.at(e.first));
	}
}

TYPED_TEST(inplace_free_list_test, copy_assignment_operator)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list from;
	std::map<std::size_t, typename inplace_free_list::value_type> expected;

	TestFixture::fill_random_diffuse(expected, from);

	std::map<std::size_t, typename inplace_free_list::value_type> expected2;
	inplace_free_list to;
	TestFixture::fill_random_diffuse(expected2, to);
	to = from;

	EXPECT_EQ(std::size(expected), from.size());

	auto mask_a = from.free_mask();
	auto mask_b = to.free_mask();

	EXPECT_EQ(mask_a, mask_b);

	for(std::size_t i{}; i < std::size(mask_a); ++i)
	{
		if (mask_a.test(i))
			continue;

		EXPECT_EQ(*from.at(i), *to.at(i));
	}

	for (const auto& e : expected)
	{
		EXPECT_TRUE(to.holds_value_at(e.first));
		EXPECT_EQ(e.second, *to.at(e.first));
	}
}

TYPED_TEST(inplace_free_list_test, move_assignment_operator)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list from;
	std::map<std::size_t, typename inplace_free_list::value_type> expected;

	TestFixture::fill_random_diffuse(expected, from);

	inplace_free_list to;
	std::map<std::size_t, typename inplace_free_list::value_type> expected2;
	TestFixture::fill_random_diffuse(expected2, to);
	to = std::move(from);

	EXPECT_TRUE(from.empty());
	EXPECT_EQ(from.size(), 0);
	EXPECT_TRUE(from.free_mask().all());

	for (const auto& e : expected)
	{
		EXPECT_TRUE(to.holds_value_at(e.first));
		EXPECT_EQ(e.second, *to.at(e.first));
	}
}

TYPED_TEST(inplace_free_list_test, destructor)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	auto value = this->random_value();
	auto ptr = v.insert(value);
	EXPECT_EQ(*ptr, value);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);
}

TYPED_TEST(inplace_free_list_test, insert_copy_erase)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	auto value = this->random_value();
	auto ptr = v.insert(value);
	EXPECT_EQ(*ptr, value);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);

	v.erase(ptr);

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
	EXPECT_TRUE(v.free_mask().all());
}

TYPED_TEST(inplace_free_list_test, insert_move_erase)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	auto value = this->random_value();
	auto to_move = value;
	auto ptr = v.insert(std::move(to_move));
	EXPECT_EQ(*ptr, value);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);

	v.erase(ptr);

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
	EXPECT_TRUE(v.free_mask().all());
}

TYPED_TEST(inplace_free_list_test, emplace_erase)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	EXPECT_EQ(*ptr, value);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);

	v.erase(ptr);

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
	EXPECT_TRUE(v.free_mask().all());
}

TYPED_TEST(inplace_free_list_test, clear)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	for(std::size_t i = 0; i < 3; ++i)
	{
		auto value = this->random_value();
		auto ptr = v.emplace(value);
		EXPECT_EQ(*ptr, value);

		EXPECT_FALSE(v.empty());
		EXPECT_EQ(v.size(), 1);

		v.clear();

		EXPECT_TRUE(v.empty());
		EXPECT_EQ(v.size(), 0);
		EXPECT_TRUE(v.free_mask().all());
	}
}

TYPED_TEST(inplace_free_list_test, as_index)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	for (std::size_t i = 01; i <= 3; ++i)
	{
		auto value = this->random_value();
		auto ptr = v.emplace(value);
		EXPECT_EQ(*ptr, value);

		auto idx = v.as_index(ptr);
		EXPECT_EQ(*v.at(idx), value);

		EXPECT_FALSE(v.empty());
		EXPECT_EQ(v.size(), i);
	}
}

TYPED_TEST(inplace_free_list_test, at)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	EXPECT_EQ(*ptr, value);

	auto idx = v.as_index(ptr);
	EXPECT_EQ(*v.at(idx), value);

	EXPECT_THROW((void)v.at(v.capacity()), std::out_of_range);
}

TYPED_TEST(inplace_free_list_test, subscript_operator)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	EXPECT_EQ(*ptr, value);

	auto idx = v.as_index(ptr);
	EXPECT_EQ(*(v[idx]), value);
}

TYPED_TEST(inplace_free_list_test, holds_value)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	EXPECT_FALSE(v.holds_value(v.data()));

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	EXPECT_EQ(*ptr, value);

	EXPECT_TRUE(v.holds_value(ptr));
}

TYPED_TEST(inplace_free_list_test, holds_value_at)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	EXPECT_FALSE(v.holds_value_at(0));

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	auto idx = v.as_index(ptr);
	EXPECT_EQ(*ptr, value);

	EXPECT_TRUE(v.holds_value_at(idx));
}

TYPED_TEST(inplace_free_list_test, owns)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	EXPECT_FALSE(v.owns(nullptr));

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	EXPECT_EQ(*ptr, value);

	EXPECT_TRUE(v.owns(ptr));
}

TYPED_TEST(inplace_free_list_test, free_mask)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	auto idx = v.as_index(ptr);

	auto free_mask = v.free_mask();
	for(std::size_t i = 0; i < std::size(free_mask); ++i)
	{
		if(i == idx)
		{
			EXPECT_FALSE(free_mask.test(i));
		}
		else
		{
			EXPECT_TRUE(free_mask.test(i));
		}
	}
}

TYPED_TEST(inplace_free_list_test, emplace_erase_multiple)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;

	inplace_free_list v;
	std::map<std::size_t, typename inplace_free_list::value_type> expected;

	for(std::size_t i = 0; i < 10; ++i)
	{
		TestFixture::fill_random_diffuse(expected, v);

		for (const auto& e : expected)
		{
			EXPECT_TRUE(v.holds_value_at(e.first));
			EXPECT_EQ(e.second, *v.at(e.first));
		}
	}
}


TYPED_TEST(inplace_free_list_test, raii)
{
	using type = typename TestFixture::inplace_free_list::value_type;
	using inplace_free_list = fox::inplace_free_list<std::shared_ptr<type>, TestFixture::inplace_free_list::capacity()>;

	std::shared_ptr<type> u = std::make_shared<type>(TestFixture::random_value());

	{
		inplace_free_list v;
		std::map<std::size_t, typename inplace_free_list::value_type> expected;

		for (std::size_t i = 0; i < 10; ++i)
		{
			TestFixture::fill_shared_ptr_diffuse(expected, v, u);

			for (const auto& e : expected)
			{
				EXPECT_TRUE(v.holds_value_at(e.first));
				EXPECT_EQ(e.second, *v.at(e.first));
			}
		}

		EXPECT_EQ(u.use_count(), v.size() + expected.size() + 1);
	}

	EXPECT_EQ(u.use_count(), 1);
}

TYPED_TEST(inplace_free_list_test, sort)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;
	inplace_free_list v;
	std::map<std::size_t, typename inplace_free_list::value_type> expected;
    TestFixture::fill_random_diffuse(expected, v);
	v.sort();
	EXPECT_TRUE(v.is_sorted());
}

TYPED_TEST(inplace_free_list_test, optimize_at)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;
	inplace_free_list v;
	std::map<std::size_t, typename inplace_free_list::value_type> expected;
	TestFixture::fill_random_diffuse(expected, v);
	auto cb = [&](inplace_free_list::offset_type from, inplace_free_list::offset_type to) {
		auto e = std::make_pair(to,(*expected.find(from)).second);
		e.first = to;
		expected.erase(from);
		expected.insert(e);
	};

	v.optimize_at(cb);

	for (auto e : expected) {
		EXPECT_TRUE(v.holds_value_at(e.first));
		EXPECT_EQ(e.second, *v.at(e.first));
	}
}

TYPED_TEST(inplace_free_list_test, optimize)
{
	using inplace_free_list = typename TestFixture::inplace_free_list;
	inplace_free_list v;
	std::map<typename inplace_free_list::value_type const*, typename inplace_free_list::value_type> expected;
	TestFixture::fill_random_diffuse(expected, v);
	auto cb = [&](typename inplace_free_list::value_type * from, typename inplace_free_list::value_type * to) {
		auto e = std::make_pair(to, (*expected.find(from)).second);
		e.first = to;
		expected.erase(from);
		expected.insert(e);
	};

	v.optimize(cb);

	for (auto e : expected) {
		EXPECT_TRUE(v.holds_value(e.first));
		EXPECT_EQ(e.second, *(e.first));
	}
}