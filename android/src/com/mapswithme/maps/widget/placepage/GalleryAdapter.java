package com.mapswithme.maps.widget.placepage;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.animation.GlideAnimation;
import com.bumptech.glide.request.target.SimpleTarget;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.util.UiUtils;

import android.content.Context;
import android.graphics.Bitmap;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import java.util.ArrayList;
import java.util.List;

public class GalleryAdapter extends RecyclerView.Adapter<GalleryAdapter.ViewHolder> {
  private static final int MAX_COUNT = 5;

  private final Context mContext;
  private List<SponsoredHotel.Image> mItems = new ArrayList<>();
  private final List<Item> mLoadedItems = new ArrayList<>();
  private RecyclerClickListener mListener;
  private final int mImageWidth;
  private final int mImageHeight;
  private final Object mMutex = new Object();
  private final List<DownloadState> mDownloadStates = new ArrayList<>();

  public GalleryAdapter(Context context) {
    mContext = context;

    mImageWidth = (int)context.getResources().getDimension(R.dimen.placepage_hotel_gallery_width);
    mImageHeight = (int)context.getResources().getDimension(R.dimen.placepage_hotel_gallery_height);
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
    return new ViewHolder(LayoutInflater.from(mContext)
            .inflate(R.layout.item_gallery, parent, false), mListener);
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position) {
    Item item = mLoadedItems.get(position);
    item.setShowMore(position == MAX_COUNT - 1);
    holder.bind(item, position);
  }

  @Override
  public int getItemCount() {
    synchronized (mMutex) {
      return mLoadedItems.size();
    }
  }

  public void setItems(List<SponsoredHotel.Image> items) {
    mItems = items;
    synchronized (mMutex) {
      mLoadedItems.clear();
      for (DownloadState state: mDownloadStates) {
        state.isCanceled = true;
      }
      mDownloadStates.clear();
    }
    loadImages();
    notifyDataSetChanged();
  }

  public void setListener(RecyclerClickListener listener) {
    mListener = listener;
  }

  private void loadImages() {
    int size = mItems.size();
    if (size > MAX_COUNT) {
      size = MAX_COUNT;
    }

    for (int i = 0; i < size; i++) {
      final DownloadState state;
      synchronized (mMutex) {
        state = new DownloadState();
        mDownloadStates.add(state);
      }
      SponsoredHotel.Image image = mItems.get(i);
      Glide.with(mContext)
              .load(image.getUrl())
              .asBitmap()
              .centerCrop()
              .into(new SimpleTarget<Bitmap>(mImageWidth, mImageHeight) {
                @Override
                public void onResourceReady(Bitmap resource,
                        GlideAnimation<? super Bitmap> glideAnimation) {
                  synchronized (mMutex) {
                    if (state.isCanceled) {
                      return;
                    }
                    int size = mLoadedItems.size();
                    mLoadedItems.add(new Item(resource));
                    notifyItemInserted(size);
                  }
                }
              });
    }
  }

  static class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
    private ImageView mImage;
    private View mMore;
    private final RecyclerClickListener mListener;
    private int mPosition;

    public ViewHolder(View itemView, RecyclerClickListener listener) {
      super(itemView);
      mListener = listener;
      mImage = (ImageView) itemView.findViewById(R.id.iv__image);
      mMore = itemView.findViewById(R.id.tv__more);
      itemView.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
      if (mListener == null) {
        return;
      }

      mListener.onItemClick(v, mPosition);
    }

    public void bind(Item item, int position) {
      mPosition = position;
      mImage.setImageBitmap(item.getBitmap());
      if (item.isShowMore()) {
        UiUtils.show(mMore);
      } else {
        UiUtils.hide(mMore);
      }
    }
  }

  static class Item {
    private final Bitmap mBitmap;
    private boolean isShowMore;

    Item(Bitmap bitmap) {
      this.mBitmap = bitmap;
    }

    public Bitmap getBitmap() {
      return mBitmap;
    }

    public void setShowMore(boolean showMore) {
      isShowMore = showMore;
    }

    public boolean isShowMore() {
      return isShowMore;
    }
  }

  static class DownloadState {
    boolean isCanceled = false;
  }
}
