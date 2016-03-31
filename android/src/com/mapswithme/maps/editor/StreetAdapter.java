package com.mapswithme.maps.editor;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Checkable;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.EditTextDialogFragment;

public class StreetAdapter extends RecyclerView.Adapter<StreetAdapter.BaseViewHolder>
{
  private static final int TYPE_ADD_STREET = 0;
  private static final int TYPE_STREET = 1;

  private final String[] mStreets;
  private final StreetFragment mFragment;
  private String mSelectedStreet;

  public StreetAdapter(@NonNull StreetFragment host, @NonNull String[] streets, @NonNull String selected)
  {
    mFragment = host;
    mStreets = streets;
    mSelectedStreet = selected;
  }

  @Override
  public BaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return viewType == TYPE_STREET ? new StreetViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_street, parent, false))
                                   : new AddViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_add_street, parent, false));
  }

  @Override
  public void onBindViewHolder(BaseViewHolder holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public int getItemCount()
  {
    return mStreets.length + 1;
  }

  @Override
  public int getItemViewType(int position)
  {
    return position == getItemCount() - 1 ? TYPE_ADD_STREET : TYPE_STREET;
  }

  public String getSelectedStreet()
  {
    return mSelectedStreet;
  }

  private void addStreet()
  {
    final Resources resources = MwmApplication.get().getResources();
    EditTextDialogFragment.show(resources.getString(R.string.street), null,
                                resources.getString(R.string.ok),
                                resources.getString(R.string.cancel), mFragment);
  }

  protected abstract class BaseViewHolder extends RecyclerView.ViewHolder
  {
    public BaseViewHolder(View itemView)
    {
      super(itemView);
    }

    public void bind(int position) {}
  }

  protected class StreetViewHolder extends BaseViewHolder
  {
    final TextView street;
    final Checkable selected;

    public StreetViewHolder(View itemView)
    {
      super(itemView);
      street = (TextView) itemView.findViewById(R.id.street);
      selected = (Checkable) itemView.findViewById(R.id.selected);
      itemView.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          mSelectedStreet = mStreets[getAdapterPosition()];
          notifyDataSetChanged();
          mFragment.saveStreet(mSelectedStreet);
        }
      });
    }

    @Override
    public void bind(int position)
    {
      final String text = mStreets[position];
      selected.setChecked(mSelectedStreet.equals(text));
      street.setText(text);
    }
  }

  protected class AddViewHolder extends BaseViewHolder
  {
    public AddViewHolder(View itemView)
    {
      super(itemView);
      itemView.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          addStreet();
        }
      });
    }
  }
}
