
package org.holoeverywhere.addon;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

import org.holoeverywhere.addon.IAddon.Addon;

public final class IAddonBasicAttacher<V extends IAddonBase<Z>, Z> implements IAddonAttacher<V> {
    private final class AddonComparator implements Comparator<V> {
        @Override
        public int compare(V lhs, V rhs) {
            final int i1 = getWeight(lhs.getParent());
            final int i2 = getWeight(rhs.getParent());
            return i1 > i2 ? 1 : i1 < i2 ? -1 : 0;
        }

        private int getWeight(IAddon addon) {
            if (addon.getClass().isAnnotationPresent(Addon.class)) {
                return addon.getClass().getAnnotation(Addon.class).weight();
            }
            return -1;
        }
    }

    private final Map<Class<? extends IAddon>, V> mAddons = new HashMap<Class<? extends IAddon>, V>();
    private final Set<V> mAddonsList = new TreeSet<V>(new AddonComparator());
    private boolean mLockAttaching = false;
    private Z mObject;

    public IAddonBasicAttacher(Z object) {
        mObject = object;
    }

    @Override
    @SuppressWarnings("unchecked")
    public <T extends V> T addon(Class<? extends IAddon> clazz) {
        T addon = (T) mAddons.get(clazz);
        if (addon == null) {
            if (mLockAttaching) {
                throw new AttachException(mObject, clazz);
            }
            addon = IAddon.obtain(clazz, mObject);
            if (addon == null) {
                return null;
            }
            mAddons.put(clazz, addon);
            mAddonsList.add(addon);
        }
        return addon;
    }

    @Override
    public void addon(Collection<Class<? extends IAddon>> classes) {
        if (classes == null) {
            return;
        }
        for (Class<? extends IAddon> clazz : classes) {
            addon(clazz);
        }
    }

    @Override
    public <T extends V> T addon(String classname) {
        return addon(IAddon.makeAddonClass(classname));
    }

    public void inhert(Collection<Class<? extends IAddon>> sourceClasses) {
        if (sourceClasses == null || sourceClasses.size() == 0) {
            return;
        }
        List<Class<? extends IAddon>> classes = new ArrayList<Class<? extends IAddon>>();
        for (Class<? extends IAddon> clazz : sourceClasses) {
            if (!clazz.isAnnotationPresent(Addon.class)) {
                continue;
            }
            Addon addon = clazz.getAnnotation(Addon.class);
            if (addon.inhert()) {
                classes.add(clazz);
            }
        }
        addon(classes);
    }

    public void inhert(IAddonAttacher<?> attacher) {
        inhert(attacher == null ? null : attacher.obtainAddonsList());
    }

    @Override
    public boolean isAddonAttached(Class<? extends IAddon> clazz) {
        return mAddons.containsKey(clazz);
    }

    @Override
    public void lockAttaching() {
        mLockAttaching = true;
    }

    @Override
    public Collection<Class<? extends IAddon>> obtainAddonsList() {
        return new ArrayList<Class<? extends IAddon>>(mAddons.keySet());
    }

    @Override
    public boolean performAddonAction(AddonCallback<V> callback) {
        if (mAddons.size() == 0) {
            return false;
        }
        callback.pre();
        boolean result = false;
        for (V addon : mAddonsList) {
            result = callback.performAction(addon);
            if (callback.mStopped) {
                return result;
            }
        }
        return callback.post();
    }

    public void reset() {
        mAddons.clear();
        mAddonsList.clear();
        mLockAttaching = false;
    }
}
