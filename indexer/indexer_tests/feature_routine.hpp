#pragma once

#include "../feature.hpp"

void FeatureBuilder2Feature(FeatureBuilderGeomRef const & fb, FeatureGeomRef & f);
void Feature2FeatureBuilder(FeatureGeomRef const & f, FeatureBuilderGeomRef & fb);

void FeatureBuilder2Feature(FeatureBuilderGeom const & fb, FeatureGeom & f);
void Feature2FeatureBuilder(FeatureGeom const & f, FeatureBuilderGeom & fb);
