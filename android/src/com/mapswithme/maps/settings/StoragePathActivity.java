package com.mapswithme.maps.settings;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckedTextView;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.MapsWithMeBaseListActivity;
import com.mapswithme.util.StoragePathManager;
import com.mapswithme.util.Utils;


public class StoragePathActivity extends MapsWithMeBaseListActivity
{
  private static String TAG = "StoragePathActivity";

  private StoragePathManager.StoragePathAdapter getAdapter()
  {
    return (StoragePathManager.StoragePathAdapter)getListView().getAdapter();
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setListAdapter(MWMApplication.get().GetPathManager().GetAdapter());
  }

  @Override
  protected void onListItemClick(final ListView l, View v, final int position, long id)
  {
    // Do not process clicks on header items.
    if (position != 0)
      getAdapter().onItemClick(position);
  }
  
  @Override
  protected void onResume()
  {
    super.onResume();
    MWMApplication.get().GetPathManager().StartExtStorageWatching(this, null);
  }
  
  @Override
  protected void onPause()
  {
    super.onPause();
    MWMApplication.get().GetPathManager().StopExtStorageWatching();
  }
}
