package app.organicmaps.scripts;

import java.util.Objects;
import java.util.Set;
import java.util.regex.Pattern;

class ObjectStyle {
  String tagsSelector;
  String zoom;
  String key;
  String value;
  String iconImage;
  Set<ObjectType> types;

  enum ObjectType {
    NODE("node"),
    WAY("way"),
    AREA("area"),
    RELATION("relation");

    final String displayName;

    ObjectType(String displayName) {
      this.displayName = displayName;
    }
  }

  ObjectStyle(String tagsSelector, String zoom, String key, String value, String iconImage, Set<ObjectType> types) {
    this.tagsSelector = tagsSelector;
    this.zoom = zoom;
    this.key = key;
    this.value = value;
    this.iconImage = iconImage;
    this.types = types;
  }

  static final Pattern fullPattern = Pattern.compile("(node|area|line|relation)\\|(z.+?)(\\[.*\\])");
  static final Pattern tagsPattern = Pattern.compile("\\[(\\w+)=(\\w+)\\]");

  @Override
  public boolean equals(Object o) {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;
    ObjectStyle that = (ObjectStyle) o;
    return Objects.equals(key, that.key) &&
            Objects.equals(value, that.value) &&
            Objects.equals(iconImage, that.iconImage) &&
            Objects.equals(types, that.types);
  }

  @Override
  public int hashCode() {
    return Objects.hash(key, value, iconImage, types);
  }
}