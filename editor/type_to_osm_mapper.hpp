#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace editor
{
    // Maps an OM type to  OSM tags.
    class TypeToOsmMapper
    {
    public:
        using OsmTags = std::vector<std::pair<std::string, std::string>>;

        static TypeToOsmMapper & Instance();

        // Returns the primary set of OSM tags
        OsmTags GetPrimaryOsmTags(std::string const & omType) const;

        // Returns the OM type for set of OSM tags
        std::string GetOmType(OsmTags const & tags) const;
    private:
        TypeToOsmMapper();
        void LoadMapping();
        // Creates a key from OSM tags
        std::string GetOsmTagsKey(OsmTags tags) const;

        // OM Type -> Vector of OSM tag pairs
        std::map<std::string, OsmTags> m_mapping;
        //  OSM tags key -> OM Type string
        std::map<std::string, std::string> m_reverse_mapping;
    };
}