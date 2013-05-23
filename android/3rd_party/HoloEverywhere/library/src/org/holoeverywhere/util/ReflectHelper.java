
package org.holoeverywhere.util;

import java.lang.reflect.Method;

public final class ReflectHelper {

    private static Class<?>[] classess(Object[] args) {
        Class<?>[] result = new Class<?>[args.length];
        for (int i = 0; i < args.length; i++) {
            Object z = args[i];
            if (z != null) {
                result[i] = z.getClass();
            }
        }
        return result;
    }

    public static <Result> Result invoke(Object object, String methodName,
            Class<Result> result, boolean superClass, Object... args) {
        try {
            Class<?>[] argsClasses = ReflectHelper.classess(args);
            Method method = (superClass ? object.getClass().getSuperclass()
                    : object.getClass()).getMethod(methodName, argsClasses);
            method.setAccessible(true);
            Object r = method.invoke(object, args);
            return result != null ? result.cast(r) : null;
        } catch (Exception e) {
            return null;
        }
    }

    public static <Result> Result invoke(Object object, String methodName,
            Class<Result> result, Object... args) {
        return ReflectHelper.invoke(object, methodName, result, false, args);
    }

}
