#include <vector>
#include <string>
#include <unordered_map>

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
	Tokenizer(const string& line, char separator) : _line(line), _sep(separator), _pos(0) {}
	~Tokenizer();
	bool nextToken(string& output)
	{
		size_t fi = _pos;
		while(fi<_line.size() && _line[fi]!=_sep) ++fi;
		_pos = fi;
		output = _line.substr(_pos, fi-_pos);
		if(output.size() > 0)
			return true;
		else
			return false;

	}
	string _line;
	char 	_sep;
	size_t _pos;

};

/*
 * time,symbol,bid,bid_size,ask,ask_size*/
struct Record
{
public:
	Record(const string& inputLine, const Tokenizer& tokenizer)
	{
		parseLine(inputLine, tokenizer);
	}
	bool parseLine(const string& input, const Tokenizer& tokenizer)
	{
		bool result = true;

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
	double price() {return _price;}
	double qty() {return _qty;}
	TimePoint lastUpdate() {return _lastUpdate;}
	virtual void update(double price, double qty) = 0;
private:
	TimePoint _lastUpdate;
	double 	  _price;
	int	   	  _qty;
};

class Bid : public Side
{
};

class Ask : public Side
{

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


class MainApp
{
public:
	MainApp(vector<string> inputFiles)
	{
		for(const string& file : inputFiles)
		{
			_inputReaderVec.push_back(InputReader(file));
		}
	}
	~MainApp() {}
	void start()
	{
		for(InputReader& reader : _inputReaderVec)
		{

		}
	}
private:
	vector<InputReader> _inputReaderVec;
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
