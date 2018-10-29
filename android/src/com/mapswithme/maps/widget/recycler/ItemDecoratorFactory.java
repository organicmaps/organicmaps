package com.mapswithme.maps.widget.recycler;

import android.content.Context;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.v4.content.ContextCompat;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.R;

public class ItemDecoratorFactory
{
  @NonNull
  public static RecyclerView.ItemDecoration createHotelGalleryDecorator(@NonNull Context context,
                                                                        int orientation)
  {
    DividerItemDecoration decoration = new HotelDividerItemDecoration(context, orientation);
    decoration.setDrawable(ContextCompat.getDrawable(context, R.drawable.divider_transparent_quarter));
    return decoration;
  }

  @NonNull
  public static RecyclerView.ItemDecoration createSponsoredGalleryDecorator(@NonNull Context context,
                                                                            int orientation)
  {
    DividerItemDecoration decoration = new SponsoredDividerItemDecoration(context, orientation);
    decoration.setDrawable(ContextCompat.getDrawable(context, R.drawable.divider_transparent_half));
    return decoration;
  }

  @NonNull
  public static RecyclerView.ItemDecoration createRatingRecordDecorator(@NonNull Context context,
                                                                        int orientation,
                                                                        @DrawableRes int dividerResId)
  {
    DividerItemDecoration decoration = new DividerItemDecoration(context, orientation);
    decoration.setDrawable(ContextCompat.getDrawable(context, dividerResId));
    return decoration;
  }

  @NonNull
  public static RecyclerView.ItemDecoration createDefaultDecorator(@NonNull Context context,
                                                                   int orientation)
  {
    return new DividerItemDecoration(context, orientation);
  }

  @NonNull
  public static RecyclerView.ItemDecoration createVerticalDefaultDecorator(@NonNull Context context)
  {
    return new DividerItemDecoration(context, DividerItemDecoration.VERTICAL);
  }

  @NonNull
  public static RecyclerView.ItemDecoration createRatingRecordDecorator(@NonNull Context context,
                                                                        int horizontal)
  {
    return createRatingRecordDecorator(context, horizontal, R.drawable.divider_transparent_base);
  }
}
