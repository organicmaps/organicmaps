#pragma once

#include "indexer/feature_decl.hpp"
#include "indexer/osm_editor.hpp"

#include "editor/xml_feature.hpp"

#include "base/exception.hpp"

namespace editor
{
DECLARE_EXCEPTION(MigrationError, RootException);

/// Tries to match xml feature with one on a new mwm and retruns FeatrueID
/// of a found feature, thows MigrationError if migration fails.
FeatureID MigrateFeatureIndex(osm::Editor::TForEachFeaturesNearByFn & forEach,
                              XMLFeature const & xml);
}  // namespace editor
