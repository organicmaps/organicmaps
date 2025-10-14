package app.organicmaps.widget.placepage;

import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseToolbarActivity;

public class WikiArticleActivity extends BaseToolbarActivity
{
  private static final String EXTRA_TITLE = "title";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return WikiArticleFragment.class;
  }

  public static void start(@NonNull Context context, @NonNull String title, @NonNull String wikiArticle)
  {
    Intent intent = new Intent(context, WikiArticleActivity.class)
                        .putExtra(WikiArticleFragment.EXTRA_WIKI_ARTICLE, wikiArticle)
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
