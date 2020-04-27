package com.mapswithme.maps.widget.placepage;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.impl.Factory;
import com.mapswithme.maps.guides.GuidesGallery;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.statistics.GalleryPlacement;

import java.util.Objects;

public class GuidesGalleryViewRenderer implements PlacePageViewRenderer<PlacePageData>,
                                                  PlacePageStateObserver
{
  @Nullable
  private GuidesGallery mGallery;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView mRecyclerView;

  @Override
  public void render(@NonNull PlacePageData data)
  {
    mGallery = (GuidesGallery) data;
    mRecyclerView.setAdapter(Factory.createGuidesAdapter(mGallery.getItems(), null,
                                                         GalleryPlacement.MAP));
  }

  @Override
  public void onHide()
  {

  }

  @Override
  public void initialize(@Nullable View view)
  {
    Objects.requireNonNull(view);
    mRecyclerView = view.findViewById(R.id.guides_gallery);
    mRecyclerView.setLayoutManager(new LinearLayoutManager(view.getContext(),
                                                           LinearLayoutManager.HORIZONTAL, false));
    // TODO: implement new item decoration.
    mRecyclerView.addItemDecoration(
        ItemDecoratorFactory.createSponsoredGalleryDecorator(view.getContext(),
                                                             LinearLayoutManager.HORIZONTAL));
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
