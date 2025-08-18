package app.organicmaps.settings;

import android.app.Activity;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.format.Formatter;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckedTextView;
import app.organicmaps.R;
import app.organicmaps.sdk.settings.StorageItem;
import app.organicmaps.sdk.settings.StoragePathManager;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.util.ThemeUtils;

class StoragePathAdapter extends BaseAdapter
{
  private final StoragePathManager mPathManager;
  private final Activity mActivity;
  private long mSizeNeeded;

  public StoragePathAdapter(StoragePathManager pathManager, Activity activity)
  {
    mPathManager = pathManager;
    mActivity = activity;
  }

  @Override
  public int getCount()
  {
    return (mPathManager.mStorages == null ? 0 : mPathManager.mStorages.size());
  }

  @Override
  public StorageItem getItem(int position)
  {
    return mPathManager.mStorages.get(position);
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
      convertView = mActivity.getLayoutInflater().inflate(R.layout.item_storage, parent, false);

    StorageItem item = mPathManager.mStorages.get(position);
    final boolean isCurrent = position == mPathManager.mCurrentStorageIndex;
    CheckedTextView checkedView = (CheckedTextView) convertView;
    checkedView.setChecked(isCurrent);
    checkedView.setEnabled(!item.mIsReadonly && (isStorageBigEnough(position) || isCurrent));

    final String size =
        mActivity.getString(R.string.maps_storage_free_size, Formatter.formatShortFileSize(mActivity, item.mFreeSize),
                            Formatter.formatShortFileSize(mActivity, item.mTotalSize));

    SpannableStringBuilder sb = new SpannableStringBuilder(item.mLabel + "\n" + size);
    sb.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(mActivity, android.R.attr.textColorSecondary)),
               sb.length() - size.length(), sb.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    sb.setSpan(new AbsoluteSizeSpan(Utils.dimen(mActivity, R.dimen.text_size_body_3)), sb.length() - size.length(),
               sb.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

    final String path = item.mPath + (item.mIsReadonly ? " (read-only)" : "");
    sb.append("\n").append(path);
    sb.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(mActivity, android.R.attr.textColorSecondary)),
               sb.length() - path.length(), sb.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    sb.setSpan(new AbsoluteSizeSpan(Utils.dimen(mActivity, R.dimen.text_size_body_4)), sb.length() - path.length(),
               sb.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

    checkedView.setText(sb);

    return convertView;
  }

  public void update(long dirSize)
  {
    mSizeNeeded = dirSize;
    notifyDataSetChanged();
  }

  public boolean isStorageBigEnough(int index)
  {
    return mPathManager.mStorages.get(index).mFreeSize >= mSizeNeeded;
  }
}
