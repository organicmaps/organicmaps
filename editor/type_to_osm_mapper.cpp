#include "editor/type_to_osm_mapper.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <sstream>

#include "cppjansson/cppjansson.hpp"

namespace editor
{
    TypeToOsmMapper & TypeToOsmMapper::Instance()
    {
      static TypeToOsmMapper instance;
      return instance;
    }

    TypeToOsmMapper::TypeToOsmMapper() { LoadMapping(); }

    void TypeToOsmMapper::LoadMapping()
    {
      std::string jsonBuffer;
      try
      {
        ReaderPtr<Reader> reader = GetPlatform().GetReader("om_type_to_osm_tags.json");
        reader.ReadAsString(jsonBuffer);
      }
      catch (RootException const & e)
      {
        LOG(LERROR, ("Failed to read om_type_to_osm_tags.json:", e.Msg()));
        return;
      }

      base::Json root(jsonBuffer.c_str());
      auto const * jsonObj = root.get();

      if (!jsonObj || !json_is_object(jsonObj))
      {
        LOG(LERROR, ("Failed to parse om_type_to_osm_tags.json: not a valid JSON object"));
        return;
      }

      // Iterate over all key-value pairs in the root object
      void * iter = json_object_iter(const_cast<json_t *>(jsonObj));
      while (iter)
      {
        std::string const omType = json_object_iter_key(iter);
        json_t * osmTagsArray = json_object_iter_value(iter);

        if (osmTagsArray && json_is_array(osmTagsArray))
        {
          OsmTags tags;
          size_t const arraySize = json_array_size(osmTagsArray);
          for (size_t i = 0; i < arraySize; ++i)
          {
            json_t * tagObj = json_array_get(osmTagsArray, i);
            if (tagObj && json_is_object(tagObj))
            {
              json_t * keyNode = json_object_get(tagObj, "key");
              json_t * valueNode = json_object_get(tagObj, "value");

              if (keyNode && valueNode && json_is_string(keyNode) && json_is_string(valueNode))
                tags.emplace_back(json_string_value(keyNode), json_string_value(valueNode));
            }
          }

          if (!tags.empty()){
            m_mapping[omType] = std::move(tags);
            m_reverse_mapping[GetOsmTagsKey(m_mapping[omType])] = omType;
          }
        }

        iter = json_object_iter_next(const_cast<json_t *>(jsonObj), iter);
      }

      LOG(LINFO, ("Loaded ", m_mapping.size(), "OM type to OSM tag mappings."));
      LOG(LINFO, ("Loaded ", m_reverse_mapping.size(), "reverse OSM tag to OM type mappings."));
    }

    TypeToOsmMapper::OsmTags TypeToOsmMapper::GetPrimaryOsmTags(std::string const & omType) const
    {
      auto const it = m_mapping.find(omType);
      if (it != m_mapping.end())
        return it->second;
      return {};
    }

    std::string TypeToOsmMapper::GetOmType(OsmTags const & tags) const
    {
      if (tags.empty())
       return {};

      auto const it = m_reverse_mapping.find(GetOsmTagsKey(tags));
      if (it != m_reverse_mapping.end())
        return it->second;
      return {};
    }

    std::string TypeToOsmMapper::GetOsmTagsKey(OsmTags tags) const
    {
      // Sort tags by key to ensure the order is same
      std::sort(tags.begin(), tags.end(), [](auto const & a, auto const & b) {
        return a.first < b.first;
      });

      std::ostringstream ss;
      for (size_t i = 0; i < tags.size(); ++i)
      {
        ss << tags[i].first << "=" << tags[i].second << (i == tags.size() - 1 ? "" : ";");
      }
      return ss.str();
    }
}