#ifndef LINKVALUE_H
#define LINKVALUE_H

#include <list>
#include <unordered_set>

#include "SharedArray.h"

typedef SharedArray<uint8_t> LinkValue;
typedef std::list<LinkValue> LinkValueList;
typedef std::unordered_set<LinkValue, LinkValue::Hash, LinkValue::Equal> LinkValueSet;

struct SharedSecret
{
  struct ConfirmScheme_
  {
    enum Type
    {
      None = 0,
      Passive = 1,
      Active = 2
    };
  };
  typedef ConfirmScheme_::Type ConfirmScheme;

  LinkValue value;
  float pFalse;
  bool confirmed;
  ConfirmScheme confirmedBy;

  struct Hash
  {
    size_t operator()(const SharedSecret &value) const
    {
      LinkValue::Hash hashFunc;
      return hashFunc(value.value);
    }
  };

  struct Equal
  {
    bool operator ()(const SharedSecret &a, const SharedSecret &b) const
    {
      LinkValue::Equal equalFunc;
      return equalFunc(a.value, b.value);
    }
  };

  struct Compare
  {
    bool operator ()(const SharedSecret &a, const SharedSecret &b) const
    {
      return a.pFalse < b.pFalse;
    }
  };

  SharedSecret()
     : value(),
       pFalse(1),
       confirmed(false),
       confirmedBy(ConfirmScheme::None)
  {
  }

  SharedSecret(bool confirmed)
     : value(),
       pFalse(confirmed ? 0 : 1),
       confirmed(confirmed),
       confirmedBy(ConfirmScheme::None)
  {
  }

  SharedSecret(ConfirmScheme confirmedBy)
     : value(),
       pFalse(0),
       confirmed(true),
       confirmedBy(confirmedBy)
  {
  }

  SharedSecret(uint8_t *arrayPtr, size_t size)
     : value(arrayPtr, size),
       pFalse(1),
       confirmed(false),
       confirmedBy(ConfirmScheme::None)
  {
  }

  SharedSecret(uint8_t *arrayPtr, size_t size, ConfirmScheme confirmedBy)
     : value(arrayPtr, size),
       pFalse(0),
       confirmed(true),
       confirmedBy(confirmedBy)
  {
  }

  void confirm(ConfirmScheme confirmedBy)
  {
    confirmed = true;
    this->confirmedBy = confirmedBy;
  }
};
typedef std::list<SharedSecret> SharedSecretList;
typedef std::unordered_set<SharedSecret, SharedSecret::Hash, SharedSecret::Equal> SharedSecretSet;

#endif // LINKVALUE_H
