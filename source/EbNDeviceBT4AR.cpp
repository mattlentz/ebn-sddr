#include "EbNDeviceBT4AR.h"

#include <openssl/aes.h>
#include <openssl/evp.h>

#include "Logger.h"

using namespace std;

EbNDeviceBT4AR::EbNDeviceBT4AR(DeviceID id, const Address &address, const LinkValueList &listenSet)
   : EbNDevice(id, address, listenSet),
     dhExchange_()
{
  updateMatching();
}

void EbNDeviceBT4AR::setAddress(const Address &address)
{
  EbNDevice::setAddress(address);
  updateMatching();
}

bool EbNDeviceBT4AR::updateMatching()
{
  EVP_CIPHER_CTX evpContext;
  EVP_CIPHER_CTX_init(&evpContext);

  LinkValueList::iterator it = matching_.begin();
  while(it != matching_.end())
  {
    const LinkValue &value = *it;

    uint8_t output[16];
    int updateSize = sizeof(output);
    int outputSize = sizeof(output);

    EVP_EncryptInit_ex(&evpContext, EVP_aes_128_ecb(), NULL, value.get(), NULL);
    EVP_EncryptUpdate(&evpContext, output, &updateSize, address_.toByteArray() + 3, 3);
    EVP_EncryptFinal_ex(&evpContext, output, &outputSize);

    if(memcmp(address_.toByteArray(), output + 13, 3) != 0)
    {
      it = matching_.erase(it);
      updatedMatching_ = true;
      it++;
    }
    else
    {
      it++;
    }
  }

  EVP_CIPHER_CTX_cleanup(&evpContext);

  matchingPFalse_ *= 1.0 / (1 << 24);
  LOG_P("EbNDeviceBT4AR", "Updated matching set to %d entries (pFalse %g) for id %d", matching_.size(), matchingPFalse_, id_);

  return updatedMatching_;
}
