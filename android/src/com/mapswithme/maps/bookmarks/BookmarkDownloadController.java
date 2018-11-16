package com.mapswithme.maps.bookmarks;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.base.Savable;

public interface BookmarkDownloadController extends Detachable<Fragment>, Savable<Bundle>
{
  boolean downloadBookmark(@NonNull String url);
  void retryDownloadBookmark();
}
