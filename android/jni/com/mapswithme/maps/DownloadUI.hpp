#pragma once

#include "../../../../../storage/storage.hpp"

#include <jni.h>

namespace android
{
  class DownloadUI
  {
  private:
    jobject m_self;

    jmethodID m_onChangeCountry;
    jmethodID m_onProgress;

  public:

    DownloadUI(jobject self);
    ~DownloadUI();

    void OnChangeCountry(storage::TIndex const & idx);
    void OnProgress(storage::TIndex const & idx, pair<int64_t, int64_t> const & p);
  };
}
