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

#include "emacsmodeplugin.h"

#include "emacsmodehandler.h"



#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/ifile.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/completionsupport.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/indenter.h>
//#include <texteditor/textblockiterator.h>

#include <find/findplugin.h>
#include <find/textfindconstants.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QtPlugin>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSettings>

#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>


using namespace EmacsMode::Internal;
using namespace TextEditor;
using namespace Core;
using namespace ProjectExplorer;




///////////////////////////////////////////////////////////////////////
//
// EmacsModeOptionPage
//
///////////////////////////////////////////////////////////////////////

namespace EmacsMode {
namespace Internal {


QWidget *EmacsModeOptionPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);

    m_group.clear();
    m_group.insert(theEmacsModeSetting(ConfigUseEmacsMode),
        m_ui.checkBoxUseEmacsMode);
//    m_group.insert(theEmacsModeSetting(ConfigReadEmacsRc)
//        m_ui.checkBoxReadEmacsRc);

    m_group.insert(theEmacsModeSetting(ConfigExpandTab),
        m_ui.checkBoxExpandTab);
    m_group.insert(theEmacsModeSetting(ConfigHlSearch),
        m_ui.checkBoxHlSearch);
    m_group.insert(theEmacsModeSetting(ConfigShiftWidth),
        m_ui.lineEditShiftWidth);

    m_group.insert(theEmacsModeSetting(ConfigSmartTab),
        m_ui.checkBoxSmartTab);
    m_group.insert(theEmacsModeSetting(ConfigStartOfLine),
        m_ui.checkBoxStartOfLine);
    m_group.insert(theEmacsModeSetting(ConfigTabStop),
        m_ui.lineEditTabStop);
    m_group.insert(theEmacsModeSetting(ConfigBackspace),
        m_ui.lineEditBackspace);

    m_group.insert(theEmacsModeSetting(ConfigAutoIndent),
        m_ui.checkBoxAutoIndent);
    m_group.insert(theEmacsModeSetting(ConfigIncSearch),
        m_ui.checkBoxIncSearch);

    connect(m_ui.pushButtonCopyTextEditorSettings, SIGNAL(clicked()),
        this, SLOT(copyTextEditorSettings()));
    connect(m_ui.pushButtonSetQtStyle, SIGNAL(clicked()),
        this, SLOT(setQtStyle()));
    connect(m_ui.pushButtonSetPlainStyle, SIGNAL(clicked()),
        this, SLOT(setPlainStyle()));

    return w;
}

void EmacsModeOptionPage::copyTextEditorSettings()
{
    TextEditor::TabSettings ts = 
        TextEditor::TextEditorSettings::instance()->tabSettings();
    
    m_ui.checkBoxExpandTab->setChecked(ts.m_spacesForTabs);
    m_ui.lineEditTabStop->setText(QString::number(ts.m_tabSize));
    m_ui.lineEditShiftWidth->setText(QString::number(ts.m_indentSize));
    m_ui.checkBoxSmartTab->setChecked(ts.m_smartBackspace);
    m_ui.checkBoxAutoIndent->setChecked(ts.m_autoIndent);
    // FIXME: Not present in core
    //m_ui.checkBoxIncSearch->setChecked(ts.m_incSearch);
}

void EmacsModeOptionPage::setQtStyle()
{
    m_ui.checkBoxExpandTab->setChecked(true);
    m_ui.lineEditTabStop->setText("4");
    m_ui.lineEditShiftWidth->setText("4");
    m_ui.checkBoxSmartTab->setChecked(true);
    m_ui.checkBoxAutoIndent->setChecked(true);
    m_ui.checkBoxIncSearch->setChecked(true);
    m_ui.lineEditBackspace->setText("indent,eol,start");
}

void EmacsModeOptionPage::setPlainStyle()
{
    m_ui.checkBoxExpandTab->setChecked(false);
    m_ui.lineEditTabStop->setText("8");
    m_ui.lineEditShiftWidth->setText("8");
    m_ui.checkBoxSmartTab->setChecked(false);
    m_ui.checkBoxAutoIndent->setChecked(false);
    m_ui.checkBoxIncSearch->setChecked(false);
    m_ui.lineEditBackspace->setText(QString());
}

