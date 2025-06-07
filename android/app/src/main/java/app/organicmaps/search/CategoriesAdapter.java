package app.organicmaps.search;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.sdk.search.DisplayedCategories;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.Language;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Locale;

class CategoriesAdapter extends RecyclerView.Adapter<CategoriesAdapter.ViewHolder>
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ ViewType.CATEGORY })
  @interface ViewType {
    int CATEGORY = 0;
  }

  @StringRes
  private int[] mCategoryResIds;
  @DrawableRes
  private int[] mIconResIds;

  private final LayoutInflater mInflater;
  private final Resources mResources;

  interface CategoriesUiListener
  {
    void onSearchCategorySelected(@Nullable String category);
  }

  private CategoriesUiListener mListener = null;
  private boolean mIsLangSupported;
  private Resources mEnglishResources;

  CategoriesAdapter(@NonNull Fragment fragment)
  {
    mResources = fragment.getResources();
    mInflater = LayoutInflater.from(fragment.requireActivity());

    if (fragment instanceof CategoriesUiListener)
    {
      mListener = (CategoriesUiListener) fragment;
      mIsLangSupported = DisplayedCategories.nativeIsLangSupported(Language.getDefaultLocale());

      Configuration config = new Configuration(mResources.getConfiguration());
      config.setLocale(new Locale("en"));
      Context localizedContext = fragment.getContext().createConfigurationContext(config);
      mEnglishResources = localizedContext.getResources();
    }
  }

  void updateCategories(@NonNull Fragment fragment)
  {
    final Activity activity = fragment.requireActivity();
    final String packageName = activity.getPackageName();

    final String[] keys = DisplayedCategories.nativeGetKeys();
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

  @SuppressLint("DiscouragedApi")
  @StringRes
  private static int getStringResIdByKey(@NonNull Resources resources, @NonNull String packageName,
                                         @NonNull String key)
  {
    return resources.getIdentifier(key, "string", packageName);
  }

  @SuppressLint("DiscouragedApi")
  @DrawableRes
  private static int getDrawableResIdByKey(@NonNull Context context,
                                           @NonNull String packageName,
                                           @NonNull String key)
  {
    final boolean isNightTheme = ThemeUtils.isNightTheme(context);
    String iconId = "ic_" + key;
    if (isNightTheme)
      iconId = iconId + "_night";
    return context.getResources().getIdentifier(iconId, "drawable", packageName);
  }

  @Override
  @ViewType
  public int getItemViewType(int position)
  {
    return ViewType.CATEGORY;
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, @ViewType int viewType)
  {
    View view;
    ViewHolder viewHolder;
    if (viewType == ViewType.CATEGORY)
    {
      view = mInflater.inflate(R.layout.item_search_category, parent, false);
      viewHolder = new ViewHolder(view, (TextView) view);
    }
    else
    {
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
      final int position = getBindingAdapterPosition();
      onItemClicked(position);
    }

    void onItemClicked(int position)
    {
      if (mListener != null)
      {
        @StringRes
        int categoryId = mCategoryResIds[position];

        String category = mIsLangSupported ? mResources.getString(categoryId) : mEnglishResources.getString(categoryId);

        /// @todo Pass the correct input language. Now the Core always matches "en" together with "m_inputLocale".
        /// We expect that Language.getDefaultLocale() will be called further inside.
        mListener.onSearchCategorySelected(category + " ");
      }
    }

    void setTextAndIcon(@StringRes int textResId, @DrawableRes int iconResId)
    {
      mTitle.setText(textResId);
      mTitle.setCompoundDrawablesRelativeWithIntrinsicBounds(iconResId, 0, 0, 0);
    }

  }
}
