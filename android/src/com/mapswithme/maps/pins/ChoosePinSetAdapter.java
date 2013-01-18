package com.mapswithme.maps.pins;

import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.RadioButton;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.pins.pins.PinSet;

public class ChoosePinSetAdapter extends AbstractPinSetAdapter implements OnItemClickListener
{

  private int mCheckedPosition = -1;

  public ChoosePinSetAdapter(Context context, List<PinSet> set, int pos)
  {
    super(context, set);
    mCheckedPosition = pos;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      convertView = LayoutInflater.from(getContext()).inflate(R.layout.set_chooser_item, null);
      convertView.setTag(new SingleChoiceHolder((TextView) convertView.findViewById(R.id.sci_set_name),
          (RadioButton) convertView.findViewById(R.id.sci_checkbox)));
    }
    SingleChoiceHolder holder = (SingleChoiceHolder) convertView.getTag();
    holder.name.setText(getItem(position).getName());
    holder.checked.setChecked(mCheckedPosition == position);
    return convertView;
  }

  private static class SingleChoiceHolder
  {
    TextView name;
    RadioButton checked;

    public SingleChoiceHolder(TextView name, RadioButton checked)
    {
      super();
      this.name = name;
      this.checked = checked;
    }
  }

  @Override
  public void onItemClick(AdapterView<?> parent, View view, int position, long id)
  {
    mCheckedPosition = position;
  }
}
