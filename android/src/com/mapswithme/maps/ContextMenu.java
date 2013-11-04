package com.mapswithme.maps;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.util.UiUtils;


public class ContextMenu
{
  private static void onAboutDialogClicked(Activity parent)
  {
    final String url = "file:///android_asset/about.html";

    final LayoutInflater inflater = LayoutInflater.from(parent);
    final View alertDialogView = inflater.inflate(R.layout.about, null);
    final WebView myWebView = (WebView) alertDialogView.findViewById(R.id.webview_about);

    myWebView.setWebViewClient(new WebViewClient()
    {
      @Override
      public void onPageFinished(WebView view, String url)
      {
        super.onPageFinished(view, url);
        UiUtils.show(myWebView);

        final AlphaAnimation aAnim = new AlphaAnimation(0, 1);
        aAnim.setDuration(750);
        myWebView.startAnimation(aAnim);
      }
    });

    String versionStr = "";
    try
    {
      versionStr = parent.getPackageManager().getPackageInfo(parent.getPackageName(), 0).versionName;
    }
    catch (final NameNotFoundException e)
    {
      e.printStackTrace();
    }

    new AlertDialog.Builder(parent)
    .setView(alertDialogView)
    .setTitle(String.format(parent.getString(R.string.version), versionStr))
    .setPositiveButton(R.string.close, new DialogInterface.OnClickListener()
    {
      @Override
      public void onClick(DialogInterface dialog, int which)
      {
        dialog.dismiss();
      }
    })
    .create()
    .show();

    myWebView.loadUrl(url);
  }

  private static void onSettingsClicked(Activity parent)
  {
    parent.startActivity(new Intent(parent, SettingsActivity.class));
  }

  public static boolean onCreateOptionsMenu(Activity parent, Menu menu)
  {
    final MenuInflater inflater = parent.getMenuInflater();
    inflater.inflate(R.menu.main, menu);
    return true;
  }

  public static boolean onOptionsItemSelected(Activity parent, MenuItem item)
  {
    final int id = item.getItemId();
    return onItemSelected(id, parent);
  }

  public static boolean onItemSelected(int itemId, Activity parent)
  {
    if (itemId == R.id.menuitem_about_dialog)
      onAboutDialogClicked(parent);
    else if (itemId == R.id.menuitem_settings_activity)
      onSettingsClicked(parent);
    else
      return false;

    return true;
  }
}