bool EmacsModeOptionPage::matches(QString const & s) const
{
  return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace EmacsMode


EmacsModePluginPrivate::EmacsModePluginPrivate(EmacsModePlugin *plugin)
{       
    q = plugin;
    m_emacsModeOptionsPage = 0;
}

EmacsModePluginPrivate::~EmacsModePluginPrivate()
{
}

void EmacsModePluginPrivate::aboutToShutdown()
{
    q->removeObject(m_emacsModeOptionsPage);
    delete m_emacsModeOptionsPage;
    m_emacsModeOptionsPage = 0;
    theEmacsModeSettings()->writeSettings(Core::ICore::instance()->settings());
    delete theEmacsModeSettings();
}

bool EmacsModePluginPrivate::initialize()
{
    m_core = Core::ICore::instance();
    m_editorManager = core()->editorManager();
    m_actionManager = core()->actionManager();
    QTC_ASSERT(actionManager(), return false);

//    m_wordCompletion = new WordCompletion;
//    q->addAutoReleasedObject(m_wordCompletion);

    Context globalcontext(Core::Constants::C_GLOBAL);

    m_emacsModeOptionsPage = new EmacsModeOptionPage;
    q->addObject(m_emacsModeOptionsPage);

//    theEmacsModeSettings()->readSettings(Core::ICore::instance()->settings());
    readSettings();

    Core::Command *cmd = 0;
    cmd = actionManager()->registerAction(theEmacsModeSetting(ConfigUseEmacsMode),
        Constants::INSTALL_HANDLER, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INSTALL_KEY));

    ActionContainer *advancedMenu =
        actionManager()->actionContainer(Core::Constants::M_EDIT_ADVANCED);
    advancedMenu->addAction(cmd, Core::Constants::G_EDIT_EDITOR);

    connect(m_core, SIGNAL(coreAboutToClose()), this, SLOT(onCoreAboutToClose()));


    // EditorManager
    connect(editorManager(), SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager(), SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));

//    connect(theEmacsModeSetting(SettingsDialog), SIGNAL(triggered()),
//        this, SLOT(showSettingsDialog()));
    connect(theEmacsModeSetting(ConfigUseEmacsMode), SIGNAL(valueChanged(QVariant)),
        this, SLOT(setUseEmacsMode(QVariant)));

    // Delayed operations.
    connect(this, SIGNAL(delayedQuitRequested(bool,Core::IEditor*)),
        this, SLOT(handleDelayedQuit(bool,Core::IEditor*)), Qt::QueuedConnection);
    connect(this, SIGNAL(delayedQuitAllRequested(bool)),
        this, SLOT(handleDelayedQuitAll(bool)), Qt::QueuedConnection);
//    maybeReadVimRc();
    //    << "MODE: " << theFakeVimSetting(ConfigUseFakeVim)->value();

}

void EmacsModePluginPrivate::readSettings()
{
  QSettings *settings = ICore::instance()->settings();

  theEmacsModeSettings()->readSettings(settings);

  /*exCommandMap() = defaultExCommandMap();
        int size = settings->beginReadArray(_(exCommandMapGroup));
        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            const QString id = settings->value(_(idKey)).toString();
            const QString re = settings->value(_(reKey)).toString();
            exCommandMap()[id] = QRegExp(re);
        }
        settings->endArray();
    }*/
}

void EmacsModePluginPrivate::showSettingsDialog()
{
    Core::ICore::instance()->showOptionsDialog("EmacsMode", "General");
}

void EmacsModePluginPrivate::triggerAction(const QString& code)
{
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    QTC_ASSERT(am, return);
    Core::Command *cmd = am->command(code);
    QTC_ASSERT(cmd, return);
    QAction *action = cmd->action();
    QTC_ASSERT(action, return);
    action->trigger();
}

