package com.mapswithme.maps.search;

import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.ColorRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

class HotelsTypeAdapter extends RecyclerView.Adapter<HotelsTypeAdapter.HotelsTypeViewHolder>
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = HotelsTypeAdapter.class.getName();

  @NonNull
  private static final HotelsFilter.HotelType[] TYPES = SearchEngine.nativeGetHotelTypes();

  @Nullable
  private final OnTypeSelectedListener mListener;
  @NonNull
  private final List<Item> mItems;

  HotelsTypeAdapter(@Nullable OnTypeSelectedListener listener)
  {
    mListener = listener;
    mItems = new ArrayList<>();
    for (HotelsFilter.HotelType type : TYPES)
      mItems.add(new Item(type));
  }

  @Override
  public HotelsTypeViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    return new HotelsTypeViewHolder(inflater.inflate(R.layout.item_tag, parent, false), mItems,
                                    mListener);
  }

  @Override
  public void onBindViewHolder(HotelsTypeViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  void updateItems(@NonNull Set<HotelsFilter.HotelType> selectedTypes)
  {
    for (Item item : mItems)
      item.mSelected = selectedTypes.contains(item.mType);

    notifyDataSetChanged();
  }

  static class HotelsTypeViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
    @NonNull
    private final View mFrame;
    @NonNull
    private final TextView mTitle;
    @NonNull
    private final List<Item> mItems;
    @Nullable
    private OnTypeSelectedListener mListener;

    HotelsTypeViewHolder(@NonNull View itemView, @NonNull List<Item> items,
                         @Nullable OnTypeSelectedListener listener)
    {
      super(itemView);

      mFrame = itemView;
      mItems = items;
      mListener = listener;
      mTitle = (TextView) itemView.findViewById(R.id.tv__tag);
      mFrame.setOnClickListener(this);
    }

    void bind(@NonNull Item item)
    {
      mTitle.setText(getStringResourceByTag(item.mType.getTag()));
      mFrame.setSelected(item.mSelected);
      updateTitleColor();
    }

    @Override
    public void onClick(View v)
    {
      int position = getAdapterPosition();
      if(position == RecyclerView.NO_POSITION)
        return;

      mFrame.setSelected(!mFrame.isSelected());
      updateTitleColor();
      Item item = mItems.get(position);
      item.mSelected = mFrame.isSelected();
      if (mListener != null)
        mListener.onTypeSelected(item.mSelected, item.mType);
    }

    @NonNull
    private String getStringResourceByTag(@NonNull String tag)
    {
      try
      {
        Context context = mFrame.getContext();
        Resources resources = context.getResources();
        String searchHotelFilter = resources.getString(R.string.search_hotel_filter);
        return resources.getString(resources.getIdentifier(String.format(searchHotelFilter, tag),
                                                    "string",
                                                    context.getPackageName()));
      }
      catch (Resources.NotFoundException e)
      {
        LOGGER.e(TAG, "Not found resource for hotel tag " + tag, e);
      }

      return tag;
    }

    private void updateTitleColor()
    {
      boolean select = mFrame.isSelected();
      @ColorRes
      int titleColor =
          select ? UiUtils.getStyledResourceId(mFrame.getContext(), R.attr.accentButtonTextColor)
                 : UiUtils.getStyledResourceId(mFrame.getContext(), android.R.attr.textColorPrimary);
      mTitle.setTextColor(ContextCompat.getColor(mFrame.getContext(), titleColor));
    }
  }

  interface OnTypeSelectedListener
  {
    void onTypeSelected(boolean selected, @NonNull HotelsFilter.HotelType type);
  }

  static class Item
  {
    @NonNull
    private final HotelsFilter.HotelType mType;
    private boolean mSelected;

    Item(@NonNull HotelsFilter.HotelType type)
    {
      mType = type;
    }
  }
}
