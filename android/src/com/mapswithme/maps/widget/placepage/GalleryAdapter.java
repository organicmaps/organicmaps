package com.mapswithme.maps.widget.placepage;

import android.content.Context;
import android.graphics.Bitmap;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.animation.GlideAnimation;
import com.bumptech.glide.request.target.SimpleTarget;
import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.Image;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.List;

class GalleryAdapter extends RecyclerView.Adapter<GalleryAdapter.ViewHolder>
{
  static final int MAX_COUNT = 5;

  private final Context mContext;
  @NonNull
  private ArrayList<Image> mItems = new ArrayList<>();
  @NonNull
  private final List<Item> mLoadedItems = new ArrayList<>();
  @NonNull
  private final List<Item> mItemsToDownload = new ArrayList<>();
  @Nullable
  private RecyclerClickListener mListener;
  private final int mImageWidth;
  private final int mImageHeight;

  GalleryAdapter(Context context)
  {
    mContext = context;

    mImageWidth = (int) context.getResources().getDimension(R.dimen.placepage_hotel_gallery_width);
    mImageHeight = (int) context.getResources()
                                .getDimension(R.dimen.placepage_hotel_gallery_height);
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(mContext)
                                        .inflate(R.layout.item_gallery, parent, false), mListener);
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    Item item = mLoadedItems.get(position);
    item.setShowMore(position == MAX_COUNT - 1 && mItems.size() > MAX_COUNT);
    holder.bind(item, position);
  }

  @Override
  public int getItemCount()
  {
    return mLoadedItems.size();
  }

  @NonNull
  public ArrayList<Image> getItems()
  {
    return mItems;
  }

  public void setItems(@NonNull ArrayList<Image> items)
  {
    mItems = items;

    for (Item item : mItemsToDownload)
    {
      item.setCanceled(true);
    }
    mItemsToDownload.clear();
    mLoadedItems.clear();
    loadImages();
    notifyDataSetChanged();
  }

  public void setListener(@Nullable RecyclerClickListener listener)
  {
    mListener = listener;
  }

  private void loadImages()
  {
    int size = Math.min(mItems.size(), MAX_COUNT);

    for (int i = 0; i < size; i++)
    {
      final Item item = new Item(null);
      mItemsToDownload.add(item);
      Image image = mItems.get(i);
      Glide.with(mContext)
           .load(image.getSmallUrl())
           .asBitmap()
           .centerCrop()
           .into(new SimpleTarget<Bitmap>(mImageWidth, mImageHeight)
           {
             @Override
             public void onResourceReady(Bitmap resource,
                                         GlideAnimation<? super Bitmap> glideAnimation)
             {
               if (item.isCanceled())
                 return;

               item.setBitmap(resource);
               int size = mLoadedItems.size();
               mLoadedItems.add(item);
               notifyItemInserted(size);
             }
           });
    }
  }

  static class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
    @NonNull
    private ImageView mImage;
    @NonNull
    private View mMore;
    @Nullable
    private final RecyclerClickListener mListener;
    private int mPosition;

    public ViewHolder(View itemView, @Nullable RecyclerClickListener listener)
    {
      super(itemView);
      mListener = listener;
      mImage = (ImageView) itemView.findViewById(R.id.iv__image);
      mMore = itemView.findViewById(R.id.tv__more);
      itemView.setOnClickListener(this);
    }

    @Override
    public void onClick(View v)
    {
      if (mListener == null)
        return;

      mListener.onItemClick(v, mPosition);
    }

    public void bind(Item item, int position)
    {
      mPosition = position;
      mImage.setImageBitmap(item.getBitmap());
      UiUtils.showIf(item.isShowMore(), mMore);
    }
  }

  static class Item
  {
    @Nullable
    private Bitmap mBitmap;
    private boolean isShowMore;
    private boolean isCanceled = false;

    Item(@Nullable Bitmap bitmap)
    {
      this.mBitmap = bitmap;
    }

    @Nullable
    Bitmap getBitmap()
    {
      return mBitmap;
    }

    void setBitmap(@Nullable Bitmap bitmap)
    {
      mBitmap = bitmap;
    }

    void setShowMore(boolean showMore)
    {
      isShowMore = showMore;
    }

    boolean isShowMore()
    {
      return isShowMore;
    }

    boolean isCanceled()
    {
      return isCanceled;
    }

    void setCanceled(boolean canceled)
    {
      isCanceled = canceled;
    }
  }
}
