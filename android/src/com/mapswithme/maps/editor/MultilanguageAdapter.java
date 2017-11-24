package com.mapswithme.maps.editor;

import android.support.design.widget.TextInputLayout;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.maps.editor.data.LocalizedName;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class MultilanguageAdapter extends RecyclerView.Adapter<MultilanguageAdapter.Holder>
{
  private final List<LocalizedName> mNames;
  private int mMandatoryNamesCount;
  private boolean mAdditionalLanguagesShown;

  MultilanguageAdapter(EditorHostFragment hostFragment)
  {
    mNames = hostFragment.getNames();
    mMandatoryNamesCount = hostFragment.getMandatoryNamesCount();
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
    holder.inputLayout.setHint(name.langName);
  }

  @Override
  public int getItemCount()
  {
    return mAdditionalLanguagesShown ? mNames.size() : mMandatoryNamesCount;
  }

  public int getNamesCount()
  {
    return mNames.size();
  }

  public LocalizedName getNameAtPos(int pos) { return mNames.get(pos); }

  public int getMandatoryNamesCount()
  {
    return mMandatoryNamesCount;
  }

  public boolean areAdditionalLanguagesShown()
  {
    return mAdditionalLanguagesShown;
  }

  public void showAdditionalLanguages(boolean show)
  {
    if (mAdditionalLanguagesShown == show)
      return;

    mAdditionalLanguagesShown = show;

    if (mNames.size() != mMandatoryNamesCount)
    {
      if (show)
      {
        notifyItemRangeInserted(mMandatoryNamesCount, mNames.size() - mMandatoryNamesCount);
      }
      else
      {
        notifyItemRangeRemoved(mMandatoryNamesCount, mNames.size() - mMandatoryNamesCount);
      }
    }
  }

  public class Holder extends RecyclerView.ViewHolder
  {
    EditText input;
    TextInputLayout inputLayout;

    public Holder(View itemView)
    {
      super(itemView);
      input = (EditText) itemView.findViewById(R.id.input);
      inputLayout = (TextInputLayout) itemView.findViewById(R.id.input_layout);
      input.addTextChangedListener(new StringUtils.SimpleTextWatcher()
      {
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count)
        {
          UiUtils.setInputError(inputLayout, Editor.nativeIsNameValid(s.toString()) ? 0 : R.string.error_enter_correct_name);
          mNames.get(getAdapterPosition()).name = s.toString();
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
