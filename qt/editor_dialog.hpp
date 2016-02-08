#pragma once

#include "indexer/feature_meta.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "std/string.hpp"

#include <QtWidgets/QDialog>

class FeatureType;
class Framework;
class QLineEdit;

class EditorDialog : public QDialog
{
  Q_OBJECT
public:
  EditorDialog(QWidget * parent, FeatureType & feature, Framework & frm);
  StringUtf8Multilang GetEditedNames() const;
  feature::Metadata GetEditedMetadata() const;
  string GetEditedStreet() const;
  string GetEditedHouseNumber() const;
};
