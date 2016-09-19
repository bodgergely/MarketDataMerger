#ifndef _RECORD_H
#define _RECORD_H

#include "TimePoint.h"
#include "Tokenizer.h"
#include "CommonDefs.h"
#include <sstream>

using namespace std;

/*
 * time,symbol,bid,bid_size,ask,ask_size*/
class Record
{
public:
	Record(const string& line, const Tokenizer tokenizer, FeedID feedID, const chrono::high_resolution_clock::time_point& timestamp) : _feedID(feedID), _receivedTime(timestamp)
	{
		//LOG("parsing line: " + line);
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

ostream& operator<<(ostream& os, const Record& record)
{
	os << record.toString();
	return os;
}

using RecordPtr = std::shared_ptr<Record>;

#endif
