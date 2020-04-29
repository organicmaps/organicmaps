package com.mapswithme.maps.widget.placepage;

import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.LinearSmoothScroller;
import androidx.recyclerview.widget.LinearSnapHelper;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.OnScrollListener;
import androidx.recyclerview.widget.SnapHelper;
import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.gallery.impl.Factory;
import com.mapswithme.maps.guides.GuidesGallery;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.statistics.GalleryPlacement;

import java.util.Objects;

public class GuidesGalleryViewRenderer implements PlacePageViewRenderer<PlacePageData>,
                                                  PlacePageStateObserver
{
  private static final String EXTRA_SNAP_VIEW_POSITION = "extra_snap_view_position";
  @SuppressWarnings("NullableProblems")
  @NonNull
  private GuidesGallery mGallery;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView mRecyclerView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SnapHelper mSnapHelper;
  private int mSnapViewPosition;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView.SmoothScroller mSmoothScroller;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView.LayoutManager mLayoutManager;
  @NonNull
  private final ItemSelectedListener<GuidesGallery.Item> mItemSelectedListener
      = new ItemSelectedListener<GuidesGallery.Item>()
  {
    @Override
    public void onItemSelected(@NonNull GuidesGallery.Item item, int position)
    {
      if (position != mSnapViewPosition)
      {
        smoothScrollToPosition(position);
        return;
      }

      // if item is activated then open web catalog, otherwise - activate it.
    }

    @Override
    public void onMoreItemSelected(@NonNull GuidesGallery.Item item)
    {
      // No op.
    }

    @Override
    public void onActionButtonSelected(@NonNull GuidesGallery.Item item, int position)
    {
      // No op.
    }
  };

  private void smoothScrollToPosition(int position)
  {
    mSmoothScroller.setTargetPosition(position);
    mLayoutManager.startSmoothScroll(mSmoothScroller);
  }

  @NonNull
  private final OnScrollListener mOnScrollListener = new OnScrollListener()
  {
    @Override
    public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState)
    {
      super.onScrollStateChanged(recyclerView, newState);
      if (newState == RecyclerView.SCROLL_STATE_IDLE)
        setActiveGuide();
    }
  };

  private void setActiveGuide()
  {
    View snapView = mSnapHelper.findSnapView(mLayoutManager);
    mSnapViewPosition = snapView == null ? 0 : mLayoutManager.getPosition(snapView);
    Items.Item item = mGallery.getItems().get(mSnapViewPosition);
    Toast.makeText(mRecyclerView.getContext(), "TODO: Shift y for = " + item.getTitle(),
                   Toast.LENGTH_SHORT).show();
  }

  @Override
  public void render(@NonNull PlacePageData data)
  {
    mGallery = (GuidesGallery) data;
    GalleryAdapter adapter = Factory.createGuidesAdapter(mGallery.getItems(), mItemSelectedListener,
                                                         GalleryPlacement.MAP);
    mRecyclerView.setAdapter(adapter);
    setActiveGuide();
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
    mLayoutManager = new LinearLayoutManager(view.getContext(),
                                             LinearLayoutManager.HORIZONTAL, false);
    mRecyclerView.setLayoutManager(mLayoutManager);
    mRecyclerView.addItemDecoration(
        ItemDecoratorFactory.createSponsoredGalleryDecorator(view.getContext(),
                                                             LinearLayoutManager.HORIZONTAL));
    mSnapHelper = new LinearSnapHelper();
    mSnapHelper.attachToRecyclerView(mRecyclerView);
    mRecyclerView.addOnScrollListener(mOnScrollListener);
    mSmoothScroller = new LinearSmoothScroller(mRecyclerView.getContext())
    {
      @Override
      protected int getHorizontalSnapPreference()
      {
        return LinearSmoothScroller.SNAP_TO_ANY;
      }
    };
  }

  @Override
  public void destroy()
  {

  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    outState.putParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA, mGallery);
    outState.putInt(EXTRA_SNAP_VIEW_POSITION, mSnapViewPosition);
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    //noinspection ConstantConditions
    mGallery = inState.getParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA);
    if (mGallery == null)
      return;

    GalleryAdapter adapter = Factory.createGuidesAdapter(mGallery.getItems(), mItemSelectedListener,
                                                         GalleryPlacement.MAP);
    mRecyclerView.setAdapter(adapter);
    mSnapViewPosition = inState.getInt(EXTRA_SNAP_VIEW_POSITION);
    mRecyclerView.post(() -> {
      smoothScrollToPosition(mSnapViewPosition);
      setActiveGuide();
    });
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
