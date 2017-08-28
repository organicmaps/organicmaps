#pragma once

#include <QWebEngineView>

class WebView : public QWebEngineView
{
  Q_OBJECT

  int static constexpr kInvalidId = -1;

public:
  WebView(std::string const & url, std::string const & login, std::string const & paswd);

  int loadProgress() const;
  bool isWebActionEnabled(QWebEnginePage::WebAction webAction) const;

public slots:
  void SetCurrentSegment(int segmentId);

signals:
  void webActionEnabledChanged(QWebEnginePage::WebAction webAction, bool enabled);

protected:
  void contextMenuEvent(QContextMenuEvent * event) override;
  QWebEngineView * createWindow(QWebEnginePage::WebWindowType type) override;

private slots:
  void OnLoadFinished(bool ok);

private:
  void GoToSegment();
  void Login();

  int m_loadProgress;
  int m_requestedSegmentId = kInvalidId;

  std::string m_url;
  std::string m_login;
  std::string m_paswd;
};
