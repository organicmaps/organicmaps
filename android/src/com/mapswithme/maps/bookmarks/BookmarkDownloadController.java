package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.mapswithme.maps.dialog.Detachable;

import java.net.MalformedURLException;

public interface BookmarkDownloadController extends Detachable<Activity>
{
  void downloadBookmark(@NonNull String url) throws MalformedURLException;
}
