
package org.holoeverywhere.addon;

import java.util.Map;

import org.holoeverywhere.ThemeManager;
import org.holoeverywhere.ThemeManager.ThemeSetter;
import org.holoeverywhere.app.ContextThemeWrapperPlus;
import org.holoeverywhere.util.WeaklyMap;

import android.content.Context;

public class IAddonThemes implements ThemeSetter {
    private static final class AddonThemeWrapper extends ContextThemeWrapperPlus {
        public AddonThemeWrapper(Context base, int themeres) {
            super(base, themeres);
        }
    }

    public interface ThemeResolver {
        public int resolveThemeForContext(Context context, int invalidTheme);
    }

    private Map<Context, AddonThemeWrapper> mContexts;
    private int mDarkTheme = -1;
    private final ThemeResolver mDefaultThemeResolver = new ThemeResolver() {
        @Override
        public int resolveThemeForContext(Context context, int invalidTheme) {
            int theme = ThemeManager.getThemeType(context);
            if (theme == ThemeManager.INVALID) {
                theme = invalidTheme & ThemeManager.getThemeMask();
                if (theme == 0) {
                    theme = ThemeManager.DARK;
                }
            }
            theme |= mThemeFlag;
            return ThemeManager.getThemeResource(theme, false);
        }
    };

    private int mLightTheme = -1;

    private int mMixedTheme = -1;

    private final int mThemeFlag;

    public IAddonThemes() {
        mThemeFlag = ThemeManager.makeNewFlag();
        ThemeManager.registerThemeSetter(this);
    }

    public Context context(Context context) {
        return context(context, ThemeManager.DARK);
    }

    public Context context(Context context, int invalidTheme) {
        return context(context, invalidTheme, mDefaultThemeResolver);
    }

    public Context context(Context context, int invalidTheme, ThemeResolver themeResolver) {
        if (context instanceof AddonThemeWrapper) {
            return context;
        }
        AddonThemeWrapper wrapper = null;
        if (mContexts != null) {
            wrapper = mContexts.get(context);
        }
        if (wrapper == null) {
            final int theme = themeResolver.resolveThemeForContext(context, invalidTheme);
            if (theme <= 0) {
                return null;
            }
            wrapper = new AddonThemeWrapper(context, theme);
            if (mContexts == null) {
                mContexts = new WeaklyMap<Context, AddonThemeWrapper>();
            }
            mContexts.put(context, wrapper);
        }
        return wrapper;
    }

    public int getDarkTheme() {
        return mDarkTheme;
    }

    public int getLightTheme() {
        return mLightTheme;
    }

    public int getMixedTheme() {
        return mMixedTheme;
    }

    public int getThemeFlag() {
        return mThemeFlag;
    }

    public void map(int darkTheme, int lightTheme, int mixedTheme) {
        mDarkTheme = darkTheme;
        mLightTheme = lightTheme;
        mMixedTheme = mixedTheme;
        setupThemes();
    }

    public void setDarkTheme(int darkTheme) {
        mDarkTheme = darkTheme;
        setupThemes();
    }

    public void setLightTheme(int lightTheme) {
        mLightTheme = lightTheme;
        setupThemes();
    }

    public void setMixedTheme(int mixedTheme) {
        mMixedTheme = mixedTheme;
        setupThemes();
    }

    @Override
    public void setupThemes() {
        ThemeManager.map(mThemeFlag | ThemeManager.DARK, mDarkTheme);
        ThemeManager.map(mThemeFlag | ThemeManager.LIGHT, mLightTheme);
        ThemeManager.map(mThemeFlag | ThemeManager.MIXED, mMixedTheme);
    }

    public Context unwrap(Context context) {
        if (context == null) {
            return null;
        }
        while (context instanceof AddonThemeWrapper) {
            context = ((AddonThemeWrapper) context).getBaseContext();
        }
        return context;
    }
}
