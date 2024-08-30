#include <fox/free_list.hpp>
#include <gtest/gtest.h>

#include <random>
#include <memory>
#include <algorithm>
#include <map>
#include <vector>

template<class T>
class free_list_test;

template<class T, std::size_t Capacity>
class free_list_test<fox::free_list<T, Capacity>> : public testing::Test
{
public:
	static inline thread_local std::mt19937 random_engine;
	using free_list = fox::free_list<T, Capacity>;

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

	void insert_helper(std::map<std::size_t, T>& expected, free_list& actual)
	{
		auto v = random_value();
		auto ptr = actual.emplace(v);

		auto idx = actual.as_index(ptr);
		EXPECT_FALSE(expected.contains(idx)) << "Object inserted into same place twice.\n";

		expected[actual.as_index(ptr)] = v;
	}

	void insert_helper(std::map<const T*, T>& expected, free_list& actual)
	{
		auto v = random_value();
		auto ptr = actual.emplace(v);

		EXPECT_FALSE(expected.contains(ptr)) << "Object inserted into same place twice.\n";

		expected[ptr] = v;
	}

	void erase_helper(std::map<std::size_t, T>& expected, free_list& actual, const T* ptr)
	{
		auto idx = actual.as_index(ptr);
		EXPECT_TRUE(expected.contains(idx));
		expected.erase(idx);
		actual.erase(ptr);
	}

	void erase_helper(std::map<const T*, T>& expected, free_list& actual, const T* ptr)
	{
		EXPECT_TRUE(expected.contains(ptr));
		expected.erase(ptr);
		actual.erase(ptr);
	}

	void fill_random_diffuse(std::map<std::size_t, T>& expected, free_list& actual)
	{
		while (std::size(expected) < 1000)
		{
			insert_helper(expected, actual);
		}

		// Pick random to delete
		auto v = std::vector<std::pair<std::size_t, T>>(std::begin(expected), std::end(expected));
		std::shuffle(std::begin(v), std::end(v), random_engine);

		for (std::size_t i = {}; i < 1000 / 2; ++i)
		{
			erase_helper(expected, actual, actual[v[i].first]);
		}
	}

	void fill_random_diffuse(std::map<const T*, T>& expected, free_list& actual)
	{
		while (std::size(expected) < 1000)
		{
			insert_helper(expected, actual);
		}

		// Pick random to delete
		auto v = std::vector<std::pair<const T*, T>>(std::begin(expected), std::end(expected));
		std::shuffle(std::begin(v), std::end(v), random_engine);

		for (std::size_t i = {}; i < 1000 / 2; ++i)
		{
			erase_helper(expected, actual, v[i].first);
		}
	}

	void fill_shared_ptr_diffuse(std::map<std::size_t, std::shared_ptr<T>>& expected, fox::free_list<std::shared_ptr<T>, Capacity>& actual, std::shared_ptr<T> value)
	{
		while (std::size(expected) < 1000)
		{
			auto ptr = actual.emplace(value);

			auto idx = actual.as_index(ptr);
			EXPECT_FALSE(expected.contains(idx)) << "Object inserted into same place twice.\n";

			expected[actual.as_index(ptr)] = value;
		}

		// Pick random to delete
		auto v = std::vector<std::pair<std::size_t, std::shared_ptr<T>>>(std::begin(expected), std::end(expected));
		std::shuffle(std::begin(v), std::end(v), random_engine);

		for (std::size_t i = {}; i < 1000 / 2; ++i)
		{
			auto idx = v[i].first;
			EXPECT_TRUE(expected.contains(idx));
			expected.erase(idx);
			actual.erase(actual.at(idx));
		}
	}

	

	bool is_sorted(free_list& list) {
		
	}
};

using free_list_test_types =
::testing::Types<
	fox::free_list<std::int32_t, 32>,
	fox::free_list<std::int32_t, 64>,
	fox::free_list<std::string, 64>
>;

TYPED_TEST_SUITE(free_list_test, free_list_test_types);

TYPED_TEST(free_list_test, default_constructor)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
}

TYPED_TEST(free_list_test, copy_constructor)
{
	using free_list = typename TestFixture::free_list;

	free_list from;
	std::map<std::size_t, typename free_list::value_type> expected;

	TestFixture::fill_random_diffuse(expected, from);

	free_list to(from);

	EXPECT_EQ(std::size(expected), from.size());

	for (const auto& e : expected)
	{
		EXPECT_TRUE(to.holds_value_at(e.first));
		EXPECT_EQ(e.second, *to.at(e.first));
	}
}

TYPED_TEST(free_list_test, move_constructor)
{
	using free_list = typename TestFixture::free_list;

	free_list from;
	std::map<std::size_t, typename free_list::value_type> expected;

	TestFixture::fill_random_diffuse(expected, from);

	free_list to(std::move(from));

	EXPECT_TRUE(from.empty());
	EXPECT_EQ(from.size(), 0);

	for (const auto& e : expected)
	{
		EXPECT_TRUE(to.holds_value_at(e.first));
		EXPECT_EQ(e.second, *to.at(e.first));
	}
}

TYPED_TEST(free_list_test, copy_assignment_operator)
{
	using free_list = typename TestFixture::free_list;

	free_list from;
	std::map<std::size_t, typename free_list::value_type> expected;

	TestFixture::fill_random_diffuse(expected, from);

	free_list to;
	std::map<std::size_t, typename free_list::value_type> expected2;
	TestFixture::fill_random_diffuse(expected2, to);
	to = from;

	EXPECT_EQ(std::size(expected), from.size());

	for (const auto& e : expected)
	{
		EXPECT_TRUE(to.holds_value_at(e.first));
		EXPECT_EQ(e.second, *to.at(e.first));
	}
}

