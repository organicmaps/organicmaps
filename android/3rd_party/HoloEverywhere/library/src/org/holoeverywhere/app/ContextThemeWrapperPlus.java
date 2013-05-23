
package org.holoeverywhere.app;

import org.holoeverywhere.SystemServiceManager;
import org.holoeverywhere.SystemServiceManager.SuperSystemService;

import android.content.Context;
import android.view.ContextThemeWrapper;

public class ContextThemeWrapperPlus extends ContextThemeWrapper implements SuperSystemService {
    public ContextThemeWrapperPlus(Context base, int themeres) {
        super(base, themeres);
    }

    @Override
    public Object getSystemService(String name) {
        return SystemServiceManager.getSystemService(this, name);
    }

    @Override
    public Object superGetSystemService(String name) {
        return super.getSystemService(name);
    }
}
