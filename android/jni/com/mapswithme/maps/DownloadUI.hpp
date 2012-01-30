#pragma once

#include "../jni/jni_method.hpp"
#include "../../../../../storage/storage.hpp"
#include "../../../../../std/scoped_ptr.hpp"

namespace android
{
  class DownloadUI
  {
  private:
    jobject m_self;

    scoped_ptr<jni::Method> m_onChangeCountry;
    scoped_ptr<jni::Method> m_onProgress;

  public:

    DownloadUI(jobject self);
    ~DownloadUI();

    void OnChangeCountry(storage::TIndex const & idx);
    void OnProgress(storage::TIndex const & idx, pair<int64_t, int64_t> const & p);
  };
}
