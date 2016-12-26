package com.mapswithme.maps.search;

import android.content.res.Resources;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.content.ContextCompat;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class SearchFilterPanelController
{
  @NonNull
  private final View mFrame;
  @NonNull
  private final TextView mShowOnMap;
  @NonNull
  private final View mFilterButton;
  @NonNull
  private final ImageView mFilterIcon;
  @NonNull
  private final TextView mFilterText;
  @NonNull
  private final View mDivider;

  @Nullable
  private HotelsFilter mFilter;

  private final float mElevation;
  @NonNull
  private final String mCategory;

  private View.OnClickListener mClearListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      if (mFilterPanelListener != null)
        mFilterPanelListener.onFilterClear();
    }
  };

  public interface FilterPanelListener
  {
    void onViewClick();
    void onFilterClick();
    void onFilterClear();
  }

  @Nullable
  private final FilterPanelListener mFilterPanelListener;

  public SearchFilterPanelController(@NonNull View frame,
                                     @Nullable FilterPanelListener listener)
  {
    this(frame, listener, R.string.search_show_on_map);
  }

  public SearchFilterPanelController(@NonNull View frame,
                                     @Nullable FilterPanelListener listener,
                                     @StringRes int populateButtonText)
  {
    mFrame = frame;
    mFilterPanelListener = listener;
    mShowOnMap = (TextView) mFrame.findViewById(R.id.show_on_map);
    mShowOnMap.setText(populateButtonText);
    mFilterButton = mFrame.findViewById(R.id.filter_button);
    mFilterIcon = (ImageView) mFilterButton.findViewById(R.id.filter_icon);
    mFilterText = (TextView) mFilterButton.findViewById(R.id.filter_text);
    mDivider = mFrame.findViewById(R.id.divider);

    Resources res = mFrame.getResources();
    mElevation = res.getDimension(R.dimen.margin_quarter);
    mCategory = res.getString(R.string.hotel).toLowerCase();

    initListeners();
  }

  public void show(boolean show, boolean showPopulateButton)
  {
    UiUtils.showIf(show, mFrame);
    showPopulateButton(showPopulateButton);
  }

  public void showPopulateButton(boolean show)
  {
    UiUtils.showIf(show, mShowOnMap);
  }

  public void showDivider(boolean show)
  {
    UiUtils.showIf(show, mDivider);
  }

  public boolean updateFilterButtonVisibility(@Nullable String category)
  {
    boolean show = !TextUtils.isEmpty(category)
                   && category.trim().toLowerCase().equals(mCategory);
    UiUtils.showIf(show, mFilterButton);

    return show;
  }

  private void initListeners()
  {
    mShowOnMap.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (mFilterPanelListener != null)
          mFilterPanelListener.onViewClick();
      }
    });
    mFilterButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (mFilterPanelListener != null)
          mFilterPanelListener.onFilterClick();
      }
    });
  }

  @Nullable
  public HotelsFilter getFilter()
  {
    return mFilter;
  }

  public void setFilter(@Nullable HotelsFilter filter)
  {
    mFilter = filter;
    if (mFilter != null)
    {
      mFilterIcon.setOnClickListener(mClearListener);
      mFilterIcon.setImageResource(R.drawable.ic_cancel);
      mFilterIcon.setColorFilter(ContextCompat.getColor(mFrame.getContext(),
          UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.accentButtonTextColor)));
      UiUtils.setBackgroundDrawable(mFilterButton, R.attr.accentButtonBackground);
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
        mFilterButton.setElevation(mElevation);
      mFilterText.setTextColor(ContextCompat.getColor(mFrame.getContext(),
          UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.accentButtonTextColor)));
    }
    else
    {
      mFilterIcon.setOnClickListener(null);
      mFilterIcon.setImageResource(R.drawable.ic_filter_list);
      mFilterIcon.setColorFilter(ContextCompat.getColor(mFrame.getContext(),
          UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.colorAccent)));
      UiUtils.setBackgroundDrawable(mFilterButton, R.attr.clickableBackground);
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
        mFilterButton.setElevation(0);
      mFilterText.setTextColor(ContextCompat.getColor(mFrame.getContext(),
          UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.colorAccent)));
    }
  }
}
