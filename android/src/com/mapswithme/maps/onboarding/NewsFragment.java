package com.mapswithme.maps.onboarding;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.downloader.UpdaterDialogFragment;
import com.mapswithme.maps.purchase.BookmarkAllSubscriptionData;
import com.mapswithme.maps.purchase.BookmarksAllSubscriptionActivity;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.util.Counters;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.UTM;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

import static com.mapswithme.maps.purchase.BookmarksAllSubscriptionPage.BOOKMARKS;
import static com.mapswithme.maps.purchase.BookmarksAllSubscriptionPage.ELEVATION;
import static com.mapswithme.maps.purchase.BookmarksAllSubscriptionPage.GUIDES;
import static com.mapswithme.maps.purchase.BookmarksAllSubscriptionPage.LONELY;

public class NewsFragment extends BaseNewsFragment implements AlertDialogCallback
{
  private class Adapter extends BaseNewsFragment.Adapter
  {
    @Override
    int getTitleKeys()
    {
      return TITLE_KEYS;
    }

    @Override
    int getSubtitles1()
    {
      return R.array.news_messages_1;
    }

    @Override
    int getSubtitles2()
    {
      return R.array.news_messages_2;
    }

    @Override
    int getButtonLabels()
    {
      return R.array.news_button_labels;
    }

    @Override
    int getButtonLinks()
    {
      return R.array.news_button_links;
    }

    @Override
    int getSwitchTitles()
    {
      return R.array.news_switch_titles;
    }

    @Override
    int getSwitchSubtitles()
    {
      return R.array.news_switch_subtitles;
    }

    @Override
    int getImages()
    {
      return R.array.news_images;
    }

    @Override
    void onPromoButtonClicked(@NonNull View view)
    {
      UiUtils.hide(view);
      BookmarksAllSubscriptionActivity.startForResult(NewsFragment.this,
                                                      PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION,
                                                      Statistics.ParamValue.WHATSNEW,
                                                      new BookmarkAllSubscriptionData(LONELY,
                                                                                      GUIDES,
                                                                                      BOOKMARKS,
                                                                                      ELEVATION));
    }
  }

  @Override
  BaseNewsFragment.Adapter createAdapter()
  {
    return new Adapter();
  }

  @Override
  protected void onDoneClick()
  {
    if (!UpdaterDialogFragment.showOn(getActivity(), getListener()))
      super.onDoneClick();
    else
      dismissAllowingStateLoss();
  }

  /**
   * Displays "What's new" dialog on given {@code activity}. Or not.
   * @return whether "What's new" dialog should be shown.
   */
  public static boolean showOn(@NonNull FragmentActivity activity,
                               final @Nullable NewsDialogListener listener)
  {
    Context context = activity.getApplicationContext();

    if (Counters.getFirstInstallVersion(context) >= BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    Fragment f = fm.findFragmentByTag(UpdaterDialogFragment.class.getName());
    if (f != null)
      return UpdaterDialogFragment.showOn(activity, listener);

    f = fm.findFragmentByTag(NewsFragment.class.getName());
    if (f != null)
    {
      NewsFragment newsFragment = (NewsFragment) f;
      newsFragment.resetListener(listener);
      return true;
    }

    String currentTitle = getCurrentTitleConcatenation(context);
    String oldTitle = SharedPropertiesUtils.getWhatsNewTitleConcatenation(context);
    if (currentTitle.equals(oldTitle) && !recreate(activity, NewsFragment.class))
      return false;

    create(activity, NewsFragment.class, listener);

    Counters.setWhatsNewShown(context);
    SharedPropertiesUtils.setWhatsNewTitleConcatenation(context, currentTitle);
    Counters.setShowReviewForOldUser(context, true);

    return true;
  }

  @NonNull
  private static String getCurrentTitleConcatenation(@NonNull Context context)
  {
    String[] keys = context.getResources().getStringArray(TITLE_KEYS);
    final int length = keys.length;
    if (length == 0)
      return "";

    StringBuilder sb = new StringBuilder("");
    for (String key : keys)
      sb.append(key);

    return sb.toString().trim();
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (resultCode != Activity.RESULT_OK)
      return;

    switch (requestCode)
    {
      case PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION:
        PurchaseUtils.showSubscriptionSuccessDialog(this,
                                                    PurchaseUtils.DIALOG_TAG_BMK_SUBSCRIPTION_SUCCESS,
                                                    PurchaseUtils.REQ_CODE_BMK_SUBS_SUCCESS_DIALOG);
        break;
      case BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY:
        if (data == null)
          break;

        BookmarkCategory category
            = data.getParcelableExtra(BookmarksCatalogActivity.EXTRA_DOWNLOADED_CATEGORY);

        if (category == null)
          throw new IllegalArgumentException("Category not found in bundle");
        Framework.nativeShowBookmarkCategory(category.getId());
        break;
    }
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    if (requestCode == PurchaseUtils.REQ_CODE_BMK_SUBS_SUCCESS_DIALOG)
      BookmarksCatalogActivity.startForResult(this,
                                              BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                              BookmarkManager.INSTANCE.getCatalogFrontendUrl(UTM.UTM_NONE));
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    // No op.
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    // No op.
  }
}
