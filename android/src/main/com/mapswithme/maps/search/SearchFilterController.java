package com.mapswithme.maps.search;

import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class SearchFilterController
{
  @NonNull
  private final View mFrame;
  @NonNull
  private final TextView mShowOnMap;
  @NonNull
  private final View mDivider;

  @Nullable
  private final FilterListener mFilterListener;

  interface FilterListener
  {
    void onShowOnMapClick();
  }

  public SearchFilterController(@NonNull View frame, @Nullable FilterListener listener,
                                @StringRes int populateButtonText)
  {
    mFrame = frame;
    mFilterListener = listener;
    mShowOnMap = mFrame.findViewById(R.id.show_on_map);
    mShowOnMap.setText(populateButtonText);
    mDivider = mFrame.findViewById(R.id.divider);

    initListeners();
  }

  public void show(boolean show)
  {
    UiUtils.showIf(show, mFrame);
    showPopulateButton(true);
  }

  void showPopulateButton(boolean show)
  {
    UiUtils.showIf(show, mShowOnMap);
  }

  void showDivider(boolean show)
  {
    UiUtils.showIf(show, mDivider);
  }

  private void initListeners()
  {
    mShowOnMap.setOnClickListener(v ->
                                  {
                                    if (mFilterListener != null)
                                      mFilterListener.onShowOnMapClick();
                                  });
  }

  public static class DefaultFilterListener implements FilterListener
  {
    @Override
    public void onShowOnMapClick()
    {
    }
  }
}
