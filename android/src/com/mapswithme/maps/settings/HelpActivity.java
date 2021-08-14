package com.mapswithme.maps.settings;

import androidx.fragment.app.Fragment;

import com.mapswithme.maps.base.BaseToolbarActivity;

public class HelpActivity extends BaseToolbarActivity
{
    @Override
    protected Class<? extends Fragment> getFragmentClass()
    {
        return HelpFragment.class;
    }
}
