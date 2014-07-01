#pragma once

#include <QString>
#include <QAtomicInt>
#include <QThread>
#include <QImage>

struct FontRange
{
  QString m_fontPath;
  bool m_validFont;
  int m_startRange;
  int m_endRange;
};

class Engine : public QObject
{
  Q_OBJECT
public:
  Engine();

  void SetFonts(QList<FontRange> const & fonts, int fontSize);
  void SetExportPath(QString const & dirName);
  bool IsReadyToExport() const;

  Q_SIGNAL void UpdatePreview(QImage img);
  Q_SIGNAL void StartEngine(int maxValue);
  Q_SIGNAL void UpdateProgress(int currentValue);
  Q_SIGNAL void EndEngine();

  void RunExport();

private:
  Q_SLOT void WorkThreadFinished();

private:
  QImage GetImage(QThread * sender = NULL) const;

private:
  void EmitStartEngine(int maxValue);
  void EmitUpdateProgress(int currentValue);
  void EmitEndEngine();

private:
  QString m_dirName;
  QAtomicInt m_dataGenerated;
  QThread * m_workThread;
};
