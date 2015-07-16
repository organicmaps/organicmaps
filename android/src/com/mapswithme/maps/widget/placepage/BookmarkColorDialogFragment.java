package com.mapswithme.maps.widget.placepage;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.GridView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.IconsAdapter;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.Icon;

import java.util.List;

public class BookmarkColorDialogFragment extends DialogFragment
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

    final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity()).setView(buildView()).
        setTitle(R.string.bookmark_color).
        setNegativeButton(getString(R.string.cancel), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
          }
        });

    return builder.create();
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
