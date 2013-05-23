
package org.holoeverywhere.util;

import java.lang.ref.WeakReference;
import java.util.AbstractCollection;
import java.util.AbstractMap;
import java.util.AbstractSet;
import java.util.Collection;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;

public class WeaklyMap<K, V> extends AbstractMap<K, V> {
    private static final class WeaklyEntry<K, V> implements Entry<K, V> {
        private final Entry<K, WeaklyReference<V>> mEntry;

        public WeaklyEntry(Entry<K, WeaklyReference<V>> entry) {
            mEntry = entry;
        }

        @Override
        public K getKey() {
            return mEntry.getKey();
        }

        @Override
        public V getValue() {
            final WeaklyReference<V> ref = mEntry.getValue();
            return ref == null ? null : ref.get();
        }

        @Override
        public V setValue(V object) {
            final WeaklyReference<V> ref = mEntry.setValue(new WeaklyReference<V>(object));
            return ref == null ? null : ref.get();
        }
    }

    private static final class WeaklyReference<T> extends WeakReference<T> {
        public WeaklyReference(T r) {
            super(r);
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof WeaklyReference)) {
                return false;
            }
            final Object o1 = ((WeaklyReference<?>) o).get();
            final Object o2 = get();
            return o1 != null && o2 != null && o1 == o2;
        }
    }

    private final WeakHashMap<K, WeaklyReference<V>> mMap;

    public WeaklyMap() {
        mMap = new WeakHashMap<K, WeaklyReference<V>>();
    }

    @Override
    public void clear() {
        mMap.clear();
    }

    @Override
    public boolean containsKey(Object key) {
        return mMap.containsKey(key);
    }

    @Override
    public boolean containsValue(Object value) {
        return mMap.containsValue(new WeaklyReference<Object>(value));
    }

    @Override
    public Set<Entry<K, V>> entrySet() {
        final Set<Entry<K, WeaklyReference<V>>> entrySet = mMap.entrySet();
        return new AbstractSet<Entry<K, V>>() {
            @Override
            public Iterator<Entry<K, V>> iterator() {
                final Iterator<Entry<K, WeaklyReference<V>>> iterator = entrySet.iterator();
                return new Iterator<Entry<K, V>>() {

                    @Override
                    public boolean hasNext() {
                        return iterator.hasNext();
                    }

                    @Override
                    public Entry<K, V> next() {
                        return new WeaklyEntry<K, V>(iterator.next());
                    }

                    @Override
                    public void remove() {
                        iterator.remove();
                    }
                };
            }

            @Override
            public int size() {
                return entrySet.size();
            }
        };
    }

    @Override
    public V get(Object key) {
        WeaklyReference<V> ref = mMap.get(key);
        return ref == null ? null : ref.get();
    }

    @Override
    public boolean isEmpty() {
        return mMap.isEmpty();
    }

    @Override
    public Set<K> keySet() {
        return mMap.keySet();
    }

    @Override
    public V put(K key, V value) {
        WeaklyReference<V> ref = mMap.put(key, new WeaklyReference<V>(value));
        return ref == null ? null : ref.get();
    }

    @Override
    public void putAll(Map<? extends K, ? extends V> map) {
        for (Entry<? extends K, ? extends V> entry : map.entrySet()) {
            mMap.put(entry.getKey(), new WeaklyReference<V>(entry.getValue()));
        }
    }

    @Override
    public V remove(Object key) {
        WeaklyReference<V> ref = mMap.remove(key);
        return ref == null ? null : ref.get();
    }

    @Override
    public int size() {
        return mMap.size();
    }

    @Override
    public Collection<V> values() {
        final Collection<WeaklyReference<V>> values = mMap.values();
        return new AbstractCollection<V>() {
            @Override
            public Iterator<V> iterator() {
                final Iterator<WeaklyReference<V>> iterator = values.iterator();
                return new Iterator<V>() {

                    @Override
                    public boolean hasNext() {
                        return iterator.hasNext();
                    }

                    @Override
                    public V next() {
                        WeaklyReference<V> ref = iterator.next();
                        return ref == null ? null : ref.get();
                    }

                    @Override
                    public void remove() {
                        iterator.remove();
                    }
                };
            }

            @Override
            public int size() {
                return values.size();
            }
        };
    }
}
