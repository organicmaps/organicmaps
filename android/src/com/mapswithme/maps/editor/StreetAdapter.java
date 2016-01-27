package com.mapswithme.maps.editor;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public class StreetAdapter extends RecyclerView.Adapter<StreetAdapter.ViewHolder>
{
  private final String[] mStreets;
  private String mSelectedStreet;

  public StreetAdapter(@NonNull String[] streets, @NonNull String selected)
  {
    mStreets = streets;
    mSelectedStreet = selected;
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_street, parent, false));
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public int getItemCount()
  {
    return mStreets.length;
  }

  public String getSelectedStreet()
  {
    return mSelectedStreet;
  }

  protected class ViewHolder extends RecyclerView.ViewHolder
  {
    final TextView street;
    final ImageView selected;

    public ViewHolder(View itemView)
    {
      super(itemView);
      street = (TextView) itemView.findViewById(R.id.street);
      selected = (ImageView) itemView.findViewById(R.id.selected);
      itemView.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          mSelectedStreet = mStreets[getAdapterPosition()];
          notifyDataSetChanged();
        }
      });
    }

    public void bind(int position)
    {
      final String text = mStreets[position];
      UiUtils.showIf(mSelectedStreet.equals(text), selected);
      street.setText(text);
    }
  }
}
