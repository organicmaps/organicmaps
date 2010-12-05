/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef EMACSMODEPLUGIN_H
#define EMACSMODEPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <QtGui/QTextEdit>
#include "ui_emacsmodeoptions.h"
#include <utils/savedaction.h>
#include <coreplugin/icore.h>

namespace EmacsMode {
namespace Internal {

class EmacsModeHandler;

class EmacsModePluginPrivate;

class EmacsModePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    EmacsModePlugin();
    ~EmacsModePlugin();

private:
    // implementation of ExtensionSystem::IPlugin
    bool initialize(const QStringList &arguments, QString *error_message);
    void aboutToShutdown();
    void extensionsInitialized();

private:
    friend class EmacsModePluginPrivate;
    EmacsModePluginPrivate *d;
};

}
}

namespace EmacsMode {
namespace Constants {

const char * const INSTALL_HANDLER        = "TextEditor.EmacsModeHandler";
const char * const MINI_BUFFER            = "TextEditor.EmacsModeMiniBuffer";
const char * const INSTALL_KEY            = "Alt+V,Alt+V";
const char * const SETTINGS_CATEGORY              = "D.EmacsMode";
const char * const SETTINGS_CATEGORY_EMACSMODE_ICON = ":/core/images/category_emacsmode.png";
const char * const SETTINGS_ID                    = "A.General";
const char * const SETTINGS_EX_CMDS_ID            = "B.ExCommands";
const char * const CMD_FILE_NEXT                  = "EmacsMode.SwitchFileNext";
const char * const CMD_FILE_PREV                  = "EmacsMode.SwitchFilePrev";

} // namespace Constants
} // namespace EmacsMode


namespace Core
{
  class IEditor;
}

namespace EmacsMode {
namespace Internal {

class EmacsModeOptionPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    EmacsModeOptionPage() {}

    // IOptionsPage
    QString id() const { return QLatin1String(Constants::SETTINGS_ID); }
    QString displayName() const { return tr("General"); }
    QString category() const { return QLatin1String(Constants::SETTINGS_CATEGORY); }
    QString displayCategory() const {return tr("EmacsMode");}
    QIcon categoryIcon() const {return QIcon(QLatin1String(Constants::SETTINGS_CATEGORY_EMACSMODE_ICON));}

    QWidget *createPage(QWidget *parent);
    void apply() { m_group.apply(Core::ICore::instance()->settings()); }
    void finish() { m_group.finish(); }
    virtual bool matches(QString const & ) const;

private slots:
    void copyTextEditorSettings();
    void setQtStyle();
    void setPlainStyle();

private:
    friend class DebuggerPlugin;
    Ui::EmacsModeOptionPage m_ui;
    QString m_searchKeywords;
    Utils::SavedActionSet m_group;
};

///////////////////////////////////////////////////////////////////////
//
// EmacsModePluginPrivate
//
///////////////////////////////////////////////////////////////////////

class EmacsModePluginPrivate : public QObject
{
    Q_OBJECT

public:
    EmacsModePluginPrivate(EmacsModePlugin *);
    ~EmacsModePluginPrivate();
    friend class EmacsModePlugin;

    bool initialize();
    void aboutToShutdown();

private slots:
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);

    void setUseEmacsMode(const QVariant &value);
    void quitEmacsMode();
    void triggerCompletions();
    void windowCommand(int key);
    void find(bool reverse);
    void findNext(bool reverse);
    void showSettingsDialog();

    void showCommandBuffer(const QString &contents);
    void showExtraInformation(const QString &msg);
    void changeSelection(const QList<QTextEdit::ExtraSelection> &selections);
    void writeFile(bool *handled, const QString &fileName, const QString &contents);
    void moveToMatchingParenthesis(bool *moved, bool *forward, QTextCursor *cursor);
    void indentRegion(int *amount, int beginLine, int endLine,  QChar typedChar);
    void handleExCommand(const QString &cmd);

    void handleDelayedQuitAll(bool forced);
    void handleDelayedQuit(bool forced, Core::IEditor *editor);

signals:
    void delayedQuitRequested(bool forced, Core::IEditor *editor);
    void delayedQuitAllRequested(bool forced);

private:
    EmacsModePlugin *q;
    EmacsModeOptionPage *m_EmacsModeOptionsPage;
    QHash<Core::IEditor *, EmacsModeHandler *> m_editorToHandler;

    void triggerAction(const QString& code);
};

} // namespace Internal
} // namespace EmacsMode

#endif // EmacsModePLUGIN_H
