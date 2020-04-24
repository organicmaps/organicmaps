package com.mapswithme.maps.widget.placepage;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.guides.GuidesGallery;

public class GuidesGalleryViewRenderer implements PlacePageViewRenderer<PlacePageData>,
                                                  PlacePageStateObserver
{
  @Nullable
  private GuidesGallery mGallery;

  @Override
  public void render(@NonNull PlacePageData data)
  {
    mGallery = (GuidesGallery) data;
  }

  @Override
  public void onHide()
  {

  }

  @Override
  public void initialize(@Nullable View view)
  {

  }

  @Override
  public void destroy()
  {

  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {

  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {

  }

  @Override
  public boolean support(@NonNull PlacePageData data)
  {
    return data instanceof GuidesGallery;
  }

  @Override
  public void onPlacePageDetails()
  {

  }

  @Override
  public void onPlacePagePreview()
  {

  }

  @Override
  public void onPlacePageClosed()
  {

  }
}
