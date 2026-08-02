#include <gtest/gtest.h>
#include <algorithm>

#include "fixed_string.h"
#include "fixed_vector.h"

TEST(FixedVector, DefaultConstructor) {
	fixed_vector<int, 10> v;
	EXPECT_TRUE(v.empty());
	EXPECT_TRUE(v.size() == 0);
	EXPECT_TRUE(v.capacity() == 10);
}

TEST(FixedVector, InitializerListConstructor) {
	fixed_vector<int, 10> v = {42, 13, 7};
	EXPECT_FALSE(v.empty());
	EXPECT_TRUE(v.size() == 3);
	EXPECT_EQ(v[0], 42);
	EXPECT_EQ(v[1], 13);
	EXPECT_EQ(v[2], 7);
}

TEST(FixedVector, CopyConstructor) {
	fixed_vector<int, 3> v = {3,2,1};
	fixed_vector<int, 3> v2 = v;
	EXPECT_EQ(v2[0], v[0]);
	EXPECT_EQ(v2[2], v[2]);
	EXPECT_TRUE(v == v2);
}

TEST(FixedVector, CopyFromFixedVectorConstructor) {
	fixed_vector<int, 5> v = {1,2,3,4,5};
	fixed_vector<int, 5> v2 = v;
	EXPECT_EQ(v2[0], 1);
	EXPECT_EQ(v2[4], 5);
	EXPECT_EQ(v2.size(), v.size());

	fixed_vector<int, 6> v3;
	v3 = v;
	EXPECT_EQ(v3.size(), v.size());
	EXPECT_EQ(v3[0], v[0]);
	EXPECT_EQ(v3[4], v[4]);
}

TEST(FixedVector, AssignmentOperator) {
	fixed_vector<int, 3> v = {0,1};
	v = {4,5,6};
	EXPECT_EQ(v[0], 4);
	EXPECT_EQ(v[1], 5);
	EXPECT_EQ(v[2], 6);

	fixed_vector<int, 3> v2 = {7,8,9};
	v = v2;
	EXPECT_EQ(v[0], 7);
	EXPECT_EQ(v[1], 8);
	EXPECT_EQ(v[2], 9);

	fixed_vector<int, 2> v3 = {10,11};
	v = v3;
	EXPECT_EQ(v.size(), v3.size());
	EXPECT_EQ(v[0], v3[0]);
	EXPECT_EQ(v[1], v3[1]);
}

TEST(FixedVector, EqualOperator) {
	fixed_vector<int, 3> v;
	fixed_vector<int, 3> v1 = {1,2,3};
	fixed_vector<int, 3> v2 = {1,2,3};
	fixed_vector<int, 3> v3 = {1,2};
	fixed_vector<int, 3> v4 = {1,2,4};
	fixed_vector<int, 3> v5 = {1,2,4};
	EXPECT_TRUE(v != v1);
	EXPECT_TRUE(v1 == v2);
	EXPECT_TRUE(v1 != v3);
	EXPECT_TRUE(v1 != v4);
	EXPECT_TRUE(v4 == v5);
}

TEST(FixedVector, PushAndPopBack) {
	fixed_vector<int, 3> v;
	v.push_back(4);
	EXPECT_EQ(v.size(), 1);
	v.push_back(5);
	EXPECT_EQ(v.size(), 2);
	v.push_back(6);
	EXPECT_EQ(v.size(), 3);

	EXPECT_EQ(v[0], 4);
	EXPECT_EQ(v[1], 5);
	EXPECT_EQ(v[2], 6);

	v.pop_back();
	EXPECT_EQ(v.size(), 2);
}

TEST(FixedVector, FrontAndBack) {
	fixed_vector<int, 3> v = {4,5,6};
	EXPECT_EQ(v.front(), 4);
	EXPECT_EQ(v.back(), 6);
}

TEST(FixedVector, SortAlgorithm) {
	fixed_vector<int, 5> v = {5,4,3,2,1};
	std::sort(v.begin(), v.end());
	EXPECT_EQ(v[0], 1);
	EXPECT_EQ(v[4], 5);
}

TEST(FixedVector, FixedStringsVector) {
	fixed_vector<fixed_string<10>, 3> v = {"Hello", "World", "Test"};
	EXPECT_EQ(v[0], "Hello");
	EXPECT_EQ(v[1], "World");
	EXPECT_EQ(v[2], "Test");

	fixed_vector<fixed_string<10>, 3> v2;
	v2.push_back("A");
	v2.push_back("B");
	v2.push_back("C");
	EXPECT_EQ(v2[0], "A");
	EXPECT_EQ(v2[1], "B");
	EXPECT_EQ(v2[2], "C");
}
