#pragma once
#include "std/target_os.hpp"
#include "std/noncopyable.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#include <FNet.h>
#pragma clang diagnostic pop

namespace downloader
{
class IHttpThreadCallback;
}
using namespace Tizen::Net::Http;

class HttpThread
    : public Tizen::Net::Http::IHttpTransactionEventListener
    , public Tizen::Net::Http::IHttpProgressEventListener
    , noncopyable
{
public:

  HttpThread(std::string const & url,
             downloader::IHttpThreadCallback & callback,
             int64_t beg, int64_t end,
             int64_t size,
             std::string const & pb);
  ~HttpThread();

  bool OnStart(void);

  ///
  ///Tizen::Net::Http::IHttpTransactionEventListener
  ///
  virtual void OnTransactionAborted (HttpSession & httpSession,
                                      HttpTransaction & httpTransaction,
                                      result r);
  virtual void OnTransactionCertVerificationRequiredN (HttpSession & httpSession,
                                                        HttpTransaction & httpTransaction,
                                                        Tizen::Base::String *pCert);
  virtual void OnTransactionCompleted (HttpSession & httpSession,
                                        HttpTransaction & httpTransaction);
  virtual void OnTransactionHeaderCompleted (HttpSession & httpSession,
                                              HttpTransaction & httpTransaction,
                                              int headerLen, bool bAuthRequired);
  virtual void OnTransactionReadyToRead (HttpSession & httpSession,
                                          HttpTransaction & httpTransaction,
                                          int availableBodyLen);
  virtual void OnTransactionReadyToWrite (HttpSession & httpSession,
                                           HttpTransaction & httpTransaction,
                                           int recommendedChunkSize);

  ///
  ///Tizen::Net::Http::IHttpProgressEventListener
  ///
  virtual void OnHttpDownloadInProgress (HttpSession & httpSession,
                                          HttpTransaction & httpTransaction,
                                          int64_t currentLength,
                                          int64_t totalLength);
  virtual void OnHttpUploadInProgress (HttpSession & httpSession,
                                        HttpTransaction & httpTransaction,
                                        int64_t currentLength,
                                        int64_t totalLength);

private:
  downloader::IHttpThreadCallback & m_callback;

  int64_t m_begRange;
  int64_t m_endRange;
  int64_t m_downloadedBytes;
  int64_t m_expectedSize;
  std::string const m_url;
  std::string const & m_pb;

  HttpSession * m_pSession;
  HttpTransaction* m_pTransaction;
};
