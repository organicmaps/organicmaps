package com.mapswithme.maps.base;

import android.database.Observable;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;

public interface ObservableHost<T extends Observable<RecyclerView.AdapterDataObserver>>
{
  @NonNull
  T getObservable();
}
