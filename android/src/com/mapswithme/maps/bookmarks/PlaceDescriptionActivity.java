package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseToolbarActivity;

public class PlaceDescriptionActivity extends BaseToolbarActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return PlaceDescriptionFragment.class;
  }

  public static void start(@NonNull Context context, @NonNull String description)
  {
    Intent intent = new Intent(context, PlaceDescriptionActivity.class)
        .putExtra(PlaceDescriptionFragment.EXTRA_DESCRIPTION, description);
    context.startActivity(intent);
  }
}
