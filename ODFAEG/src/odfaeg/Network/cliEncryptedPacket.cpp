#include "../../../include/odfaeg/Network/cliEncryptedPacket.hpp"
#include "../../../include/odfaeg/Network/network.h"
namespace odfaeg {
    namespace network {
        using namespace std;
        using namespace sf;
        Rsa& CliEncryptedPacket::rsa = CliEncryptedPacket::getRsa();
        Rsa& CliEncryptedPacket::getRsa() {
            if (&rsa == nullptr) {
                static Rsa rsa;
                return rsa;
            }
            return rsa;
        }
        int CliEncryptedPacket::getCertificate(unsigned char** out) {
            return rsa.ossl_getCertificate(out);
        }
        void CliEncryptedPacket::setCertificate(const unsigned char* in, int length) {
            rsa.ossl_setCertificate(in, length);
        }
        const void* CliEncryptedPacket::onSend (size_t& dataSize) {
            unsigned char* buffer = nullptr;
            if (Network::getSrvInstance().isRunning())
                buffer = rsa.ossl_encryptWithPbKey(static_cast<const unsigned char*> (getData()), getDataSize(), reinterpret_cast<int&>(dataSize));
            else
                buffer = rsa.ossl_encryptWithPrKey(static_cast<const unsigned char*> (getData()), getDataSize(), reinterpret_cast<int&>(dataSize));
            return &buffer[0];
        }
        void CliEncryptedPacket::onReceive (const void* data, size_t dataSize) {
            unsigned char* buffer = nullptr;
            std::size_t dstSize = 0;
            if (Network::getSrvInstance().isRunning())
                buffer = rsa.ossl_decryptWithPbKey(static_cast<const unsigned char*> (data), dataSize, reinterpret_cast<int&>(dstSize));
            else
                buffer = rsa.ossl_decryptWithPrKey(static_cast<const unsigned char*> (data), dataSize, reinterpret_cast<int&>(dstSize));
            append(&buffer[0], dstSize);
        }
    }
}

