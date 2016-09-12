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
#include <chrono>

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



typedef int FeedID;

/*
 * time,symbol,bid,bid_size,ask,ask_size*/
class Record
{
public:
	Record(const string& line, const Tokenizer tokenizer, FeedID feedID, const chrono::high_resolution_clock::time_point& timestamp) : _feedID(feedID), _receivedTime(timestamp)
	{
		LOG("parsing line: " + line);
		_parseLine(line, tokenizer);
	}

	const FeedID&    Feedid() const {return _feedID;}
	const TimePoint& Time() const {return _time;}
	const string&	 Symbol() const {return _symbol;}
	double 			 Bid() const {return _bid;}
	unsigned int     BidSize() const {return _bid_size;}
	double 			 Ask() const {return _ask;}
	unsigned int     AskSize() const {return _ask_size;}

	const chrono::high_resolution_clock::time_point& TimeStamp() const {return _receivedTime;}

	std::string toString() const
	{
		stringstream ss;
		ss << Feedid() << "," << Time().toString() << "," << Symbol() << "," << Bid() << "," << BidSize() << "," << Ask() << "," << AskSize();
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
	FeedID		_feedID;
	TimePoint 	_time;
	string 		_symbol;
	double 		_bid;
	unsigned int 		_bid_size;
	double  	_ask;
	unsigned int		  	_ask_size;
	chrono::high_resolution_clock::time_point _receivedTime;
};

using RecordPtr = std::shared_ptr<Record>;


class Side
{
public:
	Side() : _price(0.0), _qty(0) {}
	Side(double price, unsigned int qty) : _price(price), _qty(qty) {}
	~Side() {}
	double price() const {return _price;}
	unsigned int qty() const {return _qty;}
	void update(double price, unsigned int qty)
	{
		_price = price;
		_qty = qty;
	}
private:
	double 	  			 _price;
	unsigned int	   	  _qty;
};

bool operator==(const Side& lhs, const Side& rhs)
{
	if(lhs.price()==rhs.price() && lhs.qty() == rhs.qty())
		return true;
	else
		return false;
}

bool operator!=(const Side& lhs, const Side& rhs)
{
	if(lhs==rhs)
		return false;
	else
		return true;
}

enum SideEnum
{
	Bid,
	Ask
};



class Book
{
public:
	Book(const string& symbol, const FeedID& feedID) : _symbol(symbol), _feedID(feedID){}
	~Book() {}
	//accessors
	inline const string& Symbol() const {return _symbol;}
	inline const TimePoint& LastUpdateTime() const {return _lastUpdate;}
	inline const Side& bid() const {return _bid;}
	inline const Side& ask() const {return _ask;}

	void update(const Record& record)
	{
		assert(record.Feedid()==_feedID);
		_lastUpdate = record.Time();
		_bid.update(record.Bid(), record.BidSize());
		_ask.update(record.Ask(), record.AskSize());
	}

	string toString() const
	{
		stringstream ss;
		ss << LastUpdateTime().toString() << "," << Symbol() << "," << _bid.price() << "," << _bid.qty() << "," << _ask.price() << "," << _ask.qty();
		return ss.str();
	}

private:
	string 	  _symbol;
	FeedID	  _feedID;
	TimePoint _lastUpdate;
	Side	  _bid;
	Side	  _ask;
};


typedef std::shared_ptr<Book> BookPtr;


class TopLevel
{
public:
	TopLevel() {}
	// accesseros
	const Side& Bid() const {return _totalBid;}
	const Side& Ask() const {return _totalAsk;}

	unsigned int BidCount() const {return _bids.size();}
	unsigned int AskCount() const {return _asks.size();}

	void add(const FeedID& feedid, const Side& side, SideEnum s)
	{
		switch(s)
		{
		case SideEnum::Bid:
			_bids[feedid] = side;
			_removeStaleBids(side.price());
			_calcBid();
			break;
		case SideEnum::Ask:
			_asks[feedid] = side;
			_removeStaleAsks(side.price());
			_calcAsk();
			break;
		}
	}

