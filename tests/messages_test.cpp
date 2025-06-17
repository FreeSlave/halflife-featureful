#include <gtest/gtest.h>
#include "message_strings.h"

const char messages[] = R"(
{
	"HELLO": "hello world!",
	"EXAMPLE": "foo bar",
	"OBJECTIVES" : "Your tasks"
}
)";

const char messagesTranslated[] = R"(
{
	"hello": "привет, мир!",
	"objectives": "Ваши задачи"
}
)";

TEST(MessageStrings, Parse)
{
	MessageStrings strings;
	strings.ReadFromContents(messages, "");

	const char* hello = strings.GetText("HELLO");
	ASSERT_TRUE(hello != nullptr);
	EXPECT_STREQ(hello, "hello world!");

	const char* hello2 = strings.GetText("hello");
	EXPECT_EQ(hello, hello2);

	const char* example = strings.GetText("EXAMPLE");
	ASSERT_TRUE(example != nullptr);
	EXPECT_STREQ(example, "foo bar");

	const char* objectives = strings.GetText("OBJECTIVES");
	ASSERT_TRUE(objectives != nullptr);
	EXPECT_STREQ(objectives, "Your tasks");

	strings.ReadFromContents(messagesTranslated, "");
	hello = strings.GetText("HELLO");
	ASSERT_TRUE(hello != nullptr);
	EXPECT_STREQ(hello, "привет, мир!");

	const char* example2 = strings.GetText("EXAMPLE");
	ASSERT_TRUE(example2 != nullptr);
	EXPECT_EQ(example, example2);

	objectives = strings.GetText("OBJECTIVES");
	ASSERT_TRUE(objectives != nullptr);
	EXPECT_STREQ(objectives, "Ваши задачи");
}
