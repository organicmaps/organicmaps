package com.mapswithme.maps.widget;

import android.support.annotation.IdRes;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;

public abstract class BaseShadowController<V extends ViewGroup, ScrollListener>
{
  // Shadow IDs
  public static final int TOP = 0;
  public static final int BOTTOM = 1;

  protected final V mList;
  protected final SparseArray<View> mShadows = new SparseArray<>(1);
  protected ScrollListener mScrollListener;


  public BaseShadowController(V list)
  {
    this(list, R.id.shadow_top);
  }

  public BaseShadowController(V list, @IdRes int shadowId)
  {
    mList = list;
    addShadow(TOP, shadowId);
  }

  public BaseShadowController addShadow(int id, @IdRes int shadowViewId)
  {
    View shadow = ((ViewGroup)mList.getParent()).findViewById(shadowViewId);
    if (shadow != null)
      mShadows.put(id, shadow);
    return this;
  }

  public BaseShadowController setScrollListener(ScrollListener scrollListener)
  {
    mScrollListener = scrollListener;
    return this;
  }

  public void updateShadows()
  {
    for (int i = 0; i < mShadows.size(); i++)
    {
      int id = mShadows.keyAt(i);
      mShadows.get(id).setVisibility(shouldShowShadow(id) ? View.VISIBLE : View.GONE);
    }
  }

  public BaseShadowController attach()
  {
    updateShadows();
    return this;
  }

  public void detach()
  {
    for (int i = 0; i < mShadows.size(); i++)
      mShadows.get(mShadows.keyAt(i))
              .setVisibility(View.GONE);
  }

  protected abstract boolean shouldShowShadow(int id);
}
