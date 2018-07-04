package com.mapswithme.maps.bookmarks;

import android.support.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class Constants
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ CATEGORIES_PAGE_PRIVATE, CATEGORIES_PAGE_CATALOG })

  public @interface CategoriesPage {}

  public static final int CATEGORIES_PAGE_PRIVATE = 0;
  public static final int CATEGORIES_PAGE_CATALOG = 1;

  final static String ARG_CATEGORIES_PAGE = "arg_categories_page";
}
