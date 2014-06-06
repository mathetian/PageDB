#ifndef _NON_COPYABLE_H
#define _NON_COPYABLE_H

namespace utils
{

class Noncopyable
{
public:
	Noncopyable() { }
private:
	Noncopyable& operator=(const Noncopyable&);
	Noncopyable(const Noncopyable&);
};

class Halfcopyable
{
public:
	Halfcopyable() { }
	Halfcopyable(const Halfcopyable&);
private:
	Halfcopyable& operator=(const Halfcopyable&);
};

};



#endif