void EmacsModePluginPrivate::windowCommand(int key)
{
    #define control(n) (256 + n)
    QString code;
    switch (key) {
        case 'c': case 'C': case control('c'):
            code = Core::Constants::CLOSE;
            break;
        case 'n': case 'N': case control('n'):
            code = Core::Constants::GOTONEXT;
            break;
        case 'o': case 'O': case control('o'):
            code = Core::Constants::REMOVE_ALL_SPLITS;
            code = Core::Constants::REMOVE_CURRENT_SPLIT;
            break;
        case 'p': case 'P': case control('p'):
            code = Core::Constants::GOTOPREV;
            break;
        case 's': case 'S': case control('s'):
            code = Core::Constants::SPLIT;
            break;
        case 'w': case 'W': case control('w'):
            code = Core::Constants::GOTO_OTHER_SPLIT;
            break;
    }
    #undef control
    qDebug() << "RUNNING WINDOW COMMAND: " << key << code;
    if (code.isEmpty()) {
        qDebug() << "UNKNOWN WINDOWS COMMAND: " << key;
        return;
    }
    triggerAction(code);
}

void EmacsModePluginPrivate::find(bool reverse)
{
    Q_UNUSED(reverse)  // TODO: Creator needs an action for find in reverse.
    triggerAction(Find::Constants::FIND_IN_DOCUMENT);
}

void EmacsModePluginPrivate::findNext(bool reverse)
{
    if (reverse)
        triggerAction(Find::Constants::FIND_PREVIOUS);
    else
        triggerAction(Find::Constants::FIND_NEXT);
}

void EmacsModePluginPrivate::editorOpened(Core::IEditor *editor)
{
    if (!editor)
        return;

    QWidget *widget = editor->widget();
    if (!widget)
        return;

    // we can only handle QTextEdit and QPlainTextEdit
    if (!qobject_cast<QTextEdit *>(widget) && !qobject_cast<QPlainTextEdit *>(widget))
        return;
    
    //qDebug() << "OPENING: " << editor << editor->widget()
    //    << "MODE: " << theEmacsModeSetting(ConfigUseEmacsMode)->value();

    EmacsModeHandler *handler = new EmacsModeHandler(widget, widget);
    m_editorToHandler[editor] = handler;

    connect(handler, SIGNAL(extraInformationChanged(QString)),
        this, SLOT(showExtraInformation(QString)));
    connect(handler, SIGNAL(commandBufferChanged(QString)),
        this, SLOT(showCommandBuffer(QString)));
    connect(handler, SIGNAL(writeFileRequested(bool*,QString,QString)),
        this, SLOT(writeFile(bool*,QString,QString)));
    connect(handler, SIGNAL(selectionChanged(QList<QTextEdit::ExtraSelection>)),
        this, SLOT(changeSelection(QList<QTextEdit::ExtraSelection>)));
    connect(handler, SIGNAL(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)),
        this, SLOT(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)));
    connect(handler, SIGNAL(indentRegion(int*,int,int,QChar)),
        this, SLOT(indentRegion(int*,int,int,QChar)));
    connect(handler, SIGNAL(completionRequested()),
        this, SLOT(triggerCompletions()));
    connect(handler, SIGNAL(windowCommandRequested(int)),
        this, SLOT(windowCommand(int)));
    connect(handler, SIGNAL(findRequested(bool)),
        this, SLOT(find(bool)));
    connect(handler, SIGNAL(findNextRequested(bool)),
        this, SLOT(findNext(bool)));

    connect(handler, SIGNAL(handleExCommandRequested(QString)),
        this, SLOT(handleExCommand(QString)));

    handler->setCurrentFileName(editor->file()->fileName());
    handler->installEventFilter();
    
    // pop up the bar
    if (theEmacsModeSetting(ConfigUseEmacsMode)->value().toBool())
       showCommandBuffer("");
}

void EmacsModePluginPrivate::editorAboutToClose(Core::IEditor *editor)
{
    //qDebug() << "CLOSING: " << editor << editor->widget();
    m_editorToHandler.remove(editor);
}

void EmacsModePluginPrivate::setUseEmacsMode(const QVariant &value)
{
    //qDebug() << "SET USE EmacsMode" << value;
    bool on = value.toBool();
    if (on) {
        Core::EditorManager::instance()->showEditorStatusBar( 
            QLatin1String(Constants::MINI_BUFFER), 
            "vi emulation mode. Type :q to leave. Use , Ctrl-R to trigger run.",
            tr("Quit EmacsMode"), this, SLOT(quitEmacsMode()));
        foreach (Core::IEditor *editor, m_editorToHandler.keys())
            m_editorToHandler[editor]->setupWidget();
    } else {
        Core::EditorManager::instance()->hideEditorStatusBar(
            QLatin1String(Constants::MINI_BUFFER));
        foreach (Core::IEditor *editor, m_editorToHandler.keys())
            m_editorToHandler[editor]->restoreWidget();
    }
}

