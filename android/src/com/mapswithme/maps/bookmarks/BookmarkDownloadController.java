package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;
import android.support.v4.app.FragmentActivity;

import com.mapswithme.maps.dialog.Detachable;

import java.net.MalformedURLException;

public interface BookmarkDownloadController extends Detachable<FragmentActivity>
{
  void downloadBookmark(@NonNull String url) throws MalformedURLException;
}
