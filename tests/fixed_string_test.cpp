#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

#include "fixed_string.h"

TEST(FixedString, DefaultConstructor) {
	fixed_string<10> s;
	EXPECT_TRUE(s.empty());
	EXPECT_TRUE(s.size() == 0);
	EXPECT_TRUE(*s.c_str() == '\0');
}

TEST(FixedString, FromCStringConstructor) {
	const char* cstr = "Hello";
	fixed_string<10> s = cstr;

	EXPECT_TRUE(s == cstr);
	EXPECT_FALSE(s.empty());
	EXPECT_TRUE(s.size() == 5);
}

TEST(FixedString, CopyConstructor) {
	fixed_string<6> s = "Hello";
	fixed_string<6> s2 = s;
	EXPECT_TRUE(s2 == "Hello");
}

TEST(FixedString, CopyFromFixedStringConstructor) {
	fixed_string<4> s = "Boo";
	fixed_string<6> s2 = s;
	EXPECT_TRUE(s2 == "Boo");
}

TEST(FixedString, AssignmentOperator) {
	fixed_string<10> s = "Hello";
	s = "World!";
	EXPECT_TRUE(s == "World!");
	EXPECT_TRUE(s.size() == 6);

	s = "FizzBuzz";
	EXPECT_TRUE(s == "FizzBuzz");
	EXPECT_TRUE(s.size() == 8);

	fixed_string<8> s2 = "TestTest";
	s = s2;
	EXPECT_TRUE(s == "TestTes");
}

TEST(FixedString, EqualOperator) {
	fixed_string<10> s = "Hello";
	fixed_string<6> s2 = "Hello";
	fixed_string<6> s3 = "World";
	EXPECT_TRUE(s == s);
	EXPECT_TRUE(s == s2);
	EXPECT_TRUE(s != s3);
	EXPECT_TRUE(s2 != s3);
	EXPECT_FALSE(s == s3);
	EXPECT_FALSE(s2 == s3);
}

TEST(FixedString, ComparisonOperators) {
	fixed_string<3> aa = "aa";
	fixed_string<3> ab = "ab";
	fixed_string<3> ba = "ba";
	fixed_string<5> hell = "hell";
	fixed_string<6> hello = "hello";

	EXPECT_TRUE(aa <= aa);
	EXPECT_TRUE(aa <= ab);
	EXPECT_TRUE(aa <= ba);
	EXPECT_TRUE(aa < ab);
	EXPECT_TRUE(aa < ba);

	EXPECT_TRUE(ab >= aa);
	EXPECT_TRUE(ab > aa);

	EXPECT_TRUE(ba >= aa);
	EXPECT_TRUE(ba > aa);

	EXPECT_TRUE(hell <= hello);
	EXPECT_TRUE(hell < hello);
	EXPECT_TRUE(hello >= hell);
	EXPECT_TRUE(hello > hell);

	std::vector<fixed_string<10>> strings;
	strings.push_back("World");
	strings.push_back("Hello");
	strings.push_back("Test");
	std::sort(strings.begin(), strings.end());
	EXPECT_EQ(strings[0], "Hello");
	EXPECT_EQ(strings[1], "Test");
	EXPECT_EQ(strings[2], "World");
}

TEST(FixedString, Clear) {
	fixed_string<10> s = "Hello";
	s.clear();
	EXPECT_TRUE(s.empty());
}

TEST(FixedString, GetCString) {
	fixed_string<16> s = "Hello World";
	EXPECT_STREQ(s.c_str(), "Hello World");
}

TEST(FixedString, CompareToCString) {
	fixed_string<16> s = "Hello";
	EXPECT_FALSE(s == "Hello world");
	EXPECT_TRUE(s != "Hello world");
}
