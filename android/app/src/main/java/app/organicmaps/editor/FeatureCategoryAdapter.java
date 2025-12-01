package app.organicmaps.editor;

import static app.organicmaps.sdk.util.Utils.getLocalizedFeatureType;

import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.FeatureCategory;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.UiUtils;
import com.google.android.material.textfield.TextInputEditText;
import java.util.ArrayList;
import java.util.List;

public class FeatureCategoryAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  private static final int TYPE_RECENT_HEADER = 0;
  private static final int TYPE_CATEGORY = 1;
  private static final int TYPE_FOOTER = 2;

  private FeatureCategory[] mCategories;
  private final FeatureCategoryFragment mFragment;
  private final FeatureCategory mSelectedCategory;
  private List<String> mRecentTypes = new ArrayList<>();

  public interface FooterListener
  {
    void onNoteTextChanged(String newText);
    void onSendNoteClicked();
  }

  public FeatureCategoryAdapter(@NonNull FeatureCategoryFragment host, @NonNull FeatureCategory[] categories,
                                @Nullable FeatureCategory category, @NonNull List<String> recentTypes)
  {
    mFragment = host;
    mCategories = categories;
    mSelectedCategory = category;
    mRecentTypes = recentTypes;
  }

  public void setCategories(FeatureCategory[] categories, @NonNull List<String> recentTypes)
  {
    mCategories = categories;
    mRecentTypes = recentTypes;
    notifyDataSetChanged();
  }

  @Override
  public int getItemViewType(int position)
  {
    int recentCount = mRecentTypes.size();
    if (recentCount > 0)
    {
      if (position == 0)
        return TYPE_RECENT_HEADER;
      if (position > 0 && position <= recentCount)
        return TYPE_CATEGORY; // Recent items
      if (position == recentCount + 1 + mCategories.length)
        return TYPE_FOOTER;
      return TYPE_CATEGORY; // Regular categories
    }
    else
    {
      if (position == mCategories.length)
        return TYPE_FOOTER;
      else
        return TYPE_CATEGORY;
    }
  }

  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    switch (viewType)
    {
    case TYPE_RECENT_HEADER ->
    {
      return new RecentHeaderViewHolder(inflater.inflate(R.layout.item_recent_header, parent, false));
    }
    case TYPE_CATEGORY ->
    {
      return new FeatureViewHolder(inflater.inflate(R.layout.item_feature_category, parent, false));
    }
    case TYPE_FOOTER ->
    {
      return new FooterViewHolder(inflater.inflate(R.layout.item_feature_category_footer, parent, false),
                                  (FooterListener) mFragment);
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
    if (holder instanceof RecentHeaderViewHolder)
    {
      // Header doesn't need binding
    }
    else if (holder instanceof FeatureViewHolder)
    {
      ((FeatureViewHolder) holder).bind(position);
    }
    else if (holder instanceof FooterViewHolder)
    {
      ((FooterViewHolder) holder).bind(mFragment.getPendingNoteText());
    }
  }

  @Override
  public int getItemCount()
  {
    int recentCount = mRecentTypes.size();
    if (recentCount > 0)
    {
      // Header (1) + Recent items (recentCount) + All categories (mCategories.length) + Footer (1)
      return 1 + recentCount + mCategories.length + 1;
    }
    else
    {
      return mCategories.length + 1;
    }
  }
  
  private FeatureCategory getCategoryAtPosition(int position)
  {
    int recentCount = mRecentTypes.size();
    if (recentCount > 0)
    {
      if (position == 0)
        return null; // Header
      if (position <= recentCount)
      {
        // Recent item
        String recentType = mRecentTypes.get(position - 1);
        String localizedType = getLocalizedFeatureType(mFragment.requireContext(), recentType);
        return new FeatureCategory(recentType, localizedType);
      }
      else
      {
        // Regular category
        int categoryIndex = position - recentCount - 1;
        if (categoryIndex >= 0 && categoryIndex < mCategories.length)
          return mCategories[categoryIndex];
        return null; // Footer or out of bounds
      }
    }
    else
    {
      if (position >= 0 && position < mCategories.length)
        return mCategories[position];
      return null; // Footer or out of bounds
    }
  }

  protected class RecentHeaderViewHolder extends RecyclerView.ViewHolder
  {
    RecentHeaderViewHolder(@NonNull View itemView)
    {
      super(itemView);
    }
  }

  protected class FeatureViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mName;
    @NonNull
    private final View mSelected;

    FeatureViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mName = itemView.findViewById(R.id.name);
      mSelected = itemView.findViewById(R.id.selected);
      UiUtils.hide(mSelected);
      itemView.setOnClickListener(v -> {
        int position = getBindingAdapterPosition();
        if (position != RecyclerView.NO_POSITION)
          onCategorySelected(position);
      });
    }

    public void bind(int position)
    {
      FeatureCategory category = getCategoryAtPosition(position);
      if (category == null)
        return;
        
      mName.setText(category.getLocalizedTypeName());
      boolean showCondition =
          mSelectedCategory != null && category.getType().equals(mSelectedCategory.getType());
      UiUtils.showIf(showCondition, mSelected);
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

  private void onCategorySelected(int adapterPosition)
  {
    FeatureCategory category = getCategoryAtPosition(adapterPosition);
    if (category != null)
      mFragment.selectCategory(category);
  }
}
