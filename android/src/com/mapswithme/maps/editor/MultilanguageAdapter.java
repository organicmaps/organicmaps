package com.mapswithme.maps.editor;

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import com.mapswithme.maps.R;
import com.mapswithme.maps.editor.data.LocalizedName;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

import java.util.List;

public class MultilanguageAdapter extends RecyclerView.Adapter<MultilanguageAdapter.Holder>
{
  private final List<LocalizedName> mNames;
  private EditorHostFragment mHostFragment;

  // TODO(mgsergio): Refactor: don't pass the whole EditorHostFragment.
  MultilanguageAdapter(EditorHostFragment hostFragment)
  {
    mHostFragment = hostFragment;
    mNames = hostFragment.getLocalizedNames();
  }

  @Override
  public Holder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    final View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_localized_name, parent, false);
    // TODO(mgsergio): Deletion is not implemented.
    UiUtils.hide(view.findViewById(R.id.delete));
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
      input.addTextChangedListener(new StringUtils.SimpleTextWatcher()
      {
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count)
        {
          mHostFragment.setName(s.toString(), getAdapterPosition());
        }
      });

      itemView.findViewById(R.id.delete).setOnClickListener(new View.OnClickListener()
                                                            {
                                                              @Override
                                                              public void onClick(View v)
                                                              {
                                                                // TODO(mgsergio): Implement item deletion.
                                                                // int position = getAdapterPosition();
                                                                // mHostFragment.removeLocalizedName(position + 1);
                                                                // mNames.remove(position);
                                                                // notifyItemRemoved(position);
                                                              }
                                                            });
    }
  }
}