	void remove(const FeedID& feedid, SideEnum s)
	{
		switch(s)
		{
		case SideEnum::Bid:
			_bids.erase(feedid);
			_calcBid();
			break;
		case SideEnum::Ask:
			_asks.erase(feedid);
			_calcAsk();
			break;
		}
	}


	bool feedInvolved(FeedID feedid, SideEnum s)
	{
		bool res = false;
		switch(s)
		{
		case SideEnum::Bid:
			if(_bids.find(feedid)!=_bids.end())
				res = true;
			break;
		case SideEnum::Ask:
			if(_asks.find(feedid)!=_asks.end())
				res = true;
			break;
		}
		return res;
	}

private:
	// to maintain the invariant
	void _removeStaleBids(double bestPrice)
	{
		vector<FeedID> staleIDs;
		for(auto& p : _bids)
		{
			if(p.second.price() < bestPrice)
				staleIDs.push_back(p.first);
		}
		for(FeedID& id : staleIDs)
			_bids.erase(id);
	}

	void _removeStaleAsks(double bestPrice)
	{
		vector<FeedID> staleIDs;
		for(auto& p : _asks)
		{
			if(p.second.price() > bestPrice)
				staleIDs.push_back(p.first);
		}
		for(FeedID& id : staleIDs)
			_asks.erase(id);
	}



	void _calcBid()
	{
		if(_bids.size()>0)
		{
			unsigned int qty = 0;
			double price = _bids.begin()->second.price();
			for(const auto& p : _bids)
			{
				assert(p.second.price() == price);
				price = p.second.price();
				qty += p.second.qty();
			}
			_totalBid = Side(price, qty);
		}
		else
		{
			_totalBid = Side(0.0, 0);
		}

	}

	void _calcAsk()
	{
		if(_asks.size()>0)
		{
			unsigned int qty = 0;
			double price = _asks.begin()->second.price();
			for(const auto& p : _asks)
			{
				assert(p.second.price() == price);
				price = p.second.price();
				qty += p.second.qty();
			}
			_totalAsk = Side(price, qty);
		}
		else
		{
			_totalAsk = Side(0.0,0);
		}
	}


private:
	Side _totalBid;
	Side _totalAsk;
	unordered_map<FeedID, Side> _bids;
	unordered_map<FeedID, Side> _asks;
};


class BookStatistics
{
public:
	BookStatistics() {}
	BookStatistics(const string& symbol) : _symbol(symbol) {}

	void updateTopLevelChangeLatency(const chrono::high_resolution_clock::time_point& receiveTime)
	{
		increaseUpdateCount();
		unsigned latency = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - receiveTime).count();
		updateLatencyAverage(latency);
	}

	bool trySetBid(double p)
	{
		if(p < _minBid){
			_minBid = p;
			return true;
		}
		else
			return false;
	}

	bool trySetAsk(double p)
	{
		if(p > _maxAsk){
			_maxAsk = p;
			return true;
		}
		else
			return false;
	}


	const string& Symbol() const {return _symbol;}
	double MinBid() const {return _minBid;}
	double MaxAsk() const {return _maxAsk;}
	unsigned int UpdateCount() const {return _updateCount;}
	//microsec
	double		AvgUpdateTopBookLatency() const {return _avgUpdateLatency;}

	void sortLatencies()
	{
		std::sort(_latencies.begin(), _latencies.end(), std::less<unsigned>());
	}

	unsigned MinLatency() const
	{
		if(_latencies.size() > 0)
			return _latencies[0];
		else
			return 0;
	}

	unsigned MaxLatency() const
	{
		if(_latencies.size() > 0)
			return _latencies[_latencies.size()-1];
		else
			return 0;
	}

	unsigned MedianLatency() const
	{
		if(_latencies.size() > 0)
		{
			size_t mi = _latencies.size() / 2;
			return _latencies[mi];
		}
		else
			return 0;
	}

