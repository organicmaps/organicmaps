package app.organicmaps.widget.placepage;

import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseToolbarActivity;

public class PlaceDescriptionActivity extends BaseToolbarActivity
{
  private static final String EXTRA_TITLE = "title";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return PlaceDescriptionFragment.class;
  }

  public static void start(@NonNull Context context, @NonNull String title, @NonNull String description)
  {
    Intent intent = new Intent(context, PlaceDescriptionActivity.class)
                        .putExtra(PlaceDescriptionFragment.EXTRA_DESCRIPTION, description)
                        .putExtra(EXTRA_TITLE, title);
    context.startActivity(intent);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    String toolbarTitle = getIntent().getStringExtra(EXTRA_TITLE);
    this.getToolbar().setTitle(toolbarTitle);
  }
}
