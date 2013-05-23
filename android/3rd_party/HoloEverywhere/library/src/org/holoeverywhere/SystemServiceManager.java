
package org.holoeverywhere;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

import org.holoeverywhere.SystemServiceManager.SystemServiceCreator.SystemService;

import android.content.Context;

/**
 * Manager of system services
 *
 * @author prok (prototypegamez@gmail.com)
 */
public final class SystemServiceManager {
    public static interface SuperSystemService {
        public Object superGetSystemService(String name);
    }

    public static interface SystemServiceCreator<T> {
        @Target(ElementType.TYPE)
        @Retention(RetentionPolicy.RUNTIME)
        public static @interface SystemService {
            public String value();
        }

        public T createService(Context context);
    }

    private static final Map<Class<? extends SystemServiceCreator<?>>, SystemServiceCreator<?>> CREATORS_MAP = new HashMap<Class<? extends SystemServiceCreator<?>>, SystemServiceManager.SystemServiceCreator<?>>();
    private static final Map<String, Class<? extends SystemServiceCreator<?>>> MAP = new HashMap<String, Class<? extends SystemServiceCreator<?>>>();

    private static Object getSuperSystemService(Context context, String name) {
        if (context instanceof SuperSystemService) {
            return ((SuperSystemService) context).superGetSystemService(name);
        } else {
            return context.getSystemService(name);
        }
    }

    public static Object getSystemService(Context context, String name) {
        if (context == null || context.isRestricted()) {
            throw new RuntimeException("Invalid context");
        } else if (name == null || name.length() == 0) {
            return null;
        }
        Class<? extends SystemServiceCreator<?>> clazz = MAP.get(name);
        if (clazz == null) {
            return getSuperSystemService(context, name);
        }
        SystemServiceCreator<?> creator = CREATORS_MAP.get(clazz);
        if (creator == null) {
            try {
                creator = clazz.newInstance();
            } catch (Exception e) {
                e.printStackTrace();
            }
            CREATORS_MAP.put(clazz, creator);
        }
        if (creator != null) {
            Object o = creator.createService(context);
            if (o != null) {
                return o;
            }
        }
        return getSuperSystemService(context, name);
    }

    public static void register(Class<? extends SystemServiceCreator<?>> clazz) {
        if (!clazz.isAnnotationPresent(SystemService.class)) {
            throw new RuntimeException(
                    "SystemServiceCreator must be implement SystemService");
        }
        SystemService systemService = clazz.getAnnotation(SystemService.class);
        final String name = systemService.value();
        if (name == null || name.length() == 0) {
            throw new RuntimeException("SystemService has incorrect name");
        }
        MAP.put(name, clazz);
    }

    public static synchronized void unregister(
            Class<? extends SystemServiceCreator<?>> clazz) {
        if (MAP.containsValue(clazz)) {
            for (Entry<String, Class<? extends SystemServiceCreator<?>>> e : MAP
                    .entrySet()) {
                if (e.getValue() == clazz) {
                    MAP.remove(e.getKey());
                    break;
                }
            }
        }
        CREATORS_MAP.remove(clazz);
    }

    private SystemServiceManager() {
    }
}
