package app.organicmaps.search;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Build;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.sdk.search.DisplayedCategories;
import app.organicmaps.sdk.util.Language;
import app.organicmaps.util.ThemeUtils;

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
    void onSearchCategorySelected(String category);
  }

  private final CategoriesUiListener mListener;

  CategoriesAdapter(@NonNull Fragment fragment)
  {
    mListener = (CategoriesUiListener) fragment;
    mResources = fragment.getResources();
    mInflater = LayoutInflater.from(fragment.requireActivity());
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

    private final boolean mIsLangSupported;
    private Resources mEnglishResources;  // Lazy-initialized

    // Get locale/language of the translations that are used in the app UI and are visible to the user.
    // Handles cases when the primary system language translation is not supported by OM yet.
    // private @NonNull String getResourcesLanguage()
    // {
    //   final Configuration c = mResources.getConfiguration();
    //   return Build.VERSION.SDK_INT >= Build.VERSION_CODES.N ? c.getLocales().get(0).toString() : c.locale.toString();
    // }

    private @NonNull String getEnglishString(@StringRes int categoryId)
    {
      // Not thread safe, but we don't care, as it should always run on the same thread.
      if (mEnglishResources == null)
      {
        final Configuration newConfig = new Configuration(mResources.getConfiguration());
        newConfig.setLocale(new Locale("en"));
        final Context localizedContext = mInflater.getContext().createConfigurationContext(newConfig);
        mEnglishResources = localizedContext.getResources();
      }
      return mEnglishResources.getString(categoryId);
    }

    ViewHolder(@NonNull View v, @NonNull TextView tv)
    {
      super(v);
      mView = v;
      mTitle = tv;

      // TODO(AB): Change Language.getDefaultLocale() to getResourcesLanguage() and pass proper language to the search.
      mIsLangSupported = DisplayedCategories.nativeIsLangSupported(Language.getDefaultLocale());
    }

    void setupClickListeners()
    {
      mView.setOnClickListener(this);
    }

    @Override
    public final void onClick(View v)
    {
      final int position = getBindingAdapterPosition();

      @StringRes
      final int categoryId = mCategoryResIds[position];

      /// @todo Pass the correct input language. Now the Core always matches "en" together with "m_inputLocale".
      /// We expect that Language.getDefaultLocale() will be called further inside.
      if (mIsLangSupported)
        mListener.onSearchCategorySelected(mResources.getString(categoryId) + " "/*, getResourcesLanguage()*/);
      else
        mListener.onSearchCategorySelected(getEnglishString(categoryId) + " "/*, "en"*/);
    }

    void setTextAndIcon(@StringRes int textResId, @DrawableRes int iconResId)
    {
      mTitle.setText(textResId);
      mTitle.setCompoundDrawablesRelativeWithIntrinsicBounds(iconResId, 0, 0, 0);
    }
  }
}
