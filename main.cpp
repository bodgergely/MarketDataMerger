#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

#include "InputReader.h"


using namespace std;

class TimePoint
{
public:
	TimePoint(const string& time) : _time(time) {}
private:
	string _time;
};


class Tokenizer
{
	Tokenizer(char separator) : _sep(separator) {}
	~Tokenizer();
	vector<string> tokenize(const string& line) const
	{
		size_t pos = 0;
		vector<string> tokenz;

		while(pos < line.size())
		{
			size_t fi = pos;
			while(fi<line.size() && line[fi]!=_sep) ++fi;
			tokenz.push_back(line.substr(pos, fi-pos));
			pos = fi;
		}

		return tokenz;

	}

	char _sep;
};

/*
 * time,symbol,bid,bid_size,ask,ask_size*/
class Record
{
public:
	Record(const string& line, const Tokenizer tokenizer)
	{
		parseLine(line, tokenizer);
	}
	// might fail badly if the data structure is not correct
	void parseLine(const string& line, const Tokenizer& tokenizer)
	{
		bool result = true;
		vector<string> tokenz = tokenizer.tokenize(line);
		_time = TimePoint(tokenz[0]);
		_symbol = tokenz[1];
		_bid = stod(tokenz[2]);
		_bid_size = stoi(tokenz[3]);
		_ask = stod(tokenz[4]);
		_ask_size = stoi(tokenz[5]);
	}


private:
	TimePoint 	_time;
	string 		_symbol;
	double 		_bid;
	int 		_bid_size;
	double  	_ask;
	int		  	_ask_size;
};





class Side
{
public:
	Side() : _price(0.0), _qty(0) {}
	virtual ~Side() {}
	double price() {return _price;}
	double qty() {return _qty;}
	TimePoint lastUpdate() {return _lastUpdate;}
	virtual bool update(double price, double qty) = 0;
private:
	TimePoint _lastUpdate;
	double 	  _price;
	int	   	  _qty;
};

class Bid : public Side
{
	Bid() {}
	~Bid() {}
	virtual bool update(double price, double qty)
	{

	}
};

class Ask : public Side
{
	Ask() {}
	~Ask() {}
	virtual bool update(double price, double qty)
	{

	}
};


class Book
{
public:
	Book() {}
	Book(const string& symbol, const Bid& bid, const Ask& ask) :
		_symbol(symbol), _bid(bid), _ask(ask) {}
	~Book() {}
	void update(const Record& record);
private:
	string 	_symbol;
	Bid 	_bid;
	Ask 	_ask;
};



typedef unordered_map<string, Book> BookCollection;



class ConsolidatedFeed
{
public:
	using NewRecordCB = std::function<void(Record)>;
	using EndOfDayCB = std::function<void(void)>;
	ConsolidatedFeed(vector<string> inputFiles)
	{
		for(const string& file : inputFiles)
		{
			_inputReaderVec.push_back(InputReader(file));
		}
	}

	~ConsolidatedFeed()
	{

	}

	void registerNewRecordCB(const NewRecordCB& cb_) {_newRecordCB = cb_;}
	void registerEndOfDayCB(const EndOfDayCB& cb_) {_endOfDayCB = cb_;}

	// should be called after registering the callbacks
	void start();

private:

	Record _nextRecord(){}

private:
	vector<InputReader> _inputReaderVec;
	NewRecordCB			_newRecordCB;
	EndOfDayCB			_endOfDayCB;
};




class MarketDataConsumer
{

};




class MainApp
{
public:
	MainApp(vector<string> inputFiles)
	{

	}
	~MainApp() {}
	void start()
	{
		_feed.start();
	}
private:
	ConsolidatedFeed 	_feed;
	MarketDataConsumer 	_consumer;
};



int main(int argc, char** argv)
{
	vector<string> infiles;
	for(int i=1;i<argc;i++)
	{
		infiles.push_back(argv[i]);
	}

	MainApp app(infiles);
	app.start();

	return 0;

}
