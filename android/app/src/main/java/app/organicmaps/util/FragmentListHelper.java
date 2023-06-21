package app.organicmaps.util;

import android.app.Fragment;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Helper class to track fragments attached to Activity.
 * Its primary goal is to implement getFragments() that is present in Support Library
 * but is missed in native FragmentManager.
 *
 * <p/>
 * Usage:
 * <ul>
 *   <li>Create instance of FragmentListHelper in your Activity.</li>
 *   <li>Override {@link android.app.Activity#onAttachFragment(Fragment)} in your Activity and call {@link FragmentListHelper#onAttachFragment(Fragment)}.</li>
 *   <li>Call {@link FragmentListHelper#getFragments()} to obtain list of fragments currently added to your Activity.</li>
 * </ul>
 */
@SuppressWarnings("deprecation") // TODO: Remove when minSdkVersion >= 28. Replace Fragment class.
public class FragmentListHelper
{
  private final Map<String, WeakReference<Fragment>> mFragments = new HashMap<>();

  public void onAttachFragment(Fragment fragment)
  {
    mFragments.put(fragment.getClass().getName(), new WeakReference<>(fragment));
  }

  public List<Fragment> getFragments()
  {
    List<String> toRemove = null;

    List<Fragment> res = new ArrayList<>(mFragments.size());
    for (String key : mFragments.keySet())
    {
      Fragment f = mFragments.get(key).get();
      if (f == null || !f.isAdded())
      {
        if (toRemove == null)
          toRemove = new ArrayList<>();

        toRemove.add(key);
        continue;
      }

      res.add(f);
    }

    if (toRemove != null)
      for (String key : toRemove)
        mFragments.remove(key);

    return res;
  }
}
