package app.organicmaps.widget.placepage;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.GridView;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmDialogFragment;
import app.organicmaps.bookmarks.IconsAdapter;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.Icon;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.util.List;

public class BookmarkColorDialogFragment extends BaseMwmDialogFragment
{
  public static final String ICON_TYPE = "ExtraIconType";

  private int mIconColor;

  public interface OnBookmarkColorChangeListener
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

    return new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
        .setView(buildView())
        .setTitle(R.string.choose_color)
        .setNegativeButton(R.string.cancel, null)
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

    @SuppressLint("InflateParams")
    final GridView gView =
        (GridView) LayoutInflater.from(requireActivity()).inflate(R.layout.fragment_color_grid, null);
    gView.setAdapter(adapter);
    gView.setOnItemClickListener((arg0, who, pos, id) -> {
      if (mColorSetListener != null)
        mColorSetListener.onBookmarkColorSet(pos);
      dismiss();
    });

    return gView;
  }
}
