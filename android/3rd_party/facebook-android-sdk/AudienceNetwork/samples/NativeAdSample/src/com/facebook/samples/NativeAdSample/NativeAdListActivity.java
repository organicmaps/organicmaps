package com.facebook.samples.NativeAdSample;

import java.util.ArrayList;
import java.util.List;

import com.facebook.ads.Ad;
import com.facebook.ads.AdError;
import com.facebook.ads.AdListener;
import com.facebook.ads.NativeAd;

import android.app.ListActivity;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class NativeAdListActivity extends ListActivity implements AdListener {

    private ListView listView;
    private ListViewAdapter adapter;
    private NativeAd listNativeAd;

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        listNativeAd = new NativeAd(this, "YOUR_PLACEMENT_ID");
        listNativeAd.setAdListener(this);
        listNativeAd.loadAd();

        listView = getListView();
        adapter = new ListViewAdapter(getApplicationContext());
        listView.setAdapter(adapter);
    }

    @Override
    public void onAdClicked(Ad arg0) {
        Toast.makeText(this, "Ad Clicked", Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onAdLoaded(Ad ad) {
        adapter.addNativeAd((NativeAd) ad);
    }

    @Override
    public void onError(Ad ad, AdError error) {
        Toast.makeText(this, "Ad failed to load: " +  error.getErrorMessage(), Toast.LENGTH_SHORT).show();
    }

    class ListViewAdapter extends BaseAdapter {

        private LayoutInflater inflater;
        private List<Object> list;

        private NativeAd ad;
        private static final int AD_INDEX = 2;

        public ListViewAdapter(Context context) {
            list = new ArrayList<Object>();
            inflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            for (int i = 1; i <= 35; i++) {
                list.add("ListView Item #" + i);
            }
        }

        @Override
        public int getCount() {
            return list.size();
        }

        @Override
        public Object getItem(int position) {
            return list.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (position == AD_INDEX && ad != null) {
                // Return the native ad view
                return (View) list.get(position);
            } else {
                TextView view; // Default item type (non-ad)
                if (convertView != null && convertView instanceof TextView) {
                    view = (TextView) convertView;
                } else {
                    view = (TextView) inflater.inflate(R.layout.list_item, parent, false);
                }
                view.setText((String) list.get(position));
                return view;
            }
        }

        public synchronized void addNativeAd(NativeAd ad) {
            if (ad == null) {
                return;
            }
            if (this.ad != null) {
                // Clean up the old ad before inserting the new one
                this.ad.unregisterView();
                this.list.remove(AD_INDEX);
                this.ad = null;
                this.notifyDataSetChanged();
            }
            this.ad = ad;
            View adView = inflater.inflate(R.layout.ad_unit, null);
            NativeAdSampleActivity.inflateAd(ad, adView, NativeAdListActivity.this);
            list.add(AD_INDEX, adView);
            this.notifyDataSetChanged();
        }
    }
}
