#pragma once

#include "kml/types.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>

#include <string>
#include <unordered_map>

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;

class Framework;

namespace qt
{
class BookmarkDialog : public QDialog
{
  Q_OBJECT

public:
  BookmarkDialog(QWidget * parent, Framework & framework);

  void ShowModal();

private slots:
  void OnItemClick(QTreeWidgetItem * item, int column);
  void OnCloseClick();
  void OnImportClick();
  void OnExportClick();
  void OnDeleteClick();

private:
  void FillTree();
  QTreeWidgetItem * CreateTreeItem(std::string const & title, QTreeWidgetItem * parent);
  void OnAsyncLoadingStarted();
  void OnAsyncLoadingFinished();
  void OnAsyncLoadingFileSuccess(std::string const & fileName, bool isTemporaryFile);
  void OnAsyncLoadingFileError(std::string const & fileName, bool isTemporaryFile);

  QTreeWidget * m_tree;
  Framework & m_framework;
  std::unordered_map<QTreeWidgetItem *, kml::MarkGroupId> m_categories;
  std::unordered_map<QTreeWidgetItem *, kml::MarkId> m_bookmarks;
  std::unordered_map<QTreeWidgetItem *, kml::TrackId> m_tracks;
};
}  // namespace qt
