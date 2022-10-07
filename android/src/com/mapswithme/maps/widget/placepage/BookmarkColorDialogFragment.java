package com.mapswithme.maps.widget.placepage;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.GridView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.bookmarks.IconsAdapter;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Icon;

import java.util.List;

public class BookmarkColorDialogFragment extends BaseMwmDialogFragment
{
  public static final String ICON_TYPE = "ExtraIconType";

  private int mIconColor;

  interface OnBookmarkColorChangeListener
  {
    void onBookmarkColorSet(int colorPos);
  }

  private OnBookmarkColorChangeListener mColorSetListener;

  public BookmarkColorDialogFragment() {}

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    if (getArguments() != null)
      mIconColor = getArguments().getInt(ICON_TYPE);

    return new AlertDialog.Builder(requireActivity())
                          .setView(buildView())
                          .setTitle(R.string.bookmark_color)
                          .setNegativeButton(getString(R.string.cancel), null)
                          .create();
  }

  public void setOnColorSetListener(OnBookmarkColorChangeListener listener)
  {
    mColorSetListener = listener;
  }

  private View buildView()
  {
    final List<Icon> icons = BookmarkManager.ICONS;
    final IconsAdapter adapter = new IconsAdapter(requireActivity(), icons);
    adapter.chooseItem(mIconColor);

    final GridView gView = (GridView) LayoutInflater.from(requireActivity()).inflate(R.layout.fragment_color_grid, null);
    gView.setAdapter(adapter);
    gView.setOnItemClickListener(new AdapterView.OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> arg0, View who, int pos, long id)
      {
        if (mColorSetListener != null)
          mColorSetListener.onBookmarkColorSet(pos);
        dismiss();
      }
    });

    return gView;
  }

}
