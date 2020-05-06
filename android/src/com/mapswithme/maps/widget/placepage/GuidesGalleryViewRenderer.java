package com.mapswithme.maps.widget.placepage;

import android.content.res.Resources;
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
import com.mapswithme.maps.gallery.impl.Factory;
import com.mapswithme.maps.guides.GuidesGallery;
import com.mapswithme.maps.guides.GuidesGallery.Item;
import com.mapswithme.maps.maplayer.guides.GuidesManager;
import com.mapswithme.maps.maplayer.guides.OnGuidesGalleryChangedListener;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.statistics.GalleryPlacement;

import java.util.List;
import java.util.Objects;

public class GuidesGalleryViewRenderer implements PlacePageViewRenderer<PlacePageData>,
                                                  PlacePageStateObserver,
                                                  OnGuidesGalleryChangedListener
{
  private static final String EXTRA_ACTIVE_POSITION = "extra_active_position";
  @Nullable
  private GuidesGallery mGallery;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView mRecyclerView;
  @Nullable
  private Item mActiveItem;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView.SmoothScroller mSmoothScroller;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private LinearLayoutManager mLayoutManager;
  @NonNull
  private final ItemSelectedListener<Item> mItemSelectedListener
      = new ItemSelectedListener<Item>()
  {
    @Override
    public void onItemSelected(@NonNull Item item, int position)
    {
      if (item == mActiveItem)
      {
        Toast.makeText(mRecyclerView.getContext(), "Open catalog coming soon",
                       Toast.LENGTH_SHORT).show();
        return;
      }

      if (isItemAtPositionCompletelyVisible(position))
      {
        setActiveGuide(position);
        return;
      }

      smoothScrollToPosition(position);
    }

    @Override
    public void onMoreItemSelected(@NonNull Item item)
    {
      // No op.
    }

    @Override
    public void onActionButtonSelected(@NonNull Item item, int position)
    {
      // No op.
    }
  };
  @Nullable
  private GalleryAdapter mAdapter;

  @NonNull
  private final OnScrollListener mOnScrollListener = new OnScrollListener()
  {
    @Override
    public void onScrollStateChanged(@NonNull RecyclerView recyclerView, int newState)
    {
      super.onScrollStateChanged(recyclerView, newState);
      if (newState == RecyclerView.SCROLL_STATE_IDLE)
      {
        int activePosition;
        int firstVisiblePosition = mLayoutManager.findFirstCompletelyVisibleItemPosition();
        int lastVisiblePosition = mLayoutManager.findLastCompletelyVisibleItemPosition();
        if (mTargetPosition != RecyclerView.NO_POSITION)
        {
          if (mTargetPosition == firstVisiblePosition)
            activePosition = mTargetPosition;
          else if (mTargetPosition == lastVisiblePosition)
            activePosition = mTargetPosition;
          else
            activePosition = firstVisiblePosition;
          mTargetPosition = RecyclerView.NO_POSITION;
        }
        else
        {
          activePosition = firstVisiblePosition != RecyclerView.NO_POSITION ? firstVisiblePosition
                                                                            : lastVisiblePosition;
        }

        if (activePosition == RecyclerView.NO_POSITION)
          return;

        setActiveGuide(activePosition);
      }
    }
  };

  private int mActivePosition = RecyclerView.NO_POSITION;
  private int mTargetPosition = RecyclerView.NO_POSITION;

  private void smoothScrollToPosition(int position)
  {
    mSmoothScroller.setTargetPosition(position);
    mTargetPosition = position;
    mLayoutManager.startSmoothScroll(mSmoothScroller);
  }

  private boolean isItemAtPositionCompletelyVisible(int position)
  {
    int firstVisiblePosition = mLayoutManager.findFirstCompletelyVisibleItemPosition();
    if (firstVisiblePosition == RecyclerView.NO_POSITION)
      return false;

    int lastVisiblePosition = mLayoutManager.findLastCompletelyVisibleItemPosition();
    if (lastVisiblePosition == RecyclerView.NO_POSITION)
      return false;

    return position >= firstVisiblePosition && position <= lastVisiblePosition;
  }

  private void setActiveGuide(int position)
  {
    Objects.requireNonNull(mGallery);
    mActivePosition = position;
    Item item = mGallery.getItems().get(position);
    if (mActiveItem == item)
      return;

    if (mActiveItem != null)
      mActiveItem.setActivated(false);

    mActiveItem = item;
    mActiveItem.setActivated(true);
    if (mAdapter != null)
      mAdapter.notifyDataSetChanged();
    GuidesManager.from(mRecyclerView.getContext()).setActiveGuide(mActiveItem.getGuideId());
  }

  @Override
  public void render(@NonNull PlacePageData data)
  {
    mGallery = (GuidesGallery) data;
    mAdapter = Factory.createGuidesAdapter(mGallery.getItems(), mItemSelectedListener,
                                                         GalleryPlacement.MAP);
    mRecyclerView.setAdapter(mAdapter);
  }

  @Override
  public void onHide()
  {
    mActiveItem = null;
    mActivePosition = 0;
    smoothScrollToPosition(0);
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
    mRecyclerView.addOnScrollListener(mOnScrollListener);
    mSmoothScroller = new LinearSmoothScroller(mRecyclerView.getContext())
    {
      @Override
      protected int getHorizontalSnapPreference()
      {
        return LinearSmoothScroller.SNAP_TO_ANY;
      }
    };

    Resources res = mRecyclerView.getContext().getResources();
    if (res.getBoolean(R.bool.guides_gallery_enable_snapping))
    {
      SnapHelper mSnapHelper = new LinearSnapHelper();
      mSnapHelper.attachToRecyclerView(mRecyclerView);
    }
    GuidesManager.from(mRecyclerView.getContext()).addGalleryChangedListener(this);
  }

  @Override
  public void destroy()
  {
    GuidesManager.from(mRecyclerView.getContext()).removeGalleryChangedListener(this);
  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    outState.putParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA, mGallery);
    outState.putInt(EXTRA_ACTIVE_POSITION, mActivePosition);
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    mGallery = inState.getParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA);
    if (mGallery == null)
      return;

    mAdapter = Factory.createGuidesAdapter(mGallery.getItems(), mItemSelectedListener,
                                           GalleryPlacement.MAP);
    mRecyclerView.setAdapter(mAdapter);
    mActivePosition = inState.getInt(EXTRA_ACTIVE_POSITION);
    mRecyclerView.post(() -> {
      smoothScrollToPosition(mActivePosition);
      setActiveGuide(mActivePosition);
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
    // Do nothing.
  }

  @Override
  public void onPlacePagePreview()
  {
    // Do nothing.
  }

  @Override
  public void onPlacePageClosed()
  {
    // Do nothing.
  }

  @Override
  public void onGuidesGalleryChanged(boolean reloadGallery)
  {
    if (mGallery == null)
      return;

    GuidesManager manager = GuidesManager.from(mRecyclerView.getContext());
    if (reloadGallery)
    {
      mGallery = manager.getGallery();
      render(mGallery);
    }

    String guideId = manager.getActiveGuide();
    int activePosition = findPositionByGuideId(mGallery, guideId);
    smoothScrollToPosition(activePosition);
  }

  private static int findPositionByGuideId(@NonNull GuidesGallery gallery, @NonNull String guideId)
  {
    List<Item> items = gallery.getItems();
    for(int i = 0; i < items.size(); i++)
    {
      Item item = items.get(i);
      if (item.getGuideId().equals(guideId))
        return i;
    }

    throw new IllegalStateException("Guide with id '" + guideId + "' not found in gallery!");
  }
}
