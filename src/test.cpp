#include "Tokenizer.h"
#include "InputReader.h"
#include "Feed.h"
#include "Book.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace std;

using CompositeTopLevel = CompositeBook::CompositeTopLevel;

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

TEST(ConsolidatedFeed, sortByTimeStamp)
{
	vector<string> inputA {
		"09:00:00.007,SPY,205.24,1138,205.25,406",
		"09:00:00.008,EEM,39.2,49524,39.21,7413",
		"09:00:00.008,SPY,205.24,400,205.25,1306",
		"09:00:00.009,SPY,205.24,400,205.25,1306",
		"09:00:00.010,SPY,205.24,400,205.25,1306",
		"09:00:00.011,SPY,205.24,434,205.25,1306",
		"09:00:00.011,SPY,205.24,436,205.25,1306",
		"09:00:00.012,SPY,205.24,436,205.25,1306"};
	vector<string> inputB {
		"09:00:00.006,SPY,205.24,400,205.25,200",
		"09:00:00.008,EEM,39.2,20040,39.21,12100",
		"09:00:00.008,IWM,117.07,3900,117.09,450",
		"09:00:00.008,SPY,205.24,300,205.25,200",
		"09:00:00.009,SPY,205.24,267,205.25,200",
		"09:00:00.011,SPY,205.24,264,205.25,200"};

	vector<string> inputC{
		"09:00:00.009,SPY,205.24,400,205.25,200",
		"09:00:00.011,EEM,39.2,20040,39.21,12100",
		"09:00:00.011,IWM,117.07,3900,117.09,450",
		"09:00:00.011,SPY,205.24,300,205.25,200",
		"09:00:00.013,SPY,205.24,267,205.25,200",
		"09:00:00.014,SPY,205.24,264,205.25,200"};

	InputReaderPtr mockFeedA{InputReaderPtr(new MockInputReader{inputA})};
	FeedPtr feed_a{FeedPtr(new Feed(mockFeedA, 0))};

	InputReaderPtr mockFeedB{InputReaderPtr(new MockInputReader{inputB})};
	FeedPtr feed_b{FeedPtr(new Feed(mockFeedB, 0))};

	InputReaderPtr mockFeedC{InputReaderPtr(new MockInputReader{inputC})};
	FeedPtr feed_c{FeedPtr(new Feed(mockFeedC, 0))};

	ConsolidatedFeed cfeed;
	cfeed.addFeed(feed_a);
	cfeed.addFeed(feed_b);
	cfeed.addFeed(feed_c);

	RecordPtr record{nullptr};
	TimePoint prevTime("09:00:00.000");
	int recordCount = 0;
	while(record = cfeed.nextRecord())
	{
		recordCount++;
		ASSERT_EQ(true, prevTime < record->Time());
		prevTime = record->Time();
	}

	ASSERT_EQ(inputA.size()+inputB.size()+inputC.size(), recordCount);
}

Tokenizer tokenizer(',');

