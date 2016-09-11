#ifndef _LOGGER_
#define _LOGGER_

#include "Queue.h"
#include <thread>
#include <atomic>
#include <fstream>
#include <iostream>

class Logger
{
public:
	Logger(std::string file) : _logFile(file, std::ofstream::out)
	{
		_flusherThread = std::thread(&Logger::processing, this);
	}
	~Logger()
	{
		_queue.requestStop();
		if(_flusherThread.joinable())
			_flusherThread.join();
		_logFile.flush();
		_logFile.close();
	}

	inline void log(const std::string& msg)
	{
		_queue.push(msg);
	}

	inline void operator()(const std::string& msg)
	{
		log(msg);
	}

private:

	void processing()
	{
		while(true)
		{
			std::string msg;
			int c = 0;
			if(_queue.pop(msg))
			{
				_logFile << msg << "\n";
				if(++c >= _flushAfterCountMsg)
				{
					_logFile.flush();
					c = 0;
				}
			}
			else
				break;
		}

	}

private:
	BlockingQueue<std::string> _queue;
	std::thread				   _flusherThread;
	std::ofstream 			   _logFile;
	const int				   _flushAfterCountMsg{1};
};

#endif
