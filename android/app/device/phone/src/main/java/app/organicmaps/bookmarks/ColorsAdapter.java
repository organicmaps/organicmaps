package app.organicmaps.bookmarks;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import androidx.annotation.DrawableRes;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.PredefinedColors;
import app.organicmaps.util.Graphics;
import java.util.List;
import java.util.Objects;

public class ColorsAdapter extends ArrayAdapter<Integer>
{
  @PredefinedColors.Color
  private int mCheckedIconColor;

  @DrawableRes
  private final int mIconResId;

  public ColorsAdapter(Context context, List<Integer> list, @DrawableRes int iconResId)
  {
    super(context, 0, 0, list);
    mIconResId = iconResId;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    SpinnerViewHolder holder;
    if (convertView == null)
    {
      LayoutInflater inflater = LayoutInflater.from(getContext());
      convertView = inflater.inflate(R.layout.item_color, parent, false);
      holder = new SpinnerViewHolder(convertView);
      convertView.setTag(holder);
    }
    else
      holder = (SpinnerViewHolder) convertView.getTag();

    @PredefinedColors.Color
    final int color = Objects.requireNonNull(getItem(position));

    Drawable circle;
    if (color == mCheckedIconColor)
    {
      circle = Graphics.drawCircleAndImage(PredefinedColors.getColor(mCheckedIconColor), R.dimen.track_circle_size,
                                           mIconResId, R.dimen.bookmark_icon_size, getContext());
    }
    else
    {
      circle = Graphics.drawCircle(PredefinedColors.getColor(color), R.dimen.select_color_circle_size,
                                   getContext().getResources());
    }
    holder.icon.setImageDrawable(circle);
    return convertView;
  }

  private static class SpinnerViewHolder
  {
    final ImageView icon;

    SpinnerViewHolder(View convertView)
    {
      icon = convertView.findViewById(R.id.iv__color);
    }
  }

  public void chooseItem(int position)
  {
    mCheckedIconColor = position;
    notifyDataSetChanged();
  }
}
