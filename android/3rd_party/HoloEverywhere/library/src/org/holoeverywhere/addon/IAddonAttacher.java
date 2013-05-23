
package org.holoeverywhere.addon;

import java.util.Collection;

public interface IAddonAttacher<V extends IAddonBase<?>> {
    public abstract static class AddonCallback<V> {
        public boolean mStopped = false;

        public boolean action(V addon) {
            justAction(addon);
            return false;
        }

        public void justAction(V addon) {

        }

        public void justPost() {

        }

        public boolean performAction(V addon) {
            if (action(addon)) {
                stop();
                return true;
            }
            return false;
        }

        public boolean post() {
            justPost();
            return false;
        }

        public void pre() {

        }

        public void stop() {
            mStopped = true;
        }
    }

    public static class AttachException extends RuntimeException {
        private static final long serialVersionUID = 4007240742116340485L;

        public AttachException(Object object, Class<? extends IAddon> clazz) {
            super("Couldn't attach addon " + clazz.getName() + " after init of object " + object);
        }
    }

    public <T extends V> T addon(Class<? extends IAddon> clazz);

    public void addon(Collection<Class<? extends IAddon>> classes);

    public <T extends V> T addon(String classname);

    public boolean isAddonAttached(Class<? extends IAddon> clazz);

    public void lockAttaching();

    public Collection<Class<? extends IAddon>> obtainAddonsList();

    public boolean performAddonAction(AddonCallback<V> callback);
}