void EmacsModePluginPrivate::triggerCompletions()
{
/*    EmacsModeHandler *handler = qobject_cast<EmacsModeHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditorWidget *editor = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
        CompletionSupport::instance()->
            autoComplete(editor->editor(), false);
   //     editor->triggerCompletions();
*/
}

void EmacsModePluginPrivate::writeFile(bool *handled,
    const QString &fileName, const QString &contents)
{
    Q_UNUSED(contents)

    EmacsModeHandler *handler = qobject_cast<EmacsModeHandler *>(sender());
    if (!handler)
        return;

    Core::IEditor *editor = m_editorToHandler.key(handler);
    if (editor && editor->file()->fileName() == fileName) {
        // Handle that as a special case for nicer interaction with core
        Core::IFile *file = editor->file();
        Core::ICore::instance()->fileManager()->blockFileChange(file);
        file->save(fileName);
        Core::ICore::instance()->fileManager()->unblockFileChange(file);
        *handled = true;
    } 
}

void EmacsModePluginPrivate::handleExCommand(const QString &cmd)
{
    static QRegExp reNextFile("^n(ext)?!?( (.*))?$");
    static QRegExp rePreviousFile("^(N(ext)?|prev(ious)?)!?( (.*))?$");
    static QRegExp reWriteAll("^wa(ll)?!?$");
    static QRegExp reQuit("^q!?$");
    static QRegExp reQuitAll("^qa!?$");

    using namespace Core;

    EmacsModeHandler *handler = qobject_cast<EmacsModeHandler *>(sender());
    if (!handler)
        return;

    EditorManager *editorManager = EditorManager::instance();
    QTC_ASSERT(editorManager, return);

    if (reNextFile.indexIn(cmd) != -1) {
        // :n
        editorManager->goForwardInNavigationHistory();
    } else if (rePreviousFile.indexIn(cmd) != -1) {
        // :N, :prev
        editorManager->goBackInNavigationHistory();
    } else if (reWriteAll.indexIn(cmd) != -1) {
        // :wa
        FileManager *fm = ICore::instance()->fileManager();
        QList<IFile *> toSave = fm->modifiedFiles();
        QList<IFile *> failed = fm->saveModifiedFilesSilently(toSave);
        if (failed.isEmpty())
            handler->showBlackMessage(tr("Saving succeeded"));
        else
            handler->showRedMessage(tr("%n files not saved", 0, failed.size()));
    } else if (reQuit.indexIn(cmd) != -1) {
        // :q
        bool forced = cmd.contains(QChar('!'));
        emit delayedQuitRequested(forced, m_editorToHandler.key(handler));
    } else if (reQuitAll.indexIn(cmd) != -1) {
        // :qa
        bool forced = cmd.contains(QChar('!'));
        emit delayedQuitAllRequested(forced);
    } else {
        handler->showRedMessage(tr("Not an editor command: %1").arg(cmd));
    }
}

void EmacsModePluginPrivate::handleDelayedQuit(bool forced, Core::IEditor *editor)
{
    QList<Core::IEditor *> editors;
    editors.append(editor);
    Core::EditorManager::instance()->closeEditors(editors, !forced);
}

void EmacsModePluginPrivate::handleDelayedQuitAll(bool forced)
{
    Core::EditorManager::instance()->closeAllEditors(!forced);
}

