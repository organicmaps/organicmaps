#include "widget.hpp"
#include "ui_widget.h"

#include "qlistmodel.hpp"

#include <QSettings>
#include <QTextStream>
#include <QFile>
#include <QList>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

Widget::Widget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Widget)
{
  ui->setupUi(this);
  connect(ui->selectunicodeblocks, SIGNAL(clicked()), this, SLOT(LoadUnicodeBlocks()));
  connect(ui->selectExportPath, SIGNAL(clicked()), this, SLOT(ResolveExportPath()));
  connect(ui->runExport, SIGNAL(clicked()), this, SLOT(Export()));
  connect(ui->fontsize, SIGNAL(valueChanged(int)), this, SLOT(UpdateEngine(int)));
  connect(&m_engine, SIGNAL(UpdatePreview(QImage)), this, SLOT(UpdatePreview(QImage)));
  connect(&m_engine, SIGNAL(StartEngine(int)), this, SLOT(StartEngine(int)), Qt::QueuedConnection);
  connect(&m_engine, SIGNAL(UpdateProgress(int)), this, SLOT(UpdateProgress(int)), Qt::QueuedConnection);
  connect(&m_engine, SIGNAL(EndEngine()), this, SLOT(EndEngine()), Qt::QueuedConnection);

  connect(&m_engine, SIGNAL(ConvertStarted()), this, SLOT(ConvertStart()));
  connect(&m_engine, SIGNAL(ConvertEnded()), this, SLOT(ConvertEnd()));

  ui->progressBar->reset();

  QSettings s("settings.ini", QSettings::IniFormat, this);
  QString v = s.value("unicode_file", "").toString();
  if (!v.isEmpty())
    if (LoadUnicodeBlocksImpl(v) == false)
      s.setValue("unicode_file", "");

  QString exportPath = s.value("export_path", "").toString();
  if (!exportPath.isEmpty())
  {
    ResolveExportPath(exportPath);
  }

  QPixmap p(350, 350);
  p.fill(Qt::black);
  ui->preview->setPixmap(p);
  UpdateButton();
}

Widget::~Widget()
{
  QSettings s("settings.ini", QSettings::IniFormat, this);
  if (!ui->unicodesblocks->text().isEmpty())
    s.setValue("unicode_file", ui->unicodesblocks->text());
  if (!ui->exportpath->text().isEmpty())
    s.setValue("export_path", ui->exportpath->text());
  delete ui;
}

void Widget::LoadUnicodeBlocks()
{
  QString filePath = QFileDialog::getOpenFileName(this, "Open Unicode blocks discription", QString(), "Text (*.txt)");
  LoadUnicodeBlocksImpl(filePath);
}

void Widget::ResolveExportPath()
{
  QString dir = QFileDialog::getExistingDirectory(this, "ExportPath");
  ui->exportpath->setText(dir);
  m_engine.SetExportPath(dir);
  UpdateButton();
}

void Widget::ResolveExportPath(const QString & filePath)
{
  ui->exportpath->setText(filePath);
  m_engine.SetExportPath(filePath);
  UpdateButton();
}

void Widget::Export()
{
  m_engine.RunExport();
}

void Widget::UpdatePreview(QImage img)
{
  ui->preview->setPixmap(QPixmap::fromImage(img.scaled(ui->preview->size(), Qt::KeepAspectRatio)));
}

void Widget::UpdateEngine(int)
{
  m_engine.SetFonts(m_ranges, ui->fontsize->value());
}

void Widget::StartEngine(int maxValue)
{
  ui->progressBar->setMinimum(0);
  ui->progressBar->setMaximum(maxValue);
  ui->progressBar->setValue(0);
}

void Widget::UpdateProgress(int value)
{
  ui->progressBar->setValue(value);
}

void Widget::EndEngine()
{
  ui->progressBar->reset();
}

void Widget::ConvertStart()
{
  ui->progressBar->setMinimum(0);
  ui->progressBar->setMaximum(0);
  ui->progressBar->setValue(0);

  ui->runExport->setEnabled(false);
  ui->selectExportPath->setEnabled(false);
  ui->selectunicodeblocks->setEnabled(false);
}

void Widget::ConvertEnd()
{
  ui->progressBar->setMinimum(0);
  ui->progressBar->setMaximum(100);
  ui->progressBar->setValue(0);

  ui->runExport->setEnabled(true);
  ui->selectExportPath->setEnabled(true);
  ui->selectunicodeblocks->setEnabled(true);
}

void Widget::UpdateButton()
{
  ui->runExport->setEnabled(m_engine.IsReadyToExport());
}

bool Widget::LoadUnicodeBlocksImpl(const QString & filePath)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this, "Could't open file", "Unicode blocks description not valid");
    return false;
  }
  ui->unicodesblocks->setText(filePath);

  QList<FontRange> ranges;

  QString dirPath = filePath.left(filePath.lastIndexOf("/") + 1);

  QTextStream stream(&file);
  stream.setIntegerBase(16);
  while (!stream.atEnd())
  {
    FontRange range;
    stream >> range.m_fontPath;
    if (range.m_fontPath.isEmpty())
      continue;

    stream >> range.m_startRange >> range.m_endRange;
    range.m_validFont = QFile::exists(range.m_fontPath);
    if (range.m_validFont == false)
    {
      QString fontPath = dirPath + range.m_fontPath;
      range.m_validFont = QFile::exists(fontPath);
      range.m_fontPath = fontPath;
    }
    ranges.push_back(range);
  }

  m_ranges = ranges;
  UpdateEngine(0);

  QListModel * model = new QListModel(ui->fontsView, ranges);
  ui->fontsView->setModel(model);

  UpdateButton();
  return true;
}
