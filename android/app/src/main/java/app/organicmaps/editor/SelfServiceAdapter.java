package app.organicmaps.editor;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.util.Utils.TagTranslationValue;

public class SelfServiceAdapter extends RecyclerView.Adapter<SelfServiceAdapter.ViewHolder>
{
  public static final TagTranslationValue[] mItems = new TagTranslationValue[]{
          new TagTranslationValue("yes", R.string.self_service_yes),
          new TagTranslationValue("only", R.string.self_service_only),
          new TagTranslationValue("partially", R.string.self_service_partially),
          new TagTranslationValue("no", R.string.self_service_no)
  };
  private final SelfServiceFragment mFragment;
  private String mSelectedOption;


  public SelfServiceAdapter(@NonNull SelfServiceFragment host, @NonNull String selected)
  {
    mFragment = host;
    mSelectedOption = selected;
  }

  public String getSelected()
  {
    return mSelectedOption;
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new SelfServiceAdapter.ViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_selection, parent, false));
  }

  @Override
  public void onBindViewHolder(SelfServiceAdapter.ViewHolder holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public int getItemCount()
  {
    return mItems.length;
  }

  protected class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
    final TextView selfServiceDef;
    final CompoundButton selected;

    public ViewHolder(View itemView)
    {
      super(itemView);
      selfServiceDef = itemView.findViewById(R.id.self_service_default);
      selected = itemView.findViewById(R.id.self_service_selected);
      itemView.setOnClickListener(this);
      selected.setOnClickListener(v -> {
        selected.toggle();
        SelfServiceAdapter.ViewHolder.this.onClick(selected);
      });
    }

    public void bind(int position)
    {
      Context context = itemView.getContext();
      selected.setChecked(mSelectedOption.equals(mItems[position].tagValue()));
      String text = context.getString(mItems[position].translation());
      selfServiceDef.setText(text);
    }

    @Override
    public void onClick(View v)
    {
      mSelectedOption = mItems[getBindingAdapterPosition()].tagValue();
      notifyDataSetChanged();
      mFragment.saveSelection(mSelectedOption);
    }
  }
}
