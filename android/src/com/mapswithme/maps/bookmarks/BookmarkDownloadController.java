package com.mapswithme.maps.bookmarks;

import android.os.Bundle;
import android.support.annotation.NonNull;

import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.base.Savable;

public interface BookmarkDownloadController extends Detachable<BookmarkDownloadCallback>,
                                                    Savable<Bundle>
{
  boolean downloadBookmark(@NonNull String url);
  void retryDownloadBookmark();
}
