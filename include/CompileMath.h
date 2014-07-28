#ifndef COMPILEMATH_H
#define COMPILEMATH_H

template<size_t N, size_t base = 2>
struct CLog
{
  enum { value = 1 + CLog < N / base, base>::value }; 
};

template<size_t base>
struct CLog<1, base>
{
  enum { value = 1 };
};

template<size_t base>
struct CLog<0, base>
{
  enum { value = 0 };
};

#endif // COMPILEMATH_H

