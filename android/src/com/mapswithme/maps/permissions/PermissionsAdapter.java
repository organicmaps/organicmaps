package com.mapswithme.maps.permissions;

import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

class PermissionsAdapter extends RecyclerView.Adapter<PermissionsAdapter.ViewHolder>
{
  static final int TYPE_TITLE = 0;
  static final int TYPE_PERMISSION = 1;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_TITLE, TYPE_PERMISSION })
  @interface ViewHolderType {}

  private static final List<PermissionItem> ITEMS;
  static {
    ArrayList<PermissionItem> items = new ArrayList<>();
    items.add(new PermissionItem(TYPE_TITLE, R.string.onboarding_detail_permissions_title, 0, 0));
    items.add(new PermissionItem(TYPE_PERMISSION,
                                 R.string.onboarding_detail_permissions_location_title,
                                 R.string.onboarding_detail_permissions_location_message,
                                 R.drawable.ic_navigation_permission));
    ITEMS = Collections.unmodifiableList(items);
  }

  @ViewHolderType
  @Override
  public int getItemViewType(int position)
  {
    return ITEMS.get(position).mType;
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, @ViewHolderType int viewType)
  {
    switch (viewType)
    {
      case TYPE_PERMISSION:
        return new PermissionViewHolder(LayoutInflater.from(parent.getContext())
                                                      .inflate(R.layout.item_permission, parent,
                                                               false));
      case TYPE_TITLE:
        return new TitleViewHolder(LayoutInflater.from(parent.getContext())
                                                 .inflate(R.layout.item_permissions_title, parent,
                                                          false));
      default:
        return null;
    }
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(ITEMS.get(position));
  }

  @Override
  public int getItemCount()
  {
    return ITEMS.size();
  }

  private static final class PermissionItem
  {
    @ViewHolderType
    private final int mType;
    @StringRes
    private final int mTitle;
    @StringRes
    private final int mMessage;
    @DrawableRes
    private final int mIcon;

    PermissionItem(@ViewHolderType int type, @StringRes int title, @StringRes int message, int icon)
    {
      mType = type;
      mTitle = title;
      mMessage = message;
      mIcon = icon;
    }
  }

  static abstract class ViewHolder extends RecyclerView.ViewHolder
  {
    public ViewHolder(@NonNull View itemView)
    {
      super(itemView);
    }

    abstract void bind(@NonNull PermissionItem item);
  }

  private static class TitleViewHolder extends ViewHolder
  {
    private final TextView mTitle;

    TitleViewHolder(@NonNull View itemView)
    {
      super(itemView);

      mTitle = itemView.findViewById(R.id.tv__title);
    }

    @Override
    void bind(@NonNull PermissionItem item)
    {
      mTitle.setText(item.mTitle);
    }
  }

  private static class PermissionViewHolder extends ViewHolder
  {
    private final ImageView mIcon;
    private final TextView mTitle;
    private final TextView mMessage;

    PermissionViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mIcon = itemView.findViewById(R.id.iv__permission_icon);
      mTitle = itemView.findViewById(R.id.tv__permission_title);
      mMessage = itemView.findViewById(R.id.tv__permission_message);
    }

    @Override
    void bind(@NonNull PermissionItem item)
    {
      mIcon.setImageResource(item.mIcon);
      mTitle.setText(item.mTitle);
      mMessage.setText(item.mMessage);
    }
  }
}
