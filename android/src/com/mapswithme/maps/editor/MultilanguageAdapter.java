package com.mapswithme.maps.editor;

import android.support.v7.util.SortedList;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.util.SortedListAdapterCallback;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import com.mapswithme.maps.R;
import com.mapswithme.maps.editor.data.LocalizedName;

public class MultilanguageAdapter extends RecyclerView.Adapter<MultilanguageAdapter.Holder>
{
  private SortedList<LocalizedName> mNames;

  MultilanguageAdapter(LocalizedName[] names)
  {
    mNames = new SortedList<>(LocalizedName.class,
                              new SortedListAdapterCallback<LocalizedName>(this)
                              {
                                @Override
                                public int compare(LocalizedName o1, LocalizedName o2)
                                {
                                  return o1.lang.compareTo(o2.lang);
                                }

                                @Override
                                public boolean areContentsTheSame(LocalizedName oldItem, LocalizedName newItem)
                                {
                                  return TextUtils.equals(oldItem.name, newItem.name);
                                }

                                @Override
                                public boolean areItemsTheSame(LocalizedName item1, LocalizedName item2)
                                {
                                  return item1.code == item2.code;
                                }
                              },
                              names.length);

    // skip default name
    for (int i = 1; i < names.length; i++)
      mNames.add(names[i]);
  }

  public void setNames(SortedList<LocalizedName> names)
  {
    mNames = names;
  }

  public SortedList<LocalizedName> getNames()
  {
    return mNames;
  }

  @Override
  public Holder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    final View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_localized_name, parent, false);
    return new Holder(view);
  }

  @Override
  public void onBindViewHolder(Holder holder, int position)
  {
    LocalizedName name = mNames.get(position);
    holder.input.setText(name.name);
    holder.input.setHint(name.lang);
  }

  @Override
  public int getItemCount()
  {
    return mNames.size();
  }

  public class Holder extends RecyclerView.ViewHolder
  {
    EditText input;

    public Holder(View itemView)
    {
      super(itemView);
      input = (EditText) itemView.findViewById(R.id.input);
      itemView.findViewById(R.id.delete).setOnClickListener(new View.OnClickListener()
                                                            {
                                                              @Override
                                                              public void onClick(View v)
                                                              {
                                                                mNames.removeItemAt(getAdapterPosition());
                                                              }
                                                            });
    }
  }
}
