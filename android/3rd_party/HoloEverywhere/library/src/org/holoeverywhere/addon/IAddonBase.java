
package org.holoeverywhere.addon;

public abstract class IAddonBase<T> {
    private T mObject;
    private IAddon mParent;

    public final void attach(T object, IAddon parent) {
        if (mObject != null || object == null || mParent != null || parent == null) {
            throw new IllegalStateException();
        }
        mParent = parent;
        onAttach(mObject = object);
    }

    public T get() {
        return mObject;
    }

    public final IAddon getParent() {
        return mParent;
    }

    protected void onAttach(T object) {

    }
}
