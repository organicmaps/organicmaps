package com.mapswithme.maps.editor;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.mapswithme.maps.R;

public class CuisineAdapter extends RecyclerView.Adapter<CuisineAdapter.ViewHolder>
{
  private static class Item implements Comparable<Item>
  {
    String cuisineTranslated;
    String cuisineKey;
    boolean selected;

    public Item(String key, String translation, boolean selected)
    {
      this.cuisineKey = key;
      this.cuisineTranslated = translation;
      this.selected = selected;
    }

    @Override
    public int compareTo(@NonNull Item another)
    {
      return cuisineTranslated.compareTo(another.cuisineTranslated);
    }
  }

  private List<Item> mItems;

  public CuisineAdapter(@NonNull String[] selectedCuisines, @NonNull String[] cuisines, @NonNull String[] translations)
  {
    mItems = new ArrayList<>(cuisines.length);
    Arrays.sort(selectedCuisines);
    for (int i = 0; i < cuisines.length; i++)
    {
      final String key = cuisines[i];
      mItems.add(new Item(key, translations[i], Arrays.binarySearch(selectedCuisines, key) >= 0));
    }
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_cuisine, parent, false));
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  public String[] getCuisines()
  {
    final List<String> selectedList = new ArrayList<>();
    for (Item item : mItems)
    {
      if (item.selected)
        selectedList.add(item.cuisineKey);
    }

    return selectedList.toArray(new String[selectedList.size()]);
  }

  protected class ViewHolder extends RecyclerView.ViewHolder implements CompoundButton.OnCheckedChangeListener
  {
    final TextView cuisine;
    final CheckBox selected;

    public ViewHolder(View itemView)
    {
      super(itemView);
      cuisine = (TextView) itemView.findViewById(R.id.cuisine);
      selected = (CheckBox) itemView.findViewById(R.id.selected);
      selected.setOnCheckedChangeListener(this);
      itemView.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          selected.toggle();
        }
      });
    }

    public void bind(int position)
    {
      final String text = mItems.get(position).cuisineTranslated;
      cuisine.setText(text);
      selected.setOnCheckedChangeListener(null);
      selected.setChecked(mItems.get(position).selected);
      selected.setOnCheckedChangeListener(this);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
      Item item = mItems.get(getAdapterPosition());
      item.selected = isChecked;
    }
  }
}
