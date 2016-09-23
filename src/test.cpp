#include "InputReader.h"
#include "Feed.h"
#include "Book.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace std;

using CompositeTopLevel = CompositeBook::CompositeTopLevel;


class BlockingQueueTest : public testing::Test
{
public:
	struct Foo
	{
		Foo() : a(0), b(0) {}
		Foo(int a_, int b_) : a(a_), b(b_) {}
		int a;
		int b;
	};

	virtual void SetUp()
	{

	}

	virtual void TearDown()
	{
		waitForResult();
	}


	void start()
	{
		for(int i=0;i<_producerCount;i++)
			_producers.push_back(thread(&BlockingQueueTest::produce, this));

		_consumer = thread(&BlockingQueueTest::consume, this);

		_queueMonitor = thread(&BlockingQueueTest::monitorQueues, this);
	}

	void waitForResult()
	{
		for(int i=0;i<_producerCount;i++)
		{
			if(_producers[i].joinable())
			{
				_producers[i].join();
			}
		}

		if(_queueMonitor.joinable())
			_queueMonitor.join();

		if(_consumer.joinable())
			_consumer.join();

	}

	int getConsumedElementCount() const
	{
		return _consumedElementCount;
	}

protected:
	void monitorQueues()
	{
		while(true)
		{
			this_thread::sleep_for(chrono::milliseconds(10));
			int c = 0;
			for(const thread& t : _producers)
			{
				if(t.joinable())
				{
					break;
				}
				else
					c++;
			}
			if(c == _producers.size())
				break;
		}
		_queue.requestStop();
	}


	void produce()
	{
		for(int i=0;i<_elemCount;i++)
		{
			_queue.push(Foo(i, i+1));
		}
	}

	void consume()
	{
		while(true)
		{
			Foo val{};
			if(_queue.pop(val))
				_consumedElementCount++;
			else
				break;
		}
	}

protected:
	BlockingQueue<Foo> _queue;
	vector<thread>     _producers;
	thread			   _consumer;
	thread			   _queueMonitor;
	int				   _producerCount{5};
	int				   _elemCount{100000};
	int				   _consumedElementCount{0};

};

TEST_F(BlockingQueueTest, elemCount)
{
	start();
	waitForResult();
	int totalCount = getConsumedElementCount();
	ASSERT_EQ(_producerCount*_elemCount, totalCount);
}


TEST(MockInputReader, data)
{
	vector<string> feed {"09:00:00.007,SPY,205.24,1138,205.25,406", "09:00:00.008,SPY,205.24,1138,205.25,406",
		"09:00:00.009,SPY,205.24,1138,205.25,406"};
	MockInputReader reader{feed};
	ASSERT_EQ(true, reader.isValid());
	ASSERT_EQ(0, reader.numOfEntriesRead());
	ASSERT_EQ(3, reader.size());
	int i = 0;
	char line[256];
	while(reader.readLine(line))
	{
		ASSERT_EQ(feed[i], string(line));
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
	FeedPtr feed_a{FeedPtr(new Feed(mockFeedA, ',', 0))};

	InputReaderPtr mockFeedB{InputReaderPtr(new MockInputReader{inputB})};
	FeedPtr feed_b{FeedPtr(new Feed(mockFeedB, ',', 0))};

	InputReaderPtr mockFeedC{InputReaderPtr(new MockInputReader{inputC})};
	FeedPtr feed_c{FeedPtr(new Feed(mockFeedC, ',', 0))};

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
		ASSERT_EQ(true, prevTime < record->time);
		prevTime = record->time;
	}

	ASSERT_EQ(inputA.size()+inputB.size()+inputC.size(), recordCount);
}


