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
  private final SearchToolbarListener mListener;

  public interface VisibilityListener
  {
    void onSearchVisibilityChanged(boolean visible);
  }

  public FloatingSearchToolbarController(@NonNull Activity activity,
                                         @Nullable SearchToolbarListener listener)
  {
    super(activity.getWindow().getDecorView(), activity);
    mListener = listener;
  }

  @Override
  public void onUpClick()
  {
    if (mListener != null)
      mListener.onSearchUpClick(getQuery());
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
  }

  public void refreshQuery()
  {
    showProgress(false);
    CharSequence query = ParsedMwmRequest.hasRequest() ?
            ParsedMwmRequest.getCurrentRequest().getTitle()
            : SearchEngine.INSTANCE.getQuery();

    if (!TextUtils.isEmpty(query))
    {
      setQuery(query);
    }
  }

  public void cancelSearchApiAndHide(boolean clearText)
  {
    SearchEngine.INSTANCE.cancel();

    if (clearText)
      clear();

    hide();
  }

  public void show()
  {
    UiUtils.show(getToolbar());
    if (mVisibilityListener != null)
      mVisibilityListener.onSearchVisibilityChanged(true);
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
