package com.mapswithme.maps.base;

import android.database.Observable;
import android.support.v7.widget.RecyclerView;

public class DataObservable extends Observable<RecyclerView.AdapterDataObserver>
{
  public void notifyChanged() {
    for (int i = mObservers.size() - 1; i >= 0; i--) {
      mObservers.get(i).onChanged();
    }
  }
}
