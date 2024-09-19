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
import app.organicmaps.editor.data.FeatureCategory;
import app.organicmaps.util.UiUtils;

public class FeatureCategoryAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{

  private static final int TYPE_CATEGORY = 0;
  private static final int TYPE_FOOTER = 1;

  private FeatureCategory[] mCategories;
  private final FeatureCategoryFragment mFragment;
  private final FeatureCategory mSelectedCategory;

  public FeatureCategoryAdapter(@NonNull FeatureCategoryFragment host, @NonNull FeatureCategory[] categories, @Nullable FeatureCategory category)
  {
    mFragment = host;
    mCategories = categories;
    mSelectedCategory = category;
  }

  public void setCategories(FeatureCategory[] categories)
  {
    mCategories = categories;
    notifyDataSetChanged();
  }

  @Override
  public int getItemViewType(int position)
  {
    if (position == mCategories.length)
      return TYPE_FOOTER;
    else
      return TYPE_CATEGORY;
  }

  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    switch (viewType) {
      case TYPE_CATEGORY -> {
        return new FeatureViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_feature_category, parent, false));
      }
      case TYPE_FOOTER -> {
        return new FooterViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_feature_category_footer, parent, false));
      }
      default -> {
        throw new IllegalArgumentException("Unsupported");
      }
    }
  }

  @Override
  public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    if (holder instanceof FeatureViewHolder)
    {
      ((FeatureViewHolder) holder).bind(position);
    }
  }

  @Override
  public int getItemCount()
  {
    return mCategories.length + 1;
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
      itemView.setOnClickListener(v -> onCategorySelected(getBindingAdapterPosition()));
    }

    public void bind(int position)
    {
      mName.setText(mCategories[position].getLocalizedTypeName());
      boolean showCondition = mSelectedCategory != null
                              && mCategories[position].getType().equals(mSelectedCategory.getType());
      UiUtils.showIf(showCondition, mSelected);
    }
  }

  protected static class FooterViewHolder extends RecyclerView.ViewHolder
  {

    FooterViewHolder(@NonNull View itemView)
    {
      super(itemView);
      TextView categoryUnsuitableText = itemView.findViewById(R.id.editor_category_unsuitable_text);
      categoryUnsuitableText.setMovementMethod(LinkMovementMethod.getInstance());
    }
  }

  private void onCategorySelected(int adapterPosition)
  {
    mFragment.selectCategory(mCategories[adapterPosition]);
  }
}