	string toString() const
	{
		stringstream ss;
		// stupid place to do it but I am short on time

		ss << "Symbol " << Symbol() << ",AvgUpdateLatency " << AvgUpdateTopBookLatency() << ",MinLatency " << MinLatency() << ",MaxLatency " << MaxLatency() << ",MedianLatency " << MedianLatency() << ",UpdateCount " << UpdateCount() << ",MinBid " << MinBid() << ",MaxAsk " << MaxAsk();
		return ss.str();
	}

private:

	double updateLatencyAverage(double latency)
	{
		if(_avgUpdateLatency == 0.0)
		{
			_avgUpdateLatency = latency;
		}
		else
		{
			_avgUpdateLatency = _avgUpdateLatency + ((latency - _avgUpdateLatency)/_updateCount);
		}
		_latencies.push_back(latency);
	}

	inline void increaseUpdateCount() {++_updateCount;}

private:
	string		 _symbol;
	double	 	 _minBid{std::numeric_limits<double>::max()};
	double       _maxAsk{std::numeric_limits<double>::min()};
	unsigned int _updateCount{0};
	double	 	 _avgUpdateLatency{0.0};
	unsigned	 _minLatency{std::numeric_limits<unsigned>::max()};
	unsigned	 _maxLatency{0};
	// for median, percentiles
	vector<unsigned> _latencies;
};


class CompositeBook
{
public:
	class CompositeTopLevel
	{
	public:
		CompositeTopLevel() {}
		CompositeTopLevel(const string& symbol, const Side& bid, const Side& ask, const TimePoint& lastUpdate) : _symbol(symbol), _bid(bid), _ask(ask), _lastUpdate(lastUpdate) {}
		inline const string& Symbol() const {return _symbol;}
		inline const Side& Bid() const {return _bid;}
		inline const Side& Ask() const {return _ask;}
		inline const TimePoint& LastUpdate() const {return _lastUpdate;}
		string toString() const
		{
			stringstream ss;
			ss << LastUpdate().toString() << "," << Symbol() << "," << Bid().price() << "," << Bid().qty() << "," << Ask().price() << "," << Ask().qty();
			return ss.str();
		}
	private:
		string _symbol;
		Side _bid;
		Side _ask;
		TimePoint _lastUpdate;
	};

	CompositeBook(const string& symbol) : _symbol(symbol), _statistics(symbol) {}
	~CompositeBook(){}


	// these are supposed to be called on the same thread as the the one ehich updates the book - they arent thread safe
	const TimePoint& LastUpdate() const { return _lastChangeTime; }
	CompositeTopLevel getTopBook() const {return CompositeBook::CompositeTopLevel(_symbol, _topLevel.Bid(), _topLevel.Ask(), _lastChangeTime);}

	BookStatistics getStatistics() const
	{
		//std::lock_guard<std::mutex> lock(_statisticsMutex);
		return _statistics;
	}

	bool update(const Record& record)
	{
		bool topChanged = false;
		Side oldTopBid = _topLevel.Bid();
		Side oldTopAsk = _topLevel.Ask();


		const string& symbol = record.Symbol();
		const FeedID& feedid = record.Feedid();
		if(_bookPerFeed.find(feedid)==_bookPerFeed.end())
		{
			// special case of not having any book yet!
			if(_bookPerFeed.size()==0)
			{
				_topLevel.add(feedid, Side(record.Bid(), record.BidSize()), SideEnum::Bid);
				_topLevel.add(feedid, Side(record.Ask(), record.AskSize()), SideEnum::Ask);
			}

			_bookPerFeed.emplace(feedid, BookPtr(new Book(symbol, feedid)));

		}

		_bookPerFeed[feedid]->update(record);

		_mergeBid(feedid, record);
		_mergeAsk(feedid, record);

		if(_topLevel.BidCount() == 0)
			_tryReplaceBid();
		if(_topLevel.AskCount() == 0)
			_tryReplaceAsk();

		if(_topLevel.Bid()!=oldTopBid || _topLevel.Ask()!=oldTopAsk)
		{
			_lastChangeTime = record.Time();
			topChanged = true;
			updateStats(record);
		}

		return topChanged;
	}



private:
	void updateStats(const Record& record)
	{
		//std::lock_guard<std::mutex> lock(_statisticsMutex);
		_statistics.updateTopLevelChangeLatency(record.TimeStamp());
		_statistics.trySetBid(_topLevel.Bid().price());
		_statistics.trySetAsk(_topLevel.Ask().price());
	}

