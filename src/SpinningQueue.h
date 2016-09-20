#ifndef SPINNINGQUEUE_H
#define SPINNINGQUEUE_H

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
			using namespace std;if(res)
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
