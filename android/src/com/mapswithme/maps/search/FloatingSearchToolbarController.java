package com.mapswithme.maps.search;

import android.app.Activity;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.util.UiUtils;

public class FloatingSearchToolbarController extends SearchToolbarController
{
  @Nullable
  private VisibilityListener mVisibilityListener;
  @Nullable
  private SearchToolbarListener mListener;

  public interface VisibilityListener
  {
    void onSearchVisibilityChanged(boolean visible);
  }

  public FloatingSearchToolbarController(@NonNull Activity activity,
                                         @Nullable SearchToolbarListener listener,
                                         @Nullable RoomsGuestsMenuStateCallback callback)
  {
    super(activity.getWindow().getDecorView(), activity, callback);
    mListener = listener;
  }

  @Override
  public void onUpClick()
  {
    if (mListener != null)
      mListener.onSearchUpClick(getQuery());
    cancelSearchApiAndHide(true);
  }

  @Override
  protected void onQueryClick(@Nullable String query)
  {
    super.onQueryClick(query);
    if (mListener != null)
      mListener.onSearchQueryClick(getQuery());
    hide();
  }

  @Override
  protected void onClearClick()
  {
    super.onClearClick();
    if (mListener != null)
      mListener.onSearchClearClick();
    cancelSearchApiAndHide(false);
  }

  public void refreshToolbar()
  {
    showProgress(false);

    if (ParsedMwmRequest.hasRequest())
    {
      UiUtils.show(getToolbar());

      if (mVisibilityListener != null)
        mVisibilityListener.onSearchVisibilityChanged(true);

      setQuery(ParsedMwmRequest.getCurrentRequest().getTitle());
    }
    else if (!TextUtils.isEmpty(SearchEngine.INSTANCE.getQuery()))
    {
      UiUtils.show(getToolbar());

      if (mVisibilityListener != null)
        mVisibilityListener.onSearchVisibilityChanged(true);

      setQuery(SearchEngine.INSTANCE.getQuery());
    }
    else
    {
      hide();
      clear();
    }
  }

  private void cancelSearchApiAndHide(boolean clearText)
  {
    SearchEngine.INSTANCE.cancel();

    if (clearText)
      clear();

    hide();
  }

  public boolean hide()
  {
    if (!UiUtils.isVisible(getToolbar()))
      return false;

    UiUtils.hide(getToolbar());

    if (mVisibilityListener != null)
      mVisibilityListener.onSearchVisibilityChanged(false);

    return true;
  }

  public void setVisibilityListener(@Nullable VisibilityListener visibilityListener)
  {
    mVisibilityListener = visibilityListener;
  }


  public interface SearchToolbarListener
  {
    void onSearchUpClick(@Nullable String query);
    void onSearchQueryClick(@Nullable String query);
    void onSearchClearClick();
  }
}
