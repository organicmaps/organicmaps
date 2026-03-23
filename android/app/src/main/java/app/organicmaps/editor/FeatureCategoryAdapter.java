package app.organicmaps.editor;

import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.FeatureCategory;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.UiUtils;
import com.google.android.material.textfield.TextInputEditText;

public class FeatureCategoryAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  private static final int TYPE_CATEGORY = 0;
  private static final int TYPE_FOOTER = 1;
  private static final int TYPE_SECTION = 2;

  private static final int SECTION_RECENT = 0;
  private static final int SECTION_ALL = 1;

  private FeatureCategory[] mCategories;
  private FeatureCategory[] mRecentCategories;
  private final FeatureCategoryFragment mFragment;
  private final FeatureCategory mSelectedCategory;

  public interface FooterListener
  {
    void onNoteTextChanged(String newText);
    void onSendNoteClicked();
  }

  public FeatureCategoryAdapter(@NonNull FeatureCategoryFragment host, @NonNull FeatureCategory[] categories,
                                @NonNull FeatureCategory[] recentCategories, @Nullable FeatureCategory category)
  {
    mFragment = host;
    mCategories = categories;
    mRecentCategories = recentCategories;
    mSelectedCategory = category;
  }

  public void setCategories(@NonNull FeatureCategory[] categories, @NonNull FeatureCategory[] recentCategories)
  {
    mCategories = categories;
    mRecentCategories = recentCategories;
    notifyDataSetChanged();
  }

  private boolean hasRecentSection()
  {
    return mRecentCategories.length > 0;
  }

  private int getSectionPosition(int section)
  {
    if (!hasRecentSection())
      return -1;
    return section == SECTION_RECENT ? 0 : (mRecentCategories.length + 1);
  }

  private int getFooterPosition()
  {
    if (hasRecentSection())
      return mRecentCategories.length + 1 + mCategories.length + 1;
    else
      return mCategories.length;
  }

  @Nullable
  private FeatureCategory getCategoryAtPosition(int position)
  {
    if (hasRecentSection())
    {
      int recentHeaderPos = getSectionPosition(SECTION_RECENT);
      int allHeaderPos = getSectionPosition(SECTION_ALL);
      if (position == recentHeaderPos || position == allHeaderPos)
        return null;
      if (position > recentHeaderPos && position < allHeaderPos)
      {
        int recentIndex = position - recentHeaderPos - 1;
        if (recentIndex >= 0 && recentIndex < mRecentCategories.length)
          return mRecentCategories[recentIndex];
        return null;
      }
      // All categories: after all header
      int categoryIndex = position - allHeaderPos - 1;
      if (categoryIndex >= 0 && categoryIndex < mCategories.length)
        return mCategories[categoryIndex];
      return null;
    }
    else
    {
      // No recent section, direct mapping
      if (position < mCategories.length)
        return mCategories[position];
      return null;
    }
  }

  @Override
  public int getItemViewType(int position)
  {
    if (position == getFooterPosition())
      return TYPE_FOOTER;
    if (position == getSectionPosition(SECTION_RECENT) || position == getSectionPosition(SECTION_ALL))
      return TYPE_SECTION;
    return TYPE_CATEGORY;
  }

  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    switch (viewType)
    {
    case TYPE_CATEGORY ->
    {
      return new FeatureViewHolder(inflater.inflate(R.layout.item_feature_category, parent, false));
    }
    case TYPE_FOOTER ->
    {
      return new FooterViewHolder(inflater.inflate(R.layout.item_feature_category_footer, parent, false),
                                  (FooterListener) mFragment);
    }
    case TYPE_SECTION ->
    {
      return new SectionHeaderViewHolder(inflater.inflate(R.layout.item_category_title, parent, false));
    }
    default ->
    {
      throw new IllegalArgumentException("Unsupported viewType: " + viewType);
    }
    }
  }

  @Override
  public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    if (holder instanceof FeatureViewHolder)
    {
      FeatureCategory category = getCategoryAtPosition(position);
      if (category != null)
        ((FeatureViewHolder) holder).bind(category);
    }
    else if (holder instanceof FooterViewHolder)
    {
      ((FooterViewHolder) holder).bind(mFragment.getPendingNoteText());
    }
    else if (holder instanceof SectionHeaderViewHolder)
    {
      boolean isRecentHeader = position == getSectionPosition(SECTION_RECENT);
      ((SectionHeaderViewHolder) holder).bind(isRecentHeader);
    }
  }

  @Override
  public int getItemCount()
  {
    if (hasRecentSection())
      // Recent header + recent items + All header + all categories + footer
      return 1 + mRecentCategories.length + 1 + mCategories.length + 1;
    else
      return mCategories.length + 1;
  }

  protected class FeatureViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mName;
    @NonNull
    private final View mSelected;
    private FeatureCategory mCategory;

    FeatureViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mName = itemView.findViewById(R.id.name);
      mSelected = itemView.findViewById(R.id.selected);
      UiUtils.hide(mSelected);
      itemView.setOnClickListener(v -> {
        if (mCategory != null)
          mFragment.selectCategory(mCategory);
      });
    }

    public void bind(@NonNull FeatureCategory category)
    {
      mCategory = category;
      mName.setText(category.getLocalizedTypeName());
      boolean showCondition = mSelectedCategory != null && category.getType().equals(mSelectedCategory.getType());
      UiUtils.showIf(showCondition, mSelected);
    }
  }

  public static class SectionHeaderViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mTitle;

    SectionHeaderViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mTitle = itemView.findViewById(R.id.text);
    }

    public void bind(boolean isRecentHeader)
    {
      if (isRecentHeader)
        mTitle.setText(R.string.editor_add_select_category_recent_subtitle);
      else
        mTitle.setText(R.string.editor_add_select_category_all_subtitle);
    }
  }

  protected static class FooterViewHolder extends RecyclerView.ViewHolder
  {
    private final TextInputEditText mNoteEditText;
    private final View mSendNoteButton;

    FooterViewHolder(@NonNull View itemView, @NonNull FooterListener listener)
    {
      super(itemView);
      TextView categoryUnsuitableText = itemView.findViewById(R.id.editor_category_unsuitable_text);
      categoryUnsuitableText.setMovementMethod(LinkMovementMethod.getInstance());
      mNoteEditText = itemView.findViewById(R.id.note_edit_text);
      mSendNoteButton = itemView.findViewById(R.id.send_note_button);
      mSendNoteButton.setOnClickListener(v -> listener.onSendNoteClicked());
      mNoteEditText.addTextChangedListener(new StringUtils.SimpleTextWatcher() {
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count)
        {
          final String str = s.toString();
          listener.onNoteTextChanged(str);
          mSendNoteButton.setEnabled(!str.trim().isEmpty());
        }
      });
    }
    public void bind(String pendingNoteText)
    {
      if (!mNoteEditText.getText().toString().equals(pendingNoteText))
      {
        mNoteEditText.setText(pendingNoteText);
        if (pendingNoteText != null)
          mNoteEditText.setSelection(pendingNoteText.length());
      }
      mSendNoteButton.setEnabled(pendingNoteText != null && !pendingNoteText.trim().isEmpty());
    }
  }
}
