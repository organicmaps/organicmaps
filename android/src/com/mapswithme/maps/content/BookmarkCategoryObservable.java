package com.mapswithme.maps.content;

import android.app.Application;
import android.database.Observable;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.MwmApplication;

public class BookmarkCategoryObservable extends Observable<RecyclerView.AdapterDataObserver>
{
  public void notifyChanged()
  {
    for (int i = mObservers.size() - 1; i >= 0; i--)
    {
      mObservers.get(i).onChanged();
    }
  }

  @NonNull
  public static BookmarkCategoryObservable from(@NonNull Application app)
  {
    MwmApplication mwmApplication = (MwmApplication) app;
    return mwmApplication.getBookmarkCategoryObservable();
  }
}