TEST(CompositeBook, noCross)
{
	string symbol{"SPY"};
	CompositeBook cbook{symbol};
	CompositeTopLevel top;
	FeedID idA = 0;
	FeedID idB = 1;
	FeedID idC = 2;

	RecordPtr recPtr = NULL;
	vector<RecordPtr> records;
	for(int i=0;i<18;i++)
	{
	  recPtr = (Record*)RecordMemPool::malloc();
	  records.push_back(recPtr);
	}

	initRecord(records[0],"10:00:00.000", symbol.c_str(), 205.12, 500, 205.13, 200, idA);
	initRecord(records[1],"10:00:00.001", symbol.c_str(), 205.12, 600, 205.14, 200, idB);
	initRecord(records[2],"10:00:00.001", symbol.c_str(), 205.11, 300, 205.14, 200, idC);
	initRecord(records[3],"10:00:00.001", symbol.c_str(), 205.11, 320, 205.14, 200, idB);
	initRecord(records[4],"10:00:00.002", symbol.c_str(), 205.10, 200, 205.13, 200, idA);
	initRecord(records[5],"10:00:00.002", symbol.c_str(), 205.09, 200, 205.13, 200, idA);
	initRecord(records[6],"10:00:00.003", symbol.c_str(), 205.09, 400, 205.13, 200, idC);
	initRecord(records[7],"10:00:00.003", symbol.c_str(), 205.09, 400, 205.14, 1200, idA);
	initRecord(records[8],"10:00:00.004", symbol.c_str(), 205.09, 250, 205.15, 600, idC);
	initRecord(records[9],"10:00:00.005", symbol.c_str(), 205.10, 120, 205.13, 70, idB);
	initRecord(records[10],"10:00:00.005", symbol.c_str(), 205.08, 220, 205.15, 90, idB);
	initRecord(records[11],"10:00:00.005", symbol.c_str(), 205.08, 120, 205.13, 40, idA);
	initRecord(records[12],"10:00:00.005", symbol.c_str(), 205.11, 20, 205.15, 70, idA);
	initRecord(records[13],"10:00:00.005", symbol.c_str(), 205.09, 60, 205.16, 40, idA);
	initRecord(records[14],"10:00:00.005", symbol.c_str(), 205.08, 240, 205.15, 90, idB);
	initRecord(records[15],"10:00:00.004", symbol.c_str(), 205.10, 150, 205.16, 400, idC);
	initRecord(records[16],"10:00:00.004", symbol.c_str(), 205.10, 150, 205.16, 450, idC);
	initRecord(records[17],"10:00:00.005", symbol.c_str(), 205.10, 140, 205.13, 60, idB);


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

	RecordPtr recPtr = NULL;
	vector<RecordPtr> records;
	for(int i=0;i<3;i++)
	{
	  recPtr = (Record*)RecordMemPool::malloc();
	  records.push_back(recPtr);
	}

	initRecord(records[0],"10:00:00.000", symbol.c_str(), 205.09, 60, 205.16, 40, idA);
	initRecord(records[1],"10:00:00.001", symbol.c_str(), 205.10, 140, 205.13, 60, idB);
	initRecord(records[2],"10:00:00.001", symbol.c_str(), 205.10, 150, 205.16, 450, idC);

	cbook.update(records[0]);
	cbook.update(records[1]);
	cbook.update(records[2]);

	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.10, 290), top.Bid());
	ASSERT_EQ(Side(205.13, 60), top.Ask());

	recPtr = (Record*)RecordMemPool::malloc();
	records.push_back(recPtr);
	initRecord(records[3],"10:00:00.002", symbol.c_str(), 205.13, 40, 205.15, 80, idC);
	// let's do some arbitrage

	ASSERT_EQ(true, cbook.update(records[3]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.13, 40), top.Bid());
	ASSERT_EQ(Side(205.13, 60), top.Ask());


	recPtr = (Record*)RecordMemPool::malloc();
	records.push_back(recPtr);
	initRecord(records[4],"10:00:00.003", symbol.c_str(), 205.14, 20, 205.15, 120, idC);

	ASSERT_EQ(true, cbook.update(records[4]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 20), top.Bid());
	ASSERT_EQ(Side(205.13, 60), top.Ask());


	recPtr = (Record*)RecordMemPool::malloc();
	records.push_back(recPtr);
	initRecord(records[5],"10:00:00.003", symbol.c_str(), 205.14, 70, 205.15, 80, idA);


	ASSERT_EQ(true, cbook.update(records[5]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 90), top.Bid());
	ASSERT_EQ(Side(205.13, 60), top.Ask());


	recPtr = (Record*)RecordMemPool::malloc();
	records.push_back(recPtr);
	initRecord(records[6],"10:00:00.003", symbol.c_str(), 205.14, 25, 205.16, 40, idB);
	ASSERT_EQ(true, cbook.update(records[6]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 115), top.Bid());
	ASSERT_EQ(Side(205.15, 200), top.Ask());


	recPtr = (Record*)RecordMemPool::malloc();
	records.push_back(recPtr);
	initRecord(records[7],"10:00:00.004", symbol.c_str(), 205.12, 10, 205.13, 70, idB);
	ASSERT_EQ(true, cbook.update(records[7]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 90), top.Bid());
	ASSERT_EQ(Side(205.13, 70), top.Ask());

	recPtr = (Record*)RecordMemPool::malloc();
	records.push_back(recPtr);
	initRecord(records[8],"10:00:00.004", symbol.c_str(), 205.13, 15, 205.16, 40, idB);

	ASSERT_EQ(true, cbook.update(records[8]));
	top = cbook.getTopBook();
	ASSERT_EQ(Side(205.14, 90), top.Bid());
	ASSERT_EQ(Side(205.15, 200), top.Ask());


}





