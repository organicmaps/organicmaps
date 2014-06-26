#pragma once

#include "engine.hpp"
#include <QWidget>

namespace Ui {
  class Widget;
}

class Widget : public QWidget
{
  Q_OBJECT

public:
  explicit Widget(QWidget *parent = 0);
  ~Widget();

private:
  Q_SLOT void LoadUnicodeBlocks();
  Q_SLOT void ResolveExportPath();
  Q_SLOT void Export();
  Q_SLOT void UpdatePreview(QImage);
  Q_SLOT void UpdateEngine(int);

  Q_SLOT void StartEngine(int maxValue);
  Q_SLOT void UpdateProgress(int value);
  Q_SLOT void EndEngine();

private:
  void UpdateButton();
  bool LoadUnicodeBlocksImpl(QString const & filePath);

private:
  Engine m_engine;
  QList<FontRange> m_ranges;

private:
  Ui::Widget * ui;
};
