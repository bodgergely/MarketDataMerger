#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <queue>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <locale>

#include "InputReader.h"
#include "Tokenizer.h"
#include "Logger.h"
#include "Queue.h"


using namespace std;


Logger LOG("/tmp/MarketDataMergerLog");



class TimePoint
{
public:
	TimePoint() : _valid(false)
	{
	}
	explicit TimePoint(const string& time) : _valid(true)
	{
		sscanf(time.c_str(), formatString,
		    &vec[0],
		    &vec[1],
		    &vec[2],
			&vec[3]);
	}

	inline int hr() const {return vec[0];}
	inline int min() const {return vec[1];}
	inline int sec() const {return vec[2];}
	inline int millisec() const {return vec[3];}

	inline bool isValid() const {return _valid;}

	// hacky but convenient for the time comparison method
	inline const int* repr() const {return vec;}

	string toString() const
	{
		char buff[256] = {0};
		snprintf(buff, sizeof(buff)-1, formatString, hr(), min(), sec(), millisec());
		return buff;
	}


private:
	constexpr static const char* formatString{"%02d:%02d:%02d.%03d"};
	bool _valid;
	int vec[4] = {0};
};


inline bool operator<(const TimePoint& lhs, const TimePoint& rhs)
{
	for(int i=0;i<4;i++)
	{
		if(lhs.repr()[i]<rhs.repr()[i])
			return true;
		else if(lhs.repr()[i]>rhs.repr()[i])
			return false;
	}

	return true;
}

inline bool operator>(const TimePoint& lhs, const TimePoint& rhs)
{
	return !(lhs<rhs);
}




/*
 * time,symbol,bid,bid_size,ask,ask_size*/
class Record
{
public:
	Record(const string& line, const Tokenizer tokenizer)
	{
		LOG("parsing line: " + line);
		_parseLine(line, tokenizer);
	}


	const TimePoint& Time() const {return _time;}
	const string&	 Symbol() const {return _symbol;}
	double 			 Bid() const {return _bid;}
	unsigned int				 BidSize() const {return _bid_size;}
	double 			 Ask() const {return _ask;}
	unsigned int				 AskSize() const {return _ask_size;}

	std::string toString() const
	{
		stringstream ss;
		ss << Time().toString() << "," << Symbol() << "," << Bid() << "," << BidSize() << "," << Ask() << "," << AskSize();
		return ss.str();
	}

	class TimeCompare
	{
	public:
		bool operator()(const std::shared_ptr<Record> a,std::shared_ptr<Record> b)
		{
			if(a->Time() < b->Time())
				return true;
			else
				return false;
		}
	};

	class RecordInvalid : public std::exception
	{
	public:
		RecordInvalid(const std::string& line) : _line(line) {}
		virtual const char* what() const noexcept
		{
			return _line.c_str();
		}
	private:
		const std::string _line;
	};

private:
	// might fail badly if the data structure is not correct
	void _parseLine(const string& line, const Tokenizer& tokenizer)
	{
		bool result = true;
		_sanityCheck(line);
		vector<string> tokenz = tokenizer.tokenize(line);
		_time = TimePoint(tokenz[0]);
		_symbol = tokenz[1];
		_bid = stod(tokenz[2]);
		_bid_size = stoi(tokenz[3]);
		_ask = stod(tokenz[4]);
		_ask_size = stoi(tokenz[5]);
	}

	void _sanityCheck(const string& line)
	{
		if(line.size() == 0 || isspace(line[0]))
			throw RecordInvalid(line);
	}


private:
	TimePoint 	_time;
	string 		_symbol;
	double 		_bid;
	unsigned int 		_bid_size;
	double  	_ask;
	unsigned int		  	_ask_size;
};

using RecordPtr = std::shared_ptr<Record>;




class Side
{
public:
	Side() : _price(0.0), _qty(0) {}
	Side(double price, unsigned int qty) : _price(price), _qty(qty) {}
	virtual ~Side() {}
	double price() const {return _price;}
	unsigned int qty() const {return _qty;}
	TimePoint lastUpdate() const {return _lastUpdate;}
	virtual bool update(const TimePoint& time, double price, unsigned int qty) = 0;
protected:
	TimePoint _lastUpdate;
	double 	  _price;
	unsigned int	   	  _qty;
};

class Bid : public Side
{
public:
	Bid() {}
	Bid(double price, unsigned int qty) : Side(price, qty) {}
	virtual ~Bid() {}
	virtual bool update(const TimePoint& time, double price, unsigned int qty)
	{
		bool updated = false;
		if(price > _price)
		{
			_price = price;
			_qty = qty;
			updated = true;
		}
		else if(price == _price && qty != _qty)
		{
			_qty = qty;
			updated = true;
		}
		return updated;
	}
};