TYPED_TEST(free_list_test, move_assignment_operator)
{
	using free_list = typename TestFixture::free_list;

	free_list from;
	std::map<std::size_t, typename free_list::value_type> expected;

	TestFixture::fill_random_diffuse(expected, from);

	free_list to;
	std::map<std::size_t, typename free_list::value_type> expected2;
	TestFixture::fill_random_diffuse(expected2, to);
	to = std::move(from);

	EXPECT_TRUE(from.empty());
	EXPECT_EQ(from.size(), 0);

	for (const auto& e : expected)
	{
		EXPECT_TRUE(to.holds_value_at(e.first));
		EXPECT_EQ(e.second, *to.at(e.first));
	}
}

TYPED_TEST(free_list_test, destructor)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	auto value = this->random_value();
	auto ptr = v.insert(value);
	EXPECT_EQ(*ptr, value);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);
}

TYPED_TEST(free_list_test, insert_copy_erase)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	auto value = this->random_value();
	auto ptr = v.insert(value);
	EXPECT_EQ(*ptr, value);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);

	v.erase(ptr);

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
}

TYPED_TEST(free_list_test, insert_move_erase)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	auto value = this->random_value();
	auto to_move = value;
	auto ptr = v.insert(std::move(to_move));
	EXPECT_EQ(*ptr, value);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);

	v.erase(ptr);

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
}

TYPED_TEST(free_list_test, emplace_erase)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	EXPECT_EQ(*ptr, value);

	EXPECT_FALSE(v.empty());
	EXPECT_EQ(v.size(), 1);

	v.erase(ptr);

	EXPECT_TRUE(v.empty());
	EXPECT_EQ(v.size(), 0);
}

TYPED_TEST(free_list_test, clear)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	for (std::size_t i = 0; i < 3; ++i)
	{
		auto value = this->random_value();
		auto ptr = v.emplace(value);
		EXPECT_EQ(*ptr, value);

		EXPECT_FALSE(v.empty());
		EXPECT_EQ(v.size(), 1);

		v.clear();

		EXPECT_TRUE(v.empty());
		EXPECT_EQ(v.size(), 0);
	}
}

TYPED_TEST(free_list_test, as_index)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

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

TYPED_TEST(free_list_test, at)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	EXPECT_EQ(*ptr, value);

	auto idx = v.as_index(ptr);
	EXPECT_EQ(*v.at(idx), value);

	EXPECT_THROW((void)v.at(v.capacity()), std::out_of_range);
}

TYPED_TEST(free_list_test, subscript_operator)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	EXPECT_EQ(*ptr, value);

	auto idx = v.as_index(ptr);
	EXPECT_EQ(*(v[idx]), value);
}

TYPED_TEST(free_list_test, holds_value)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	auto value = this->random_value();
	auto ptr = v.emplace(value);

	EXPECT_FALSE(v.holds_value(ptr + 1)); // Should be valid unless capacity is 0
	EXPECT_EQ(*ptr, value);

	EXPECT_TRUE(v.holds_value(ptr));
}

TYPED_TEST(free_list_test, holds_value_at)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	auto idx = v.as_index(ptr);
	EXPECT_EQ(*ptr, value);

	EXPECT_TRUE(v.holds_value_at(idx));
	EXPECT_FALSE(v.holds_value_at(idx + 1));
}

TYPED_TEST(free_list_test, owns)
{
	using free_list = typename TestFixture::free_list;

	free_list v;

	EXPECT_FALSE(v.owns(nullptr));

	auto value = this->random_value();
	auto ptr = v.emplace(value);
	EXPECT_EQ(*ptr, value);

	EXPECT_TRUE(v.owns(ptr));
}

TYPED_TEST(free_list_test, emplace_erase_multiple)
{
	using free_list = typename TestFixture::free_list;

	free_list v;
	std::map<std::size_t, typename free_list::value_type> expected;

	for (std::size_t i = 0; i < 10; ++i)
	{
		TestFixture::fill_random_diffuse(expected, v);

		for (const auto& e : expected)
		{
			EXPECT_TRUE(v.holds_value_at(e.first));
			EXPECT_EQ(e.second, *v.at(e.first));
		}
	}
}

TYPED_TEST(free_list_test, raii)
{
	using type = typename TestFixture::free_list::value_type;
	using free_list = fox::free_list<std::shared_ptr<type>, TestFixture::free_list::chunk_capacity()>;

	std::shared_ptr<type> u = std::make_shared<type>(TestFixture::random_value());

	{
		free_list v;
		std::map<std::size_t, typename free_list::value_type> expected;

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

TYPED_TEST(free_list_test, optimize_at) {
	using type = typename TestFixture::free_list::value_type;
	using free_list = typename TestFixture::free_list;
	free_list v;
	std::map<std::size_t, typename free_list::value_type> expected;
	TestFixture::fill_random_diffuse(expected, v);
	auto cb = [&](free_list::size_type from, free_list::size_type to) {
		auto e = std::make_pair(to, (*expected.find(from)).second);
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

TYPED_TEST(free_list_test, optimize) {
	using type = typename TestFixture::free_list::value_type;
	using free_list = typename TestFixture::free_list;
	free_list v;
	std::map<typename free_list::value_type const *, typename free_list::value_type> expected;
	TestFixture::fill_random_diffuse(expected, v);
	auto cb = [&](free_list::value_type* from, free_list::value_type* to) {
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