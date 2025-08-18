#pragma once

#include "indexer/feature_meta.hpp"

using MetadataID = feature::Metadata::EType;
enum MWMEditorCellType
{
  MWMEditorCellTypeCoordinate = MetadataID::FMD_COUNT + 1,
  MWMEditorCellTypeBookmark,
  MWMEditorCellTypeEditButton,
  MWMEditorCellTypeAddBusinessButton,
  MWMEditorCellTypeAddPlaceButton,
  MWMEditorCellTypeReportButton,
  MWMEditorCellTypeCategory,
  MWMEditorCellTypeName,
  MWMEditorCellTypeAdditionalName,
  MWMEditorCellTypeAddAdditionalName,
  MWMEditorCellTypeAddAdditionalNamePlaceholder,
  MWMEditorCellTypeStreet,
  MWMEditorCellTypeBuilding,
  MWMEditorCellTypeNote,
  MWMEditorCellTypeBookingMore,
};

// Combines MetadataID and MWMEditorCellType.
using MWMEditorCellID = std::underlying_type<MetadataID>::type;