class Ask : public Side
{
public:
	Ask() {}
	Ask(double price, unsigned int qty) : Side(price, qty) {}
	virtual ~Ask() {}
	virtual bool update(const TimePoint& time, double price, unsigned int qty)
	{
		bool updated = false;
		if(price < _price)
		{
			_price = price;
			_qty = qty;
			updated = true;
		}
		else if(price == _price && qty != _qty)
		{
			_qty = qty;
			updated = true;
		}
		return updated;
	}
};



class Book
{
public:
	Book() {}
	Book(const Book& other) : _symbol(other.Symbol()), _lastUpdate(other.LastUpdateTime()), _bid(other.bid()), _ask(other.ask())
	{
	}
	Book(const Record& rec) : _symbol(rec.Symbol()), _lastUpdate(rec.Time()), _bid(rec.Bid(), rec.BidSize()), _ask(rec.Ask(), rec.AskSize())
	{

	}
	Book(const string& symbol, const Bid& bid, const Ask& ask) :
		_symbol(symbol), _bid(bid), _ask(ask) {}
	~Book() {}

	//accessors
	inline const string& Symbol() const {return _symbol;}
	inline const TimePoint& LastUpdateTime() const {return _lastUpdate;}
	inline const Bid& bid() const {return _bid;}
	inline const Ask& ask() const {return _ask;}

	bool update(const Record& record)
	{
		bool bidUpdated = _bid.update(record.Time(), record.Bid(), record.BidSize());
		bool askUpated = _ask.update(record.Time(), record.Ask(), record.AskSize());
		bool updated = bidUpdated || askUpated;
		// update the _lastUpdate member in Book too
		if(updated)
		{
			_lastUpdate = record.Time();
		}
		return updated;
	}

	string toString() const
	{
		stringstream ss;
		ss << LastUpdateTime().toString() << "," << Symbol() << "," << _bid.price() << "," << _bid.qty() << "," << _ask.price() << "," << _ask.qty();
		return ss.str();
	}

private:
	string 	_symbol;
	TimePoint _lastUpdate;
	Bid 	_bid;
	Ask 	_ask;
};

std::ostream& operator<<(std::ostream& os, const Book& book)
{
	const Bid& bid = book.bid();
	const Ask& ask = book.ask();
	os << book.LastUpdateTime().toString() << "," << book.Symbol() << "," << bid.price() << "," << bid.qty() << "," << ask.price() << "," << ask.qty();
}


typedef std::shared_ptr<Book> BookPtr;


typedef unordered_map<string, Book> BookMap;





class Feed
{
public:
	Feed(const InputReaderPtr& input) : _input(input), _cache(nullptr)
	{
	}
	~Feed() {}
	const bool 	 		readNextRecordToCache()
	{
		string line;
		Tokenizer tokenizer(',');
		if(_input->isValid())
		{
			_input->readLine(line);
			//cout << "Line read " << line << endl;
			try
			{
				_cache = RecordPtr(new Record(line, tokenizer));
				return true;
			}
			catch(const Record::RecordInvalid& e)
			{
				LOG("record invalid exception:" + string(e.what()) + "End of line");
				//cout << "record invalid exception:" << string(e.what()) << "End of line" << endl;
				_cache = nullptr;
			}
		}

		return false;


	}
	const RecordPtr&	 cache() const {return _cache;}
	void				 clearCache() { _cache = nullptr; }

	inline bool			 isValid() const {return _input->isValid();}
protected:
	InputReaderPtr 	_input;
	RecordPtr		_cache;
};

typedef std::unique_ptr<Feed> FeedPtr;


class ConsolidatedFeed
{
public:
	void	  addFeed(FeedPtr&& feed)
	{
		_feeds.push_back(std::forward<FeedPtr>(feed));
	}
	RecordPtr nextRecord()
	{
		RecordPtr oldestRecord{nullptr};
		TimePoint oldestTime;
		int		  oldestFeedIndex = 0;
		for(int i=0;i<_feeds.size();i++)
		{
			FeedPtr& feed = _feeds[i];
			if(feed->isValid())
			{
				TimePoint time;
				if(feed->cache()!=nullptr)
				{
					time = feed->cache()->Time();
				}
				else
				{
					if(feed->readNextRecordToCache())
						time = feed->cache()->Time();
				}

				if(!oldestTime.isValid())
				{
					oldestTime = time;
					oldestFeedIndex = i;
				}
				else if(oldestTime>time)
				{
					oldestTime = time;
					oldestFeedIndex = i;
				}
			}
		}

		if(oldestTime.isValid())
		{
			oldestRecord = _feeds[oldestFeedIndex]->cache();
			_feeds[oldestFeedIndex]->clearCache();
		}

		if(oldestRecord)
			cout << oldestRecord->toString() << endl;

		return oldestRecord;

	}
private:
	vector<FeedPtr> _feeds;
};