void EmacsModePluginPrivate::moveToMatchingParenthesis(bool *moved, bool *forward,
        QTextCursor *cursor)
{
    *moved = false;

    bool undoFakeEOL = false;
    if (cursor->atBlockEnd() && cursor->block().length() > 1) {
        cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
        undoFakeEOL = true;
    }
    TextEditor::TextBlockUserData::MatchType match
        = TextEditor::TextBlockUserData::matchCursorForward(cursor);
    if (match == TextEditor::TextBlockUserData::Match) {
        *moved = true;
        *forward = true;
    } else {
        if (undoFakeEOL)
            cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        if (match == TextEditor::TextBlockUserData::NoMatch) {
            // backward matching is according to the character before the cursor
            bool undoMove = false;
            if (!cursor->atBlockEnd()) {
                cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                undoMove = true;
            }
            match = TextEditor::TextBlockUserData::matchCursorBackward(cursor);
            if (match == TextEditor::TextBlockUserData::Match) {
                *moved = true;
                *forward = false;
            } else if (undoMove) {
                cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            }
        }
    }
}

void EmacsModePluginPrivate::indentRegion(int *amount, int beginLine, int endLine,
      QChar typedChar)
{
    EmacsModeHandler *handler = qobject_cast<EmacsModeHandler *>(sender());
    if (!handler)
        return;

    BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget());
    if (!bt)
        return;

    const TabSettings oldTabSettings = bt->tabSettings();
    TabSettings tabSettings;
    tabSettings.m_indentSize = theEmacsModeSetting(ConfigShiftWidth)->value().toInt();
    tabSettings.m_tabSize = theEmacsModeSetting(ConfigTabStop)->value().toInt();
    tabSettings.m_spacesForTabs = theEmacsModeSetting(ConfigExpandTab)->value().toBool();
    bt->setTabSettings(tabSettings);

    QTextDocument *doc = bt->document();
    QTextBlock startBlock = doc->findBlockByNumber(beginLine);

    // Record line lenghts for mark adjustments
    QVector<int> lineLengths(endLine - beginLine + 1);
    QTextBlock block = startBlock;

    for (int i = beginLine; i <= endLine; ++i) {
        lineLengths[i - beginLine] = block.text().length();
        if (typedChar == 0 && block.text().simplified().isEmpty()) {
            // clear empty lines
            QTextCursor cursor(block);
            while (!cursor.atBlockEnd())
                cursor.deleteChar();
        } else {
            bt->indenter()->indentBlock(doc, block, typedChar, bt);
        }
        block = block.next();
    }

    bt->setTabSettings(oldTabSettings);
}

void EmacsModePluginPrivate::quitEmacsMode()
{
    theEmacsModeSetting(ConfigUseEmacsMode)->setValue(false);
}

void EmacsModePluginPrivate::showCommandBuffer(const QString &contents)
{
    //qDebug() << "SHOW COMMAND BUFFER" << contents;
    Core::EditorManager::instance()->showEditorStatusBar( 
        QLatin1String(Constants::MINI_BUFFER), contents,
        tr("Quit EmacsMode"), this, SLOT(quitEmacsMode()));
}

void EmacsModePluginPrivate::showExtraInformation(const QString &text)
{
    EmacsModeHandler *handler = qobject_cast<EmacsModeHandler *>(sender());
    if (handler)
        QMessageBox::information(handler->widget(), tr("EmacsMode Information"), text);
}

void EmacsModePluginPrivate::changeSelection
    (const QList<QTextEdit::ExtraSelection> &selection)
{
    if (EmacsModeHandler *handler = qobject_cast<EmacsModeHandler *>(sender()))
        if (BaseTextEditorWidget *bt = qobject_cast<BaseTextEditorWidget *>(handler->widget()))
            bt->setExtraSelections(BaseTextEditorWidget::FakeVimSelection, selection);
}


///////////////////////////////////////////////////////////////////////
//
// EmacsModePlugin
//
///////////////////////////////////////////////////////////////////////

EmacsModePlugin::EmacsModePlugin()
    : d(new EmacsModePluginPrivate(this))
{}

EmacsModePlugin::~EmacsModePlugin()
{
    delete d;
}

bool EmacsModePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    return d->initialize();
}

ExtensionSystem::IPlugin::ShutdownFlag EmacsModePlugin::aboutToShutdown()
{
    d->aboutToShutdown();
    return SynchronousShutdown;
}

void EmacsModePlugin::extensionsInitialized()
{
}

#include "moc_emacsmodeplugin.cpp"

Q_EXPORT_PLUGIN(EmacsModePlugin)
