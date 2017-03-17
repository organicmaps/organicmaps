#pragma once

#include <string>

#include <QtWidgets/QComboBox>

class QWidget;

class LanguagesList : public QComboBox
{
public:
  explicit LanguagesList(QWidget * parent);

  void Select(std::string const & lang);
};
