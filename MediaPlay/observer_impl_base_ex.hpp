#ifndef OBSERVER_IMPL_BASE_EX_HPP
#define OBSERVER_IMPL_BASE_EX_HPP

#include <map>

template <typename ReturnT, typename ParamT>
class ReceiverExImplBase;

template <typename ReturnT, typename ParamT>
class ObserverExImplBase
{
public:
	virtual void AddReceiver(ReceiverExImplBase<ReturnT, ParamT>* receiver) = 0;
	virtual void RemoveReceiver(ReceiverExImplBase<ReturnT, ParamT>* receiver) = 0;
	virtual ReturnT Broadcast(ParamT param) = 0;
	virtual ReturnT RBroadcast(ParamT param) = 0;
	virtual ReturnT Notify(ParamT param) = 0;
};

template <typename ReturnT, typename ParamT>
class ReceiverExImplBase
{
public:
	virtual void AddObserver(ObserverExImplBase<ReturnT, ParamT>* observer) = 0;
	virtual void RemoveObserver() = 0;
	virtual ReturnT Receive(ParamT param) = 0;
	virtual ReturnT Respond(ParamT param, ObserverExImplBase<ReturnT, ParamT>* observer) = 0;
};

template <typename ReturnT, typename ParamT>
class ReceiverExImpl;

template <typename ReturnT, typename ParamT>
class ObserverExImpl : public ObserverExImplBase<ReturnT, ParamT>
{
	template <typename ReturnT, typename ParamT>
	friend class IteratorEx;
public:
	ObserverExImpl()
	{}

	virtual ~ObserverExImpl()	{}

	virtual void AddReceiver(ReceiverExImplBase<ReturnT, ParamT>* receiver)
	{
		if (receiver == NULL)
			return;

		receivers_.push_back(receiver);
		receiver->AddObserver(this);
	}

	virtual void RemoveReceiver(ReceiverExImplBase<ReturnT, ParamT>* receiver)
	{
		if (receiver == NULL)
			return;

		ReceiversVector::iterator it = receivers_.begin();
		for (; it != receivers_.end(); ++it)
		{
			if (*it == receiver)
			{
				receivers_.erase(it);
				break;
			}
		}
	}

	virtual ReturnT Broadcast(ParamT param)
	{
		ReceiversVector::iterator it = receivers_.begin();
		for (; it != receivers_.end(); ++it)
		{
			(*it)->Receive(param);
		}

		return ReturnT();
	}

	virtual ReturnT RBroadcast(ParamT param)
	{
		ReceiversVector::reverse_iterator it = receivers_.rbegin();
		for (; it != receivers_.rend(); ++it)
		{
			(*it)->Receive(param);
		}

		return ReturnT();
	}

	virtual ReturnT Notify(ParamT param)
	{
		ReceiversVector::iterator it = receivers_.begin();
		for (; it != receivers_.end(); ++it)
		{
			(*it)->Respond(param, this);
		}

		return ReturnT();
	}

	template <typename ReturnT, typename ParamT>
	class IteratorEx
	{
		ObserverExImpl<ReturnT, ParamT> & _tbl;
		int index;
		ReceiverExImplBase<ReturnT, ParamT>* ptr;
	public:
		IteratorEx( ObserverExImpl & table )
			: _tbl( table ), index(0), ptr(NULL)
		{}

		IteratorEx( const IteratorEx & v )
			: _tbl( v._tbl ), index(v.index), ptr(v.ptr)
		{}

		ReceiverExImplBase<ReturnT, ParamT>* next()
		{
			if ( index >= _tbl.receivers_.size() )
				return NULL;

			for ( ; index < _tbl.receivers_.size(); )
			{
				ptr = _tbl.receivers_[ index++ ];
				if ( ptr )
					return ptr;
			}
			return NULL;
		}
	};

protected:
	typedef std::vector<ReceiverExImplBase<ReturnT, ParamT>*> ReceiversVector;
	ReceiversVector receivers_;
};


template <typename ReturnT, typename ParamT>
class ReceiverExImpl : public ReceiverExImplBase<ReturnT, ParamT>
{
public:
	ReceiverExImpl()
	{}

	virtual ~ReceiverExImpl()	{}

	virtual void AddObserver(ObserverExImplBase<ReturnT, ParamT>* observer)
	{
		observers_.push_back(observer);
	}

	virtual void RemoveObserver()
	{
		ObserversVector::iterator it = observers_.begin();
		for (; it != observers_.end(); ++it)
		{
			(*it)->RemoveReceiver(this);
		}
	}

	virtual ReturnT Receive(ParamT param)
	{
		return ReturnT();
	}

	virtual ReturnT Respond(ParamT param, ObserverExImplBase<ReturnT, ParamT>* observer)
	{
		return ReturnT();
	}

protected:
	typedef std::vector<ObserverExImplBase<ReturnT, ParamT>*> ObserversVector;
	ObserversVector observers_;
};

#endif // OBSERVER_IMPL_BASE_HPP