package com.mapswithme.maps.widget;

import android.support.annotation.IdRes;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public abstract class BaseShadowController<V extends ViewGroup>
{
  // Shadow IDs
  protected static final int TOP = 0;
  protected static final int BOTTOM = 1;

  private final SparseArray<View> mShadows = new SparseArray<>(1);
  protected final V mList;

  public BaseShadowController(V list)
  {
    this(list, R.id.shadow_top);
  }

  public BaseShadowController(V list, @IdRes int shadowId)
  {
    mList = list;
    addShadow(TOP, shadowId);
  }

  protected BaseShadowController addShadow(int id, @IdRes int shadowViewId)
  {
    View shadow = ((ViewGroup)mList.getParent()).findViewById(shadowViewId);
    if (shadow != null)
      mShadows.put(id, shadow);
    return this;
  }

  public BaseShadowController addBottomShadow()
  {
    return addShadow(BOTTOM, R.id.shadow_bottom);
  }

  public void updateShadows()
  {
    for (int i = 0; i < mShadows.size(); i++)
    {
      int id = mShadows.keyAt(i);
      UiUtils.showIf(shouldShowShadow(id), mShadows.get(id));
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
