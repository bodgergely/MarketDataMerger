#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <queue>
#include <cstring>
#include <mutex>
#include <condition_variable>

#include "InputReader.h"


using namespace std;

class TimePoint
{
public:
	TimePoint()
	{
	}
	TimePoint(const string& time)
	{
		sscanf(time.c_str(), "%2d:%2d:%2d.%3d",
		    &vec[0],
		    &vec[1],
		    &vec[2],
			&vec[3]);
	}

	inline int hr() const {return vec[0];}
	inline int min() const {return vec[1];}
	inline int sec() const {return vec[2];}
	inline int millisec() const {return vec[3];}

	// hacky but convenient for the time comparison method
	inline const int* repr() const {return vec;}


private:
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


class Tokenizer
{
public:
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

private:
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

	const TimePoint& Time() {return _time;}
	const string&	 Symbol() {return _symbol;}
	double 			 Bid() {return _bid;}
	int				 BidSize() {return _bid_size;}
	double 			 Ask() {return _ask;}
	int				 AskSize() {return _ask_size;}


	class TimeCompare
	{
		bool operator()(const std::shared_ptr<Record> a,std::shared_ptr<Record> b)
		{
			if(a->Time() < b->Time())
				return true;
			else
				return false;
		}
	};


private:
	TimePoint 	_time;
	string 		_symbol;
	double 		_bid;
	int 		_bid_size;
	double  	_ask;
	int		  	_ask_size;
};

using RecordPtr = std::shared_ptr<Record>;




class Side
{
public:
	Side() : _price(0.0), _qty(0) {}
	virtual ~Side() {}
	double price() {return _price;}
	double qty() {return _qty;}
	TimePoint lastUpdate() {return _lastUpdate;}
	virtual bool update(double price, double qty) = 0;
protected:
	TimePoint _lastUpdate;
	double 	  _price;
	int	   	  _qty;
};

class Bid : public Side
{
public:
	Bid() {}
	~Bid() {}
	virtual bool update(double price, double qty)
	{

	}
};

class Ask : public Side
{
public:
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
	bool update(const Record& record)
	{
		//TODO
	}
private:
	string 	_symbol;
	Bid 	_bid;
	Ask 	_ask;
};

typedef std::shared_ptr<Book> BookPtr;


typedef unordered_map<string, Book> BookMap;




class ConsolidatedFeed
{
public:
	using NewRecordCB = std::function<void(const RecordPtr&)>;
	using EndOfDayCB = std::function<void(void)>;
	ConsolidatedFeed() {}
	ConsolidatedFeed(const vector<InputReaderPtr>& feeds) : _inputReaderVec(feeds)
	{

	}

	~ConsolidatedFeed()
	{

	}

	void addFeed(const InputReaderPtr& feed) {_inputReaderVec.push_back(feed);}

	void registerNewRecordCB(const NewRecordCB& cb_) {_newRecordCB = cb_;}
	void registerEndOfDayCB(const EndOfDayCB& cb_) {_endOfDayCB = cb_;}

	// should be called after registering the callbacks
	void start()
	{
		_recordProducerThread = thread(&ConsolidatedFeed::_process, this);
	}


private:
	using PriorityQueue = priority_queue<RecordPtr, vector<RecordPtr>, Record::TimeCompare>;

	void _process()
	{
		Tokenizer tokenizer(',');
		while(true)
		{
			int invalidReaderCount = 0;
			for(InputReaderPtr& reader : _inputReaderVec)
			{
				string line;
				if(reader->isValid())
				{
					if(reader->readLine(line))
					{
						RecordPtr record = RecordPtr(new Record(line, tokenizer));
						_recordQueue.push(record);
						RecordPtr rec = _recordQueue.top();
						// callback to consumer
						_newRecordCB(rec);
						_recordQueue.pop();
					}
				}
				else
				{
					++invalidReaderCount;
				}
			}
			// exit if all the readers are invalid
			if(invalidReaderCount == _inputReaderVec.size())
			{
				_endOfDayCB();
				return;
			}
		}
	}


private:
	vector<InputReaderPtr>  		_inputReaderVec;
	NewRecordCB						_newRecordCB;
	EndOfDayCB						_endOfDayCB;
	thread							_recordProducerThread;
	PriorityQueue					_recordQueue;
};


template<class T>
class Queue
{
public:
	Queue() {}
	virtual ~Queue() {}
	virtual void push(const T& val) = 0;
	virtual T pop() = 0;	// return by value
protected:
	std::queue<T> _q;
};

template<class T>
class BlockingQueue : public Queue<T>
{
public:
	BlockingQueue() {}
	virtual ~BlockingQueue(){}
	virtual void push(const T& val)
	{
		std::unique_lock<std::mutex> lock(_m);
		Queue<T>::_q.push(val);
		_cv.notify_all();
	}


	virtual T pop()
	{
		std::unique_lock<std::mutex> lock(_m);
		while(Queue<T>::_q.size()==0)
			_cv.wait(lock);

		T elem = Queue<T>::_q.top();
		Queue<T>::_q.pop();
		return std::move(elem);
	}


private:
	std::mutex _m;
	std::condition_variable _cv;
};


class BookGroupProcessor
{
public:
	BookGroupProcessor()
	{
		_processorThread = std::thread(&BookGroupProcessor::_processing, this);
	}
	void send(const RecordPtr& rec) {_recordQueue.push(rec);}

private:
	void _processing()
	{
		RecordPtr rec =_recordQueue.pop();
		string& symbol = rec->Symbol();
		_books[symbol].update(*rec);
	}

private:
	BlockingQueue<RecordPtr> _recordQueue;
	BookMap 				 _books;
	std::thread 			 _processorThread;
};



class Reporter
{
public:
	Reporter() {}
	virtual ~Reporter() {}

	void	publish(const BookPtr& topOfBook)
	{
		_topOfBookChangedQueue.push(topOfBook);
	}


protected:
	virtual void _report(const BookPtr& book) = 0;

private:
	void _processing()
	{
		while(true)
		{
			BookPtr book = _topOfBookChangedQueue.pop();
			if(book)
				_report(book);
			else
				break;
		}
	}
private:
	BlockingQueue<BookPtr> _topOfBookChangedQueue;
	std::thread			  _consumerThread;
};

typedef unique_ptr<Reporter> ReporterPtr;

class StandardOutputReporter : public Reporter
{
protected:
	virtual void _report(const BookPtr& book)
	{
		cout << *book << "\n";
	}
};




class MarketDataConsumer
{
public:
	MarketDataConsumer(int numOfProcessors, ReporterPtr reporter) : _numOfProcessors(numOfProcessors),
																	_feedEnded(false),
																	_processorPool(numOfProcessors),
																	_reporter(reporter)
	{

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

	void multiplexer()
	{
		while(true)
		{
			RecordPtr record = _incomingRecordsQueue.pop();
			if(record)
				multiplex(record);
			else
				break;
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
	hash<std::string>									_hasher;
	ReporterPtr						    				_reporter;
};




class MainApp
{
public:
	MainApp(vector<string> inputFiles, int processingGroupCount) : _consumer(processingGroupCount, new StandardOutputReporter())
	{
		for(const string& file : inputFiles)
		{
			_feed.addFeed(InputReaderPtr(new FileInputReader(file)));
		}
		_feed.registerNewRecordCB(std::bind(&MarketDataConsumer::push,_consumer, placeholders::_1));
		_feed.registerEndOfDayCB(std::bind(&MarketDataConsumer::feedEnded, _consumer));
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

	MainApp app(infiles, 6);
	app.start();

	return 0;

}
