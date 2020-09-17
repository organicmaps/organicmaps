package com.mapswithme.maps.bookmarks;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import com.mapswithme.maps.R;

public interface BookmarkCategoriesPageResProvider
{
  @StringRes
  int getHeaderText();

  @StringRes
  int getFooterText();

  @DrawableRes
  int getFooterImage();

  @NonNull
  Button getHeaderBtn();

  class Default implements BookmarkCategoriesPageResProvider
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
      return R.string.guides;
    }

    @Override
    public int getFooterImage()
    {
      return R.drawable.ic_download;
    }

    @Override
    public int getFooterText()
    {
      return R.string.download_guides;
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
