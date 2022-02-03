//role : noncopyable base class.
//Author : Ventery

#ifndef BASE_NONCOPYABLE_H
#define BASE_NONCOPYABLE_H

namespace Global
{
class Noncopyable
{
protected:
  Noncopyable() {}
  ~Noncopyable() {}

private:
  Noncopyable(const Noncopyable &);
  const Noncopyable &operator=(const Noncopyable &);
};

} // namespace Global

#endif // BASE_NONCOPYABLE_H