package com.mapswithme.maps.search;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ActionMenuView;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.List;

class CategoriesAdapter extends RecyclerView.Adapter<CategoriesAdapter.ViewHolder>
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_CATEGORY, TYPE_PROMO_CATEGORY })
  @interface ViewType {}
  private static final int TYPE_CATEGORY = 0;
  private static final int TYPE_PROMO_CATEGORY = 1;

  @StringRes
  private int mCategoryResIds[];
  @DrawableRes
  private int mIconResIds[];

  private final LayoutInflater mInflater;
  private final Resources mResources;

  interface CategoriesUiListener
  {
    void onSearchCategorySelected(@Nullable String category);
    void onPromoCategorySelected(@NonNull PromoCategory promo);
    void onAdsRemovalSelected();
  }

  private CategoriesUiListener mListener;

  CategoriesAdapter(@NonNull Fragment fragment)
  {
    if (fragment instanceof CategoriesUiListener)
      mListener = (CategoriesUiListener) fragment;
    mResources = fragment.getResources();
    mInflater = LayoutInflater.from(fragment.getActivity());
  }

  void updateCategories(@NonNull Fragment fragment)
  {
    final Activity activity = fragment.getActivity();
    final String packageName = activity.getPackageName();

    final String[] keys = getAllCategories();
    final int numKeys = keys.length;

    mCategoryResIds = new int[numKeys];
    mIconResIds = new int[numKeys];
    for (int i = 0; i < numKeys; i++)
    {
      String key = keys[i];
      mCategoryResIds[i] = getStringResIdByKey(activity.getResources(), packageName, key);

      if (mCategoryResIds[i] == 0)
        throw new IllegalStateException("Can't get string resource id for category:" + key);

      mIconResIds[i] = getDrawableResIdByKey(activity.getApplicationContext(), packageName, key);
      if (mIconResIds[i] == 0)
        throw new IllegalStateException("Can't get icon resource id for category:" + key);
    }
  }

  @NonNull
  private static String[] getAllCategories()
  {
    String[] searchCategories = DisplayedCategories.getKeys();
    List<PromoCategory> promos = PromoCategory.supportedValues();
    int amountSize = searchCategories.length + promos.size();
    String[] allCategories = new String[amountSize];
    for (PromoCategory promo : promos)
    {
      if (promo.getPosition() >= amountSize)
        throw new AssertionError("Promo position must be in range: "
                                 + "[0 - " + amountSize + ")");

      allCategories[promo.getPosition()] = promo.name();
    }

    for (int i = 0, j = 0; i < amountSize; i++)
    {
      if (allCategories[i] == null)
      {
        allCategories[i] = searchCategories[j];
        j++;
      }
    }

    return allCategories;
  }

  @StringRes
  private static int getStringResIdByKey(@NonNull Resources resources, @NonNull String packageName,
                                         @NonNull String key)
  {
    try
    {
      PromoCategory promoCategory = PromoCategory.valueOf(key);
      return promoCategory.getStringId();
    }
    catch (IllegalArgumentException ex)
    {
      return resources.getIdentifier(key, "string", packageName);
    }
  }

  @DrawableRes
  private static int getDrawableResIdByKey(@NonNull Context context,
                                           @NonNull String packageName,
                                           @NonNull String key)
  {
    final boolean isNightTheme = ThemeUtils.isNightTheme(context);
    try
    {
      PromoCategory promoCategory = PromoCategory.valueOf(key);
      return promoCategory.getIconId(isNightTheme);
    }
    catch (IllegalArgumentException ex)
    {
      String iconId = "ic_category_" + key;
      if (isNightTheme)
        iconId = iconId + "_night";
      return context.getResources().getIdentifier(iconId, "drawable", packageName);
    }
  }

  @Override
  @ViewType
  public int getItemViewType(int position)
  {
    PromoCategory promo = PromoCategory.findByStringId(mCategoryResIds[position]);
    if (promo != null)
      return TYPE_PROMO_CATEGORY;
    return TYPE_CATEGORY;
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, @ViewType int viewType)
  {
    View view;
    ViewHolder viewHolder;
    switch (viewType)
    {
      case TYPE_CATEGORY:
        view = mInflater.inflate(R.layout.item_search_category, parent, false);
        viewHolder = new ViewHolder(view, (TextView) view);
        break;
      case TYPE_PROMO_CATEGORY:
        view = mInflater.inflate(R.layout.item_search_promo_category, parent, false);
        viewHolder = new PromoViewHolder(view, view.findViewById(R.id.promo_title));
        break;
      default:
        throw new AssertionError("Unsupported type detected: " + viewType);
    }

    viewHolder.setupClickListeners();
    return viewHolder;
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.setTextAndIcon(mCategoryResIds[position], mIconResIds[position]);
  }

  @Override
  public int getItemCount()
  {
    return mCategoryResIds.length;
  }

  private class PromoViewHolder extends ViewHolder
  {
    @NonNull
    private final ImageView mIcon;
    @NonNull
    private final View mRemoveAds;
    @NonNull
    private final TextView mCallToActionView;

    PromoViewHolder(@NonNull View v, @NonNull TextView tv)
    {
      super(v, tv);
      mIcon = v.findViewById(R.id.promo_icon);
      mRemoveAds = v.findViewById(R.id.remove_ads);
      mCallToActionView = v.findViewById(R.id.promo_action);
      Resources res = v.getResources();
      int crossArea = res.getDimensionPixelSize(R.dimen.margin_base);
      UiUtils.expandTouchAreaForView(mRemoveAds, crossArea);
    }

    @Override
    void setupClickListeners()
    {
      View action = getView().findViewById(R.id.promo_action);
      action.setOnClickListener(this);
      mRemoveAds.setOnClickListener(new RemoveAdsClickListener());
    }

    @Override
    void onItemClicked(int position)
    {
      @StringRes
      int categoryId = mCategoryResIds[position];
      PromoCategory promo = PromoCategory.findByStringId(categoryId);
      if (promo != null)
      {
        String event = Statistics.EventName.SEARCH_SPONSOR_CATEGORY_SELECTED;
        Statistics.INSTANCE.trackSearchPromoCategory(event, promo.getProvider());
        if (mListener != null)
          mListener.onPromoCategorySelected(promo);
      }
    }

    @Override
    void setTextAndIcon(int textResId, int iconResId)
    {
      getTitle().setText(textResId);
      mIcon.setImageResource(iconResId);
      @StringRes
      int categoryId = mCategoryResIds[getAdapterPosition()];
      PromoCategory promo = PromoCategory.findByStringId(categoryId);
      if (promo != null)
      {
        mCallToActionView.setText(promo.getCallToActionText());
        String event = Statistics.EventName.SEARCH_SPONSOR_CATEGORY_SHOWN;
        Statistics.INSTANCE.trackSearchPromoCategory(event, promo.getProvider());
      }
    }

    private class RemoveAdsClickListener implements View.OnClickListener
    {
      @Override
      public void onClick(View v)
      {
        if (mListener != null)
          mListener.onAdsRemovalSelected();
      }
    }
  }

  class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
    @NonNull
    private final TextView mTitle;
    @NonNull
    private final View mView;

    ViewHolder(@NonNull View v, @NonNull TextView tv)
    {
      super(v);
      mView = v;
      mTitle = tv;
    }

    void setupClickListeners()
    {
      mView.setOnClickListener(this);
    }

    @Override
    public final void onClick(View v)
    {
      final int position = getAdapterPosition();
      onItemClicked(position);
    }

    void onItemClicked(int position)
    {
      String categoryEntryName = mResources.getResourceEntryName(mCategoryResIds[position]);
      Statistics.INSTANCE.trackSearchCategoryClicked(categoryEntryName);
      if (mListener != null)
      {
        @StringRes
        int categoryId = mCategoryResIds[position];
        mListener.onSearchCategorySelected(mResources.getString(categoryId) + " ");
      }
    }

    void setTextAndIcon(@StringRes int textResId, @DrawableRes int iconResId)
    {
      mTitle.setText(textResId);
      mTitle.setCompoundDrawablesWithIntrinsicBounds(iconResId, 0, 0, 0);
    }

    @NonNull
    TextView getTitle()
    {
      return mTitle;
    }

    @NonNull
    View getView()
    {
      return mView;
    }
  }
}
