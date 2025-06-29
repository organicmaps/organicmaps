package app.organicmaps.widget.placepage.sections;

import android.app.KeyguardManager;
import android.content.Context;
import android.os.Build;
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

public class PlacePhoneAdapter extends RecyclerView.Adapter<PlacePhoneAdapter.ViewHolder>
{
  private final ArrayList<String> mPhoneData = new ArrayList<>();

  public PlacePhoneAdapter() {}

  public void refreshPhones(String phones)
  {
    if (TextUtils.isEmpty(phones))
      return;

    mPhoneData.clear();
    for (String p : phones.split(";"))
    {
      p = p.trim();
      if (TextUtils.isEmpty(p))
        continue;
      mPhoneData.add(p);
    }

    notifyDataSetChanged();
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    return new ViewHolder(
        LayoutInflater.from(parent.getContext()).inflate(R.layout.place_page_phone_item, parent, false));
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

  public static class ViewHolder
      extends RecyclerView.ViewHolder implements View.OnClickListener, View.OnLongClickListener
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

      KeyguardManager keyguardManager = (KeyguardManager) ctx.getSystemService(Context.KEYGUARD_SERVICE);
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU || keyguardManager.isDeviceLocked())
      {
        Utils.showSnackbarAbove(view.getRootView().findViewById(R.id.pp_buttons_layout), view,
                                ctx.getString(R.string.copied_to_clipboard, phoneNumber));
      }
      return true;
    }
  }
}
