#ifndef _SPSCLOCKFREEQUEUE_H
#define _SPSCLOCKFREEQUEUE_H

#include <atomic>
#include <memory>

/*
 * Based on the Listing7.13 A single-producer, single-consumer lock-free queue in Concurrency in Action Anthony Williams
February 2012 ISBN 9781933988771
 *
 * */

template<class T>
class SPSCLockFreeQueue
{
public:
	SPSCLockFreeQueue() : _head(new Node), _tail(_head.load()) {}
	SPSCLockFreeQueue(const SPSCLockFreeQueue& ) = delete;
	SPSCLockFreeQueue& operator=(const SPSCLockFreeQueue& ) = delete;
	~SPSCLockFreeQueue()
	{
		while(const Node* oldHead = _head.load())
		{
			_head.store(oldHead->_next);
			delete oldHead;
		}
	}

	std::shared_ptr<T> pop()
	{
		Node* oldHead = popHead();
		if(!oldHead)
		{
			return std::shared_ptr<T>();
		}
		const std::shared_ptr<T> res(oldHead->_data);
		delete oldHead;
		return res;
	}

	void push(T newValue)
	{
		std::shared_ptr<T> newData(std::make_shared<T>(newValue));
		Node* p = new Node;
		Node* oldTail = _tail.load();
		oldTail->_data.swap(newData);
		oldTail->_next = p;
		_tail.store(p);
	}

private:
	struct Node
	{
		Node() : _next(nullptr) {}
		Node* _next;
		std::shared_ptr<T> _data;
	};

	Node* popHead()
	{
		Node* oldHead = _head.load();
		if(oldHead == _tail.load())
		{
			return nullptr;
		}
		_head.store(oldHead->_next);
		return oldHead;
	}

	std::atomic<Node*> _head;
	std::atomic<Node*> _tail;
};

#endif
