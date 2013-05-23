
package org.holoeverywhere.app;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.holoeverywhere.HoloEverywhere;
import org.holoeverywhere.HoloEverywhere.PreferenceImpl;
import org.holoeverywhere.IHolo;
import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.LayoutInflater.LayoutInflaterCreator;
import org.holoeverywhere.SystemServiceManager;
import org.holoeverywhere.SystemServiceManager.SuperSystemService;
import org.holoeverywhere.ThemeManager;
import org.holoeverywhere.ThemeManager.SuperStartActivity;
import org.holoeverywhere.addon.AddonSherlock;
import org.holoeverywhere.addon.IAddon;
import org.holoeverywhere.addon.IAddonApplication;
import org.holoeverywhere.addon.IAddonAttacher;
import org.holoeverywhere.addon.IAddonBasicAttacher;
import org.holoeverywhere.preference.PreferenceManagerHelper;
import org.holoeverywhere.preference.SharedPreferences;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Build.VERSION;
import android.os.Bundle;

public class Application extends android.app.Application implements
        IHolo, SuperStartActivity, SuperSystemService, IAddonAttacher<IAddonApplication> {
    private static List<Class<? extends IAddon>> sInitialAddons;
    private static Application sLastInstance;
    static {
        SystemServiceManager.register(LayoutInflaterCreator.class);
        addInitialAddon(AddonSherlock.class);
    }

    public static void addInitialAddon(Class<? extends IAddon> clazz) {
        if (sInitialAddons == null) {
            sInitialAddons = new ArrayList<Class<? extends IAddon>>();
        }
        sInitialAddons.add(clazz);
    }

    public static Application getLastInstance() {
        return Application.sLastInstance;
    }

    public static void init() {
    }

    private final IAddonBasicAttacher<IAddonApplication, Application> mAttacher =
            new IAddonBasicAttacher<IAddonApplication, Application>(this);

    public Application() {
        Application.sLastInstance = this;
    }

    @Override
    public <T extends IAddonApplication> T addon(Class<? extends IAddon> clazz) {
        return mAttacher.addon(clazz);
    }

    @Override
    public void addon(Collection<Class<? extends IAddon>> classes) {
        mAttacher.addon(classes);
    }

    @Override
    public <T extends IAddonApplication> T addon(String classname) {
        return mAttacher.addon(classname);
    }

    @Override
    public SharedPreferences getDefaultSharedPreferences() {
        return PreferenceManagerHelper.getDefaultSharedPreferences(this);
    }

    @Override
    public SharedPreferences getDefaultSharedPreferences(PreferenceImpl impl) {
        return PreferenceManagerHelper.getDefaultSharedPreferences(this, impl);
    }

    @Override
    public LayoutInflater getLayoutInflater() {
        return LayoutInflater.from(this);
    }

    @Override
    public SharedPreferences getSharedPreferences(PreferenceImpl impl, String name, int mode) {
        return PreferenceManagerHelper.wrap(this, impl, name, mode);
    }

    @Override
    public SharedPreferences getSharedPreferences(String name, int mode) {
        return PreferenceManagerHelper.wrap(this, name, mode);
    }

    @Override
    public Application getSupportApplication() {
        return this;
    }

    @Override
    public Object getSystemService(String name) {
        return SystemServiceManager.getSystemService(this, name);
    }

    @Override
    public boolean isAddonAttached(Class<? extends IAddon> clazz) {
        return mAttacher.isAddonAttached(clazz);
    }

    @Override
    public void lockAttaching() {
        mAttacher.lockAttaching();
    }

    @Override
    public Collection<Class<? extends IAddon>> obtainAddonsList() {
        return mAttacher.obtainAddonsList();
    }

    @Override
    public void onCreate() {
        addon(sInitialAddons);
        performAddonAction(new AddonCallback<IAddonApplication>() {
            @Override
            public void justAction(IAddonApplication addon) {
                addon.onPreCreate();
            }
        });
        lockAttaching();
        super.onCreate();
        performAddonAction(new AddonCallback<IAddonApplication>() {
            @Override
            public void justAction(IAddonApplication addon) {
                addon.onCreate();
            }
        });
    }

    @Override
    public boolean performAddonAction(AddonCallback<IAddonApplication> callback) {
        return mAttacher.performAddonAction(callback);
    }

    @Override
    @SuppressLint("NewApi")
    public void startActivities(Intent[] intents) {
        startActivities(intents, null);
    }

    @Override
    @SuppressLint("NewApi")
    public void startActivities(Intent[] intents, Bundle options) {
        for (Intent intent : intents) {
            startActivity(intent, options);
        }
    }

    @Override
    @SuppressLint("NewApi")
    public void startActivity(Intent intent) {
        startActivity(intent, null);
    }

    @Override
    public void startActivity(Intent intent, Bundle options) {
        if (HoloEverywhere.ALWAYS_USE_PARENT_THEME) {
            ThemeManager.startActivity(this, intent, options);
        } else {
            superStartActivity(intent, -1, options);
        }
    }

    public android.content.SharedPreferences superGetSharedPreferences(
            String name, int mode) {
        return super.getSharedPreferences(name, mode);
    }

    @Override
    public Object superGetSystemService(String name) {
        return super.getSystemService(name);
    }

    @Override
    @SuppressLint("NewApi")
    public void superStartActivity(Intent intent, int requestCode,
            Bundle options) {
        if (VERSION.SDK_INT >= 16) {
            super.startActivity(intent, options);
        } else {
            super.startActivity(intent);
        }
    }
}
