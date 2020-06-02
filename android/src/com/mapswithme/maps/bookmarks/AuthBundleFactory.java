package com.mapswithme.maps.bookmarks;

import android.os.Bundle;

import androidx.annotation.NonNull;
import com.mapswithme.util.statistics.Statistics;

public abstract class AuthBundleFactory
{

  public static Bundle saveReview()
  {
    return buildBundle(Statistics.EventParam.AFTER_SAVE_REVIEW);
  }

  public static Bundle bookmarksBackup()
  {
    return buildBundle(Statistics.EventParam.BOOKMARKS_BACKUP);
  }

  public static Bundle guideCatalogue()
  {
    return buildBundle(Statistics.EventParam.GUIDE_CATALOGUE);
  }

  public static Bundle subscription()
  {
    return buildBundle(Statistics.EventParam.SUBSCRIPTION);
  }

  public static Bundle exportBookmarks()
  {
    return buildBundle(Statistics.EventParam.EXPORT_BOOKMARKS);
  }

  private static Bundle buildBundle(@NonNull String value)
  {
    Bundle bundle = new Bundle();
    bundle.putString(Statistics.EventParam.FROM, value);
    return bundle;
  }
}
