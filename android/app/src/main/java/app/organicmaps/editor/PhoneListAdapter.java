package app.organicmaps.editor;

import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.UiUtils;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;
import java.util.ArrayList;
import java.util.List;

public class PhoneListAdapter extends RecyclerView.Adapter<PhoneListAdapter.ViewHolder>
{
  private List<String> phonesData = new ArrayList<>();

  public PhoneListAdapter(String phoneList)
  {
    if (TextUtils.isEmpty(phoneList))
    {
      phonesData.add("");
      return;
    }

    phonesData = new ArrayList<>();
    for (String p : phoneList.split(";"))
    {
      p = p.trim();
      if (TextUtils.isEmpty(p))
        continue;
      phonesData.add(p);
    }

    phonesData.add("");
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_phone, parent, false));
  }

  @Override
  public void onBindViewHolder(@NonNull PhoneListAdapter.ViewHolder holder, int position)
  {
    holder.setPosition(position);
    holder.setPhone(phonesData.get(position));
    if (position == phonesData.size() - 1)
      holder.setNewPhoneRow(true);
  }

  @Override
  public int getItemCount()
  {
    return phonesData.size();
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  public void appendPhone()
  {
    phonesData.add("");
    notifyDataSetChanged();
  }

  public String getPhoneList()
  {
    StringBuilder sb = new StringBuilder();
    for (int i = 0; i < phonesData.size(); i++)
    {
      final String p = phonesData.get(i).trim();
      if (!TextUtils.isEmpty(p))
      {
        if (sb.length() > 0)
          sb.append(';');
        sb.append(p);
      }
    }
    return sb.toString();
  }

  protected class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
    private int mPosition = -1;
    private boolean isNewPhoneRow = false;
    private final TextInputEditText mInput;
    private final ImageView deleteButton;

    public ViewHolder(@NonNull View itemView)
    {
      super(itemView);

      mInput = itemView.findViewById(R.id.input);
      final TextInputLayout phoneInput = itemView.findViewById(R.id.phone_input);
      mInput.addTextChangedListener(new StringUtils.SimpleTextWatcher() {
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count)
        {
          UiUtils.setInputError(phoneInput,
                                Editor.nativeIsPhoneValid(s.toString()) ? 0 : R.string.error_enter_correct_phone);
          PhoneListAdapter.this.updatePhoneItem(mPosition, mInput.getText().toString());
          if (isNewPhoneRow)
            setNewPhoneRow(false); // When user edits text in new phone row then it's not a new phone row anymore
        }
      });

      deleteButton = itemView.findViewById(R.id.delete_icon);
      deleteButton.setOnClickListener(this);
      // TODO: setting icons from code because icons defined in layout XML are white.
      deleteButton.setImageResource(R.drawable.ic_delete);

      setNewPhoneRow(isNewPhoneRow);
    }

    public void setPosition(int position)
    {
      mPosition = position;
    }

    public void setPhone(String phone)
    {
      if (!mInput.getText().toString().equals(phone))
        mInput.setText(phone);
    }

    public void setNewPhoneRow(boolean newPhoneRow)
    {
      this.isNewPhoneRow = newPhoneRow;

      if (deleteButton != null)
        UiUtils.visibleIf(!newPhoneRow, deleteButton);

      if (mInput != null)
      {
        if (newPhoneRow)
          mInput.setHint(R.string.editor_new_phone);
        else
          mInput.setHint(null);
      }
    }

    @Override
    public void onClick(View view)
    {
      if (view.getId() == R.id.delete_icon)
        PhoneListAdapter.this.deleteItem(mPosition);
    }
  }

  private void updatePhoneItem(int position, String text)
  {
    if (position == -1)
      return;
    phonesData.set(position, text);
    if (position == phonesData.size() - 1 && !text.isEmpty())
    {
      phonesData.add("");
      notifyDataSetChanged();
    }
  }

  void deleteItem(int position)
  {
    phonesData.remove(position);
    notifyDataSetChanged();
  }

  public boolean allPhonesValid()
  {
    for (String number : phonesData)
    {
      if (!Editor.nativeIsPhoneValid(number))
        return false;
    }

    return true;
  }
}
