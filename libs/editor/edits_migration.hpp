#pragma once

#include "editor/osm_editor.hpp"
#include "editor/xml_feature.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/feature_source.hpp"

#include "base/exception.hpp"

#include <functional>

namespace editor
{
DECLARE_EXCEPTION(MigrationError, RootException);

using GenerateIDFn = std::function<FeatureID()>;

/// Tries to match xml feature with one on a new mwm and returns FeatureID
/// of a found feature, throws MigrationError if migration fails.
FeatureID MigrateFeatureIndex(osm::Editor::ForEachFeaturesNearByFn & forEach, XMLFeature const & xml,
                              FeatureStatus const featureStatus, GenerateIDFn const & generateID);
}  // namespace editor
