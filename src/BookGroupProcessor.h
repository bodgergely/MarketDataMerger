#ifndef _BOOKGROUPPROCESSOR_H
#define _BOOKGROUPPROCESSOR_H

#include <thread>
#include <unordered_map>
#include <pthread.h>

#include "Book.h"
#include "Record.h"
#include "Queue.h"
#include "Reporter.h"

using namespace std;


class BookGroupProcessor
{
public:
	BookGroupProcessor()
	{
		_processorThread = std::thread(&BookGroupProcessor::_processing, this);
	}

#ifdef __unix__
	BookGroupProcessor(int schedulingPolicy, int threadPriority) : BookGroupProcessor()
	{
		_setScheduling(schedulingPolicy, threadPriority);
	}
#endif

	~BookGroupProcessor()
	{
		join();
	}

	void registerReporter(const ReporterPtr& reporter)
	{
		_reporter = reporter;
	}

#ifdef __unix__
	void setSchedulingPolicy(int schedulingPolicy, int threadPriority)
	{
		_setScheduling(schedulingPolicy, threadPriority);
	}
#endif

	void send(const RecordPtr& rec) {_recordQueue.push(rec);}

	void join()
	{
		if(_processorThread.joinable())
			_processorThread.join();
	}

	const unordered_map<string, BookStatistics>& bookStats() const {return _bookStats;}

private:

#ifdef __unix__
	void _setScheduling(int schedulingPolicy, int threadPriority)
	{
		sched_param sch_params;
		sch_params.sched_priority = threadPriority;
		pthread_setschedparam(_processorThread.native_handle(), schedulingPolicy, &sch_params);
	}
#endif

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
					delete rec;

				}
				else
				{
					_reporter->publish(CompositeBook::CompositeTopLevel{});
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

#endif

