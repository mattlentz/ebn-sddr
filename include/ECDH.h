#ifndef ECDH_H
#define	ECDH_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/ecdh.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

#include "LinkValue.h"

class ECDH
{
private:
  typedef std::shared_ptr<EC_KEY> ECKeySharedPtr;
  typedef std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)> ECPointPtr; 

private:
  size_t keySize_;
  ECKeySharedPtr secret_; 
  std::vector<uint8_t> localPublic_;

public:
  ECDH(); 
  ECDH(size_t keySize); 

  const uint8_t* getPublic() const;
  const uint8_t* getPublicX() const;
  bool getPublicY() const;

  size_t getKeySize() const;
  size_t getPublicSize() const; 

  void generateSecret();

  bool computeSharedSecret(SharedSecret &dest, const uint8_t *remotePublicX, bool remotePublicY) const;
  bool computeSharedSecret(SharedSecret &dest, const uint8_t *remotePublic) const; 

private:
  static void* derivateKey(const void *in, size_t inLength, void *out, size_t *outLength);
};

inline const uint8_t* ECDH::getPublic() const
{
  return localPublic_.data();
}

inline const uint8_t* ECDH::getPublicX() const
{
  return localPublic_.data() + 1;
}

inline bool ECDH::getPublicY() const
{
  return localPublic_[0] & 0x01;
}

inline size_t ECDH::getKeySize() const
{
  return keySize_;
}

inline size_t ECDH::getPublicSize() const
{
  return (keySize_ / 8) + 1;
}

#endif // ECDH_H