class FeedManager
{
public:
	using NewRecordCB = std::function<void(const RecordPtr&)>;
	using EndOfDayCB = std::function<void(void)>;
	FeedManager() {}
	~FeedManager() {}

	void addFeed(FeedPtr&& feed)
	{
		_consolidatedFeed.addFeed(std::forward<FeedPtr>(feed));
	}

	void registerNewRecordCB(const NewRecordCB& cb_) {_newRecordCB = cb_;}
	//void registerEndOfDayCB(const EndOfDayCB& cb_) {_endOfDayCB = cb_;}

	// should be called after registering the callbacks
	void start()
	{
		_recordProducerThread = thread(&FeedManager::_process, this);
	}

	void join()
	{
		if(_recordProducerThread.joinable())
			_recordProducerThread.join();
	}


private:
	void _process()
	{
		while(true)
		{
			RecordPtr rec = _consolidatedFeed.nextRecord();
			_newRecordCB(rec);
			if(!rec)
				break;
		}
		LOG("Reached end of all feeds.");
	}


private:
	ConsolidatedFeed				_consolidatedFeed;
	NewRecordCB						_newRecordCB;
	//EndOfDayCB						_endOfDayCB;
	thread							_recordProducerThread;
};



class Reporter
{
public:
	Reporter() : _finished(false)
	{
		_consumerThread = std::thread(&Reporter::_processing, this);
	}
	virtual ~Reporter()
	{
		join();
	}

	void	publish(const BookPtr& topOfBook)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if(!_finished)
		{
			if(!topOfBook)
				_finished = true;
			_topOfBookChangedQueue.push(topOfBook);
		}
	}

	void join()
	{
		if(_consumerThread.joinable())
			_consumerThread.join();
	}


protected:
	virtual void _report(const BookPtr& book) = 0;

private:
	void _processing()
	{
		while(true)
		{
			BookPtr book{nullptr};
			if(_topOfBookChangedQueue.pop(book))
			{
				if(book)
					_report(book);
				else
					break;
			}
			else
				break;
		}
	}
private:
	BlockingQueue<BookPtr> _topOfBookChangedQueue;
	std::thread			  _consumerThread;
	std::mutex			  _mutex;
	bool				  _finished;

};

typedef shared_ptr<Reporter> ReporterPtr;


class StandardOutputReporter : public Reporter
{
protected:
	virtual void _report(const BookPtr& book)
	{
		//cout << book->toString() << endl;
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
		_reporter->publish(nullptr);
		_reporter->join();
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
						_books[symbol] = Book(*rec);
						doPublish = true;
					}
					else if(_books[symbol].update(*rec))
					{
						doPublish = true;
					}

					if(doPublish && _reporter)
					{
						_reporter->publish(BookPtr(new Book(_books[symbol])));
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
	}

private:
	BlockingQueue<RecordPtr> _recordQueue;
	BookMap 				 _books;
	std::thread 			 _processorThread;
	ReporterPtr				 _reporter;
};








class MarketDataConsumer
{
public:
	MarketDataConsumer(int numOfProcessors, const ReporterPtr& reporter) : _numOfProcessors(numOfProcessors),
																	_feedEnded(false),
																	_processorPool(numOfProcessors)
	{
		for(size_t i=0;i<_processorPool.size();i++)
			_processorPool[i].registerReporter(reporter);

	}
	~MarketDataConsumer(){}



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
			p.join();
		}
		_feedEnded = true;
	}

	inline void multiplex(const RecordPtr& record)
	{
		size_t bucket = hash(record->Symbol(), _numOfProcessors);
		_processorPool[bucket].send(record);
	}

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
};


typedef std::shared_ptr<MarketDataConsumer> MarketDataConsumerPtr;



class MainApp
{
public:
	MainApp(vector<string> inputFiles, int processingGroupCount) :
													_consumer(new MarketDataConsumer(processingGroupCount, ReporterPtr(new StandardOutputReporter())))
	{

		for(const string& file : inputFiles)
		{
			FeedPtr feed{new Feed(InputReaderPtr(new FileInputReader(file)))};
			_feed.addFeed(std::move(feed));
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


	}
private:
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