	void _tryReplaceBid()
	{
		vector<pair<FeedID,Side>> replacements;
		double bestPrice = std::numeric_limits<double>::min();
		for(auto& p : _bookPerFeed)
		{
			FeedID feedid = p.first;
			const Side& bid = p.second->bid();
			if(bid.price() > bestPrice)
				bestPrice = bid.price();
		}
		// second pass
		for(auto& p : _bookPerFeed)
		{
			FeedID feedid = p.first;
			const Side& bid = p.second->bid();
			if(bid.price() == bestPrice)
				replacements.push_back(make_pair(feedid, bid));
		}

		for(auto& p : replacements)
		{
			_topLevel.add(p.first, p.second, SideEnum::Bid);
		}


	}

	void _tryReplaceAsk()
	{
		vector<pair<FeedID,Side>> replacements;
		double bestPrice = std::numeric_limits<double>::max();
		for(auto& p : _bookPerFeed)
		{
			FeedID feedid = p.first;
			const Side& ask = p.second->ask();
			if(ask.price() < bestPrice)
				bestPrice = ask.price();
		}
		// second pass
		for(auto& p : _bookPerFeed)
		{
			FeedID feedid = p.first;
			const Side& ask = p.second->ask();
			if(ask.price() == bestPrice)
				replacements.push_back(make_pair(feedid, ask));
		}

		for(auto& p : replacements)
		{
			_topLevel.add(p.first, p.second, SideEnum::Ask);
		}
	}


	void _mergeBid(const FeedID& feedid, const Record& record)
	{
		if(_topLevel.feedInvolved(feedid, SideEnum::Bid))
		{
			if(record.Bid() < _topLevel.Bid().price())
			{
				_topLevel.remove(feedid, SideEnum::Bid);
			}
			else
				_topLevel.add(feedid, Side(record.Bid(), record.BidSize()), SideEnum::Bid);
		}
		else
		{
			if(record.Bid() >= _topLevel.Bid().price())
				_topLevel.add(feedid, Side(record.Bid(), record.BidSize()), SideEnum::Bid);
		}
	}

	void _mergeAsk(const FeedID& feedid, const Record& record)
	{
		if(_topLevel.feedInvolved(feedid, SideEnum::Ask))
		{
			if(record.Ask() > _topLevel.Ask().price())
			{
				_topLevel.remove(feedid, SideEnum::Ask);
			}
			else
				_topLevel.add(feedid, Side(record.Ask(), record.AskSize()), SideEnum::Ask);
		}
		else
		{
			if(record.Ask() <= _topLevel.Ask().price())
				_topLevel.add(feedid, Side(record.Ask(), record.AskSize()), SideEnum::Ask);
		}
	}

private:
	string						   _symbol;
	unordered_map<FeedID, BookPtr> _bookPerFeed;
	TopLevel					   _topLevel;
	TimePoint					   _lastChangeTime;
	//mutable	std::mutex			   _statisticsMutex;
	BookStatistics				   _statistics;
};



typedef std::shared_ptr<CompositeBook> CompositeBookPtr;
typedef unordered_map<string, CompositeBookPtr> CompositeBookMap;



class Feed
{
public:
	Feed(const InputReaderPtr& input, FeedID feedID) : _feedID(feedID), _input(input), _cache(nullptr)
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
				_cache = RecordPtr(new Record(line, tokenizer, _feedID, chrono::high_resolution_clock::now()));
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
	FeedID			_feedID;
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
