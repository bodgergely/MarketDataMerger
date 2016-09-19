#include "Tokenizer.h"
#include "InputReader.h"
#include "Feed.h"
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


TEST(MockInputReader, data)
{
	vector<string> feed {"09:00:00.007,SPY,205.24,1138,205.25,406", "09:00:00.008,SPY,205.24,1138,205.25,406",
		"09:00:00.009,SPY,205.24,1138,205.25,406"};
	MockInputReader reader{feed};
	ASSERT_EQ(true, reader.isValid());
	ASSERT_EQ(0, reader.numOfEntriesRead());
	ASSERT_EQ(3, reader.size());
	string in;
	int i = 0;
	while(reader.readLine(in))
	{
		ASSERT_EQ(feed[i], in);
		i++;
	}
	ASSERT_EQ(false, reader.isValid());
	ASSERT_EQ(3, reader.numOfEntriesRead());
}

TEST(TimePoint, time)
{
	TimePoint tp1("09:00:00.007");
	TimePoint tp2("09:00:00.008");
	TimePoint tp3("09:00:00.008");

	ASSERT_EQ(true, tp1 < tp2);
	ASSERT_EQ(false, tp1 > tp2);
	ASSERT_EQ(false, tp1 == tp2);
	ASSERT_EQ(true, tp1 <= tp2);

	ASSERT_EQ(true, tp3 == tp2);

}

TEST(ConsolidatedFeed, goodSequence)
{
	vector<string> inputA {"09:00:00.007,SPY,205.24,1138,205.25,406",
		"09:00:00.008,EEM,39.2,49524,39.21,7413",
		"09:00:00.008,SPY,205.24,400,205.25,1306",
		"09:00:00.009,SPY,205.24,400,205.25,1306",
		"09:00:00.010,SPY,205.24,400,205.25,1306",
		"09:00:00.011,SPY,205.24,434,205.25,1306",
		"09:00:00.011,SPY,205.24,436,205.25,1306"};
	vector<string> inputB {"09:00:00.006,SPY,205.24,400,205.25,200",
		"09:00:00.008,EEM,39.2,20040,39.21,12100",
		"09:00:00.008,IWM,117.07,3900,117.09,450",
		"09:00:00.008,SPY,205.24,300,205.25,200",
		"09:00:00.009,SPY,205.24,267,205.25,200",
		"09:00:00.011,SPY,205.24,264,205.25,200"};

	InputReaderPtr mockFeedA{InputReaderPtr(new MockInputReader{inputA})};
	FeedPtr feed_a{FeedPtr(new Feed(mockFeedA, 0))};

	InputReaderPtr mockFeedB{InputReaderPtr(new MockInputReader{inputB})};
	FeedPtr feed_b{FeedPtr(new Feed(mockFeedB, 0))};

	ConsolidatedFeed cfeed;
	cfeed.addFeed(feed_a);
	cfeed.addFeed(feed_b);

	RecordPtr record{nullptr};
	TimePoint prevTime("09:00:00.000");
	int recordCount = 0;
	while(record = cfeed.nextRecord())
	{
		recordCount++;
		cout << *record << endl;
		ASSERT_EQ(true, prevTime < record->Time());
		prevTime = record->Time();
	}

	ASSERT_EQ(inputA.size()+inputB.size(), recordCount);
}

TEST(CompositeBook, correctness)
{

}






