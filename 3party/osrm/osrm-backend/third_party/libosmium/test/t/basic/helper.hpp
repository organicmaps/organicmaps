#ifndef TEST_BASIC_HELPER_HPP
#define TEST_BASIC_HELPER_HPP

#include <tuple>
#include <utility>
#include <vector>

#include <osmium/builder/osm_object_builder.hpp>

inline void add_tags(osmium::memory::Buffer& buffer, osmium::builder::Builder& builder, const std::vector<std::pair<const char*, const char*>>& tags) {
    osmium::builder::TagListBuilder tl_builder(buffer, &builder);
    for (auto& tag : tags) {
        tl_builder.add_tag(tag.first, tag.second);
    }
}

inline osmium::Node& buffer_add_node(osmium::memory::Buffer& buffer, const char* user, const std::vector<std::pair<const char*, const char*>>& tags, const osmium::Location& location) {
    osmium::builder::NodeBuilder builder(buffer);
    builder.add_user(user);
    add_tags(buffer, builder, tags);
    buffer.commit();
    return builder.object().set_location(location);
}

inline osmium::Way& buffer_add_way(osmium::memory::Buffer& buffer, const char* user, const std::vector<std::pair<const char*, const char*>>& tags, const std::vector<osmium::object_id_type>& nodes) {
    osmium::builder::WayBuilder builder(buffer);
    builder.add_user(user);
    add_tags(buffer, builder, tags);
    osmium::builder::WayNodeListBuilder wnl_builder(buffer, &builder);
    for (const osmium::object_id_type ref : nodes) {
        wnl_builder.add_node_ref(ref);
    }
    buffer.commit();
    return builder.object();
}

inline osmium::Way& buffer_add_way(osmium::memory::Buffer& buffer, const char* user, const std::vector<std::pair<const char*, const char*>>& tags, const std::vector<std::pair<osmium::object_id_type, osmium::Location>>& nodes) {
    osmium::builder::WayBuilder builder(buffer);
    builder.add_user(user);
    add_tags(buffer, builder, tags);
    osmium::builder::WayNodeListBuilder wnl_builder(buffer, &builder);
    for (auto& p : nodes) {
        wnl_builder.add_node_ref(p.first, p.second);
    }
    buffer.commit();
    return builder.object();
}

inline osmium::Relation& buffer_add_relation(
        osmium::memory::Buffer& buffer,
        const char* user,
        const std::vector<std::pair<const char*, const char*>>& tags, const std::vector<std::tuple<char, osmium::object_id_type, const char*>>& members) {
    osmium::builder::RelationBuilder builder(buffer);
    builder.add_user(user);
    add_tags(buffer, builder, tags);
    osmium::builder::RelationMemberListBuilder rml_builder(buffer, &builder);
    for (const auto& member : members) {
        rml_builder.add_member(osmium::char_to_item_type(std::get<0>(member)), std::get<1>(member), std::get<2>(member));
    }
    buffer.commit();
    return builder.object();
}

inline osmium::Area& buffer_add_area(osmium::memory::Buffer& buffer, const char* user,
        const std::vector<std::pair<const char*, const char*>>& tags,
        const std::vector<std::pair<bool,
            std::vector<std::pair<osmium::object_id_type, osmium::Location>>>>& rings) {
    osmium::builder::AreaBuilder builder(buffer);
    builder.add_user(user);
    add_tags(buffer, builder, tags);

    for (auto& ring : rings) {
        if (ring.first) {
            osmium::builder::OuterRingBuilder ring_builder(buffer, &builder);
            for (auto& p : ring.second) {
                ring_builder.add_node_ref(p.first, p.second);
            }
        } else {
            osmium::builder::InnerRingBuilder ring_builder(buffer, &builder);
            for (auto& p : ring.second) {
                ring_builder.add_node_ref(p.first, p.second);
            }
        }
    }
    buffer.commit();
    return builder.object();
}

inline osmium::Changeset& buffer_add_changeset(osmium::memory::Buffer& buffer, const char* user, const std::vector<std::pair<const char*, const char*>>& tags) {
    osmium::builder::ChangesetBuilder builder(buffer);
    builder.add_user(user);
    add_tags(buffer, builder, tags);
    buffer.commit();
    return builder.object();
}

#endif // TEST_BASIC_HELPER_HPP
