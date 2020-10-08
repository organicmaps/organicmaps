package com.mapswithme.maps.widget.recycler;

import android.content.Context;

import androidx.annotation.DimenRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.RecyclerView;

import com.mapswithme.maps.R;

import java.util.Objects;

public class ItemDecoratorFactory
{
  @NonNull
  public static RecyclerView.ItemDecoration createHotelGalleryDecorator(@NonNull Context context,
                                                                        int orientation)
  {
    DividerItemDecoration decoration = new HotelDividerItemDecoration(context, orientation);
    @DrawableRes
    int dividerId = R.drawable.divider_transparent_quarter;
    decoration.setDrawable(Objects.requireNonNull(ContextCompat.getDrawable(context, dividerId)));
    return decoration;
  }

  @NonNull
  public static RecyclerView.ItemDecoration createSponsoredGalleryDecorator(@NonNull Context context,
                                                                            int orientation)
  {
    DividerItemDecoration decoration = new SponsoredDividerItemDecoration(context, orientation);
    @DrawableRes
    int dividerId = R.drawable.divider_transparent_half;
    decoration.setDrawable(Objects.requireNonNull(ContextCompat.getDrawable(context, dividerId)));
    return decoration;
  }

  public static RecyclerView.ItemDecoration createPlacePagePromoGalleryDecorator(@NonNull Context context,
                                                                                 int orientation)
  {
    DividerItemDecoration decoration = new SponsoredDividerItemDecoration(context, orientation);
    @DrawableRes
    int dividerId = R.drawable.divider_transparent_quarter;
    decoration.setDrawable(Objects.requireNonNull(ContextCompat.getDrawable(context, dividerId)));
    return decoration;
  }

  @NonNull
  public static RecyclerView.ItemDecoration createRatingRecordDecorator(@NonNull Context context,
                                                                        int orientation,
                                                                        @DrawableRes int dividerResId)
  {
    DividerItemDecoration decoration = new DividerItemDecoration(context, orientation);
    decoration.setDrawable(Objects.requireNonNull(ContextCompat.getDrawable(context, dividerResId)));
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
  public static RecyclerView.ItemDecoration createDecoratorWithPadding(@NonNull Context context)
  {
    @DrawableRes
    int dividerRes = R.drawable.divider_base;
    @DimenRes
    int marginDimen = R.dimen.margin_quadruple_plus_half;
    return new DividerItemDecorationWithPadding(
        Objects.requireNonNull(context.getDrawable(dividerRes)),
        context.getResources().getDimensionPixelSize(marginDimen));
  }

  @NonNull
  public static RecyclerView.ItemDecoration createRatingRecordDecorator(@NonNull Context context,
                                                                        int horizontal)
  {
    return createRatingRecordDecorator(context, horizontal, R.drawable.divider_transparent_base);
  }
}
