package com.mapswithme.maps.bookmarks;

import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;

import com.mapswithme.maps.R;

public interface AdapterResourceProvider
{
  @StringRes
  int getHeaderText();

  @StringRes
  int getFooterText();

  @DrawableRes
  int getFooterImage();

  @NonNull
  Button getHeaderBtn();

  class Default implements AdapterResourceProvider
  {
    @NonNull
    private final Button mBtn;

    public Default(@NonNull Button btn)
    {
      mBtn = btn;
    }

    public Default()
    {
      this(new Button());
    }

    @Override
    public int getHeaderText()
    {
      return R.string.bookmarks_groups;
    }

    @Override
    public int getFooterText()
    {
      return R.string.bookmarks_create_new_group;
    }

    @Override
    public int getFooterImage()
    {
      return R.drawable.ic_checkbox_add;
    }

    @NonNull
    @Override
    public Button getHeaderBtn()
    {
      return mBtn;
    }
  }

  class Catalog extends Default
  {
    @Override
    public int getHeaderText()
    {
      return R.string.bookmarks_groups_cached;
    }

    @Override
    public int getFooterImage()
    {
      return R.drawable.ic_download;
    }

    @Override
    public int getFooterText()
    {
      return R.string.downloader_download_routers;
    }
  }

  class Button
  {
    @StringRes
    public int getSelectModeText()
    {
      return R.string.bookmarks_groups_show_all;
    }

    @StringRes
    public int getUnSelectModeText()
    {
      return R.string.bookmarks_groups_hide_all;
    }
  }
}
