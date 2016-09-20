#ifndef _MARKETDATACONSUMER_H
#define _MARKETDATACONSUMER_H

#include <vector>
#include "Queue.h"
#include "Reporter.h"
#include "BookGroupProcessor.h"
#include "Book.h"
#include "Record.h"

using namespace std;

class MarketDataConsumer
{
public:
	MarketDataConsumer(int numOfProcessors, const ReporterPtr& reporter) : _numOfProcessors(numOfProcessors),
																	_feedEnded(false),
																	_processorPool(numOfProcessors),
																	_reporter(reporter)
	{
		for(size_t i=0;i<_processorPool.size();i++)
			_processorPool[i].registerReporter(reporter);

	}
	~MarketDataConsumer()
	{
		join();
	}



	void start()
	{
		_multiplexerThread = std::thread(&MarketDataConsumer::multiplexer, this);
	}

	void push(const RecordPtr& rec)
	{
		_incomingRecordsQueue.push(rec);
	}

	void feedEnded(){/*TODO*/}


	void join()
	{
		if(_multiplexerThread.joinable())
			_multiplexerThread.join();
	}

	unordered_map<string, BookStatistics> getBookStatistics()
	{
		unordered_map<string, BookStatistics> stats;
		for(const auto& procpool : _processorPool)
		{
			const unordered_map<string, BookStatistics>& bs = procpool.bookStats();
			for(const auto& p : bs)
			{
				stats.insert(p);
			}
		}

		return std::move(stats);
	}

private:
	void multiplexer()
	{
		while(true)
		{
			RecordPtr record{nullptr};
			if(_incomingRecordsQueue.pop(record))
			{
				if(record)
					multiplex(record);
				else
					break;
			}
			else
				break;
		}
		for(auto& p : _processorPool)
		{
			p.send(nullptr);
		}
		_feedEnded = true;
	}

	inline void multiplex(const RecordPtr& record)
	{
		size_t bucket = hash(record->Symbol(), _numOfProcessors);
		_processorPool[bucket].send(record);
	}

	// we might do different load balancing - especially if we know that certain symbols are very traffic heavy
	inline unsigned int hash(const std::string& symbolName, int bucketCount) const
	{
		return _hasher(symbolName) % bucketCount;
	}



private:
	int													_numOfProcessors;
	bool					 							_feedEnded;
	SpinningQueue<RecordPtr> 							_incomingRecordsQueue;
	vector<BookGroupProcessor> 		 		 			_processorPool;
	thread					 							_multiplexerThread;
	std::hash<std::string>								_hasher;
	ReporterPtr											_reporter;
};


typedef std::shared_ptr<MarketDataConsumer> MarketDataConsumerPtr;

#endif
