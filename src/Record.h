#ifndef _RECORD_H
#define _RECORD_H

#include "TimePoint.h"
#include "Tokenizer.h"
#include "CommonDefs.h"
#include <sstream>

using namespace std;


struct Record
{
	double 		bid;
	double  	ask;
	char 		symbol[8];
	TimePoint 	time;
	FeedID		feedID;
	unsigned int bid_size;
	unsigned int ask_size;
};

using RecordPtr = Record*;

inline std::string toString(const Record& record)
{
	stringstream ss;
	ss << record.feedID << "," << record.time.toString() << "," << record.symbol << "," << record.bid << "," << record.bid_size << "," << record.ask << "," << record.ask_size;
	return ss.str();
}

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


void _sanityCheck(const string& line)
{
	if(line.size() == 0 || isspace(line[0]))
		throw RecordInvalid(line);
}

// might fail badly if the data structure is not correct
void _parseLine(RecordPtr rec, const string& line, const Tokenizer& tokenizer)
{
	bool result = true;
	_sanityCheck(line);		//TODO
	vector<string> tokenz = tokenizer.tokenize(line);  // TODO
	rec->time = TimePoint(tokenz[0]);
	strcpy(rec->symbol, tokenz[1].c_str());	//TODO
	rec->bid = stod(tokenz[2]);
	rec->bid_size = stoi(tokenz[3]);
	rec->ask = stod(tokenz[4]);
	rec->ask_size = stoi(tokenz[5]);
}

void initRecord(RecordPtr rec, const string& line, const Tokenizer& tokenizer, FeedID feedID)
{
	rec->symbol;
	rec->feedID = feedID;
	_parseLine(rec, line, tokenizer);
}

void initRecord(RecordPtr rec, const TimePoint& tp, const char* symbol_, double bidPrice, uint bidSize, double askPrice, uint askSize, const FeedID& feedid_)
{
	rec->bid = bidPrice;
	rec->ask = askPrice;
	strcpy(rec->symbol, symbol_);
	rec->time = tp;
	rec->feedID = feedid_;
	rec->bid_size = bidSize;
	rec->ask_size = askSize;
}



ostream& operator<<(ostream& os, const Record& record)
{
	os << toString(record);
	return os;
}



class TimeCompare
{
public:
	inline bool operator()(const RecordPtr a, const RecordPtr b)
	{
		if(a->time < b->time)
			return true;
		else
			return false;
	}
};


struct RecordPoolTag { };
typedef boost::singleton_pool<RecordPoolTag, sizeof(Record)> RecordMemPool;



#endif
