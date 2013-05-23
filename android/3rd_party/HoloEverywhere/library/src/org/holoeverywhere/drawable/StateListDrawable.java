
package org.holoeverywhere.drawable;

import java.io.IOException;

import org.holoeverywhere.R;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.StateSet;

public class StateListDrawable extends DrawableContainer {
    static final class StateListState extends DrawableContainerState {
        int[][] mStateSets;

        StateListState(StateListState orig, StateListDrawable owner, Resources res) {
            super(orig, owner, res);

            if (orig != null) {
                mStateSets = orig.mStateSets;
            } else {
                mStateSets = new int[getChildren().length][];
            }
        }

        int addStateSet(int[] stateSet, Drawable drawable) {
            final int pos = addChild(drawable);
            mStateSets[pos] = stateSet;
            return pos;
        }

        @Override
        public void growArray(int oldSize, int newSize) {
            super.growArray(oldSize, newSize);
            final int[][] newStateSets = new int[newSize][];
            System.arraycopy(mStateSets, 0, newStateSets, 0, oldSize);
            mStateSets = newStateSets;
        }

        private int indexOfStateSet(int[] stateSet) {
            final int[][] stateSets = mStateSets;
            final int N = getChildCount();
            for (int i = 0; i < N; i++) {
                if (StateSet.stateSetMatches(stateSets[i], stateSet)) {
                    return i;
                }
            }
            return -1;
        }

        @Override
        public Drawable newDrawable() {
            return new StateListDrawable(this, null);
        }

        @Override
        public Drawable newDrawable(Resources res) {
            return new StateListDrawable(this, res);
        }
    }

    private static final boolean DEFAULT_DITHER = true;
    private boolean mMutated;
    private final StateListState mStateListState;

    public StateListDrawable() {
        this(null, null);
    }

    private StateListDrawable(StateListState state, Resources res) {
        StateListState as = new StateListState(state, this, res);
        mStateListState = as;
        setConstantState(as);
        onStateChange(getState());
    }

    public void addState(int[] stateSet, Drawable drawable) {
        if (drawable != null) {
            mStateListState.addStateSet(stateSet, drawable);
            // in case the new state matches our current state...
            onStateChange(getState());
        }
    }

    /**
     * Gets the number of states contained in this drawable.
     *
     * @return The number of states contained in this drawable.
     * @hide pending API council
     * @see #getStateSet(int)
     * @see #getStateDrawable(int)
     */
    public int getStateCount() {
        return mStateListState.getChildCount();
    }

    /**
     * Gets the drawable at an index.
     *
     * @param index The index of the drawable.
     * @return The drawable at the index.
     * @hide pending API council
     * @see #getStateCount()
     * @see #getStateSet(int)
     */
    public Drawable getStateDrawable(int index) {
        return mStateListState.getChildren()[index];
    }

    /**
     * Gets the index of the drawable with the provided state set.
     *
     * @param stateSet the state set to look up
     * @return the index of the provided state set, or -1 if not found
     * @hide pending API council
     * @see #getStateDrawable(int)
     * @see #getStateSet(int)
     */
    public int getStateDrawableIndex(int[] stateSet) {
        return mStateListState.indexOfStateSet(stateSet);
    }

    StateListState getStateListState() {
        return mStateListState;
    }

    /**
     * Gets the state set at an index.
     *
     * @param index The index of the state set.
     * @return The state set at the index.
     * @hide pending API council
     * @see #getStateCount()
     * @see #getStateDrawable(int)
     */
    public int[] getStateSet(int index) {
        return mStateListState.mStateSets[index];
    }

    @Override
    public void inflate(Resources r, XmlPullParser parser,
            AttributeSet attrs)
            throws XmlPullParserException, IOException {
        TypedArray a = r.obtainAttributes(attrs,
                R.styleable.StateListDrawable);
        super.setVisible(a.getBoolean(R.styleable.StateListDrawable_android_visible, true), false);
        mStateListState.setVariablePadding(a.getBoolean(
                R.styleable.StateListDrawable_android_variablePadding, false));
        mStateListState.setConstantSize(a.getBoolean(
                R.styleable.StateListDrawable_android_constantSize, false));
        mStateListState.setEnterFadeDuration(a.getInt(
                R.styleable.StateListDrawable_android_enterFadeDuration, 0));
        mStateListState.setExitFadeDuration(a.getInt(
                R.styleable.StateListDrawable_android_exitFadeDuration, 0));
        setDither(a.getBoolean(R.styleable.StateListDrawable_android_dither, DEFAULT_DITHER));
        a.recycle();
        int type;
        final int innerDepth = parser.getDepth() + 1;
        int depth;
        while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                && ((depth = parser.getDepth()) >= innerDepth
                || type != XmlPullParser.END_TAG)) {
            if (type != XmlPullParser.START_TAG) {
                continue;
            }
            if (depth > innerDepth || !parser.getName().equals("item")) {
                continue;
            }
            int drawableRes = 0;
            int i;
            int j = 0;
            final int numAttrs = attrs.getAttributeCount();
            int[] states = new int[numAttrs];
            for (i = 0; i < numAttrs; i++) {
                final int stateResId = attrs.getAttributeNameResource(i);
                if (stateResId == 0) {
                    break;
                }
                if (stateResId == android.R.attr.drawable) {
                    drawableRes = attrs.getAttributeResourceValue(i, 0);
                } else {
                    states[j++] = attrs.getAttributeBooleanValue(i, false)
                            ? stateResId
                            : -stateResId;
                }
            }
            states = StateSet.trimStateSet(states, j);
            Drawable dr;
            if (drawableRes != 0) {
                dr = DrawableCompat.getDrawable(r, drawableRes);
            } else {
                while ((type = parser.next()) == XmlPullParser.TEXT) {
                }
                if (type != XmlPullParser.START_TAG) {
                    throw new XmlPullParserException(
                            parser.getPositionDescription()
                                    + ": <item> tag requires a 'drawable' attribute or "
                                    + "child tag defining a drawable");
                }
                dr = DrawableCompat.createFromXmlInner(r, parser, attrs);
            }
            mStateListState.addStateSet(states, dr);
        }
        onStateChange(getState());
    }

    @Override
    public boolean isStateful() {
        return true;
    }

    @Override
    public Drawable mutate() {
        if (!mMutated && super.mutate() == this) {
            mStateListState.mStateSets = mStateListState.mStateSets.clone();
            mMutated = true;
        }
        return this;
    }

    @Override
    protected boolean onStateChange(int[] stateSet) {
        int idx = mStateListState.indexOfStateSet(stateSet);
        if (idx < 0) {
            idx = mStateListState.indexOfStateSet(StateSet.WILD_CARD);
        }
        if (selectDrawable(idx)) {
            return true;
        }
        return super.onStateChange(stateSet);
    }
}
