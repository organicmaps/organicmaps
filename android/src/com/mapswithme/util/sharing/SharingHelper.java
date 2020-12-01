package com.mapswithme.util.sharing;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.text.TextUtils;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import com.cocosw.bottomsheet.BottomSheet;
import com.google.gson.Gson;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.BookmarkSharingResult;
import com.mapswithme.maps.dialog.DialogUtils;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public enum SharingHelper implements Initializable<Context>
{
  INSTANCE;

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = SharingHelper.class.getSimpleName();
  private static final String PREFS_STORAGE = "sharing";
  private static final String PREFS_KEY_ITEMS = "items";
  private static final String KMZ_MIME_TYPE = "application/vnd.google-earth.kmz";

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private SharedPreferences mPrefs;
  private final Map<String, SharingTarget> mItems = new HashMap<>();

  @Nullable
  private ProgressDialog mProgressDialog;

  @Override
  public void initialize(@Nullable Context context)
  {
    mContext = context;
    mPrefs = MwmApplication.from(context).getSharedPreferences(PREFS_STORAGE, Context.MODE_PRIVATE);

    ThreadPool.getStorage().execute(
        () ->
        {
          SharingTarget[] items;
          String json = mPrefs.getString(PREFS_KEY_ITEMS, null);
          items = parse(json);

          if (items != null)
          {
            for (SharingTarget item : items)
              INSTANCE.mItems.put(item.packageName, item);
          }
        });
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  private static SharingTarget[] parse(String json)
  {
    if (TextUtils.isEmpty(json))
      return null;

    try
    {
      return new Gson().fromJson(json, SharingTarget[].class);
    } catch (Exception e)
    {
      return null;
    }
  }

  private void save()
  {
    String json = new Gson().toJson(mItems.values());
    mPrefs.edit().putString(PREFS_KEY_ITEMS, json).apply();
  }

  private static String guessAppName(PackageManager pm, ResolveInfo ri)
  {
    CharSequence name = ri.activityInfo.loadLabel(pm);

    if (TextUtils.isEmpty(name))
    {
      name = ri.loadLabel(pm);

      if (TextUtils.isEmpty(name))
        name = ri.activityInfo.packageName;
    }

    return name.toString();
  }

  private List<SharingTarget> findItems(BaseShareable data)
  {
    Set<String> missed = new HashSet<>(mItems.keySet());

    Intent it = data.getTargetIntent(null);
    PackageManager pm = MwmApplication.from(mContext).getPackageManager();
    List<ResolveInfo> rlist = pm.queryIntentActivities(it, 0);

    final List<SharingTarget> res = new ArrayList<>(rlist.size());
    for (ResolveInfo ri : rlist)
    {
      ActivityInfo ai = ri.activityInfo;
      if (ai == null)
        continue;

      missed.remove(ai.packageName);
      SharingTarget target = new SharingTarget(ai.packageName);
      target.name = guessAppName(pm, ri);
      target.activityName = ai.name;

      SharingTarget original = mItems.get(ai.packageName);
      if (original != null)
        target.usageCount = original.usageCount;

      target.drawableIcon = ai.loadIcon(pm);

      res.add(target);
    }

    Collections.sort(res, SharingTarget::compareTo);

    for (String item : missed)
      mItems.remove(item);

    if (!missed.isEmpty())
      save();

    return res;
  }

  public static void shareOutside(BaseShareable data)
  {
    shareOutside(data, R.string.share);
  }

  public static void shareOutside(final BaseShareable data, @StringRes int titleRes)
  {
    shareInternal(data, titleRes, INSTANCE.findItems(data));
  }

  private static void shareInternal(final BaseShareable data, int titleRes, final List<SharingTarget> items)
  {
    final BottomSheet.Builder builder = BottomSheetHelper.createGrid(data.getActivity(), titleRes)
                                                         .limit(R.integer.sharing_initial_rows);

    int i = 0;
    for (SharingTarget item : items)
      builder.sheet(i++, item.drawableIcon, item.name);

    builder.listener((dialog, which) ->
                     {
                       if (which < 0)
                         return;

                       SharingTarget target = items.get(which);
                       INSTANCE.updateItem(target);

                       data.share(target);
                     });

    UiThread.runLater(builder::show, 500);
  }

  private void updateItem(SharingTarget item)
  {
    SharingTarget stored = mItems.get(item.packageName);
    if (stored == null)
    {
      stored = new SharingTarget(item.packageName);
      mItems.put(stored.packageName, stored);
    }

    stored.usageCount++;
    save();
  }

  public void prepareBookmarkCategoryForSharing(@NonNull Activity context, long catId)
  {
    mProgressDialog = DialogUtils.createModalProgressDialog(context, R.string.please_wait);
    mProgressDialog.show();
    BookmarkManager.INSTANCE.prepareCategoryForSharing(catId);
  }

  public void onPreparedFileForSharing(@NonNull Activity context,
                                       @NonNull BookmarkSharingResult result)
  {
    if (mProgressDialog != null && mProgressDialog.isShowing())
      mProgressDialog.dismiss();
    shareBookmarksCategory(context, result);
  }

  private static void shareBookmarksCategory(@NonNull Activity context,
                                             @NonNull BookmarkSharingResult result)
  {
    switch (result.getCode())
    {
      case BookmarkSharingResult.SUCCESS:
        String name = new File(result.getSharingPath()).getName();
        shareOutside(new LocalFileShareable(context, result.getSharingPath(), KMZ_MIME_TYPE)
                         .setText(context.getString(R.string.share_bookmarks_email_body))
                         .setSubject(R.string.share_bookmarks_email_subject));
        break;
      case BookmarkSharingResult.EMPTY_CATEGORY:
        DialogUtils.showAlertDialog(context, R.string.bookmarks_error_title_share_empty,
                                    R.string.bookmarks_error_message_share_empty);
        break;
      case BookmarkSharingResult.ARCHIVE_ERROR:
      case BookmarkSharingResult.FILE_ERROR:
        DialogUtils.showAlertDialog(context, R.string.dialog_routing_system_error,
                                    R.string.bookmarks_error_message_share_general);
        String catName = BookmarkManager.INSTANCE.getCategoryById(result.getCategoryId()).getName();
        LOGGER.e(TAG, "Failed to share bookmark category '" + catName + "', error code: "
                      + result.getCode());
        break;
      default:
        throw new AssertionError("Unsupported bookmark sharing code: " + result.getCode());
    }
  }

  public static void shareViralEditor(Activity context, @DrawableRes int imageId, @StringRes int subject, @StringRes int text)
  {
    shareOutside(new ViralEditorShareable(context, imageId)
                     .setText(text)
                     .setSubject(subject));
  }

}
