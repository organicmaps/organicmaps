package com.mapswithme.maps.adapter;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SimpleExpandableListAdapter;

import java.util.List;
import java.util.Map;

/**
 * Disables child selections, also fixes bug with SimpleExpandableListAdapter not switching expandedGroupLayout and collapsedGroupLayout correctly.
 */
public class DisabledChildSimpleExpandableListAdapter extends SimpleExpandableListAdapter
{
  public DisabledChildSimpleExpandableListAdapter(Context context, List<? extends Map<String, ?>> groupData, int expandedGroupLayout, int collapsedGroupLayout, String[] groupFrom, int[] groupTo, List<? extends List<? extends Map<String, ?>>> childData, int childLayout, String[] childFrom, int[] childTo)
  {
    super(context, groupData, expandedGroupLayout, collapsedGroupLayout, groupFrom, groupTo, childData, childLayout, childFrom, childTo);
  }

  @Override
  public boolean isChildSelectable(int groupPosition, int childPosition)
  {
    return false;
  }

  /*
   * Quick bugfix, pass convertView param null always to change expanded-collapsed groupview correctly.
   * See http://stackoverflow.com/questions/19520037/simpleexpandablelistadapter-and-expandedgrouplayout for details
   */
  @Override
  public View getGroupView(int groupPosition, boolean isExpanded, View convertView,
                           ViewGroup parent)
  {
    return super.getGroupView(groupPosition, isExpanded, null, parent);
  }
}
