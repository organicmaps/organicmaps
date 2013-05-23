
package org.holoeverywhere.addon;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.util.HashMap;
import java.util.Map;

import org.holoeverywhere.HoloEverywhere;
import org.holoeverywhere.util.WeaklyMap;

public abstract class IAddon {
    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.TYPE)
    public @interface Addon {
        public boolean inhert() default false;

        public int weight() default -1;
    }

    private static final Map<Class<? extends IAddon>, IAddon> sAddonsMap = new HashMap<Class<? extends IAddon>, IAddon>();

    @SuppressWarnings("unchecked")
    public static <T extends IAddon> T addon(Class<T> clazz) {
        try {
            T t = (T) sAddonsMap.get(clazz);
            if (t == null) {
                t = clazz.newInstance();
                sAddonsMap.put(clazz, t);
            }
            return t;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public static <T extends IAddon> T addon(String classname) {
        Class<T> clazz = makeAddonClass(classname);
        return addon(clazz);
    }

    @SuppressWarnings("unchecked")
    public static <T extends IAddon> Class<T> makeAddonClass(String classname) {
        if (classname.contains(".")) {
            try {
                return (Class<T>) Class.forName(classname);
            } catch (ClassNotFoundException e) {
                throw new RuntimeException(e);
            }
        } else {
            return makeAddonClass(HoloEverywhere.PACKAGE + ".addon.Addon" + classname);
        }
    }

    public static <T extends IAddon, Z extends IAddonBase<V>, V> Z obtain(Class<T> clazz, V object) {
        return addon(clazz).obtain(object);
    }

    public static <T extends IAddon, Z extends IAddonBase<V>, V> Z obtain(String classname, V object) {
        return addon(classname).obtain(object);
    }

    private final Map<Object, Object> mStatesMap = new WeaklyMap<Object, Object>();
    private final Map<Class<?>, Class<? extends IAddonBase<?>>> mTypesMap = new HashMap<Class<?>, Class<? extends IAddonBase<?>>>();

    @SuppressWarnings("unchecked")
    public <T, V extends IAddonBase<T>> V obtain(T object) {
        try {
            V addon = (V) mStatesMap.get(object);
            if (addon != null) {
                return addon;
            }
            Class<?> clazz = object.getClass();
            while (!mTypesMap.containsKey(clazz)) {
                if (clazz == Object.class) {
                    // Nothing was found
                    return null;
                }
                clazz = clazz.getSuperclass();
            }
            addon = ((Class<V>) mTypesMap.get(clazz)).newInstance();
            addon.attach(object, this);
            mStatesMap.put(object, addon);
            return addon;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public <T> void register(Class<T> clazz, Class<? extends IAddonBase<T>> addonClazz) {
        mTypesMap.put(clazz, addonClazz);
    }

    public void unregister(Class<?> clazz) {
        mTypesMap.remove(clazz);
    }
}
