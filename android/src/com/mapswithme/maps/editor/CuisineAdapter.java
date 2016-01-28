package com.mapswithme.maps.editor;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TextView;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Metadata;

public class CuisineAdapter extends RecyclerView.Adapter<CuisineAdapter.ViewHolder>
{
  private static class Item implements Comparable<Item>
  {
    String translatedCuisine;
    String cuisineStringName;
    boolean selected;

    public Item(String cuisine, String cuisineKey, boolean selected)
    {
      this.translatedCuisine = cuisine;
      this.cuisineStringName = cuisineKey;
      this.selected = selected;
    }

    @Override
    public int compareTo(@NonNull Item another)
    {
      return translatedCuisine.compareTo(another.translatedCuisine);
    }
  }

  private List<Item> mTranslatedItems;
  private List<Item> mItems;

  public CuisineAdapter(@NonNull String cuisine)
  {
    initAllCuisinesFromStrings();
    String[] selectedCuisines = Metadata.splitCuisines(cuisine);
    Arrays.sort(selectedCuisines);
    mItems = new ArrayList<>(mTranslatedItems.size());
    for (Item item : mTranslatedItems)
    {
      final String cuisineString = item.translatedCuisine;
      mItems.add(new Item(cuisineString, item.cuisineStringName,
                          Arrays.binarySearch(selectedCuisines, Metadata.stringNameToOsmCuisine(item.cuisineStringName)) >= 0));
    }
  }

  private void initAllCuisinesFromStrings()
  {
    if (mTranslatedItems != null)
      return;

    mTranslatedItems = new ArrayList<>();
    final Resources resources = MwmApplication.get().getResources();
    for (Field stringField : R.string.class.getDeclaredFields())
    {
      try
      {
        final String fieldName = stringField.getName();
        if (Metadata.isCuisineString(fieldName))
        {
          int resId = stringField.getInt(stringField);
          final String cuisine = resources.getString(resId);
          mTranslatedItems.add(new Item(cuisine, fieldName, false));
        }

      } catch (IllegalAccessException ignored) { }
    }

    Collections.sort(mTranslatedItems);
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
    return mTranslatedItems.size();
  }

  public String getCuisine()
  {
    final List<String> selectedList = new ArrayList<>();
    for (Item item : mItems)
    {
      if (item.selected)
        selectedList.add(Metadata.stringNameToOsmCuisine(item.cuisineStringName));
    }

    String[] cuisines = new String[selectedList.size()];
    cuisines = selectedList.toArray(cuisines);
    return Metadata.combineCuisines(cuisines);
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
      final String text = mTranslatedItems.get(position).translatedCuisine;
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
