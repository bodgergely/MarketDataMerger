#ifndef _SPINNINGQUEUE_H
#define _SPINNINGQUEUE_H

#include <atomic>
#include <queue>

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

/*
 * probability spin lock
 *
 * */
class spinlock2
{
public:
	spinlock2() : _flag(false) {}
	void lock()
	{
		bool val = true;
		while(true)
		{
			while((val = _flag.load(std::memory_order_relaxed)));

			if(_flag.compare_exchange_weak(val, true, std::memory_order_relaxed))
				break;
		}
	}
	void unlock()
	{
		_flag.store(false, std::memory_order_relaxed);
	}
private:
	std::atomic<bool> _flag;
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
				_q.pop();
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
