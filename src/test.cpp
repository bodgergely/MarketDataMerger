#include "Tokenizer.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace std;

TEST(Tokenizer,tokenize)
{
	Tokenizer tokenizer(',');
	string s{"09:00:00.007,SPY,205.24,1138,205.25,406"};
	vector<string> tokz = tokenizer.tokenize(s);
	ASSERT_EQ(6,tokz.size());
	ASSERT_EQ("09:00:00.007",tokz[0]);
	ASSERT_EQ("SPY",tokz[1]);
	ASSERT_EQ("205.24",tokz[2]);
	ASSERT_EQ("1138",tokz[3]);
	ASSERT_EQ("205.25",tokz[4]);
	ASSERT_EQ("406",tokz[5]);
}
