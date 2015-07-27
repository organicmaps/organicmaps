#include "country_helper.hpp"

namespace storage_utils
{
  ::Framework * frm() { return g_framework->NativeFramework(); }

  storage::ActiveMapsLayout & GetMapLayout() { return frm()->GetCountryTree().GetActiveMapLayout(); }
  storage::CountryTree & GetTree() { return frm()->GetCountryTree(); }

  storage::ActiveMapsLayout::TGroup ToGroup(int group) { return static_cast<storage::ActiveMapsLayout::TGroup>(group); }
  MapOptions ToOptions(int options) { return static_cast<MapOptions>(options); }
  jlongArray ToArray(JNIEnv * env, storage::LocalAndRemoteSizeT const & size)
  {
    jlongArray result = env->NewLongArray(2);

    jlong * arr = env->GetLongArrayElements(result, NULL);
    arr[0] = size.first;
    arr[1] = size.second;
    env->ReleaseLongArrayElements(result, arr, 0);

    return result;
  }
}
