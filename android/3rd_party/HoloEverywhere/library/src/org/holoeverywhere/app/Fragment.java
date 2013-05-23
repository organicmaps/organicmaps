
package org.holoeverywhere.app;

import java.util.Collection;

import org.holoeverywhere.LayoutInflater;
import org.holoeverywhere.addon.IAddon;
import org.holoeverywhere.addon.IAddonBasicAttacher;
import org.holoeverywhere.addon.IAddonFragment;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app._HoloFragment;
import android.view.View;

import com.actionbarsherlock.view.ActionMode;
import com.actionbarsherlock.view.ActionMode.Callback;

public class Fragment extends _HoloFragment {
    public static <T extends Fragment> T instantiate(Class<T> clazz) {
        return instantiate(clazz, null);
    }

    public static <T extends Fragment> T instantiate(Class<T> clazz, Bundle args) {
        try {
            T fragment = clazz.newInstance();
            if (args != null) {
                args.setClassLoader(clazz.getClassLoader());
                fragment.setArguments(args);
            }
            return fragment;
        } catch (Exception e) {
            throw new InstantiationException("Unable to instantiate fragment " + clazz
                    + ": make sure class name exists, is public, and has an"
                    + " empty constructor that is public", e);
        }
    }

    @Deprecated
    public static Fragment instantiate(Context context, String fname) {
        return instantiate(context, fname, null);
    }

    @SuppressWarnings("unchecked")
    @Deprecated
    public static Fragment instantiate(Context context, String fname, Bundle args) {
        try {
            return instantiate((Class<? extends Fragment>) Class.forName(fname, true,
                    context.getClassLoader()), args);
        } catch (Exception e) {
            throw new InstantiationException("Unable to instantiate fragment " + fname
                    + ": make sure class name exists, is public, and has an"
                    + " empty constructor that is public", e);
        }
    }

    private final IAddonBasicAttacher<IAddonFragment, Fragment> mAttacher =
            new IAddonBasicAttacher<IAddonFragment, Fragment>(this);

    private LayoutInflater mLayoutInflater;

    @Override
    public <T extends IAddonFragment> T addon(Class<? extends IAddon> clazz) {
        return mAttacher.addon(clazz);
    }

    @Override
    public void addon(Collection<Class<? extends IAddon>> classes) {
        mAttacher.addon(classes);
    }

    @Override
    public <T extends IAddonFragment> T addon(String classname) {
        return mAttacher.addon(classname);
    }

    @Override
    public LayoutInflater getLayoutInflater() {
        if (mLayoutInflater == null) {
            mLayoutInflater = getSupportActivity().getLayoutInflater().
                    obtainFragmentChildInflater(this);
        }
        return mLayoutInflater;
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
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mAttacher.reset();
        mAttacher.inhert(activity);
    }

    @Override
    public void onCreate(final Bundle savedInstanceState) {
        lockAttaching();
        performAddonAction(new AddonCallback<IAddonFragment>() {
            @Override
            public void justAction(IAddonFragment addon) {
                addon.onPreCreate(savedInstanceState);
            }
        });
        super.onCreate(savedInstanceState);
        performAddonAction(new AddonCallback<IAddonFragment>() {
            @Override
            public void justAction(IAddonFragment addon) {
                addon.onCreate(savedInstanceState);
            }
        });
    }

    @Override
    public void onViewCreated(final View view, final Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        performAddonAction(new AddonCallback<IAddonFragment>() {
            @Override
            public void justAction(IAddonFragment addon) {
                addon.onViewCreated(view, savedInstanceState);
            }
        });
    }

    @Override
    public boolean performAddonAction(AddonCallback<IAddonFragment> callback) {
        return mAttacher.performAddonAction(callback);
    }

    @Override
    public ActionMode startActionMode(Callback callback) {
        return getSupportActivity().startActionMode(callback);
    }
}
