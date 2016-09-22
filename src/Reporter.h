#ifndef _REPORTER_H
#define _REPORTER_H

#include <thread>
#include <mutex>
#include "Queue.h"
#include "Book.h"

using namespace std;

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
		//cout << top.toString() << endl;
	}
};


class KnowsAboutFeedsStandardOutputReporter : public StandardOutputReporter
{
public:
	KnowsAboutFeedsStandardOutputReporter(int numOfFeeds) : StandardOutputReporter(), _numOfFeeds(numOfFeeds) {}
	virtual ~KnowsAboutFeedsStandardOutputReporter()
	{
		requestStop();
		join();
	}
protected:
	virtual void _report(const CompositeBook::CompositeTopLevel& top)
	{
		if(top.Symbol() == "")
		{
			++_numOfFeedEnded;
			if(_numOfFeedEnded == _numOfFeeds)
				cout << "Done\n";
		}
		StandardOutputReporter::_report(top);
	}
private:
	int _numOfFeeds;
	int _numOfFeedEnded{0};
};

#endif
