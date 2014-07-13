package com.mapswithme.maps;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.guides.GuideInfo;
import com.mapswithme.maps.guides.GuidesUtils;

import java.util.HashMap;
import java.util.Map;

public class MoreAppsAdapter extends BaseAdapter
{
  private final Context mContext;

  private String[] mGuideIds;
  private LayoutInflater mInflater;
  private Map<String, Integer> mAppIcons = new HashMap<String, Integer>(); // maps App IDs of Guides to icon resources

  public MoreAppsAdapter(Context context)
  {
    mGuideIds = Framework.getGuideIds();
    mContext = context;
    mInflater = LayoutInflater.from(context);

    initIcons();
  }

  private void initIcons()
  {
    mAppIcons.clear();
    mAppIcons.put("com.guidewithme.california", R.drawable.ic_us_ca_guides_icon);
    mAppIcons.put("com.guidewithme.czech.republic", R.drawable.ic_cz_guides_icon);
    mAppIcons.put("com.guidewithme.estonia", R.drawable.ic_ee_guides_icon);
    mAppIcons.put("com.guidewithme.france", R.drawable.ic_fr_guides_icon);
    mAppIcons.put("com.guidewithme.germany", R.drawable.ic_de_guides_icon);
    mAppIcons.put("com.guidewithme.greece", R.drawable.ic_gr_guides_icon);
    mAppIcons.put("com.guidewithme.hawaii", R.drawable.ic_us_hi_guides_icon);
    mAppIcons.put("com.guidewithme.hong.kong", R.drawable.ic_hk_guides_icon);
    mAppIcons.put("com.guidewithme.italy", R.drawable.ic_it_guides_icon);
    mAppIcons.put("com.guidewithme.japan", R.drawable.ic_jp_guides_icon);
    mAppIcons.put("com.guidewithme.latvia", R.drawable.ic_lv_guides_icon);
    mAppIcons.put("com.guidewithme.lithuania", R.drawable.ic_lt_guides_icon);
    mAppIcons.put("com.guidewithme.malaysia", R.drawable.ic_my_guides_icon);
    mAppIcons.put("com.guidewithme.montenegro", R.drawable.ic_me_guides_icon);
    mAppIcons.put("com.guidewithme.nepal", R.drawable.ic_np_guides_icon);
    mAppIcons.put("com.guidewithme.new.zealand", R.drawable.ic_nz_guides_icon);
    mAppIcons.put("com.guidewithme.poland", R.drawable.ic_pl_guides_icon);
    mAppIcons.put("com.guidewithme.russia", R.drawable.ic_ru_guides_icon);
    mAppIcons.put("com.guidewithme.singapore", R.drawable.ic_sg_guides_icon);
    mAppIcons.put("com.guidewithme.spain", R.drawable.ic_es_guides_icon);
    mAppIcons.put("com.guidewithme.sri.lanka", R.drawable.ic_lk_guides_icon);
    mAppIcons.put("com.guidewithme.switzerland", R.drawable.ic_hk_guides_icon);
    mAppIcons.put("com.guidewithme.taiwan", R.drawable.ic_tw_guides_icon);
    mAppIcons.put("com.guidewithme.thailand", R.drawable.ic_th_guides_icon);
    mAppIcons.put("com.guidewithme.turkey", R.drawable.ic_tr_guides_icon);
    mAppIcons.put("com.guidewithme.uk", R.drawable.ic_uk_guides_icon);
  }

  @Override
  public int getCount()
  {
    return mGuideIds.length;
  }

  @Override
  public GuideInfo getItem(int position)
  {
    return Framework.getGuideInfoForIdWithApiCheck(mGuideIds[position]);
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    View view = convertView;
    ViewHolder holder;
    if (view == null)
    {
      view = mInflater.inflate(R.layout.list_item_more_apps, parent, false);
      holder = new ViewHolder(view);
      view.setTag(holder);
    }
    else
      holder = (ViewHolder) view.getTag();

    final GuideInfo info = getItem(position);
    if (info != null)
    {
      holder.name.setText(info.mName);
      int res = 0;
      if (mAppIcons.get(info.mAppId) != null)
        res = mAppIcons.get(info.mAppId);
      holder.image.setImageResource(res);
      view.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          GuidesUtils.openOrDownloadGuide(info, mContext);
        }
      });
    }

    return view;
  }

  private static class ViewHolder
  {
    public ImageView image;
    public TextView name;

    public ViewHolder(View v)
    {
      image = (ImageView) v.findViewById(R.id.iv_guide);
      name = (TextView) v.findViewById(R.id.tv_guide_name);
    }
  }

}
