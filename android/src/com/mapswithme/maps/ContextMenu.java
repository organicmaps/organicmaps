package com.mapswithme.maps;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.webkit.WebView;

import com.mapswithme.maps.settings.SettingsActivity;


public class ContextMenu
{
  private static void onAboutDialogClicked(Activity parent)
  {
    LayoutInflater inflater = LayoutInflater.from(parent);

    View alertDialogView = inflater.inflate(R.layout.about, null);
    WebView myWebView = (WebView) alertDialogView.findViewById(R.id.webview_about);
    myWebView.loadUrl("file:///android_asset/about.html");

    new AlertDialog.Builder(parent)
    .setView(alertDialogView)
    .setTitle(R.string.about)
    .setPositiveButton(R.string.close, new DialogInterface.OnClickListener()
    {
      @Override
      public void onClick(DialogInterface dialog, int which)
      {
        dialog.cancel();
      }
    })
    .create()
    .show();
  }

  private static void onSettingsClicked(Activity parent)
  {
    parent.startActivity(new Intent(parent, SettingsActivity.class));
  }

  public static boolean onCreateOptionsMenu(Activity parent, Menu menu)
  {
    MenuInflater inflater = parent.getMenuInflater();
    inflater.inflate(R.menu.main, menu);
    return true;
  }

  public static boolean onOptionsItemSelected(Activity parent, MenuItem item)
  {
    final int id = item.getItemId();

    if (id == R.id.menuitem_about_dialog)
      onAboutDialogClicked(parent);
    else if (id == R.id.menuitem_settings_activity)
      onSettingsClicked(parent);
    else
      return false;

    return true;
  }
}