TEST(CompositeBook, noCross)
{
	string symbol{"SPY"};
	CompositeBook cbook{symbol};
	CompositeTopLevel top;
	FeedID idA = 0;
	FeedID idB = 1;
	FeedID idC = 2;

	vector<Record> records{	Record("10:00:00.000", symbol, 205.12, 500, 205.13, 200, idA),
							Record("10:00:00.001", symbol, 205.12, 600, 205.14, 200, idB),
							Record("10:00:00.001", symbol, 205.11, 300, 205.14, 200, idC),			//should not change top
							Record("10:00:00.001", symbol, 205.11, 320, 205.14, 200, idB),			//should change top as idB pulls out from the best level total qty
							Record("10:00:00.002", symbol, 205.10, 200, 205.13, 200, idA),			// should change top of bid to the above for idB and idC becomes best
							Record("10:00:00.002", symbol, 205.09, 200, 205.13, 200, idA),			// should not change
							Record("10:00:00.003", symbol, 205.09, 400, 205.13, 200, idC),			// should change
							Record("10:00:00.003", symbol, 205.09, 400, 205.14, 1200, idA),
							Record("10:00:00.004", symbol, 205.09, 250, 205.15, 600, idC),
							Record("10:00:00.005", symbol, 205.10, 120, 205.13, 70, idB),
							Record("10:00:00.005", symbol, 205.08, 220, 205.15, 90, idB),
							Record("10:00:00.005", symbol, 205.08, 120, 205.13, 40, idA),
							Record("10:00:00.005", symbol, 205.11, 20, 205.15, 70, idA),
							Record("10:00:00.005", symbol, 205.09, 60, 205.16, 40, idA),
							Record("10:00:00.005", symbol, 205.08, 240, 205.15, 90, idB),
							Record("10:00:00.004", symbol, 205.10, 150, 205.16, 400, idC),
							Record("10:00:00.004", symbol, 205.10, 150, 205.16, 450, idC),
							Record("10:00:00.005", symbol, 205.10, 140, 205.13, 60, idB)
						  };

	ASSERT_EQ(true, cbook.update(records[0]));	// first entry
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.12, 500), top.Bid());
	ASSERT_EQ(Side(205.13, 200), top.Ask());


	ASSERT_EQ(true, cbook.update(records[1]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.12, 1100), top.Bid());
	ASSERT_EQ(Side(205.13, 200),  top.Ask());


	ASSERT_EQ(false, cbook.update(records[2]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.12, 1100), top.Bid());
	ASSERT_EQ(Side(205.13, 200),  top.Ask());

	ASSERT_EQ(true, cbook.update(records[3]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.12, 500), top.Bid());
	ASSERT_EQ(Side(205.13, 200),  top.Ask());

	ASSERT_EQ(true, cbook.update(records[4]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.11, 620), top.Bid());
	ASSERT_EQ(Side(205.13, 200),  top.Ask());

	ASSERT_EQ(false, cbook.update(records[5]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.11, 620), top.Bid());
	ASSERT_EQ(Side(205.13, 200),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[6]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.11, 320), top.Bid());
	ASSERT_EQ(Side(205.13, 400),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[7]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.11, 320), top.Bid());
	ASSERT_EQ(Side(205.13, 200),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[8]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.11, 320), top.Bid());
	ASSERT_EQ(Side(205.14, 1400),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[9]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.10, 120), top.Bid());
	ASSERT_EQ(Side(205.13, 70),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[10]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.09, 650), top.Bid());
	ASSERT_EQ(Side(205.14, 1200),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[11]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.09, 250), top.Bid());
	ASSERT_EQ(Side(205.13, 40),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[12]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.11, 20), top.Bid());
	ASSERT_EQ(Side(205.15, 760),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[13]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.09, 310), top.Bid());
	ASSERT_EQ(Side(205.15, 690),  top.Ask());


	ASSERT_EQ(false, cbook.update(records[14]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.09, 310), top.Bid());
	ASSERT_EQ(Side(205.15, 690),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[15]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.10, 150), top.Bid());
	ASSERT_EQ(Side(205.15, 90),  top.Ask());

	ASSERT_EQ(false, cbook.update(records[16]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.10, 150), top.Bid());
	ASSERT_EQ(Side(205.15, 90),  top.Ask());


	ASSERT_EQ(true, cbook.update(records[17]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.10, 290), top.Bid());
	ASSERT_EQ(Side(205.13, 60),  top.Ask());

}


TEST(CompositeBook, arbitrage)
{
	string symbol{"SPY"};
	CompositeBook cbook{symbol};
	CompositeTopLevel top;
	FeedID idA = 0;
	FeedID idB = 1;
	FeedID idC = 2;

	vector<Record> records{	Record("10:00:00.000", symbol, 205.09, 60, 205.16, 40, idA),
							Record("10:00:00.001", symbol, 205.10, 140, 205.13, 60, idB),
							Record("10:00:00.001", symbol, 205.10, 150, 205.16, 450, idC)
						  };

	cbook.update(records[0]);
	cbook.update(records[1]);
	cbook.update(records[2]);

	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.10, 290), top.Bid());
	ASSERT_EQ(Side(205.13, 60), top.Ask());

	// let's do some arbitrage
	records.push_back(Record("10:00:00.002", symbol, 205.13, 40, 205.15, 80, idC));

	ASSERT_EQ(true, cbook.update(records[3]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.13, 40), top.Bid());
	ASSERT_EQ(Side(205.13, 60), top.Ask());

	records.push_back(Record("10:00:00.003", symbol, 205.14, 20, 205.15, 120, idC));

	ASSERT_EQ(true, cbook.update(records[4]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 20), top.Bid());
	ASSERT_EQ(Side(205.13, 60), top.Ask());

	records.push_back(Record("10:00:00.003", symbol, 205.14, 70, 205.15, 80, idA));

	ASSERT_EQ(true, cbook.update(records[5]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 90), top.Bid());
	ASSERT_EQ(Side(205.13, 60), top.Ask());

	records.push_back(Record("10:00:00.003", symbol, 205.14, 25, 205.16, 40, idB));
	ASSERT_EQ(true, cbook.update(records[6]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 115), top.Bid());
	ASSERT_EQ(Side(205.15, 200), top.Ask());

	records.push_back(Record("10:00:00.004", symbol, 205.12, 10, 205.13, 70, idB));
	ASSERT_EQ(true, cbook.update(records[7]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 90), top.Bid());
	ASSERT_EQ(Side(205.13, 70), top.Ask());

	records.push_back(Record("10:00:00.004", symbol, 205.13, 15, 205.16, 40, idB));
	ASSERT_EQ(true, cbook.update(records[8]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 90), top.Bid());
	ASSERT_EQ(Side(205.15, 200), top.Ask());


}





