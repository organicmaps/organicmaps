package com.mapswithme.maps.widget.placepage;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

import java.util.ArrayList;
import java.util.List;

class FacilitiesAdapter extends RecyclerView.Adapter<FacilitiesAdapter.ViewHolder>
{
  static final int MAX_COUNT = 6;

  @NonNull
  private List<Sponsored.FacilityType> mItems = new ArrayList<>();
  private boolean isShowAll = false;

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(parent.getContext())
                                        .inflate(R.layout.item_facility, parent, false));
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public int getItemCount()
  {
    if (mItems.size() > MAX_COUNT && !isShowAll)
    {
      return MAX_COUNT;
    }
    return mItems.size();
  }

  public void setItems(@NonNull List<Sponsored.FacilityType> items)
  {
    this.mItems = items;
    notifyDataSetChanged();
  }

  void setShowAll(boolean showAll)
  {
    isShowAll = showAll;
    notifyDataSetChanged();
  }

  boolean isShowAll()
  {
    return isShowAll;
  }

  static class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
//    ImageView mIcon;
    TextView mName;

    public ViewHolder(View view)
    {
      super(view);
//      TODO we need icons from designer
//      mIcon = (ImageView) view.findViewById(R.id.iv__icon);
      mName = (TextView) view.findViewById(R.id.tv__facility);
      view.setOnClickListener(this);
    }

    @Override
    public void onClick(View v)
    {
      Utils.showSnackbar(v.getRootView(), mName.getText().toString());
    }

    public void bind(Sponsored.FacilityType facility)
    {
//      TODO map facility key to image resource id
//      mIcon.setImageResource(R.drawable.ic_entrance);
      mName.setText(facility.getName());
    }
  }
}
