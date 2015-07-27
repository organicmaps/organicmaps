#pragma once

#include "../core/jni_helper.hpp"
#include "../maps/MapStorage.hpp"
#include "../maps/Framework.hpp"

#include "platform/country_defines.hpp"

namespace storage_utils
{
  ::Framework * frm();

  storage::ActiveMapsLayout & GetMapLayout();
  storage::CountryTree & GetTree();

  storage::ActiveMapsLayout::TGroup ToGroup(int group);
  MapOptions ToOptions(int options);
  jlongArray ToArray(JNIEnv * env, storage::LocalAndRemoteSizeT const & sizes);
}
