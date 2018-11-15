package com.mapswithme.maps.bookmarks;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.base.Savable;

import java.net.MalformedURLException;

public interface BookmarkDownloadController extends Detachable<Fragment>, Savable<Bundle>
{
  void downloadBookmark(@NonNull String url) throws MalformedURLException;
  void retryDownloadBookmark() throws MalformedURLException;
}
