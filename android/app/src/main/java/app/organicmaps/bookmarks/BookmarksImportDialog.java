package app.organicmaps.bookmarks;

import android.app.Activity;
import android.app.Dialog;
import android.content.res.Resources;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import androidx.core.widget.TextViewCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkImportResult;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public final class BookmarksImportDialog
{
  public interface OnCategorySelectedListener
  {
    void onCategorySelected(long categoryId);
  }

  private BookmarksImportDialog() {}

  @Nullable
  public static Dialog show(@NonNull Activity activity, @NonNull BookmarkImportResult result,
                            @NonNull OnCategorySelectedListener listener)
  {
    long[] categoryIds = result.getCategoryIds();
    String[] failedFileNames = result.getFailedFileNames();
    if (categoryIds.length == 0 && failedFileNames.length == 0)
      return null;

    if (categoryIds.length == 1 && failedFileNames.length == 0)
    {
      listener.onCategorySelected(categoryIds[0]);
      return null;
    }

    View view = LayoutInflater.from(activity).inflate(R.layout.dialog_bookmarks_import, null);
    LinearLayout content = view.findViewById(R.id.import_results);
    if (failedFileNames.length > 0)
    {
      addSectionHeader(content, activity.getString(R.string.load_kmz_failed));
      for (String fileName : failedFileNames)
        addResultRow(content, fileName, false /* selectable */, null);
    }

    Dialog[] dialog = new Dialog[1];
    if (categoryIds.length > 0)
    {
      addSectionHeader(content, activity.getString(R.string.load_kmz_successful));
      for (long categoryId : categoryIds)
      {
        BookmarkCategory category = BookmarkManager.INSTANCE.getCategoryById(categoryId);
        addResultRow(content, category.getName(), true /* selectable */, v -> {
          dialog[0].dismiss();
          listener.onCategorySelected(categoryId);
        });
      }
    }

    dialog[0] = new MaterialAlertDialogBuilder(activity, R.style.MwmTheme_AlertDialog)
                    .setTitle(R.string.load_kmz_title)
                    .setView(view)
                    .setPositiveButton(R.string.ok, null)
                    .create();
    dialog[0].show();
    return dialog[0];
  }

  private static void addSectionHeader(@NonNull LinearLayout content, @NonNull String title)
  {
    TextView header = new TextView(content.getContext());
    TextViewCompat.setTextAppearance(header, R.style.MwmTextAppearance_Body2);
    header.setText(title);
    ViewCompat.setAccessibilityHeading(header, true);
    int margin = dimension(content, R.dimen.margin_half);
    header.setPadding(0, margin, 0, 0);
    content.addView(header);
  }

  private static void addResultRow(@NonNull LinearLayout content, @NonNull String title, boolean selectable,
                                   @Nullable View.OnClickListener listener)
  {
    TextView row = new TextView(content.getContext());
    TextViewCompat.setTextAppearance(row, R.style.MwmTextAppearance_Body1);
    row.setText(title);
    row.setGravity(Gravity.CENTER_VERTICAL);
    row.setMinHeight(dimension(content, R.dimen.height_block_base));
    if (selectable)
    {
      TypedValue background = new TypedValue();
      content.getContext().getTheme().resolveAttribute(android.R.attr.selectableItemBackground, background, true);
      row.setBackgroundResource(background.resourceId);
      row.setClickable(true);
      row.setFocusable(true);
      row.setOnClickListener(listener);
    }
    else
      row.setTextColor(ContextCompat.getColor(content.getContext(), R.color.base_red));
    content.addView(row);
  }

  private static int dimension(@NonNull View view, int resourceId)
  {
    Resources resources = view.getResources();
    return resources.getDimensionPixelSize(resourceId);
  }
}
