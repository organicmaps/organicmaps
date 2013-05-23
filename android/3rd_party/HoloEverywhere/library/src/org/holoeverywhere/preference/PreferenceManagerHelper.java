
package org.holoeverywhere.preference;

import org.holoeverywhere.HoloEverywhere;
import org.holoeverywhere.HoloEverywhere.PreferenceImpl;

import android.content.Context;
import android.util.Log;

public class PreferenceManagerHelper {
    static interface PreferenceManagerImpl {
        SharedPreferences getDefaultSharedPreferences(Context context, PreferenceImpl impl);

        int obtainThemeTag();

        SharedPreferences wrap(Context context, PreferenceImpl impl, String name, int mode);
    }

    private static PreferenceManagerImpl IMPL;
    static {
        try {
            Class<?> clazz = Class
                    .forName(HoloEverywhere.PACKAGE + ".preference._PreferenceManagerImpl");
            IMPL = (PreferenceManagerImpl) clazz.newInstance();
        } catch (Exception e) {
            IMPL = null;
            if (HoloEverywhere.DEBUG) {
                Log.w("HoloEverywhere",
                        "Cannot find PreferenceManager class. Preference framework are disabled.",
                        e);
            }
        }
    }

    private static void checkImpl() {
        if (IMPL == null) {
            throw new UnsatisfiedLinkError("HoloEverywhere: PreferenceFramework not found");
        }
    }

    public static SharedPreferences getDefaultSharedPreferences(Context context) {
        return getDefaultSharedPreferences(context, HoloEverywhere.PREFERENCE_IMPL);
    }

    public static SharedPreferences getDefaultSharedPreferences(Context context,
            PreferenceImpl impl) {
        checkImpl();
        return IMPL.getDefaultSharedPreferences(context, impl);
    }

    public static int obtainThemeTag() {
        checkImpl();
        return IMPL.obtainThemeTag();
    }

    public static SharedPreferences wrap(Context context, PreferenceImpl impl, String name,
            int mode) {
        checkImpl();
        return IMPL.wrap(context, impl, name, mode);
    }

    public static SharedPreferences wrap(Context context, String name,
            int mode) {
        return wrap(context, HoloEverywhere.PREFERENCE_IMPL, name, mode);
    }

    private PreferenceManagerHelper() {

    }
}
