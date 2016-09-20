#ifndef _QUEUE_H
#define _QUEUE_H

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>


template<class T>
class Queue
{
public:
	Queue() {}
	virtual ~Queue() {}
	virtual void push(const T& val) = 0;
	virtual bool pop(T& val) = 0;	// return by value
protected:
	std::queue<T> _q;
};

template<class T>
class BlockingQueue : public Queue<T>
{
public:
	BlockingQueue(std::chrono::milliseconds timeout=std::chrono::milliseconds(10)) : _timeoutDuration(timeout){}
	virtual ~BlockingQueue(){}
	virtual void push(const T& val)
	{
		std::unique_lock<std::mutex> lock(_m);
		if(!_stopRequested.load())
		{
			Queue<T>::_q.push(val);
			_cv.notify_all();
		}
	}


	virtual bool pop(T& val)
	{
		std::unique_lock<std::mutex> lock(_m);
		while(Queue<T>::_q.size()==0)
		{
			if(_cv.wait_for(lock, _timeoutDuration) == std::cv_status::timeout)
			{
				if(_stopRequested.load() == true && Queue<T>::_q.size()==0)
					return false;
			}
		}

		val = Queue<T>::_q.front();
		Queue<T>::_q.pop();
		return true;
	}

	void requestStop() {_stopRequested.store(true);}


private:
	std::mutex _m;
	std::condition_variable _cv;
	std::atomic<bool> _stopRequested{false};
	std::chrono::milliseconds _timeoutDuration;
};


class spinlock
{
public:
	spinlock() : _flag(ATOMIC_FLAG_INIT) {}
	void lock()
	{
		while(_flag.test_and_set(std::memory_order_acquire));
	}
	void unlock()
	{
		_flag.clear(std::memory_order_release);
	}
private:
	std::atomic_flag _flag;
};


template<class T>
class SpinningQueue
{
public:
	void push(const T& val)
	{
		_lock.lock();
		_q.push(val);
		_lock.unlock();
	}

	bool pop(T& val)
	{
		bool res = false;
		while(true)
		{
			_lock.lock();
			if(!_q.empty())
			{
				val = _q.front();
				res = true;
			}
			_lock.unlock();
			if(res)
				break;
			else if(_stopRequested.load() == true)
				return false;
		}

		return true;
	}

	void requestStop() {_stopRequested.store(true);}
private:
	spinlock _lock;
	std::queue<T> _q;
	std::atomic<bool> _stopRequested{false};
};



#endif
