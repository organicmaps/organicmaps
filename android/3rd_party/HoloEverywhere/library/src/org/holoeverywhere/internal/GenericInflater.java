
package org.holoeverywhere.internal;

import java.io.IOException;
import java.lang.reflect.Constructor;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.Context;
import android.content.res.XmlResourceParser;
import android.util.AttributeSet;
import android.util.Xml;
import android.view.InflateException;

public abstract class GenericInflater<T, P extends GenericInflater.Parent<T>> {
    public interface Factory<T> {
        public T onCreateItem(String name, Context context, AttributeSet attrs);
    }

    public interface Parent<T> {
        public void addItemFromInflater(T child);
    }

    private static final HashMap<Class<?>, Constructor<?>> sConstructorMap = new HashMap<Class<?>, Constructor<?>>();
    private ClassLoader mClassLoader;
    protected final Object[] mConstructorArgs = new Object[2];
    protected final Class<?>[] mConstructorSignature = new Class<?>[] {
            Context.class, AttributeSet.class
    };
    private final Context mContext;
    private final List<Factory<T>> mFactoryList;
    private final List<String> mPackages = new ArrayList<String>();

    protected GenericInflater(Context context) {
        mContext = context;
        mFactoryList = new ArrayList<Factory<T>>();
    }

    protected GenericInflater(GenericInflater<T, P> original, Context newContext) {
        mContext = newContext;
        mFactoryList = new ArrayList<Factory<T>>(original.mFactoryList);
    }

    public void addFactory(Factory<T> factory) {
        mFactoryList.add(factory);
    }

    public abstract GenericInflater<T, P> cloneInContext(Context newContext);

    @SuppressWarnings("unchecked")
    public final T createItem(String name, String prefix, AttributeSet attrs)
            throws ClassNotFoundException, InflateException {
        if (prefix != null) {
            name = prefix + name;
        }
        Constructor<?> constructor = GenericInflater.sConstructorMap.get(name);
        try {
            if (constructor == null) {
                if (mClassLoader == null) {
                    mClassLoader = getClassLoader();
                    if (mClassLoader == null) {
                        mClassLoader = mContext.getClassLoader();
                    }
                }
                Class<?> clazz = mClassLoader.loadClass(name);
                constructor = findConstructor(clazz);
                GenericInflater.sConstructorMap.put(clazz, constructor);
            }
            return (T) constructor.newInstance(obtainConstructorArgs(name, attrs,
                    constructor));
        } catch (NoSuchMethodException e) {
            InflateException ie = new InflateException(
                    attrs.getPositionDescription() + ": Error inflating class " + name);
            ie.initCause(e);
            throw ie;
        } catch (Exception e) {
            InflateException ie = new InflateException(
                    attrs.getPositionDescription() + ": Error inflating class "
                            + constructor.toString());
            ie.initCause(e);
            throw ie;
        }
    }

    private final T createItemFromTag(XmlPullParser parser, String name,
            AttributeSet attrs) {
        try {
            T item = null;
            for (Factory<T> factory : mFactoryList) {
                try {
                    item = factory.onCreateItem(name, mContext, attrs);
                    if (item != null) {
                        break;
                    }
                } catch (Exception e) {
                }
            }
            if (item == null) {
                if (name.indexOf('.') < 0) {
                    item = onCreateItem(name, attrs);
                } else {
                    item = createItem(name, null, attrs);
                }
            }
            return item;
        } catch (InflateException e) {
            throw e;
        } catch (ClassNotFoundException e) {
            InflateException ie = new InflateException(
                    attrs.getPositionDescription() + ": Error inflating class "
                            + name);
            ie.initCause(e);
            throw ie;
        } catch (Exception e) {
            InflateException ie = new InflateException(
                    attrs.getPositionDescription() + ": Error inflating class "
                            + name);
            ie.initCause(e);
            throw ie;
        }
    }

    protected Constructor<?> findConstructor(Class<?> clazz)
            throws NoSuchMethodException {
        return clazz.getConstructor(mConstructorSignature);
    }

    public ClassLoader getClassLoader() {
        return mClassLoader;
    }

