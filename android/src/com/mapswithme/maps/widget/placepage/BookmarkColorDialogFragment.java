package com.mapswithme.maps.widget.placepage;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.GridView;

import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.bookmarks.IconsAdapter;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Icon;

public class BookmarkColorDialogFragment extends BaseMwmDialogFragment
{
  public static final String ICON_TYPE = "ExtraIconType";

  private String mIconType;

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
      mIconType = getArguments().getString(ICON_TYPE);

    return new AlertDialog.Builder(getActivity())
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
    final List<Icon> icons = BookmarkManager.getIcons();
    final IconsAdapter adapter = new IconsAdapter(getActivity(), icons);
    adapter.chooseItem(mIconType);

    final GridView gView = (GridView) LayoutInflater.from(getActivity()).inflate(R.layout.fragment_color_grid, null);
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
