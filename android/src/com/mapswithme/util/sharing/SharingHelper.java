package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.support.annotation.DrawableRes;
import android.support.annotation.StringRes;
import android.text.TextUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.cocosw.bottomsheet.BottomSheet;
import com.google.gson.Gson;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;

public final class SharingHelper
{
  private static final String PREFS_STORAGE = "sharing";
  private static final String PREFS_KEY_ITEMS = "items";

  private static boolean sInitialized;
  private static final SharingHelper sInstance = new SharingHelper();

  private final SharedPreferences mPrefs = MwmApplication.get().getSharedPreferences(PREFS_STORAGE, Context.MODE_PRIVATE);
  private final Map<String, SharingTarget> mItems = new HashMap<>();

  private SharingHelper()
  {}

  public static void prepare()
  {
    if (sInitialized)
      return;

    sInitialized = true;

    ThreadPool.getStorage().execute(new Runnable()
    {
      @Override
      public void run()
      {
        SharingTarget[] items;
        String json = sInstance.mPrefs.getString(PREFS_KEY_ITEMS, null);
        items = parse(json);

        if (items != null)
          for (SharingTarget item : items)
            sInstance.mItems.put(item.packageName, item);
      }
    });
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
    PackageManager pm = MwmApplication.get().getPackageManager();
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

    Collections.sort(res, new Comparator<SharingTarget>()
    {
      @Override
      public int compare(SharingTarget left, SharingTarget right)
      {
        return left.compareTo(right);
      }
    });

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
    shareInternal(data, titleRes, sInstance.findItems(data));
  }

  private static void shareInternal(final BaseShareable data, int titleRes, final List<SharingTarget> items)
  {
    boolean showing = BottomSheetHelper.isShowing();
    final BottomSheet.Builder builder = BottomSheetHelper.createGrid(data.getActivity(), titleRes)
                                                         .limit(R.integer.sharing_initial_rows);

    int i = 0;
    for (SharingTarget item : items)
      builder.sheet(i++, item.drawableIcon, item.name);

    builder.listener(new DialogInterface.OnClickListener()
    {
      @Override
      public void onClick(DialogInterface dialog, int which)
      {
        if (which < 0)
          return;

        SharingTarget target = items.get(which);
        sInstance.updateItem(target);

        data.share(target);
      }
    });

    if (!showing)
    {
      builder.show();
      return;
    }

    UiThread.runLater(new Runnable()
    {
      @Override
      public void run()
      {
        builder.show();
      }
    }, 500);
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

  public static void shareBookmarksCategory(Activity context, long id)
  {
    final String path = MwmApplication.get().getTempPath() + "/";
    String name = BookmarkManager.INSTANCE.saveToKmzFile(id, path);
    if (name == null)
      return;

    shareOutside(new LocalFileShareable(context, path + name + ".kmz", "application/vnd.google-earth.kmz")
                              // TODO fix translation for some languages, that doesn't contain holder for filename
                     .setText(context.getString(R.string.share_bookmarks_email_body, name))
                     .setSubject(R.string.share_bookmarks_email_subject));
  }

  public static void shareViralEditor(Activity context, @DrawableRes int imageId, @StringRes int subject, @StringRes int text)
  {
    shareOutside(new ViralEditorShareable(context, imageId)
                     .setText(text)
                     .setSubject(subject));
  }

}