    public Context getContext() {
        return mContext;
    }

    @Deprecated
    public final Factory<T> getFactory() {
        return getFactory(0);
    }

    public final Factory<T> getFactory(int position) {
        return mFactoryList.get(position);
    }

    public final int getFactoryCount() {
        return mFactoryList.size();
    }

    public T inflate(int resource) {
        return inflate(resource, null, false);
    }

    public T inflate(int resource, P root) {
        return inflate(resource, root, root != null);
    }

    public T inflate(int resource, P root, boolean attachToRoot) {
        XmlResourceParser parser = getContext().getResources().getXml(resource);
        try {
            return inflate(parser, root, attachToRoot);
        } finally {
            parser.close();
        }
    }

    public T inflate(XmlPullParser parser) {
        return inflate(parser, null, false);
    }

    public T inflate(XmlPullParser parser, P root) {
        return inflate(parser, root, root != null);
    }

    @SuppressWarnings("unchecked")
    public synchronized T inflate(XmlPullParser parser, P root, boolean attachToRoot) {
        final AttributeSet attrs = Xml.asAttributeSet(parser);
        T result = (T) root;
        try {
            int type;
            while ((type = parser.next()) != XmlPullParser.START_TAG
                    && type != XmlPullParser.END_DOCUMENT) {
                ;
            }
            if (type != XmlPullParser.START_TAG) {
                throw new InflateException(parser.getPositionDescription()
                        + ": No start tag found!");
            }
            T xmlRoot = createItemFromTag(parser, parser.getName(), attrs);
            result = (T) onMergeRoots(root, attachToRoot, (P) xmlRoot);
            rInflate(parser, result, attrs);
        } catch (InflateException e) {
            throw e;
        } catch (XmlPullParserException e) {
            InflateException ex = new InflateException(e.getMessage());
            ex.initCause(e);
            throw ex;
        } catch (IOException e) {
            InflateException ex = new InflateException(
                    parser.getPositionDescription() + ": " + e.getMessage());
            ex.initCause(e);
            throw ex;
        }
        return result;
    }

    protected Object[] obtainConstructorArgs(String name, AttributeSet attrs,
            Constructor<?> constructor) {
        final Object[] args = mConstructorArgs;
        args[0] = mContext;
        args[1] = attrs;
        return args;
    }

    protected boolean onCreateCustomFromTag(XmlPullParser parser, T parent,
            final AttributeSet attrs) throws XmlPullParserException {
        return false;
    }

    protected T onCreateItem(String name, AttributeSet attrs)
            throws ClassNotFoundException {
        for (String sPackage : mPackages) {
            return createItem(name, sPackage + ".", attrs);
        }
        return null;
    }

    protected P onMergeRoots(P givenRoot, boolean attachToGivenRoot, P xmlRoot) {
        return xmlRoot;
    }

    public void registerPackage(String name) {
        name = Package.getPackage(name).getName();
        if (!mPackages.contains(name)) {
            mPackages.add(name);
        }
    }

    public void removeFactory(Factory<T> factory) {
        mFactoryList.remove(factory);
    }

    @SuppressWarnings("unchecked")
    private void rInflate(XmlPullParser parser, T parent,
            final AttributeSet attrs) throws XmlPullParserException,
            IOException {
        final int depth = parser.getDepth();
        int type;
        while (((type = parser.next()) != XmlPullParser.END_TAG || parser
                .getDepth() > depth) && type != XmlPullParser.END_DOCUMENT) {
            if (type != XmlPullParser.START_TAG) {
                continue;
            }
            if (onCreateCustomFromTag(parser, parent, attrs)) {
                continue;
            }
            String name = parser.getName();
            T item = createItemFromTag(parser, name, attrs);
            ((P) parent).addItemFromInflater(item);
            rInflate(parser, item, attrs);
        }
    }

    public void setClassLoader(ClassLoader classLoader) {
        mClassLoader = classLoader;
    }

    public void setFactory(Factory<T> factory) {
        mFactoryList.add(0, factory);
    }

    public void unregisterPackage(String string) {
        mPackages.remove(string);
    }
}
