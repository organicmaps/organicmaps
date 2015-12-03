#pragma once

#include "std/string.hpp"

#include <QtWidgets/QDialog>

class FeatureType;
class QLineEdit;

class EditorDialog : public QDialog
{
  Q_OBJECT
public:
  EditorDialog(QWidget * parent, FeatureType const & feature);
};
