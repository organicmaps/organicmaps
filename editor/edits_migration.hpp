#pragma once

#include "editor/osm_editor.hpp"
#include "editor/xml_feature.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/feature_source.hpp"

#include "base/exception.hpp"

#include "std/functional.hpp"

namespace editor
{
DECLARE_EXCEPTION(MigrationError, RootException);

using TGenerateIDFn = function<FeatureID()>;

/// Tries to match xml feature with one on a new mwm and retruns FeatrueID
/// of a found feature, thows MigrationError if migration fails.
FeatureID MigrateFeatureIndex(osm::Editor::ForEachFeaturesNearByFn & forEach,
                              XMLFeature const & xml,
                              FeatureStatus const featureStatus,
                              TGenerateIDFn const & generateID);
}  // namespace editor
