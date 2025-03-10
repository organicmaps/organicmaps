package app.organicmaps.scripts;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MapStyleParser {
  private static final String GITHUB_BASE_URL = "https://raw.githubusercontent.com/organicmaps/organicmaps/master/data/styles/default/light/symbols/";

  public static void main(String[] args) {
    List<File> outdoorsInputFilePaths = new ArrayList<>(
            Arrays.asList(
                    Paths.get("..", "data", "styles", "outdoors", "include", "Basemap_label.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "outdoors", "include", "Basemap.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "outdoors", "include", "defaults_new.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "outdoors", "include", "Icons.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "outdoors", "include", "Roads_label.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "outdoors", "include", "Roads.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "outdoors", "include", "Subways.mapcss").toFile()
            )
    );
    List<File> defaultInputFilePaths = new ArrayList<>(
            Arrays.asList(
                    Paths.get("..", "data", "styles", "default", "include", "Basemap_label.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "default", "include", "Basemap.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "default", "include", "defaults_new.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "default", "include", "Icons.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "default", "include", "Roads_label.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "default", "include", "Roads.mapcss").toFile(),
                    Paths.get("..", "data", "styles", "default", "include", "Subways.mapcss").toFile()
            )
    );
    try {
      JSONArray defaultTags = parseTagsFromFiles(defaultInputFilePaths);
      JSONArray outdoorsTags = parseTagsFromFiles(outdoorsInputFilePaths);
      outdoorsTags.putAll(defaultTags);
      writeJsonToFile(defaultTags, "Organic Maps (default)", "data/styles/default/", "taginfo.json");
      writeJsonToFile(outdoorsTags, "Organic Maps (outdoors)", "data/styles/outdoors/", "taginfo.json");
      System.out.println("JSON file has been created successfully!");
    } catch (JSONException e) {
      System.err.println("Error processing files: " + e.getMessage());
    } catch (IOException e) {
      System.err.println("Error processing files: " + e.getMessage());
    }
  }

  private static void writeJsonToFile(JSONArray tagArray, String styleName, String filePath, String fileName) throws IOException {
    JSONObject mainObject = new JSONObject();

    mainObject.put("data_format", 1);
    mainObject.put("data_url", "https://raw.githubusercontent.com/organicmaps/organicmaps/master/" + filePath + fileName);
    mainObject.put("data_updated", DateTimeFormatter.ISO_INSTANT.format(Instant.now()));

    JSONObject projectObject = new JSONObject();
    projectObject.put("name", styleName);
    projectObject.put("description", "Free Android & iOS offline maps app for travelers, tourists, hikers, and cyclists");
    projectObject.put("project_url", "https://organicmaps.app");
    projectObject.put("doc_url", "https://github.com/organicmaps/organicmaps");
    projectObject.put("icon_url", "https://organicmaps.app/logos/green-on-transparent.svg");
    projectObject.put("contact_name", "Organic Maps");
    projectObject.put("contact_email", "hello@organicmaps.app");

    // Add project object to main object
    mainObject.put("project", projectObject);
    mainObject.put("tags", tagArray);

    Path outputPath = Paths.get("..", filePath, fileName);
    writeJSON(mainObject, outputPath.toFile());
  }

  private static JSONArray parseTagsFromFiles(List<File> inputFiles) throws JSONException, IOException {
    HashMap<String, ObjectStyle> styleMap = new HashMap<>();

    for (File inputFile : inputFiles) {
      parseTagsSingleFile(inputFile, styleMap);
    }

    // Convert to JSON format
    JSONArray jsonArray = new JSONArray();
    for (Map.Entry<String, ObjectStyle> entry : styleMap.entrySet()) {
      ObjectStyle rule = entry.getValue();
      JSONObject obj = new JSONObject();
      obj.put("key", rule.key);
      obj.put("value", rule.value);
      if (rule.iconImage != null) {
        obj.put("icon_url", GITHUB_BASE_URL + rule.iconImage);
      }
      JSONArray objectTypes = new JSONArray();
      for (ObjectStyle.ObjectType objectType : rule.types) {
        objectTypes.put(objectType.displayName);
      }
      obj.put("object_types", objectTypes);
      jsonArray.put(obj);
    }

    return jsonArray;
  }

  private static void parseTagsSingleFile(File inputFile, HashMap<String, ObjectStyle> styleMap) throws JSONException, IOException {
    String content = readFile(inputFile);

    // Split content into style blocks
    String[] blocks = content.split("}");

    for (String block : blocks) {
      String[] lines = block.trim().split("\\n");
      String iconImage = null;
      List<String> selectors = new ArrayList<>();

      for (String line : lines) {
        line = line.trim();
        if (line.startsWith("node|") || line.startsWith("area|") || line.startsWith("line|") || line.startsWith("relation|")) {
          if (line.endsWith(",")) {
            line = line.substring(0, line.length() - 1);
          }
          selectors.add(line);
        } else if (line.contains("icon-image:")) {
          Pattern pattern = Pattern.compile("icon-image:\\s*([^;]+);?");
          Matcher matcher = pattern.matcher(line);
          if (matcher.find()) {
            iconImage = matcher.group(1).trim();
            if (iconImage.equals("none")) {
              iconImage = null;
            }
          }
        }
      }

      for (String selector : selectors) {
        ObjectStyle newObjectStyle = parseString(selector, iconImage);
        if (newObjectStyle != null) {
          ObjectStyle existingObjectStyle = styleMap.get(newObjectStyle.tagsSelector);
          if (existingObjectStyle == null) {
            styleMap.put(newObjectStyle.tagsSelector, newObjectStyle);
          } else {
            existingObjectStyle.types.addAll(newObjectStyle.types);
            if (newObjectStyle.iconImage != null) {
              existingObjectStyle.iconImage = newObjectStyle.iconImage;
            }
          }
        }
      }
    }
  }

  private static ObjectStyle parseString(String selector, String iconImage) {
    Matcher matcher = ObjectStyle.fullPattern.matcher(selector);
    if (matcher.find()) {
      String type = matcher.group(1);
      String zoom = matcher.group(2);
      String tagsSelector = matcher.group(3);

      Set<ObjectStyle.ObjectType> objectTypes = new HashSet<>();
      switch (type) {
        case "node":
          objectTypes.add(ObjectStyle.ObjectType.NODE);
          break;
        case "line":
          objectTypes.add(ObjectStyle.ObjectType.WAY);
          break;
        case "area":
          objectTypes.add(ObjectStyle.ObjectType.AREA);
          break;
        case "relation":
          objectTypes.add(ObjectStyle.ObjectType.RELATION);
          break;
      }

      Matcher tagsMatcher = ObjectStyle.tagsPattern.matcher(tagsSelector);
      if (tagsMatcher.find()) {
        String key = tagsMatcher.group(1);
        String value = tagsMatcher.group(2);
        return new ObjectStyle(tagsSelector, zoom, key, value, iconImage, objectTypes);
      }
    }
    return null;
  }

  private static String readFile(File filepath) throws IOException {
    StringBuilder content = new StringBuilder();
    try (BufferedReader reader = new BufferedReader(new FileReader(filepath))) {
      String line;
      while ((line = reader.readLine()) != null) {
        content.append(line).append("\n");
      }
    }
    return content.toString();
  }

  private static void writeJSON(JSONObject jsonObject, File path) throws JSONException, IOException {
    try (FileWriter writer = new FileWriter(path)) {
      writer.write(jsonObject.toString(2));
    }
  }
}