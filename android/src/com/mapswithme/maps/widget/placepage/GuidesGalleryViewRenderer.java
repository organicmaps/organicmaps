package com.mapswithme.maps.widget.placepage;

import android.content.res.Resources;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;

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
import com.mapswithme.maps.guides.GuidesGalleryListener;
import com.mapswithme.maps.maplayer.guides.GuidesManager;
import com.mapswithme.maps.maplayer.guides.OnGuidesGalleryChangedListener;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Destination;
import com.mapswithme.util.statistics.GalleryPlacement;
import com.mapswithme.util.statistics.GalleryState;
import com.mapswithme.util.statistics.GalleryType;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;
import java.util.Objects;

public class GuidesGalleryViewRenderer implements PlacePageViewRenderer<PlacePageData>,
                                                  PlacePageStateListener,
                                                  OnGuidesGalleryChangedListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = GuidesGalleryViewRenderer.class.getSimpleName();
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
        String url = mActiveItem.getUrl();
        if (!TextUtils.isEmpty(url) && mGalleryListener != null)
        {
          mGalleryListener.onGalleryGuideSelected(url);
          Statistics.INSTANCE.trackGalleryProductItemSelected(GalleryType.PROMO,
                                                              GalleryPlacement.MAP, position,
                                                              Destination.CATALOGUE);
        }
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
  @Nullable
  private final GuidesGalleryListener mGalleryListener;

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

  private int mTargetPosition = RecyclerView.NO_POSITION;

  GuidesGalleryViewRenderer(@Nullable GuidesGalleryListener galleryListener)
  {
    mGalleryListener = galleryListener;
  }

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
    if (mActiveItem.isDownloaded())
      Statistics.INSTANCE.trackGalleryUserItemShown(GalleryType.PROMO, GalleryState.ONLINE,
                                                    GalleryPlacement.MAP, position,
                                                    mActiveItem.getGuideId());
  }

  @Override
  public void render(@NonNull PlacePageData data)
  {
    mGallery = (GuidesGallery) data;
    setAdapterForGallery(mGallery);
    GuidesManager manager = GuidesManager.from(mRecyclerView.getContext());
    String guideId = manager.getActiveGuide();
    scrollToActiveGuide(mGallery, guideId, false);
  }

  private void setAdapterForGallery(@NonNull GuidesGallery gallery)
  {
    mAdapter = Factory.createGuidesAdapter(gallery.getItems(), mItemSelectedListener,
                                           GalleryPlacement.MAP);
    mRecyclerView.setAdapter(mAdapter);
  }

  @Override
  public void onHide()
  {
    // Do nothing.
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
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    mGallery = inState.getParcelable(PlacePageUtils.EXTRA_PLACE_PAGE_DATA);
    if (mGallery == null)
      return;

    setAdapterForGallery(mGallery);
    GuidesManager manager = GuidesManager.from(mRecyclerView.getContext());
    String guideId = manager.getActiveGuide();
    scrollToActiveGuide(mGallery, guideId, false);
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
    LOGGER.d(TAG, "Guides gallery changed, reloadGallery: " + reloadGallery);
    if (mGallery == null)
      return;

    GuidesManager manager = GuidesManager.from(mRecyclerView.getContext());
    if (reloadGallery)
    {
      mGallery = manager.getGallery();
      setAdapterForGallery(mGallery);
    }

    String guideId = manager.getActiveGuide();
    scrollToActiveGuide(mGallery, guideId, true);
  }

  private void scrollToActiveGuide(@NonNull GuidesGallery gallery, @NonNull String guideId,
                                   boolean animate)
  {
    int activePosition = findPositionByGuideId(gallery, guideId);
    if (activePosition == RecyclerView.NO_POSITION)
      return;

    if (animate)
    {
      mRecyclerView.post(() -> smoothScrollToPosition(activePosition));
      return;
    }

    setActiveGuide(activePosition);
    mLayoutManager.scrollToPosition(activePosition);
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

    return RecyclerView.NO_POSITION;
  }
}
