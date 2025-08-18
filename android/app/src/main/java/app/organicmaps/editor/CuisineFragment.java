package app.organicmaps.editor;

import androidx.annotation.NonNull;
import app.organicmaps.base.BaseMwmRecyclerFragment;

public class CuisineFragment extends BaseMwmRecyclerFragment<CuisineAdapter>
{
  private CuisineAdapter mAdapter;

  @NonNull
  @Override
  protected CuisineAdapter createAdapter()
  {
    mAdapter = new CuisineAdapter();
    return mAdapter;
  }

  @NonNull
  public String[] getCuisines()
  {
    return mAdapter.getCuisines();
  }

  public void setFilter(String filter)
  {
    mAdapter.setFilter(filter);
  }
}
