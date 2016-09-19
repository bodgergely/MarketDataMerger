#ifndef _TIMEPOINT_H
#define _TIMEPOINT_H

#include <string>
#include <chrono>

using namespace std;

class TimePoint
{
public:
	TimePoint() : _valid(false)
	{
	}
	explicit TimePoint(const string& time) : _valid(true)
	{
		sscanf(time.c_str(), formatString,
		    &vec[0],
		    &vec[1],
		    &vec[2],
			&vec[3]);
	}

	inline int hr() const {return vec[0];}
	inline int min() const {return vec[1];}
	inline int sec() const {return vec[2];}
	inline int millisec() const {return vec[3];}

	inline bool isValid() const {return _valid;}

	// hacky but convenient for the time comparison method
	inline const int* repr() const {return vec;}

	string toString() const
	{
		char buff[256] = {0};
		snprintf(buff, sizeof(buff)-1, formatString, hr(), min(), sec(), millisec());
		return buff;
	}


private:
	constexpr static const char* formatString{"%02d:%02d:%02d.%03d"};
	bool _valid;
	int vec[4] = {0};
};



inline bool operator<(const TimePoint& lhs, const TimePoint& rhs)
{
	for(int i=0;i<4;i++)
	{
		if(lhs.repr()[i]<rhs.repr()[i])
			return true;
		else if(lhs.repr()[i]>rhs.repr()[i])
			return false;
	}

	return true;
}

inline bool operator<=(const TimePoint& lhs, const TimePoint& rhs)
{
	return (lhs<rhs);
}

inline bool operator>(const TimePoint& lhs, const TimePoint& rhs)
{
	return !(lhs<rhs);
}


inline bool operator==(const TimePoint& lhs, const TimePoint& rhs)
{
	for(int i=0;i<4;i++)
	{
		if(lhs.repr()[i]!=rhs.repr()[i])
			return false;
	}
	return true;
}



#endif
