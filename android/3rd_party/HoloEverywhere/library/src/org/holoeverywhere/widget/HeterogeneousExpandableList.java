
package org.holoeverywhere.widget;

public interface HeterogeneousExpandableList {
    int getChildType(int groupPosition, int childPosition);

    int getChildTypeCount();

    int getGroupType(int groupPosition);

    int getGroupTypeCount();
}
