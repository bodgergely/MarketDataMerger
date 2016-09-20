#ifndef _BOOK_H
#define _BOOK_H

#include "CommonDefs.h"
#include "Record.h"
#include <utility>
#include <cassert>
#include <unordered_map>
#include <algorithm>

using namespace std;

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

	void updateLatencyAverage(double latency)
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

bool operator==(const CompositeBook::CompositeTopLevel& lhs, const CompositeBook::CompositeTopLevel& rhs)
{
	return lhs.Symbol() == rhs.Symbol() && lhs.Bid() == rhs.Bid() && lhs.Ask() == rhs.Ask();
}

bool operator!=(const CompositeBook::CompositeTopLevel& lhs, const CompositeBook::CompositeTopLevel& rhs)
{
	return !(lhs==rhs);
}



typedef std::shared_ptr<CompositeBook> CompositeBookPtr;
typedef unordered_map<string, CompositeBookPtr> CompositeBookMap;

#endif
