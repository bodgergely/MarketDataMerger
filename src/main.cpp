#include <iostream>
#include <cstdio>
#include <vector>
#include <string>

#include <functional>
#include <thread>
#include <queue>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <cassert>

#include <locale>
#include <chrono>

#include "Feed.h"
#include "Book.h"
#include "InputReader.h"
#include "Tokenizer.h"
#include "Logger.h"
#include "Queue.h"


using namespace std;


Logger LOG("/tmp/MarketDataMergerLog");








class Reporter
{
public:
	Reporter() : _topOfBookChangedQueue(std::chrono::milliseconds(1000))
	{
		_consumerThread = std::thread(&Reporter::_processing, this);
	}
	virtual ~Reporter()
	{
		requestStop();
		join();
	}

	void	publish(const CompositeBook::CompositeTopLevel& topOfBook)
	{
		_topOfBookChangedQueue.push(topOfBook);
	}

	void requestStop()
	{
		_topOfBookChangedQueue.requestStop();
	}

	void join()
	{
		if(_consumerThread.joinable())
			_consumerThread.join();
	}


protected:
	virtual void _report(const CompositeBook::CompositeTopLevel& book) = 0;

private:
	void _processing()
	{
		while(true)
		{
			CompositeBook::CompositeTopLevel top;
			if(_topOfBookChangedQueue.pop(top))
			{
				_report(top);
			}
			else
				break;
		}
	}
private:
	BlockingQueue<CompositeBook::CompositeTopLevel> _topOfBookChangedQueue;
	std::thread			  _consumerThread;
	std::mutex			  _mutex;

};

typedef shared_ptr<Reporter> ReporterPtr;


class StandardOutputReporter : public Reporter
{
public:
	StandardOutputReporter() {}
	virtual ~StandardOutputReporter()
	{
		requestStop();
		join();
	}
protected:
	virtual void _report(const CompositeBook::CompositeTopLevel& top)
	{
		cout << top.toString() << endl;
	}
};



class BookGroupProcessor
{
public:
	BookGroupProcessor()
	{
		_processorThread = std::thread(&BookGroupProcessor::_processing, this);
	}
	~BookGroupProcessor()
	{
		join();
	}

	void registerReporter(const ReporterPtr& reporter)
	{
		_reporter = reporter;
	}

	void send(const RecordPtr& rec) {_recordQueue.push(rec);}

	void join()
	{
		if(_processorThread.joinable())
			_processorThread.join();
	}

	const unordered_map<string, BookStatistics>& bookStats() const {return _bookStats;}

private:


	void _processing()
	{
		while(true)
		{
			RecordPtr rec{nullptr};
			if(_recordQueue.pop(rec))
			{
				bool doPublish = false;
				if(rec)
				{
					const string& symbol = rec->Symbol();
					if(_books.find(symbol)==_books.end())
					{
						_books[symbol] = CompositeBookPtr(new CompositeBook(symbol));
					}

					CompositeBookPtr& book = _books[symbol];
					bool topofBookChanged = book->update(*rec);
					if(topofBookChanged)
					{
						CompositeBook::CompositeTopLevel top = book->getTopBook();
						if(_reporter)
							_reporter->publish(top);
					}

				}
				else
				{
					break;
				}
			}
			else
				break;

		}

		_prepareBookStatistics();

	}

	void _prepareBookStatistics()
	{
		for(const auto& p : _books)
		{
			_bookStats[p.first] = p.second->getStatistics();
		}
	}

private:
	unordered_map<string, BookStatistics> _bookStats;
	BlockingQueue<RecordPtr> _recordQueue;
	CompositeBookMap 		 _books;
	std::thread 			 _processorThread;
	ReporterPtr				 _reporter;
};








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
	BlockingQueue<RecordPtr> 							_incomingRecordsQueue;
	vector<BookGroupProcessor> 		 		 			_processorPool;
	thread					 							_multiplexerThread;
	std::hash<std::string>								_hasher;
	ReporterPtr											_reporter;
};


typedef std::shared_ptr<MarketDataConsumer> MarketDataConsumerPtr;



class MainApp
{
public:
	MainApp(vector<string> inputFiles, int processingGroupCount) : _reporter(ReporterPtr(new StandardOutputReporter())),
													_consumer(new MarketDataConsumer(processingGroupCount, _reporter))
	{

		FeedID feedid = 0;
		for(const string& file : inputFiles)
		{
			FeedPtr feed{new Feed(InputReaderPtr(new FileInputReader(file)), feedid)};
			_feed.addFeed(std::move(feed));
			feedid++;
		}

		_feed.registerNewRecordCB(std::bind(&MarketDataConsumer::push, _consumer, placeholders::_1));
		//_feed.registerEndOfDayCB(std::bind(&MarketDataConsumer::feedEnded, _consumer));
	}
	~MainApp() {}
	void start()
	{
		_consumer->start();
		_feed.start();
		//loop until done
		_feed.join();
		_consumer->join();
		_reporter->requestStop();
		_reporter->join();

		// report different statistics
		_reportBookStatistics();


	}

private:
	void _reportBookStatistics()
	{
		cout << "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
		cout << "Reporting book statistics:\n";
		cout << "Latency(microsec) on market update changing the top of the book.\n";
		cout << "Latency(microsec) is measured from first reading the market data entry from one of the feeds until the point we updated the book.\n";
		cout << "\n";
		unordered_map<string, BookStatistics> bookstats = _consumer->getBookStatistics();
		for(auto& p : bookstats)
		{
			p.second.sortLatencies();
			cout << p.second.toString() << endl;
		}
		cout << "\nEnd of Book Statistics\n";
		cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
	}

private:
	ReporterPtr								_reporter;
	FeedManager 							_feed;
	MarketDataConsumerPtr 					_consumer;
};



int main(int argc, char** argv)
{
	vector<string> infiles;
	for(int i=1;i<argc;i++)
	{
		infiles.push_back(argv[i]);
	}

	MainApp app(infiles, 6);
	app.start();

	return 0;
}
