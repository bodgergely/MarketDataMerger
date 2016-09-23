#ifndef _FEED_H
#define _FEED_H

#include "Record.h"
#include "InputReader.h"
#include "Logger.h"
#include "CommonDefs.h"

using namespace std;

class Feed
{
public:
	Feed(const InputReaderPtr& input, char separator, FeedID feedID) : _feedID(feedID), _input(input), _cache(nullptr), _separator(separator)
	{
	}
	~Feed() {}
	const bool 	 		readNextRecordToCache()
	{
		char line[256];
		if(_input->isValid())
		{
			_input->readLine(line);
			//cout << "Line read " << line << endl;
			try
			{
				RecordPtr rec = (Record*)RecordMemPool::malloc();
				initRecord(rec, line, _separator, _feedID);
				_cache = rec;
				return true;
			}
			catch(const RecordInvalid& e)
			{
				//LOG("record invalid exception:" + string(e.what()) + "End of line");
				//cout << "record invalid exception:" << string(e.what()) << "End of line" << endl;
				_cache = nullptr;
			}
		}

		return false;


	}
	const RecordPtr  	 cache() const {return _cache;}
	void				 clearCache() { _cache = nullptr; }

	inline bool			 isValid() const {return _input->isValid();}
protected:
	FeedID			_feedID;
	InputReaderPtr 	_input;
	RecordPtr		_cache;
	char			_separator;
};

typedef std::shared_ptr<Feed> FeedPtr;


class ConsolidatedFeed
{
public:
	void	  addFeed(const FeedPtr& feed)
	{
		_feeds.push_back(feed);
	}
	RecordPtr nextRecord()
	{
		RecordPtr oldestRecord{nullptr};
		TimePoint oldestTime;
		int		  oldestFeedIndex = 0;
		for(int i=0;i<_feeds.size();i++)
		{
			FeedPtr& feed = _feeds[i];
			if(feed->isValid())
			{
				TimePoint time;
				if(feed->cache()!=nullptr)
				{
					time = feed->cache()->time;
				}
				else
				{
					if(feed->readNextRecordToCache())
						time = feed->cache()->time;
				}

				if(!oldestTime.isValid())
				{
					oldestTime = time;
					oldestFeedIndex = i;
				}
				else if(oldestTime>time)
				{
					oldestTime = time;
					oldestFeedIndex = i;
				}
			}
		}

		if(oldestTime.isValid())
		{
			oldestRecord = _feeds[oldestFeedIndex]->cache();
			_feeds[oldestFeedIndex]->clearCache();
		}

		return oldestRecord;

	}
private:
	vector<FeedPtr> _feeds;
};





class FeedManager
{
public:
	using NewRecordCB = std::function<void(const RecordPtr)>;
	using EndOfDayCB = std::function<void(void)>;
	FeedManager() {}
	~FeedManager() {}

	void addFeed(FeedPtr&& feed)
	{
		_consolidatedFeed.addFeed(std::forward<FeedPtr>(feed));
	}

	void registerNewRecordCB(const NewRecordCB& cb_) {_newRecordCB = cb_;}
	//void registerEndOfDayCB(const EndOfDayCB& cb_) {_endOfDayCB = cb_;}

	// should be called after registering the callbacks
	void start()
	{
		_process();
	}


private:
	void _process()
	{
		while(true)
		{
			RecordPtr rec = _consolidatedFeed.nextRecord();
			_newRecordCB(rec);
			if(!rec)
				break;
		}
		//LOG("Reached end of all feeds.");
	}


private:
	ConsolidatedFeed				_consolidatedFeed;
	NewRecordCB						_newRecordCB;
	//EndOfDayCB						_endOfDayCB;
};

#endif
