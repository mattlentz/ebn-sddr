#ifndef SHAREDARRAY_H
#define SHAREDARRAY_H

#include <cstdint>
#include <memory>
#include <string>

#include "SipHash.h"

template<typename T>
class SharedArray
{
private:
  class Deleter
  {
  public:
    void operator ()(T *arrayPtr) const
    {
      delete[] arrayPtr;
    }
  };

public:
  struct Hash
  {
    size_t operator()(const SharedArray<T> &value) const
    {
      return GSipHash().digest((uint8_t *)value.get(), value.size() * sizeof(T));
    }
  };

  struct Equal
  {
    bool operator ()(const SharedArray<T> &a, const SharedArray<T> &b) const
    {
      if(a.size() != b.size())
      {
        return false;
      }

      for(int i = 0; i < a.size(); i++)
      {
        if(a.get()[i] != b.get()[i])
        {
          return false;
        }
      }
      return true;
    }
  };

private:
  std::shared_ptr<T> arrayPtr_;
  size_t size_;

public:
  SharedArray()
     : arrayPtr_(),
       size_(0)
  {
  }

  SharedArray(T *arrayPtr, size_t size)
     : arrayPtr_(arrayPtr, Deleter()),
       size_(size)
  {
  }

  inline T* get()
  {
    return arrayPtr_.get();
  }

  inline const T* get() const
  {
    return arrayPtr_.get();
  }

  inline T& operator [](size_t index)
  {
    return arrayPtr_.get()[index];
  }

  inline const T& operator [](size_t index) const
  {
    return arrayPtr_.get()[index];
  }

  inline size_t size() const
  {
    return size_;
  }

  std::string toString() const
  {
    return std::string("SharedArray");
  }
};

template<> std::string SharedArray<uint8_t>::toString() const; 

#endif // SHAREDARRAY_H
