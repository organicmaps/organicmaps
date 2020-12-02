package com.mapswithme.maps.editor;

import android.content.res.Resources;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.maps.editor.data.LocalizedStreet;
import com.mapswithme.util.UiUtils;

public class StreetAdapter extends RecyclerView.Adapter<StreetAdapter.BaseViewHolder>
{
  private static final int TYPE_ADD_STREET = 0;
  private static final int TYPE_STREET = 1;

  private final LocalizedStreet[] mStreets;
  private final StreetFragment mFragment;
  private LocalizedStreet mSelectedStreet;

  public StreetAdapter(@NonNull StreetFragment host, @NonNull LocalizedStreet[] streets, @NonNull LocalizedStreet selected)
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

  public LocalizedStreet getSelectedStreet()
  {
    return mSelectedStreet;
  }

  private void addStreet()
  {
    final Resources resources = MwmApplication.from(mFragment.requireContext()).getResources();
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

  protected class StreetViewHolder extends BaseViewHolder implements View.OnClickListener
  {
    final TextView streetDef;
    final TextView streetLoc;
    final CompoundButton selected;

    public StreetViewHolder(View itemView)
    {
      super(itemView);
      streetDef = (TextView) itemView.findViewById(R.id.street_default);
      streetLoc = (TextView) itemView.findViewById(R.id.street_localized);
      selected = (CompoundButton) itemView.findViewById(R.id.selected);
      itemView.setOnClickListener(this);
      selected.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          selected.toggle();
          StreetViewHolder.this.onClick(selected);
        }
      });
    }

    @Override
    public void bind(int position)
    {
      selected.setChecked(mSelectedStreet.defaultName.equals(mStreets[position].defaultName));
      streetDef.setText(mStreets[position].defaultName);
      UiUtils.setTextAndHideIfEmpty(streetLoc, mStreets[position].localizedName);
    }

    @Override
    public void onClick(View v)
    {
      mSelectedStreet = mStreets[getAdapterPosition()];
      notifyDataSetChanged();
      mFragment.saveStreet(mSelectedStreet);
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
