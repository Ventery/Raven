// role : noncopyable base class.
// Author : Ventery

#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

class Noncopyable
{
protected:
  Noncopyable() {}
  ~Noncopyable() {}

private:
  Noncopyable(const Noncopyable &);
  const Noncopyable &operator=(const Noncopyable &);
};

#endif // NONCOPYABLE_H