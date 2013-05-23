
package org.holoeverywhere.widget;

import java.util.ArrayList;

public class ExpandableListPosition {
    public final static int CHILD = 1;
    public final static int GROUP = 2;
    private static final int MAX_POOL_SIZE = 5;
    private static ArrayList<ExpandableListPosition> sPool =
            new ArrayList<ExpandableListPosition>(MAX_POOL_SIZE);

    private static ExpandableListPosition getRecycledOrCreate() {
        ExpandableListPosition elp;
        synchronized (sPool) {
            if (sPool.size() > 0) {
                elp = sPool.remove(0);
            } else {
                return new ExpandableListPosition();
            }
        }
        elp.resetState();
        return elp;
    }

    static ExpandableListPosition obtain(int type, int groupPos, int childPos, int flatListPos) {
        ExpandableListPosition elp = getRecycledOrCreate();
        elp.type = type;
        elp.groupPos = groupPos;
        elp.childPos = childPos;
        elp.flatListPos = flatListPos;
        return elp;
    }

    static ExpandableListPosition obtainChildPosition(int groupPosition, int childPosition) {
        return obtain(CHILD, groupPosition, childPosition, 0);
    }

    static ExpandableListPosition obtainGroupPosition(int groupPosition) {
        return obtain(GROUP, groupPosition, 0, 0);
    }

    static ExpandableListPosition obtainPosition(long packedPosition) {
        if (packedPosition == ExpandableListView.PACKED_POSITION_VALUE_NULL) {
            return null;
        }

        ExpandableListPosition elp = getRecycledOrCreate();
        elp.groupPos = ExpandableListView.getPackedPositionGroup(packedPosition);
        if (ExpandableListView.getPackedPositionType(packedPosition) == ExpandableListView.PACKED_POSITION_TYPE_CHILD) {
            elp.type = CHILD;
            elp.childPos = ExpandableListView.getPackedPositionChild(packedPosition);
        } else {
            elp.type = GROUP;
        }
        return elp;
    }

    public int childPos;

    int flatListPos;

    public int groupPos;

    public int type;

    private ExpandableListPosition() {
    }

    long getPackedPosition() {
        if (type == CHILD) {
            return ExpandableListView.getPackedPositionForChild(groupPos, childPos);
        } else {
            return ExpandableListView.getPackedPositionForGroup(groupPos);
        }
    }

    public void recycle() {
        synchronized (sPool) {
            if (sPool.size() < MAX_POOL_SIZE) {
                sPool.add(this);
            }
        }
    }

    private void resetState() {
        groupPos = 0;
        childPos = 0;
        flatListPos = 0;
        type = 0;
    }
}
