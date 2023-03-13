package app.organicmaps.widget.placepage.sections;

import android.content.Context;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.util.Utils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class PlacePhoneAdapter extends RecyclerView.Adapter<PlacePhoneAdapter.ViewHolder>
{

  private List<String> mPhoneData = Collections.emptyList();

  public PlacePhoneAdapter() {}

  public PlacePhoneAdapter(String phones) {
    refreshPhones(phones);
  }

  public void refreshPhones(String phones) {
    if (TextUtils.isEmpty(phones))
      return;

    mPhoneData = new ArrayList<>();
    for (String p : phones.split(";"))
    {
      p = p.trim();
      if (TextUtils.isEmpty(p)) continue;
      mPhoneData.add(p);
    }

    notifyDataSetChanged();
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(parent.getContext())
                                        .inflate(R.layout.place_page_phone_item, parent, false));
  }

  @Override
  public void onBindViewHolder(@NonNull PlacePhoneAdapter.ViewHolder holder, int position)
  {
    holder.setPhone(mPhoneData.get(position));
  }

  @Override
  public int getItemCount()
  {
    return mPhoneData.size();
  }

  public List<String> getPhonesList()
  {
    return new ArrayList<>(mPhoneData);
  }

  public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener, View.OnLongClickListener
  {
    private final TextView mPhone;

    public ViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mPhone = itemView.findViewById(R.id.tv__place_phone);
      itemView.setVisibility(View.VISIBLE);
      itemView.setOnClickListener(this);
      itemView.setOnLongClickListener(this);
    }

    public void setPhone(String phoneNumber)
    {
      mPhone.setText(phoneNumber);
    }

    @Override
    public void onClick(View view)
    {
      Utils.callPhone(view.getContext(), mPhone.getText().toString());
    }

    @Override
    public boolean onLongClick(View view)
    {
      final String phoneNumber = mPhone.getText().toString();
      final Context ctx = view.getContext();
      Utils.copyTextToClipboard(ctx, phoneNumber);
      Utils.showSnackbarAbove(view, view.getRootView().findViewById(R.id.pp_buttons_layout),
                              ctx.getString(R.string.copied_to_clipboard, phoneNumber));
      return true;
    }
  }
}
