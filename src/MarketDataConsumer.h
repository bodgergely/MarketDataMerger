#ifndef _MARKETDATACONSUMER_H
#define _MARKETDATACONSUMER_H

#include <vector>
#include "SpinningQueue.h"
#include "Reporter.h"
#include "Book.h"
#include "Record.h"

using namespace std;

class MarketDataConsumer
{
public:
	MarketDataConsumer() : _feedEnded(false)

	{
	}
	~MarketDataConsumer()
	{
	}

	void read(const RecordPtr rec)
	{
		if(rec)
			update(rec);
	}

	const unordered_map<string, BookStatistics>& writeBookStatistics() const
	{
		for(const auto& p : _books)
		{
			cout << p.second->getStatistics().toString() << "\n";
		}
	}

private:

	void update(const RecordPtr record)
	{

		const string& symbol = record->symbol;
		if(_books.find(symbol)==_books.end())
		{
			_books[symbol] = new CompositeBook(symbol);
		}

		CompositeBookPtr book = _books[symbol];
		if(book->update(record))
		{
			const CompositeBook::CompositeTopLevel top = book->getTopBook();
			//cout << top.toString() << "\n";
		}
		RecordMemPool::free(record);

	}



private:
	bool					 							_feedEnded;
	CompositeBookMap 		 							_books;
};


typedef MarketDataConsumer* MarketDataConsumerPtr;

#endif
