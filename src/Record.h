#ifndef _RECORD_H
#define _RECORD_H

#include "TimePoint.h"
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


void _sanityCheck(const char* line)
{
	if(strlen(line) == 0 || isspace(line[0]))
		throw RecordInvalid(line);
}


void _splitLine(char* line, char separator)
{
	while(*line!='\0')
	{
		if(*line == '\r')
			*line = '\0';

		if(*line == separator)
			*line = '\0';
		++line;
	}
}



// might fail badly if the data structure is not correct
void _parseLine(RecordPtr rec, char* line, char separator)
{
	bool result = true;
	char* pos = line;
	_sanityCheck(line);		//TODO
	_splitLine(line, separator);
	rec->time = TimePoint(pos);
	while(*pos!='\0') pos++;
	pos++;
	strcpy(rec->symbol, pos);
	while(*pos!='\0') pos++;
	pos++;
	rec->bid = atof(pos);
	while(*pos!='\0') pos++;
	pos++;
	rec->bid_size = atoi(pos);
	while(*pos!='\0') pos++;
	pos++;
	rec->ask = atof(pos);
	while(*pos!='\0') pos++;
	pos++;
	rec->ask_size = atoi(pos);
}

void initRecord(RecordPtr rec, char* line, char separator, FeedID feedID)
{
	rec->symbol;
	rec->feedID = feedID;
	_parseLine(rec, line, separator);
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
