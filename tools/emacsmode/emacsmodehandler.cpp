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

#include "emacsmodehandler.h"
#include <boost/bind.hpp>

//
// ATTENTION:
//
// 1 Please do not add any direct dependencies to other Qt Creator code here.
//   Instead emit signals and let the EmacsModePlugin channel the information to
//   Qt Creator. The idea is to keep this file here in a "clean" state that
//   allows easy reuse with any QTextEdit or QPlainTextEdit derived class.
//
// 2 There are a few auto tests located in ../../../tests/auto/EmacsMode.
//   Commands that are covered there are marked as "// tested" below.
//
// 3 Some conventions:
//
//   Use 1 based line numbers and 0 based column numbers. Even though
//   the 1 based line are not nice it matches vim's and QTextEdit's 'line'
//   concepts.
//
//   Do not pass QTextCursor etc around unless really needed. Convert
//   early to  line/column.
//
//   There is always a "current" cursor (m_tc). A current "region of interest"
//   spans between m_anchor (== anchor()) and  m_tc.position() (== position())
//   The value of m_tc.anchor() is not used.
//

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>
#include <QtCore/QtAlgorithms>
#include <QtCore/QStack>

#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QLineEdit>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QScrollBar>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocumentFragment>
#include <QtGui/QTextEdit>

#include <climits>
#include <ctype.h>

//#define DEBUG_KEY  1
#if DEBUG_KEY
#   define KEY_DEBUG(s) qDebug() << s
#else
#   define KEY_DEBUG(s)
#endif

//#define DEBUG_UNDO  1
#if DEBUG_UNDO
#   define UNDO_DEBUG(s) qDebug() << << m_tc.document()->availableUndoSteps() << s
#else
#   define UNDO_DEBUG(s)
#endif

using namespace Utils;

namespace EmacsMode {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// EmacsModeHandler
//
///////////////////////////////////////////////////////////////////////

#define StartOfLine     QTextCursor::StartOfLine
#define EndOfLine       QTextCursor::EndOfLine
#define MoveAnchor      QTextCursor::MoveAnchor
#define KeepAnchor      QTextCursor::KeepAnchor
#define Up              QTextCursor::Up
#define Down            QTextCursor::Down
#define Right           QTextCursor::Right
#define Left            QTextCursor::Left
#define EndOfDocument   QTextCursor::End
#define StartOfDocument QTextCursor::Start

#define EDITOR(s) (m_textedit ? m_textedit->s : m_plaintextedit->s)

const int ParagraphSeparator = 0x00002029;

using namespace Qt;


enum Mode
{
    InsertMode,
    CommandMode,
    ExMode,
    SearchForwardMode,
    SearchBackwardMode,
};

enum SubMode
{
    NoSubMode,
    ChangeSubMode,     // used for c
    DeleteSubMode,     // used for d
    FilterSubMode,     // used for !
    IndentSubMode,     // used for =
    RegisterSubMode,   // used for "
    ReplaceSubMode,    // used for R and r
    ShiftLeftSubMode,  // used for <
    ShiftRightSubMode, // used for >
    WindowSubMode,     // used for Ctrl-w
    YankSubMode,       // used for y
    ZSubMode,          // used for z
    CapitalZSubMode    // used for Z
};

enum SubSubMode
{
    // typically used for things that require one more data item
    // and are 'nested' behind a mode
    NoSubSubMode,
    FtSubSubMode,       // used for f, F, t, T
    MarkSubSubMode,     // used for m
    BackTickSubSubMode, // used for `
    TickSubSubMode,     // used for '
};

enum VisualMode
{
    NoVisualMode,
    VisualCharMode,
    VisualLineMode,
    VisualBlockMode,
};

enum MoveType
{
    MoveExclusive,
    MoveInclusive,
    MoveLineWise,
};

enum RangeMode
{
    RangeCharMode,  // v
    RangeLineMode,  // V
    RangeBlockMode, // Ctrl-v
    RangeBlockAndTailMode, // Ctrl-v for D and X
};

enum EventResult
{
    EventHandled,
    EventUnhandled,
    EventPassedToCore
};

struct Indentation
{
    Indentation(int p, int l) : physical(p), logical(l) {}
    int physical; // number of characters
    int logical;
};

struct CursorPosition
{
    // for jump history
    CursorPosition() : position(-1), scrollLine(-1) {}
    CursorPosition(int pos, int line) : position(pos), scrollLine(line) {}
    int position; // Position in document
    int scrollLine; // First visible line
};

struct Register
{
    Register() : rangemode(RangeCharMode) {}
    Register(const QString &c, RangeMode m) : contents(c), rangemode(m) {}
    QString contents;
    RangeMode rangemode;
};

struct Range
{
    Range()
        : beginPos(-1), endPos(-1), rangemode(RangeCharMode)
    {}

    Range(int b, int e, RangeMode m = RangeCharMode)
        : beginPos(qMin(b, e)), endPos(qMax(b, e)), rangemode(m)
    {}

    QString toString() const
    {
        return QString("%1-%2 (mode: %3)").arg(beginPos).arg(endPos)
            .arg(rangemode);
    }

    int beginPos;
    int endPos;
    RangeMode rangemode;
};

QDebug &operator<<(QDebug &ts, const QList<QTextEdit::ExtraSelection> &sels)
{
    foreach (QTextEdit::ExtraSelection sel, sels)
        ts << "SEL: " << sel.cursor.anchor() << sel.cursor.position();
    return ts;
}

QString quoteUnprintable(const QString &ba)
{
    QString res;
    for (int i = 0, n = ba.size(); i != n; ++i) {
        QChar c = ba.at(i);
        if (c.isPrint())
            res += c;
        else
            res += QString("\\x%1").arg(c.unicode(), 2, 16);
    }
    return res;
}

inline QString msgE20MarkNotSet(const QString &text)
{
    return EmacsModeHandler::tr("E20: Mark '%1' not set").arg(text);
}

class EmacsModeHandler::Private
{
public:
    Private(EmacsModeHandler *parent, QWidget *widget);

    EventResult handleEvent(QKeyEvent *ev);
    bool wantsOverride(QKeyEvent *ev);
    void handleCommand(const QString &cmd); // sets m_tc + handleExCommand
    void handleExCommand(const QString &cmd);
    void fixMarks(int positionAction, int positionChange); //Updates marks positions by the difference in positionChange

    void installEventFilter();
    void setupWidget();
    void restoreWidget();

    friend class EmacsModeHandler;
    static int shift(int key) { return key + 32; }
    static int control(int key) { return key + 256; }

    void init();
    EventResult handleKey(int key, int unmodified, const QString &text);
    EventResult handleInsertMode(int key, int unmodified, const QString &text);
    EventResult handleCommandMode(int key, int unmodified, const QString &text);
    EventResult handleRegisterMode(int key, int unmodified, const QString &text);
    EventResult handleMiniBufferModes(int key, int unmodified, const QString &text);
    void finishMovement(const QString &text = QString());
    void search(const QString &needle, bool forward);
    void highlightMatches(const QString &needle);

    int mvCount() const { return m_mvcount.isEmpty() ? 1 : m_mvcount.toInt(); }
    int opCount() const { return m_opcount.isEmpty() ? 1 : m_opcount.toInt(); }
    int count() const { return mvCount() * opCount(); }
    int leftDist() const { return m_tc.position() - m_tc.block().position(); }
    int rightDist() const { return m_tc.block().length() - leftDist() - 1; }
    bool atEndOfLine() const
        { return m_tc.atBlockEnd() && m_tc.block().length() > 1; }

    int lastPositionInDocument() const; // last valid pos in doc
    int firstPositionInLine(int line) const; // 1 based line, 0 based pos
    int lastPositionInLine(int line) const; // 1 based line, 0 based pos
    int lineForPosition(int pos) const;  // 1 based line, 0 based pos
    QString lineContents(int line) const; // 1 based line
    void setLineContents(int line, const QString &contents) const; // 1 based line

    int linesOnScreen() const;
    int columnsOnScreen() const;
    int linesInDocument() const;

    // all zero-based counting
    int cursorLineOnScreen() const;
    int cursorLineInDocument() const;
    int cursorColumnInDocument() const;
    int firstVisibleLineInDocument() const;
    void scrollToLineInDocument(int line);
    void scrollUp(int count);
    void scrollDown(int count) { scrollUp(-count); }

    CursorPosition cursorPosition() const
        { return CursorPosition(position(), firstVisibleLineInDocument()); }
    void setCursorPosition(const CursorPosition &p)
        { setPosition(p.position); scrollToLineInDocument(p.scrollLine); }

    // helper functions for indenting
    bool isElectricCharacter(QChar c) const
        { return c == '{' || c == '}' || c == '#'; }
    void indentRegion(QChar lastTyped = QChar());

    void shiftRegionLeft(int repeat = 1);
    void shiftRegionRight(int repeat = 1);

    void moveToFirstNonBlankOnLine();
    void moveToTargetColumn();
    void setTargetColumn() {
        m_targetColumn = leftDist();
        //qDebug() << "TARGET: " << m_targetColumn;
    }
    void moveToNextWord(bool simple);
    void moveToMatchingParanthesis();
    void moveToWordBoundary(bool simple, bool forward);

    // to reduce line noise
    void moveToEndOfDocument() { m_tc.movePosition(EndOfDocument, MoveAnchor); }
    void moveToStartOfLine();
    void moveToEndOfLine();
    void moveBehindEndOfLine();
    void moveUp(int n = 1) { m_tc.movePosition(Up, m_moveMode, n); }
    void moveDown(int n = 1) { m_tc.movePosition(Down, m_moveMode, n); }
    void moveRight(int n = 1) { m_tc.movePosition(Right, m_moveMode, n); }
    void moveLeft(int n = 1) { m_tc.movePosition(Left, m_moveMode, n); }
    void setAnchor() { m_anchor = m_tc.position(); }
    void setAnchor(int position) { m_anchor = position; }
    void setPosition(int position) { m_tc.setPosition(position, MoveAnchor); }

    void handleFfTt(int key);

    // helper function for handleExCommand. return 1 based line index.
    int readLineCode(QString &cmd);
    void selectRange(int beginLine, int endLine);

    void enterInsertMode();
    void enterCommandMode();
    void enterExMode();
    void showRedMessage(const QString &msg);
    void showBlackMessage(const QString &msg);
    void notImplementedYet();
    void updateMiniBuffer();
    void updateSelection();
    QWidget *editor() const;
    QChar characterAtCursor() const
        { return m_tc.document()->characterAt(m_tc.position()); }
    void beginEditBlock() { UNDO_DEBUG("BEGIN EDIT BLOCK"); m_tc.beginEditBlock(); }
    void beginEditBlock(int pos) { setUndoPosition(pos); beginEditBlock(); }
    void endEditBlock() { UNDO_DEBUG("END EDIT BLOCK"); m_tc.endEditBlock(); }
    void joinPreviousEditBlock() { UNDO_DEBUG("JOIN EDIT BLOCK"); m_tc.joinPreviousEditBlock(); }

    // this asks the layer above (e.g. the fake vim plugin or the
    // stand-alone test application to handle the command)
    void passUnknownExCommand(const QString &cmd);

    bool isVisualMode() const { return m_visualMode != NoVisualMode; }
    bool isNoVisualMode() const { return m_visualMode == NoVisualMode; }
    bool isVisualCharMode() const { return m_visualMode == VisualCharMode; }
    bool isVisualLineMode() const { return m_visualMode == VisualLineMode; }
    bool isVisualBlockMode() const { return m_visualMode == VisualBlockMode; }

public:
    QTextEdit *m_textedit;
    QPlainTextEdit *m_plaintextedit;
    bool m_wasReadOnly; // saves read-only state of document

    EmacsModeHandler *q;
    Mode m_mode;
    bool m_passing; // let the core see the next event
    SubMode m_submode;
    SubSubMode m_subsubmode;
    int m_subsubdata;
    QString m_input;
    QTextCursor m_tc;
    int m_oldPosition; // copy from last event to check for external changes
    int m_anchor;
    static QHash<int, Register> m_registers;
    int m_register;
    QString m_mvcount;
    QString m_opcount;
    MoveType m_movetype;
    RangeMode m_rangemode;

    bool m_fakeEnd;

    bool isSearchMode() const
        { return m_mode == SearchForwardMode || m_mode == SearchBackwardMode; }
    int m_gflag;  // whether current command started with 'g'

    QString m_commandBuffer;
    QString m_currentFileName;
    QString m_currentMessage;

    bool m_lastSearchForward;
    QString m_lastInsertion;

    int anchor() const { return m_anchor; }
    int position() const { return m_tc.position(); }

    void removeSelectedText();
    void removeText(const Range &range);

    QString selectedText() const { return text(Range(position(), anchor())); }
    QString text(const Range &range) const;

    void yankSelectedText();
    void yankText(const Range &range, int toregister = '"');

    void pasteText(bool afterCursor);

    // undo handling
    void undo();
    void redo();
    void setUndoPosition(int pos);
    QMap<int, int> m_undoCursorPosition; // revision -> position

    // extra data for '.'
    void replay(const QString &text, int count);
    void setDotCommand(const QString &cmd) { m_dotCommand = cmd; }
    void setDotCommand(const QString &cmd, int n) { m_dotCommand = cmd.arg(n); }
    QString m_dotCommand;
    bool m_inReplay; // true if we are executing a '.'

    // extra data for ';'
    QString m_semicolonCount;
    int m_semicolonType;  // 'f', 'F', 't', 'T'
    int m_semicolonKey;

    // history for '/'
    QString lastSearchString() const;
    static QStringList m_searchHistory;
    int m_searchHistoryIndex;

    // history for ':'
    static QStringList m_commandHistory;
    int m_commandHistoryIndex;

    // visual line mode
    void enterVisualMode(VisualMode visualMode);
    void leaveVisualMode();
    VisualMode m_visualMode;

    // marks as lines
    QHash<int, int> m_marks;
    QString m_oldNeedle;

    // vi style configuration
    QVariant config(int code) const { return theEmacsModeSetting(code)->value(); }
    bool hasConfig(int code) const { return config(code).toBool(); }
    bool hasConfig(int code, const char *value) const // FIXME
        { return config(code).toString().contains(value); }

    // for restoring cursor position
    int m_savedYankPosition;
    int m_targetColumn;

    int m_cursorWidth;

    // auto-indent
    QString tabExpand(int len) const;
    Indentation indentation(const QString &line) const;
    void insertAutomaticIndentation(bool goingDown);
    bool removeAutomaticIndentation(); // true if something removed
    // number of autoindented characters
    int m_justAutoIndented;
    void handleStartOfLine();

    void recordJump();
    void recordNewUndo();
    QVector<CursorPosition> m_jumpListUndo;
    QVector<CursorPosition> m_jumpListRedo;

    QList<QTextEdit::ExtraSelection> m_searchSelections;


    typedef std::list<Emacs::Shortcut> TShortcutList;
    TShortcutList m_shortcuts;
    TShortcutList m_partialShortcuts;

    QTextCursor::MoveMode m_moveMode;

    void setMoveMode(QTextCursor::MoveMode moveMode);
    void anchorCurrentPos();

    QStringList m_killBuffer;

    void cleanKillBuffer();
    void killLine();

    void yank();

    void copySelected();
    void killSelected();

    bool m_isAppendingKillBuffer;
    void setKillBufferAppending(bool flag);

    void saveToFile(QString const & fileName);
    void saveCurrentFile();

    void commentOutRegion();
    void uncommentRegion();
};

QStringList EmacsModeHandler::Private::m_searchHistory;
QStringList EmacsModeHandler::Private::m_commandHistory;
QHash<int, Register> EmacsModeHandler::Private::m_registers;

EmacsModeHandler::Private::Private(EmacsModeHandler *parent, QWidget *widget)
{
    q = parent;
    m_textedit = qobject_cast<QTextEdit *>(widget);
    m_plaintextedit = qobject_cast<QPlainTextEdit *>(widget);
    init();
}

void EmacsModeHandler::Private::init()
{
    m_mode = CommandMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_passing = false;
    m_fakeEnd = false;
    m_lastSearchForward = true;
    m_register = '"';
    m_gflag = false;
    m_visualMode = NoVisualMode;
    m_targetColumn = 0;
    m_movetype = MoveInclusive;
    m_anchor = 0;
    m_savedYankPosition = 0;
    m_cursorWidth = EDITOR(cursorWidth());
    m_inReplay = false;
    m_justAutoIndented = 0;
    m_rangemode = RangeCharMode;

    m_moveMode = QTextCursor::MoveAnchor;
    m_isAppendingKillBuffer = false;
    cleanKillBuffer();

    m_shortcuts.push_back(Emacs::Shortcut("<META>|p")
                            .addFn(boost::bind(&EmacsModeHandler::Private::moveUp, this, 1))
                            .addFn(boost::bind(&EmacsModeHandler::Private::setKillBufferAppending, this, false)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|n")
                            .addFn(boost::bind(&EmacsModeHandler::Private::moveDown, this, 1))
                            .addFn(boost::bind(&EmacsModeHandler::Private::setKillBufferAppending, this, false)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|f")
                            .addFn(boost::bind(&EmacsModeHandler::Private::moveRight, this, 1))
                            .addFn(boost::bind(&EmacsModeHandler::Private::setKillBufferAppending, this, false)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|b")
                            .addFn(boost::bind(&EmacsModeHandler::Private::moveLeft, this, 1))
                            .addFn(boost::bind(&EmacsModeHandler::Private::setKillBufferAppending, this, false)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|e")
                            .addFn(boost::bind(&EmacsModeHandler::Private::moveToEndOfLine, this))
                            .addFn(boost::bind(&EmacsModeHandler::Private::setKillBufferAppending, this, false)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|a")
                            .addFn(boost::bind(&EmacsModeHandler::Private::moveToStartOfLine, this))
                            .addFn(boost::bind(&EmacsModeHandler::Private::setKillBufferAppending, this, false)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|<SHIFT>|<UNDERSCORE>")
                            .addFn(boost::bind(&EmacsModeHandler::Private::undo, this))
                            .addFn(boost::bind(&EmacsModeHandler::Private::setKillBufferAppending, this, false)));
    m_shortcuts.push_back(Emacs::Shortcut("<TAB>")
                            .addFn(boost::bind(&EmacsModeHandler::Private::indentRegion, this, QChar('{')))
                            .addFn(boost::bind(&EmacsModeHandler::Private::setKillBufferAppending, this, false)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|<SPACE>")
                            .addFn(boost::bind(&EmacsModeHandler::Private::anchorCurrentPos, this))
                            .addFn(boost::bind(&EmacsModeHandler::Private::setMoveMode, this, QTextCursor::KeepAnchor)));
    m_shortcuts.push_back(Emacs::Shortcut("<ESC>|<ESC>")
                            .addFn(boost::bind(&EmacsModeHandler::Private::setMoveMode, this, QTextCursor::MoveAnchor))
                            .addFn(boost::bind(&EmacsModeHandler::Private::anchorCurrentPos, this)));

    m_shortcuts.push_back(Emacs::Shortcut("<META>|w").addFn(boost::bind(&EmacsModeHandler::Private::killSelected, this)));
    m_shortcuts.push_back(Emacs::Shortcut("<ALT>|w").addFn(boost::bind(&EmacsModeHandler::Private::copySelected, this)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|k").addFn(boost::bind(&EmacsModeHandler::Private::killLine, this)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|y").addFn(boost::bind(&EmacsModeHandler::Private::yank, this)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|x|s").addFn(boost::bind(&EmacsModeHandler::Private::saveCurrentFile, this)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|i|c").addFn(boost::bind(&EmacsModeHandler::Private::commentOutRegion, this)));
    m_shortcuts.push_back(Emacs::Shortcut("<META>|i|u").addFn(boost::bind(&EmacsModeHandler::Private::uncommentRegion, this)));
}

void EmacsModeHandler::Private::saveCurrentFile()
{
    saveToFile(m_currentFileName);
}

void EmacsModeHandler::Private::commentOutRegion()
{
    int beginLine = lineForPosition(m_tc.anchor());
    int endLine = lineForPosition(m_tc.position());

    if (m_tc.atBlockStart() && m_tc.atBlockEnd())
      if (beginLine > endLine)
        ++endLine;
      else
        --endLine;

    if (beginLine > endLine)
        qSwap(beginLine, endLine);

    /// find out whether we need to uncomment it
    int firstPos = firstPositionInLine(beginLine);

    beginEditBlock(firstPos);

    for (int line = beginLine; line <= endLine; ++line) {
        setPosition(firstPositionInLine(line));
        m_tc.insertText("//");
    }
    endEditBlock();

    setPosition(firstPos);
    moveToFirstNonBlankOnLine();
}

void EmacsModeHandler::Private::uncommentRegion()
{
    int beginLine = lineForPosition(m_tc.anchor());
    int endLine = lineForPosition(m_tc.position());

    if (m_tc.atBlockStart() && m_tc.atBlockEnd())
      if (beginLine > endLine)
        ++endLine;
      else
        --endLine;

    if (beginLine > endLine)
        qSwap(beginLine, endLine);


    /// find out whether we need to uncomment it
    int firstPos = firstPositionInLine(beginLine);

    beginEditBlock(firstPos);

    for (int line = beginLine; line <= endLine; ++line) {
        setPosition(firstPositionInLine(line));
        m_tc.deleteChar();
        m_tc.deleteChar();
    }
    endEditBlock();

    setPosition(firstPos);
    moveToFirstNonBlankOnLine();
}

void EmacsModeHandler::Private::saveToFile(QString const & fileName)
{
    int beginLine = 0;
    int endLine = linesInDocument();
    QFile file1(fileName);
    bool exists = file1.exists();
    if (file1.open(QIODevice::ReadWrite))
    {
        file1.close();
        QTextCursor tc = m_tc;
        Range range(firstPositionInLine(beginLine),
            firstPositionInLine(endLine), RangeLineMode);
        QString contents = text(range);
        m_tc = tc;
        bool handled = false;
        emit q->writeFileRequested(&handled, fileName, contents);
        // nobody cared, so act ourselves
        if (!handled) {
            //qDebug() << "HANDLING MANUAL SAVE TO " << fileName;
            QFile::remove(fileName);
            QFile file2(fileName);
            if (file2.open(QIODevice::ReadWrite)) {
                QTextStream ts(&file2);
                ts << contents;
            } else {
                showRedMessage(EmacsModeHandler::tr
                   ("Cannot open file '%1' for writing").arg(fileName));
            }
        }
        // check result by reading back
        QFile file3(fileName);
        file3.open(QIODevice::ReadOnly);
        QByteArray ba = file3.readAll();
        showBlackMessage(EmacsModeHandler::tr("\"%1\" %2 %3L, %4C written")
            .arg(fileName).arg(exists ? " " : " [New] ")
            .arg(ba.count('\n')).arg(ba.size()));
    }
    else
    {
        showRedMessage(EmacsModeHandler::tr
            ("Cannot open file '%1' for reading").arg(fileName));
    }
}

void EmacsModeHandler::Private::setKillBufferAppending(bool flag)
{
    m_isAppendingKillBuffer = flag;
}

void EmacsModeHandler::Private::copySelected()
{
    if (!m_isAppendingKillBuffer)
        m_killBuffer.clear();
    m_isAppendingKillBuffer = true;
    m_killBuffer.append(m_tc.selectedText());
    anchorCurrentPos();
    setKillBufferAppending(true);
    setMoveMode(MoveAnchor);
}

void EmacsModeHandler::Private::killSelected()
{
    if (!m_isAppendingKillBuffer)
        m_killBuffer.clear();
    m_isAppendingKillBuffer = true;
    m_killBuffer.append(m_tc.selectedText());
    m_tc.removeSelectedText();
    anchorCurrentPos();
    setKillBufferAppending(true);
    setMoveMode(MoveAnchor);
}

void EmacsModeHandler::Private::cleanKillBuffer()
{
    m_killBuffer.clear();
}

void EmacsModeHandler::Private::yank()
{
    m_tc.insertText(m_killBuffer.join(""));
}

void EmacsModeHandler::Private::killLine()
{
    if (!m_isAppendingKillBuffer)
        m_killBuffer.clear();
    m_isAppendingKillBuffer = true;
    bool isEndOfLine = (atEndOfLine() || (m_tc.block().length() == 1));

    if (!isEndOfLine)
    {
      m_tc.setPosition(m_tc.position(), MoveAnchor);
      m_tc.movePosition(QTextCursor::EndOfLine, KeepAnchor);
      m_killBuffer.append(m_tc.selectedText());
    }
    else
    {
      m_tc.setPosition(m_tc.position(), MoveAnchor);
      m_tc.movePosition(QTextCursor::NextCharacter, KeepAnchor);
      m_killBuffer.append(m_tc.selectedText());
    }
    m_tc.removeSelectedText();
}

void EmacsModeHandler::Private::setMoveMode(QTextCursor::MoveMode moveMode)
{
    m_moveMode = moveMode;
}

void EmacsModeHandler::Private::anchorCurrentPos()
{
    m_tc.setPosition(m_tc.position(), MoveAnchor);
}

bool EmacsModeHandler::Private::wantsOverride(QKeyEvent *ev)
{
  TShortcutList shortcuts = m_partialShortcuts.empty() ? m_shortcuts : m_partialShortcuts;
  for (TShortcutList::const_iterator it = shortcuts.begin(); it != shortcuts.end(); ++it)
    if (it->isAccepted(ev))
      return true;
  return false;
}

EventResult EmacsModeHandler::Private::handleEvent(QKeyEvent *ev)
{
  m_tc = EDITOR(textCursor());
  if (m_partialShortcuts.empty())
    m_partialShortcuts = m_shortcuts;

  TShortcutList newPartialShortcuts;
  bool executed = false;
  bool isAccepted = false;

  for (TShortcutList::const_iterator it = m_partialShortcuts.begin(); it != m_partialShortcuts.end(); ++it)
  {
    if (it->isAccepted(ev))
    {
      isAccepted = true;
      if (it->hasFollower(ev))
        newPartialShortcuts.push_back(it->getFollower(ev));
      else
      {
        it->exec();
        executed = true;
        m_partialShortcuts.clear();
        break;
      }
    }
  }

  if (!executed)
    m_partialShortcuts = newPartialShortcuts;


  EDITOR(setTextCursor(m_tc));

  return isAccepted ? EventHandled : EventPassedToCore;


//  // Position changed externally
//  if (m_tc.position() != m_oldPosition) {
//    setTargetColumn();
//    if (m_mode == InsertMode) {
//      int dist = m_tc.position() - m_oldPosition;
//      // Try to compensate for code completion
//      if (dist > 0 && dist <= cursorColumnInDocument()) {
//        Range range(m_oldPosition, m_tc.position());
//        m_lastInsertion.append(text(range));
//      }
//    }
//  }
//
//  m_tc.setVisualNavigation(true);
//
//  if (m_fakeEnd)
//    moveRight();
//
//  if ((kev->modifiers() & Qt::ControlModifier) != 0) {
//    key += 256;
//    key += 32; // make it lower case
//  } else if (key >= Key_A && key <= Key_Z && (mods & Qt::ShiftModifier) == 0) {
//    key += 32;
//  }
//
//  //if (m_mode == InsertMode)
//  //    joinPreviousEditBlock();
//  //else
//  //    beginEditBlock();
//  EventResult result = handleKey(key, um, ev->text());
//  //endEditBlock();
//
//  // We fake vi-style end-of-line behaviour
//  m_fakeEnd = atEndOfLine() && m_mode == CommandMode && !isVisualBlockMode();
//
//  if (m_fakeEnd)
//    moveLeft();
//
//    EDITOR(setTextCursor(m_tc));
//  m_oldPosition = m_tc.position();
//  return result;
}

void EmacsModeHandler::Private::installEventFilter()
{
    EDITOR(installEventFilter(q));
}

void EmacsModeHandler::Private::setupWidget()
{
    enterCommandMode();
    //EDITOR(setCursorWidth(QFontMetrics(ed->font()).width(QChar('x')));
    if (m_textedit) {
        m_textedit->setLineWrapMode(QTextEdit::NoWrap);
    } else if (m_plaintextedit) {
        m_plaintextedit->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
    m_wasReadOnly = EDITOR(isReadOnly());
    //EDITOR(setReadOnly(true));

    QTextCursor tc = EDITOR(textCursor());
    if (tc.hasSelection()) {
        int pos = tc.position();
        int anc = tc.anchor();
        m_marks['<'] = anc;
        m_marks['>'] = pos;
        m_anchor = anc;
        m_visualMode = VisualCharMode;
        tc.clearSelection();
        EDITOR(setTextCursor(tc));
        m_tc = tc; // needed in updateSelection
        updateSelection();
    }

    //showBlackMessage("vi emulation mode. Type :q to leave. Use , Ctrl-R to trigger run.");
    updateMiniBuffer();
}

void EmacsModeHandler::Private::restoreWidget()
{
    //showBlackMessage(QString());
    //updateMiniBuffer();
    //EDITOR(removeEventFilter(q));
    EDITOR(setReadOnly(m_wasReadOnly));
    EDITOR(setCursorWidth(m_cursorWidth));
    EDITOR(setOverwriteMode(false));

    if (isVisualLineMode()) {
        m_tc = EDITOR(textCursor());
        int beginLine = lineForPosition(m_marks['<']);
        int endLine = lineForPosition(m_marks['>']);
        m_tc.setPosition(firstPositionInLine(beginLine), MoveAnchor);
        m_tc.setPosition(lastPositionInLine(endLine), KeepAnchor);
        EDITOR(setTextCursor(m_tc));
    } else if (isVisualCharMode()) {
        m_tc = EDITOR(textCursor());
        m_tc.setPosition(m_marks['<'], MoveAnchor);
        m_tc.setPosition(m_marks['>'], KeepAnchor);
        EDITOR(setTextCursor(m_tc));
    }

    m_visualMode = NoVisualMode;
    updateSelection();
}

EventResult EmacsModeHandler::Private::handleKey(int key, int unmodified,
    const QString &text)
{
    //qDebug() << " CURSOR POS: " << m_undoCursorPosition;
    setUndoPosition(m_tc.position());
    //qDebug() << "KEY: " << key << text << "POS: " << m_tc.position();
    if (m_mode == InsertMode)
        return handleInsertMode(key, unmodified, text);
    if (m_mode == CommandMode)
        return handleCommandMode(key, unmodified, text);
    if (m_mode == ExMode || m_mode == SearchForwardMode
            || m_mode == SearchBackwardMode)
        return handleMiniBufferModes(key, unmodified, text);
    return EventUnhandled;
}

void EmacsModeHandler::Private::setUndoPosition(int pos)
{
    //qDebug() << " CURSOR POS: " << m_undoCursorPosition;
    m_undoCursorPosition[m_tc.document()->availableUndoSteps()] = pos;
}

//void EmacsModeHandler::Private::moveDown(int n)
//{
//#if 1
//    // does not work for "hidden" documents like in the autotests
//    m_tc.movePosition(Down, m_moveMode, n);
//#else
//    const int col = m_tc.position() - m_tc.block().position();
//    const int lastLine = m_tc.document()->lastBlock().blockNumber();
//    const int targetLine = qMax(0, qMin(lastLine, m_tc.block().blockNumber() + n));
//    const QTextBlock &block = m_tc.document()->findBlockByNumber(targetLine);
//    const int pos = block.position();
//    setPosition(pos + qMin(block.length() - 1, col));
//    moveToTargetColumn();
//#endif
//}

void EmacsModeHandler::Private::moveToEndOfLine()
{
#if 1
    // does not work for "hidden" documents like in the autotests
    m_tc.movePosition(EndOfLine, m_moveMode);
#else
    const QTextBlock &block = m_tc.block();
    setPosition(block.position() + block.length() - 1);
#endif
}

void EmacsModeHandler::Private::moveBehindEndOfLine()
{
    const QTextBlock &block = m_tc.block();
    int pos = qMin(block.position() + block.length(), lastPositionInDocument());
    setPosition(pos);
}

void EmacsModeHandler::Private::moveToStartOfLine()
{
#if 1
    // does not work for "hidden" documents like in the autotests
    m_tc.movePosition(StartOfLine, m_moveMode);
#else
    const QTextBlock &block = m_tc.block();
    setPosition(block.position());
#endif
}

void EmacsModeHandler::Private::finishMovement(const QString &dotCommand)
{
    //qDebug() << "ANCHOR: " << position() << anchor();
    if (m_submode == FilterSubMode) {
        int beginLine = lineForPosition(anchor());
        int endLine = lineForPosition(position());
        setPosition(qMin(anchor(), position()));
        enterExMode();
        m_currentMessage.clear();
        m_commandBuffer = QString(".,+%1!").arg(qAbs(endLine - beginLine));
        m_commandHistory.append(QString());
        m_commandHistoryIndex = m_commandHistory.size() - 1;
        updateMiniBuffer();
        return;
    }

    if (isNoVisualMode())
        m_marks['>'] = m_tc.position();

    if (m_submode == ChangeSubMode) {
        if (m_movetype == MoveInclusive)
            moveRight(); // correction
        if (anchor() >= position())
           m_anchor++;
        if (!dotCommand.isEmpty())
            setDotCommand("c" + dotCommand);
        //QString text = removeSelectedText();
        //qDebug() << "CHANGING TO INSERT MODE" << text;
        //m_registers[m_register] = text;
        yankSelectedText();
        removeSelectedText();
        enterInsertMode();
        m_submode = NoSubMode;
    } else if (m_submode == DeleteSubMode) {
        if (m_rangemode == RangeCharMode) {
           if (m_movetype == MoveInclusive)
               moveRight(); // correction
           if (anchor() >= position())
              m_anchor++;
        }
        if (!dotCommand.isEmpty())
            setDotCommand("d" + dotCommand);
        yankSelectedText();
        removeSelectedText();
        m_submode = NoSubMode;
        if (atEndOfLine())
            moveLeft();
        else
            setTargetColumn();
    } else if (m_submode == YankSubMode) {
        yankSelectedText();
        m_submode = NoSubMode;
        if (m_register != '"') {
            setPosition(m_marks[m_register]);
            moveToStartOfLine();
        } else {
            setPosition(m_savedYankPosition);
        }
    } else if (m_submode == ReplaceSubMode) {
        m_submode = NoSubMode;
    } else if (m_submode == IndentSubMode) {
        recordJump();
        indentRegion();
        m_submode = NoSubMode;
        updateMiniBuffer();
    } else if (m_submode == ShiftRightSubMode) {
        recordJump();
        shiftRegionRight(1);
        m_submode = NoSubMode;
        updateMiniBuffer();
    } else if (m_submode == ShiftLeftSubMode) {
        recordJump();
        shiftRegionLeft(1);
        m_submode = NoSubMode;
        updateMiniBuffer();
    }

    m_movetype = MoveInclusive;
    m_mvcount.clear();
    m_opcount.clear();
    m_gflag = false;
    m_register = '"';
    m_tc.clearSelection();
    m_rangemode = RangeCharMode;

    updateSelection();
    updateMiniBuffer();
}

void EmacsModeHandler::Private::updateSelection()
{
    QList<QTextEdit::ExtraSelection> selections = m_searchSelections;
    if (isVisualMode()) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = m_tc;
        sel.format = m_tc.blockCharFormat();
#if 1
        sel.format.setFontWeight(QFont::Bold);
        sel.format.setFontUnderline(true);
#else
        sel.format.setForeground(Qt::white);
        sel.format.setBackground(Qt::black);
#endif
        int cursorPos = m_tc.position();
        int anchorPos = m_marks['<'];
        //qDebug() << "POS: " << cursorPos << " ANCHOR: " << anchorPos;
        if (isVisualCharMode()) {
            sel.cursor.setPosition(qMin(cursorPos, anchorPos), MoveAnchor);
            sel.cursor.setPosition(qMax(cursorPos, anchorPos) + 1, KeepAnchor);
            selections.append(sel);
        } else if (isVisualLineMode()) {
            sel.cursor.setPosition(qMin(cursorPos, anchorPos), MoveAnchor);
            sel.cursor.movePosition(StartOfLine, MoveAnchor);
            sel.cursor.setPosition(qMax(cursorPos, anchorPos), KeepAnchor);
            sel.cursor.movePosition(EndOfLine, KeepAnchor);
            selections.append(sel);
        } else if (isVisualBlockMode()) {
            QTextCursor tc = m_tc;
            tc.setPosition(anchorPos);
            int anchorColumn = tc.columnNumber();
            int cursorColumn = m_tc.columnNumber();
            int anchorRow    = tc.blockNumber();
            int cursorRow    = m_tc.blockNumber();
            int startColumn  = qMin(anchorColumn, cursorColumn);
            int endColumn    = qMax(anchorColumn, cursorColumn);
            int diffRow      = cursorRow - anchorRow;
            if (anchorRow > cursorRow) {
                tc.setPosition(cursorPos);
                diffRow = -diffRow;
            }
            tc.movePosition(StartOfLine, MoveAnchor);
            for (int i = 0; i <= diffRow; ++i) {
                if (startColumn < tc.block().length() - 1) {
                    int last = qMin(tc.block().length(), endColumn + 1);
                    int len = last - startColumn;
                    sel.cursor = tc;
                    sel.cursor.movePosition(Right, MoveAnchor, startColumn);
                    sel.cursor.movePosition(Right, KeepAnchor, len);
                    selections.append(sel);
                }
                tc.movePosition(Down, MoveAnchor, 1);
            }
        }
    }
    //qDebug() << "SELECTION: " << selections;
    emit q->selectionChanged(selections);
}

void EmacsModeHandler::Private::updateMiniBuffer()
{
    QString msg;
    if (m_passing) {
        msg = "-- PASSING --  ";
    } else if (!m_currentMessage.isEmpty()) {
        msg = m_currentMessage;
    } else if (m_mode == CommandMode && isVisualMode()) {
        if (isVisualCharMode()) {
            msg = "-- VISUAL --";
        } else if (isVisualLineMode()) {
            msg = "-- VISUAL LINE --";
        } else if (isVisualBlockMode()) {
            msg = "-- VISUAL BLOCK --";
        }
    } else if (m_mode == InsertMode) {
        if (m_submode == ReplaceSubMode)
            msg = "-- REPLACE --";
        else
            msg = "-- INSERT --";
    } else {
        if (m_mode == SearchForwardMode)
            msg += '/';
        else if (m_mode == SearchBackwardMode)
            msg += '?';
        else if (m_mode == ExMode)
            msg += ':';
        foreach (QChar c, m_commandBuffer) {
            if (c.unicode() < 32) {
                msg += '^';
                msg += QChar(c.unicode() + 64);
            } else {
                msg += c;
            }
        }
        if (!msg.isEmpty() && m_mode != CommandMode)
            msg += QChar(10073); // '|'; // FIXME: Use a real "cursor"
    }

    emit q->commandBufferChanged(msg);

    int linesInDoc = linesInDocument();
    int l = cursorLineInDocument();
    QString status;
    const QString pos = QString::fromLatin1("%1,%2").arg(l + 1).arg(cursorColumnInDocument() + 1);
    // FIXME: physical "-" logical
    if (linesInDoc != 0) {
        status = EmacsModeHandler::tr("%1%2%").arg(pos, -10).arg(l * 100 / linesInDoc, 4);
    } else {
        status = EmacsModeHandler::tr("%1All").arg(pos, -10);
    }
    emit q->statusDataChanged(status);
}

void EmacsModeHandler::Private::showRedMessage(const QString &msg)
{
    //qDebug() << "MSG: " << msg;
    m_currentMessage = msg;
    updateMiniBuffer();
}

void EmacsModeHandler::Private::showBlackMessage(const QString &msg)
{
    //qDebug() << "MSG: " << msg;
    m_commandBuffer = msg;
    updateMiniBuffer();
}

void EmacsModeHandler::Private::notImplementedYet()
{
    qDebug() << "Not implemented in EmacsMode";
    showRedMessage(EmacsModeHandler::tr("Not implemented in EmacsMode"));
    updateMiniBuffer();
}

EventResult EmacsModeHandler::Private::handleCommandMode(int key, int unmodified,
    const QString &text)
{
    EventResult handled = EventHandled;

    if (m_submode == WindowSubMode) {
        emit q->windowCommandRequested(key);
        m_submode = NoSubMode;
    } else if (m_submode == RegisterSubMode) {
        m_register = key;
        m_submode = NoSubMode;
        m_rangemode = RangeLineMode;
    } else if (m_submode == ChangeSubMode && key == 'c') { // tested
        moveDown(count() - 1);
        moveToEndOfLine();
        moveLeft();
        setAnchor();
        moveToStartOfLine();
        setTargetColumn();
        moveUp(count() - 1);
        m_movetype = MoveLineWise;
        m_lastInsertion.clear();
        setDotCommand("%1cc", count());
        finishMovement();
    } else if (m_submode == DeleteSubMode && key == 'd') { // tested
        m_movetype = MoveLineWise;
        int endPos = firstPositionInLine(lineForPosition(position()) + count() - 1);
        Range range(position(), endPos, RangeLineMode);
        yankText(range);
        removeText(range);
        setDotCommand("%1dd", count());
        m_submode = NoSubMode;
        moveToFirstNonBlankOnLine();
        setTargetColumn();
        finishMovement();
    } else if (m_submode == ShiftLeftSubMode && key == '<') {
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1<<", count());
        finishMovement();
    } else if (m_submode == ShiftRightSubMode && key == '>') {
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1>>", count());
        finishMovement();
    } else if (m_submode == IndentSubMode && key == '=') {
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1==", count());
        finishMovement();
    } else if (m_submode == ZSubMode) {
        //qDebug() << "Z_MODE " << cursorLineInDocument() << linesOnScreen();
        if (key == Key_Return || key == 't') { // cursor line to top of window
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(- cursorLineOnScreen());
            if (key == Key_Return)
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else if (key == '.' || key == 'z') { // cursor line to center of window
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(linesOnScreen() / 2 - cursorLineOnScreen());
            if (key == '.')
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else if (key == '-' || key == 'b') { // cursor line to bottom of window
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(linesOnScreen() - cursorLineOnScreen());
            if (key == '-')
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            qDebug() << "IGNORED Z_MODE " << key << text;
        }
        m_submode = NoSubMode;
    } else if (m_submode == CapitalZSubMode) {
        // Recognize ZZ and ZQ as aliases for ":x" and ":q!".
        m_submode = NoSubMode;
        if (key == 'Z')
            handleCommand("x");
        else if (key == 'Q')
            handleCommand("q!");
    } else if (m_subsubmode == FtSubSubMode) {
        m_semicolonType = m_subsubdata;
        m_semicolonKey = key;
        handleFfTt(key);
        m_subsubmode = NoSubSubMode;
        finishMovement(QString("%1%2%3")
            .arg(count())
            .arg(QChar(m_semicolonType))
            .arg(QChar(m_semicolonKey)));
    } else if (m_submode == ReplaceSubMode) {
        if (count() <= (rightDist() + atEndOfLine()) && text.size() == 1
                && (text.at(0).isPrint() || text.at(0).isSpace())) {
            if (atEndOfLine())
                moveLeft();
            setAnchor();
            moveRight(count());
            removeSelectedText();
            m_tc.insertText(QString(count(), text.at(0)));
            m_movetype = MoveExclusive;
            setDotCommand("%1r" + text, count());
        }
        setTargetColumn();
        m_submode = NoSubMode;
        finishMovement();
    } else if (m_subsubmode == MarkSubSubMode) {
        m_marks[key] = m_tc.position();
        m_subsubmode = NoSubSubMode;
    } else if (m_subsubmode == BackTickSubSubMode
            || m_subsubmode == TickSubSubMode) {
        if (m_marks.contains(key)) {
            setPosition(m_marks[key]);
            if (m_subsubmode == TickSubSubMode)
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            showRedMessage(msgE20MarkNotSet(text));
        }
        m_subsubmode = NoSubSubMode;
    } else if (key >= '0' && key <= '9') {
        if (key == '0' && m_mvcount.isEmpty()) {
            moveToStartOfLine();
            setTargetColumn();
            finishMovement();
        } else {
            m_mvcount.append(QChar(key));
        }
    } else if (key == '^') {
        moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (0 && key == ',') {
        // FIXME: EmacsMode uses ',' by itself, so it is incompatible
        m_subsubmode = FtSubSubMode;
        // HACK: toggle 'f' <-> 'F', 't' <-> 'T'
        m_subsubdata = m_semicolonType ^ 32;
        handleFfTt(m_semicolonKey);
        m_subsubmode = NoSubSubMode;
        finishMovement();
    } else if (key == ';') {
        m_subsubmode = FtSubSubMode;
        m_subsubdata = m_semicolonType;
        handleFfTt(m_semicolonKey);
        m_subsubmode = NoSubSubMode;
        finishMovement();
    } else if (key == ':') {
        enterExMode();
        m_currentMessage.clear();
        m_commandBuffer.clear();
        if (isVisualMode())
            m_commandBuffer = "'<,'>";
        m_commandHistory.append(QString());
        m_commandHistoryIndex = m_commandHistory.size() - 1;
        updateMiniBuffer();
    } else if (key == '/' || key == '?') {
        if (hasConfig(ConfigIncSearch)) {
            // re-use the core dialog.
            emit q->findRequested(key == '?');
        } else {
            // FIXME: make core find dialog sufficiently flexible to
            // produce the "default vi" behaviour too. For now, roll our own.
            enterExMode(); // to get the cursor disabled
            m_currentMessage.clear();
            m_mode = (key == '/') ? SearchForwardMode : SearchBackwardMode;
            m_commandBuffer.clear();
            m_searchHistory.append(QString());
            m_searchHistoryIndex = m_searchHistory.size() - 1;
            updateMiniBuffer();
        }
    } else if (key == '`') {
        m_subsubmode = BackTickSubSubMode;
    } else if (key == '#' || key == '*') {
        // FIXME: That's not proper vim behaviour
        m_tc.select(QTextCursor::WordUnderCursor);
        QString needle = "\\<" + m_tc.selection().toPlainText() + "\\>";
        m_searchHistory.append(needle);
        m_lastSearchForward = (key == '*');
        updateMiniBuffer();
        search(needle, m_lastSearchForward);
        recordJump();
    } else if (key == '\'') {
        m_subsubmode = TickSubSubMode;
    } else if (key == '|') {
        moveToStartOfLine();
        moveRight(qMin(count(), rightDist()) - 1);
        setTargetColumn();
        finishMovement();
    } else if (key == '!' && isNoVisualMode()) {
        m_submode = FilterSubMode;
    } else if (key == '!' && isVisualMode()) {
        enterExMode();
        m_currentMessage.clear();
        m_commandBuffer = "'<,'>!";
        m_commandHistory.append(QString());
        m_commandHistoryIndex = m_commandHistory.size() - 1;
        updateMiniBuffer();
    } else if (key == '"') {
        m_submode = RegisterSubMode;
    } else if (unmodified == Key_Return) {
        moveToStartOfLine();
        moveDown();
        moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (key == '-') {
        moveToStartOfLine();
        moveUp();
        moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (key == Key_Home) {
        moveToStartOfLine();
        setTargetColumn();
        finishMovement();
    } else if (key == '$' || key == Key_End) {
        int submode = m_submode;
        moveToEndOfLine();
        m_movetype = MoveExclusive;
        setTargetColumn();
        if (submode == NoSubMode)
            m_targetColumn = -1;
        finishMovement("$");
    } else if (key == ',') {
        // FIXME: use some other mechanism
        //m_passing = true;
        m_passing = !m_passing;
        updateMiniBuffer();
    } else if (key == '.') {
        //qDebug() << "REPEATING" << quoteUnprintable(m_dotCommand) << count();
        QString savedCommand = m_dotCommand;
        m_dotCommand.clear();
        replay(savedCommand, count());
        enterCommandMode();
        m_dotCommand = savedCommand;
    } else if (key == '<' && isNoVisualMode()) {
        m_submode = ShiftLeftSubMode;
    } else if (key == '<' && isVisualMode()) {
        shiftRegionLeft(1);
        leaveVisualMode();
    } else if (key == '>' && isNoVisualMode()) {
        m_submode = ShiftRightSubMode;
    } else if (key == '>' && isVisualMode()) {
        shiftRegionRight(1);
        leaveVisualMode();
    } else if (key == '=' && isNoVisualMode()) {
        m_submode = IndentSubMode;
    } else if (key == '=' && isVisualMode()) {
        indentRegion();
        leaveVisualMode();
    } else if (key == '%') {
        m_movetype = MoveExclusive;
        moveToMatchingParanthesis();
        finishMovement();
    } else if (key == 'a') {
        enterInsertMode();
        m_lastInsertion.clear();
        if (!atEndOfLine())
            moveRight();
        updateMiniBuffer();
    } else if (key == 'A') {
        enterInsertMode();
        moveToEndOfLine();
        setDotCommand("A");
        m_lastInsertion.clear();
    } else if (key == control('a')) {
        // FIXME: eat it to prevent the global "select all" shortcut to trigger
    } else if (key == 'b') {
        m_movetype = MoveExclusive;
        moveToWordBoundary(false, false);
        finishMovement();
    } else if (key == 'B') {
        m_movetype = MoveExclusive;
        moveToWordBoundary(true, false);
        finishMovement();
    } else if (key == 'c' && isNoVisualMode()) {
        setAnchor();
        m_submode = ChangeSubMode;
    } else if (key == 'c' && isVisualCharMode()) {
        leaveVisualMode();
        m_submode = ChangeSubMode;
        finishMovement();
    } else if (key == 'C') {
        setAnchor();
        moveToEndOfLine();
        yankSelectedText();
        removeSelectedText();
        enterInsertMode();
        setDotCommand("C");
        finishMovement();
    } else if (key == control('c')) {
        showBlackMessage("Type Alt-v,Alt-v  to quit EmacsMode mode");
    } else if (key == 'd' && isNoVisualMode()) {
        if (m_rangemode == RangeLineMode) {
            m_savedYankPosition = m_tc.position();
            moveToEndOfLine();
            setAnchor();
            setPosition(m_savedYankPosition);
        } else {
            if (atEndOfLine())
                moveLeft();
            setAnchor();
        }
        m_opcount = m_mvcount;
        m_mvcount.clear();
        m_submode = DeleteSubMode;
    } else if ((key == 'd' || key == 'x') && isVisualCharMode()) {
        leaveVisualMode();
        m_submode = DeleteSubMode;
        finishMovement();
    } else if ((key == 'd' || key == 'x') && isVisualLineMode()) {
        leaveVisualMode();
        m_rangemode = RangeLineMode;
        yankSelectedText();
        removeSelectedText();
        moveToFirstNonBlankOnLine();
    } else if ((key == 'd' || key == 'x') && isVisualBlockMode()) {
        leaveVisualMode();
        m_rangemode = RangeBlockMode;
        yankSelectedText();
        removeSelectedText();
        setPosition(qMin(position(), anchor()));
    } else if (key == 'D' && isNoVisualMode()) {
        setAnchor();
        m_submode = DeleteSubMode;
        moveDown(qMax(count() - 1, 0));
        m_movetype = MoveExclusive;
        moveToEndOfLine();
        setDotCommand("D");
        finishMovement();
    } else if ((key == 'D' || key == 'X') &&
         (isVisualCharMode() || isVisualLineMode())) {
        leaveVisualMode();
        m_rangemode = RangeLineMode;
        m_submode = NoSubMode;
        yankSelectedText();
        removeSelectedText();
        moveToFirstNonBlankOnLine();
    } else if ((key == 'D' || key == 'X') && isVisualBlockMode()) {
        leaveVisualMode();
        m_rangemode = RangeBlockAndTailMode;
        yankSelectedText();
        removeSelectedText();
        setPosition(qMin(position(), anchor()));
    } else if (key == control('d')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveDown(linesOnScreen() / 2);
        handleStartOfLine();
        scrollToLineInDocument(cursorLineInDocument() - sline);
        finishMovement();
    } else if (key == 'e') { // tested
        m_movetype = MoveInclusive;
        moveToWordBoundary(false, true);
        finishMovement("e");
    } else if (key == 'E') {
        m_movetype = MoveInclusive;
        moveToWordBoundary(true, true);
        finishMovement();
    } else if (key == control('e')) {
        // FIXME: this should use the "scroll" option, and "count"
        if (cursorLineOnScreen() == 0)
            moveDown(1);
        scrollDown(1);
        finishMovement();
    } else if (key == 'f') {
        m_subsubmode = FtSubSubMode;
        m_movetype = MoveInclusive;
        m_subsubdata = key;
    } else if (key == 'F') {
        m_subsubmode = FtSubSubMode;
        m_movetype = MoveExclusive;
        m_subsubdata = key;
    } else if (key == 'g') {
        if (m_gflag) {
            m_gflag = false;
            m_tc.setPosition(firstPositionInLine(1), KeepAnchor);
            handleStartOfLine();
            finishMovement();
        } else {
            m_gflag = true;
        }
    } else if (key == 'G') {
        int n = m_mvcount.isEmpty() ? linesInDocument() : count();
        m_tc.setPosition(firstPositionInLine(n), KeepAnchor);
        handleStartOfLine();
        finishMovement();
    } else if (key == 'h' || key == Key_Left
            || key == Key_Backspace || key == control('h')) {
        int n = qMin(count(), leftDist());
        if (m_fakeEnd && m_tc.block().length() > 1)
            ++n;
        moveLeft(n);
        setTargetColumn();
        finishMovement("h");
    } else if (key == 'H') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, 0)));
        moveDown(qMax(count() - 1, 0));
        handleStartOfLine();
        finishMovement();
    } else if (key == 'i' || key == Key_Insert) {
        setDotCommand("i"); // setDotCommand("%1i", count());
        enterInsertMode();
        updateMiniBuffer();
        if (atEndOfLine())
            moveLeft();
    } else if (key == 'I') {
        setDotCommand("I"); // setDotCommand("%1I", count());
        enterInsertMode();
        if (m_gflag)
            moveToStartOfLine();
        else
            moveToFirstNonBlankOnLine();
        m_tc.clearSelection();
    } else if (key == control('i')) {
        if (!m_jumpListRedo.isEmpty()) {
            m_jumpListUndo.append(cursorPosition());
            setCursorPosition(m_jumpListRedo.last());
            m_jumpListRedo.pop_back();
        }
    } else if (key == 'j' || key == Key_Down) {
        if (m_submode == NoSubMode || m_submode == ZSubMode
                || m_submode == CapitalZSubMode || m_submode == RegisterSubMode) {
            moveDown(count());
        } else {
            m_movetype = MoveLineWise;
            moveToStartOfLine();
            setAnchor();
            moveDown(count() + 1);
        }
        finishMovement("j");
    } else if (key == 'J') {
        setDotCommand("%1J", count());
        beginEditBlock();
        if (m_submode == NoSubMode) {
            for (int i = qMax(count(), 2) - 1; --i >= 0; ) {
                moveToEndOfLine();
                setAnchor();
                moveRight();
                if (m_gflag) {
                    removeSelectedText();
                } else {
                    while (characterAtCursor() == ' '
                        || characterAtCursor() == '\t')
                        moveRight();
                    removeSelectedText();
                    m_tc.insertText(" ");
                }
            }
            if (!m_gflag)
                moveLeft();
        }
        endEditBlock();
        finishMovement();
    } else if (key == 'k' || key == Key_Up) {
        if (m_submode == NoSubMode || m_submode == ZSubMode
                || m_submode == CapitalZSubMode || m_submode == RegisterSubMode) {
            moveUp(count());
        } else {
            m_movetype = MoveLineWise;
            moveToStartOfLine();
            moveDown();
            setAnchor();
            moveUp(count() + 1);
        }
        finishMovement("k");
    } else if (key == 'l' || key == Key_Right || key == ' ') {
        m_movetype = MoveExclusive;
        moveRight(qMin(count(), rightDist()));
        setTargetColumn();
        finishMovement("l");
    } else if (key == 'L') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()))));
        moveUp(qMax(count(), 1));
        handleStartOfLine();
        finishMovement();
    } else if (key == control('l')) {
        // screen redraw. should not be needed
    } else if (key == 'm') {
        m_subsubmode = MarkSubSubMode;
    } else if (key == 'M') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()) / 2)));
        handleStartOfLine();
        finishMovement();
    } else if (key == 'n') { // FIXME: see comment for '/'
        if (hasConfig(ConfigIncSearch))
            emit q->findNextRequested(false);
        else
            search(lastSearchString(), m_lastSearchForward);
        recordJump();
    } else if (key == 'N') {
        if (hasConfig(ConfigIncSearch))
            emit q->findNextRequested(true);
        else
            search(lastSearchString(), !m_lastSearchForward);
        recordJump();
    } else if (key == 'o' || key == 'O') {
        beginEditBlock();
        setDotCommand("%1o", count());
        enterInsertMode();
        moveToFirstNonBlankOnLine();
        if (key == 'O')
            moveUp();
        moveToEndOfLine();
        m_tc.insertText("\n");
        insertAutomaticIndentation(key == 'o');
        endEditBlock();
    } else if (key == control('o')) {
        if (!m_jumpListUndo.isEmpty()) {
            m_jumpListRedo.append(cursorPosition());
            setCursorPosition(m_jumpListUndo.last());
            m_jumpListUndo.pop_back();
        }
    } else if (key == 'p' || key == 'P') {
        pasteText(key == 'p');
        setTargetColumn();
        setDotCommand("%1p", count());
        finishMovement();
    } else if (key == 'r') {
        m_submode = ReplaceSubMode;
        setDotCommand("r");
    } else if (key == 'R') {
        // FIXME: right now we repeat the insertion count() times,
        // but not the deletion
        m_lastInsertion.clear();
        enterInsertMode();
        m_submode = ReplaceSubMode;
        setDotCommand("R");
    } else if (key == control('r')) {
        redo();
    } else if (key == 's') {
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        moveRight(qMin(count(), rightDist()));
        yankSelectedText();
        removeSelectedText();
        setDotCommand("%1s", count());
        m_opcount.clear();
        m_mvcount.clear();
        enterInsertMode();
    } else if (key == 't') {
        m_movetype = MoveInclusive;
        m_subsubmode = FtSubSubMode;
        m_subsubdata = key;
    } else if (key == 'T') {
        m_movetype = MoveExclusive;
        m_subsubmode = FtSubSubMode;
        m_subsubdata = key;
    } else if (key == 'u') {
        undo();
    } else if (key == control('u')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveUp(linesOnScreen() / 2);
        handleStartOfLine();
        scrollToLineInDocument(cursorLineInDocument() - sline);
        finishMovement();
    } else if (key == 'v') {
        enterVisualMode(VisualCharMode);
    } else if (key == 'V') {
        enterVisualMode(VisualLineMode);
    } else if (key == control('v')) {
        enterVisualMode(VisualBlockMode);
    } else if (key == 'w') { // tested
        // Special case: "cw" and "cW" work the same as "ce" and "cE" if the
        // cursor is on a non-blank.
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(false, true);
            m_movetype = MoveInclusive;
        } else {
            moveToNextWord(false);
            m_movetype = MoveExclusive;
        }
        finishMovement("w");
    } else if (key == 'W') {
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(true, true);
            m_movetype = MoveInclusive;
        } else {
            moveToNextWord(true);
            m_movetype = MoveExclusive;
        }
        finishMovement("W");
    } else if (key == control('w')) {
        m_submode = WindowSubMode;
    } else if (key == 'x' && isNoVisualMode()) { // = "dl"
        m_movetype = MoveExclusive;
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        m_submode = DeleteSubMode;
        moveRight(qMin(count(), rightDist()));
        setDotCommand("%1x", count());
        finishMovement();
    } else if (key == 'X') {
        if (leftDist() > 0) {
            setAnchor();
            moveLeft(qMin(count(), leftDist()));
            yankSelectedText();
            removeSelectedText();
        }
        finishMovement();
    } else if ((m_submode == YankSubMode && key == 'y')
            || (key == 'Y' && isNoVisualMode()))  {
        const int line = cursorLineInDocument() + 1;
        m_savedYankPosition = position();
        setAnchor(firstPositionInLine(line));
        setPosition(lastPositionInLine(line+count() - 1));
        if (count() > 1)
            showBlackMessage(QString("%1 lines yanked").arg(count()));
        m_rangemode = RangeLineMode;
        m_movetype = MoveLineWise;
        m_submode = YankSubMode;
        finishMovement();
    } else if (key == 'y' && isNoVisualMode()) {
        if (m_rangemode == RangeLineMode) {
            m_savedYankPosition = position();
            setAnchor(firstPositionInLine(cursorLineInDocument() + 1));
        } else {
            m_savedYankPosition = position();
            if (atEndOfLine())
                moveLeft();
            setAnchor();
            m_rangemode = RangeCharMode;
        }
        m_submode = YankSubMode;
    } else if (key == 'y' && isVisualCharMode()) {
        Range range(position(), anchor(), RangeCharMode);
        range.endPos++; // MoveInclusive
        yankText(range, m_register);
        setPosition(qMin(position(), anchor()));
        leaveVisualMode();
        finishMovement();
    } else if ((key == 'y' && isVisualLineMode())
            || (key == 'Y' && isVisualLineMode())
            || (key == 'Y' && isVisualCharMode())) {
        m_rangemode = RangeLineMode;
        yankSelectedText();
        setPosition(qMin(position(), anchor()));
        moveToStartOfLine();
        leaveVisualMode();
        finishMovement();
    } else if ((key == 'y' || key == 'Y') && isVisualBlockMode()) {
        m_rangemode = RangeBlockMode;
        yankSelectedText();
        setPosition(qMin(position(), anchor()));
        leaveVisualMode();
        finishMovement();
    } else if (key == 'z') {
        m_submode = ZSubMode;
    } else if (key == 'Z') {
        m_submode = CapitalZSubMode;
    } else if (key == '~' && !atEndOfLine()) {
        beginEditBlock();
        setAnchor();
        moveRight(qMin(count(), rightDist()));
        QString str = selectedText();
        removeSelectedText();
        for (int i = str.size(); --i >= 0; ) {
            QChar c = str.at(i);
            str[i] = c.isUpper() ? c.toLower() : c.toUpper();
        }
        m_tc.insertText(str);
        endEditBlock();
    } else if (key == Key_PageDown || key == control('f')) {
        moveDown(count() * (linesOnScreen() - 2) - cursorLineOnScreen());
        scrollToLineInDocument(cursorLineInDocument());
        handleStartOfLine();
        finishMovement();
    } else if (key == Key_PageUp || key == control('b')) {
        moveUp(count() * (linesOnScreen() - 2) + cursorLineOnScreen());
        scrollToLineInDocument(cursorLineInDocument() + linesOnScreen() - 2);
        handleStartOfLine();
        finishMovement();
    } else if (key == Key_Delete) {
        setAnchor();
        moveRight(qMin(1, rightDist()));
        removeSelectedText();
    } else if (key == Key_BracketLeft || key == Key_BracketRight) {

    } else if (key == Key_Escape) {
        if (isVisualMode()) {
            leaveVisualMode();
        } else if (m_submode != NoSubMode) {
            m_submode = NoSubMode;
            m_subsubmode = NoSubSubMode;
            finishMovement();
        }
    } else {
        qDebug() << "IGNORED IN COMMAND MODE: " << key << text
            << " VISUAL: " << m_visualMode;
        handled = EventUnhandled;
    }

    return handled;
}

EventResult EmacsModeHandler::Private::handleInsertMode(int key, int,
    const QString &text)
{
    if (key == Key_Escape || key == 27 || key == control('c') ||
			key == 379 /* ^[ */) {
        // start with '1', as one instance was already physically inserted
        // while typing
        QString data = m_lastInsertion;
        for (int i = 1; i < count(); ++i) {
            m_tc.insertText(m_lastInsertion);
            data += m_lastInsertion;
        }
        moveLeft(qMin(1, leftDist()));
        setTargetColumn();
        m_dotCommand += m_lastInsertion;
        m_dotCommand += QChar(27);
        recordNewUndo();
        enterCommandMode();
    } else if (key == Key_Insert) {
        if (m_submode == ReplaceSubMode) {
            EDITOR(setCursorWidth(m_cursorWidth));
            EDITOR(setOverwriteMode(false));
            m_submode = NoSubMode;
        } else {
            EDITOR(setCursorWidth(m_cursorWidth));
            EDITOR(setOverwriteMode(true));
            m_submode = ReplaceSubMode;
        }
    } else if (key == Key_Left) {
        moveLeft(count());
        setTargetColumn();
        m_lastInsertion.clear();
    } else if (key == Key_Down) {
        //removeAutomaticIndentation();
        m_submode = NoSubMode;
        moveDown(count());
        m_lastInsertion.clear();
    } else if (key == Key_Up) {
        //removeAutomaticIndentation();
        m_submode = NoSubMode;
        moveUp(count());
        m_lastInsertion.clear();
    } else if (key == Key_Right) {
        moveRight(count());
        setTargetColumn();
        m_lastInsertion.clear();
    } else if (key == Key_Return) {
        m_submode = NoSubMode;
        m_tc.insertBlock();
        m_lastInsertion += "\n";
        insertAutomaticIndentation(true);
        setTargetColumn();
    } else if (key == Key_Backspace || key == control('h')) {
        if (!removeAutomaticIndentation()
            && (!m_lastInsertion.isEmpty()
                || hasConfig(ConfigBackspace, "start")))
            {
                int line = cursorLineInDocument() + 1;
                int col = cursorColumnInDocument();
                QString data = lineContents(line);
                Indentation ind = indentation(data);
                if (col <= ind.logical) {
                    int ts = config(ConfigTabStop).toInt();
                    int newcol = col - 1 - (col - 1) % ts;
                    data = tabExpand(newcol) + data.mid(col);
                    setLineContents(line, data);
                    m_lastInsertion.clear(); // FIXME
                } else {
                    m_tc.deletePreviousChar();
                    m_lastInsertion.chop(1);
                }
                setTargetColumn();
            }
    } else if (key == Key_Delete) {
        m_tc.deleteChar();
        m_lastInsertion.clear();
    } else if (key == Key_PageDown || key == control('f')) {
        removeAutomaticIndentation();
        moveDown(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (key == Key_PageUp || key == control('b')) {
        removeAutomaticIndentation();
        moveUp(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (key == Key_Tab && hasConfig(ConfigExpandTab)) {
        int ts = config(ConfigTabStop).toInt();
        int col = cursorColumnInDocument();
        QString str = QString(ts - col % ts, ' ');
        m_lastInsertion.append(str);
        m_tc.insertText(str);
        setTargetColumn();
    } else if (key >= control('a') && key <= control('z')) {
        // ignore these
    } else if (!text.isEmpty()) {
        m_justAutoIndented = false;
        m_lastInsertion.append(text);
        if (m_submode == ReplaceSubMode) {
            if (atEndOfLine())
                m_submode = NoSubMode;
            else
                m_tc.deleteChar();
        }
        m_tc.insertText(text);
        if (0 && hasConfig(ConfigAutoIndent) && isElectricCharacter(text.at(0))) {
            const QString leftText = m_tc.block().text()
                .left(m_tc.position() - 1 - m_tc.block().position());
            if (leftText.simplified().isEmpty())
                indentRegion(text.at(0));
        }

        if (!m_inReplay)
            emit q->completionRequested();
        setTargetColumn();
    } else {
        return EventUnhandled;
    }
    updateMiniBuffer();
    return EventHandled;
}

EventResult EmacsModeHandler::Private::handleMiniBufferModes(int key, int unmodified,
    const QString &text)
{
    Q_UNUSED(text)

    if (key == Key_Escape || key == control('c')) {
        m_commandBuffer.clear();
        enterCommandMode();
        updateMiniBuffer();
    } else if (key == Key_Backspace) {
        if (m_commandBuffer.isEmpty()) {
            enterCommandMode();
        } else {
            m_commandBuffer.chop(1);
        }
        updateMiniBuffer();
    } else if (key == Key_Left) {
        // FIXME:
        if (!m_commandBuffer.isEmpty())
            m_commandBuffer.chop(1);
        updateMiniBuffer();
    } else if (unmodified == Key_Return && m_mode == ExMode) {
        if (!m_commandBuffer.isEmpty()) {
            m_commandHistory.takeLast();
            m_commandHistory.append(m_commandBuffer);
            handleExCommand(m_commandBuffer);
            leaveVisualMode();
        }
    } else if (unmodified == Key_Return && isSearchMode()) {
        if (!m_commandBuffer.isEmpty()) {
            m_searchHistory.takeLast();
            m_searchHistory.append(m_commandBuffer);
            m_lastSearchForward = (m_mode == SearchForwardMode);
            search(lastSearchString(), m_lastSearchForward);
            recordJump();
        }
        enterCommandMode();
        updateMiniBuffer();
    } else if ((key == Key_Up || key == Key_PageUp) && isSearchMode()) {
        // FIXME: This and the three cases below are wrong as vim
        // takes only matching entries in the history into account.
        if (m_searchHistoryIndex > 0) {
            --m_searchHistoryIndex;
            showBlackMessage(m_searchHistory.at(m_searchHistoryIndex));
        }
    } else if ((key == Key_Up || key == Key_PageUp) && m_mode == ExMode) {
        if (m_commandHistoryIndex > 0) {
            --m_commandHistoryIndex;
            showBlackMessage(m_commandHistory.at(m_commandHistoryIndex));
        }
    } else if ((key == Key_Down || key == Key_PageDown) && isSearchMode()) {
        if (m_searchHistoryIndex < m_searchHistory.size() - 1) {
            ++m_searchHistoryIndex;
            showBlackMessage(m_searchHistory.at(m_searchHistoryIndex));
        }
    } else if ((key == Key_Down || key == Key_PageDown) && m_mode == ExMode) {
        if (m_commandHistoryIndex < m_commandHistory.size() - 1) {
            ++m_commandHistoryIndex;
            showBlackMessage(m_commandHistory.at(m_commandHistoryIndex));
        }
    } else if (key == Key_Tab) {
        m_commandBuffer += QChar(9);
        updateMiniBuffer();
    } else if (QChar(key).isPrint()) {
        m_commandBuffer += QChar(key);
        updateMiniBuffer();
    } else {
        qDebug() << "IGNORED IN MINIBUFFER MODE: " << key << text;
        return EventUnhandled;
    }
    return EventHandled;
}

// 1 based.
int EmacsModeHandler::Private::readLineCode(QString &cmd)
{
    //qDebug() << "CMD: " << cmd;
    if (cmd.isEmpty())
        return -1;
    QChar c = cmd.at(0);
    cmd = cmd.mid(1);
    if (c == '.')
        return cursorLineInDocument() + 1;
    if (c == '$')
        return linesInDocument();
    if (c == '\'' && !cmd.isEmpty()) {
        int mark = m_marks.value(cmd.at(0).unicode());
        if (!mark) {
            showRedMessage(msgE20MarkNotSet(cmd.at(0)));
            cmd = cmd.mid(1);
            return -1;
        }
        cmd = cmd.mid(1);
        return lineForPosition(mark);
    }
    if (c == '-') {
        int n = readLineCode(cmd);
        return cursorLineInDocument() + 1 - (n == -1 ? 1 : n);
    }
    if (c == '+') {
        int n = readLineCode(cmd);
        return cursorLineInDocument() + 1 + (n == -1 ? 1 : n);
    }
    if (c == '\'' && !cmd.isEmpty()) {
        int pos = m_marks.value(cmd.at(0).unicode(), -1);
        //qDebug() << " MARK: " << cmd.at(0) << pos << lineForPosition(pos);
        if (pos == -1) {
            showRedMessage(msgE20MarkNotSet(cmd.at(0)));
            cmd = cmd.mid(1);
            return -1;
        }
        cmd = cmd.mid(1);
        return lineForPosition(pos);
    }
    if (c.isDigit()) {
        int n = c.unicode() - '0';
        while (!cmd.isEmpty()) {
            c = cmd.at(0);
            if (!c.isDigit())
                break;
            cmd = cmd.mid(1);
            n = n * 10 + (c.unicode() - '0');
        }
        //qDebug() << "N: " << n;
        return n;
    }
    // not parsed
    cmd = c + cmd;
    return -1;
}

void EmacsModeHandler::Private::selectRange(int beginLine, int endLine)
{
    if (beginLine == -1)
        beginLine = cursorLineInDocument();
    if (endLine == -1)
        endLine = cursorLineInDocument();
    if (beginLine > endLine)
        qSwap(beginLine, endLine);
    setAnchor(firstPositionInLine(beginLine));
    if (endLine == linesInDocument())
       setPosition(lastPositionInLine(endLine));
    else
       setPosition(firstPositionInLine(endLine + 1));
}

void EmacsModeHandler::Private::handleCommand(const QString &cmd)
{
    m_tc = EDITOR(textCursor());
    handleExCommand(cmd);
    EDITOR(setTextCursor(m_tc));
}

// result: (needle, replacement, opions)
static bool isSubstitution(const QString &cmd0, QStringList *result)
{
    QString cmd;
    if (cmd0.startsWith("substitute"))
        cmd = cmd0.mid(10);
    else if (cmd0.startsWith('s') && cmd0.size() > 1
            && !isalpha(cmd0.at(1).unicode()))
        cmd = cmd0.mid(1);
    else
        return false;
    // we have /{pattern}/{string}/[flags]  now
    if (cmd.isEmpty())
        return false;
    const QChar separator = cmd.at(0);
    int pos1 = -1;
    int pos2 = -1;
    int i;
    for (i = 1; i < cmd.size(); ++i) {
        if (cmd.at(i) == separator && cmd.at(i - 1) != '\\') {
            pos1 = i;
            break;
        }
    }
    if (pos1 == -1)
        return false;
    for (++i; i < cmd.size(); ++i) {
        if (cmd.at(i) == separator && cmd.at(i - 1) != '\\') {
            pos2 = i;
            break;
        }
    }
    if (pos2 == -1)
        pos2 = cmd.size();

    result->append(cmd.mid(1, pos1 - 1));
    result->append(cmd.mid(pos1 + 1, pos2 - pos1 - 1));
    result->append(cmd.mid(pos2 + 1));
    return true;
}

void EmacsModeHandler::Private::handleExCommand(const QString &cmd0)
{
    QString cmd = cmd0;
    // FIXME: that seems to be different for %w and %s
    if (cmd.startsWith(QLatin1Char('%')))
        cmd = "1,$" + cmd.mid(1);

    int beginLine = -1;
    int endLine = -1;

    int line = readLineCode(cmd);
    if (line != -1)
        beginLine = line;

    if (cmd.startsWith(',')) {
        cmd = cmd.mid(1);
        line = readLineCode(cmd);
        if (line != -1)
            endLine = line;
    }

    //qDebug() << "RANGE: " << beginLine << endLine << cmd << cmd0 << m_marks;

    static QRegExp reQuit("^qa?!?$");
    static QRegExp reDelete("^d( (.*))?$");
    static QRegExp reHistory("^his(tory)?( (.*))?$");
    static QRegExp reNormal("^norm(al)?( (.*))?$");
    static QRegExp reSet("^set?( (.*))?$");
    static QRegExp reWrite("^[wx]q?a?!?( (.*))?$");
    QStringList arguments;

    enterCommandMode();
    showBlackMessage(QString());

    if (cmd.isEmpty()) {
        setPosition(firstPositionInLine(beginLine));
        showBlackMessage(QString());
    } else if (reDelete.indexIn(cmd) != -1) { // :d
        selectRange(beginLine, endLine);
        QString reg = reDelete.cap(2);
        QString text = selectedText();
        removeSelectedText();
        if (!reg.isEmpty()) {
            Register &r = m_registers[reg.at(0).unicode()];
            r.contents = text;
            r.rangemode = RangeLineMode;
        }
    } else if (reWrite.indexIn(cmd) != -1) { // :w and :x
        bool noArgs = (beginLine == -1);
        if (beginLine == -1)
            beginLine = 0;
        if (endLine == -1)
            endLine = linesInDocument();
        //qDebug() << "LINES: " << beginLine << endLine;
        int indexOfSpace = cmd.indexOf(QChar(' '));
        QString prefix;
        if (indexOfSpace < 0)
            prefix = cmd;
        else
            prefix = cmd.left(indexOfSpace);
        bool forced = prefix.contains(QChar('!'));
        bool quit = prefix.contains(QChar('q')) || prefix.contains(QChar('x'));
        bool quitAll = quit && prefix.contains(QChar('a'));
        QString fileName = reWrite.cap(2);
        if (fileName.isEmpty())
            fileName = m_currentFileName;
        QFile file1(fileName);
        bool exists = file1.exists();
        if (exists && !forced && !noArgs) {
            showRedMessage(EmacsModeHandler::tr
                ("File '%1' exists (add ! to override)").arg(fileName));
        } else if (file1.open(QIODevice::ReadWrite)) {
            file1.close();
            QTextCursor tc = m_tc;
            Range range(firstPositionInLine(beginLine),
                firstPositionInLine(endLine), RangeLineMode);
            QString contents = text(range);
            m_tc = tc;
            //qDebug() << "LINES: " << beginLine << endLine;
            bool handled = false;
            emit q->writeFileRequested(&handled, fileName, contents);
            // nobody cared, so act ourselves
            if (!handled) {
                //qDebug() << "HANDLING MANUAL SAVE TO " << fileName;
                QFile::remove(fileName);
                QFile file2(fileName);
                if (file2.open(QIODevice::ReadWrite)) {
                    QTextStream ts(&file2);
                    ts << contents;
                } else {
                    showRedMessage(EmacsModeHandler::tr
                       ("Cannot open file '%1' for writing").arg(fileName));
                }
            }
            // check result by reading back
            QFile file3(fileName);
            file3.open(QIODevice::ReadOnly);
            QByteArray ba = file3.readAll();
            showBlackMessage(EmacsModeHandler::tr("\"%1\" %2 %3L, %4C written")
                .arg(fileName).arg(exists ? " " : " [New] ")
                .arg(ba.count('\n')).arg(ba.size()));
            if (quitAll)
                passUnknownExCommand(forced ? "qa!" : "qa");
            else if (quit)
                passUnknownExCommand(forced ? "q!" : "q");
        } else {
            showRedMessage(EmacsModeHandler::tr
                ("Cannot open file '%1' for reading").arg(fileName));
        }
    } else if (cmd.startsWith(QLatin1String("r "))) { // :r
        m_currentFileName = cmd.mid(2);
        QFile file(m_currentFileName);
        file.open(QIODevice::ReadOnly);
        QTextStream ts(&file);
        QString data = ts.readAll();
        EDITOR(setPlainText(data));
        showBlackMessage(EmacsModeHandler::tr("\"%1\" %2L, %3C")
            .arg(m_currentFileName).arg(data.count('\n')).arg(data.size()));
    } else if (cmd.startsWith(QLatin1Char('!'))) {
        selectRange(beginLine, endLine);
        QString command = cmd.mid(1).trimmed();
        QString text = selectedText();
        removeSelectedText();
        QProcess proc;
        proc.start(cmd.mid(1));
        proc.waitForStarted();
        proc.write(text.toUtf8());
        proc.closeWriteChannel();
        proc.waitForFinished();
        QString result = QString::fromUtf8(proc.readAllStandardOutput());
        m_tc.insertText(result);
        leaveVisualMode();
        setPosition(firstPositionInLine(beginLine));
        //qDebug() << "FILTER: " << command;
        showBlackMessage(EmacsModeHandler::tr("%n lines filtered", 0,
            text.count('\n')));
    } else if (cmd.startsWith(QLatin1Char('>'))) {
        m_anchor = firstPositionInLine(beginLine);
        setPosition(firstPositionInLine(endLine));
        shiftRegionRight(1);
        leaveVisualMode();
        showBlackMessage(EmacsModeHandler::tr("%n lines >ed %1 time", 0,
            (endLine - beginLine + 1)).arg(1));
    } else if (cmd == "red" || cmd == "redo") { // :redo
        redo();
        updateMiniBuffer();
    } else if (reNormal.indexIn(cmd) != -1) { // :normal
        //qDebug() << "REPLAY: " << reNormal.cap(3);
        replay(reNormal.cap(3), 1);
    } else if (isSubstitution(cmd, &arguments)) { // :substitute
        QString needle = arguments.at(0);
        const QString replacement = arguments.at(1);
        QString flags = arguments.at(2);
        needle.replace('$', '\n');
        needle.replace("\\\n", "\\$");
        QRegExp pattern(needle);
        if (flags.contains('i'))
            pattern.setCaseSensitivity(Qt::CaseInsensitive);
        const bool global = flags.contains('g');
        beginEditBlock();
        for (int line = endLine; line >= beginLine; --line) {
            QString origText = lineContents(line);
            QString text = origText;
            int pos = 0;
            while (true) {
                pos = pattern.indexIn(text, pos, QRegExp::CaretAtZero);
                if (pos == -1)
                    break;
                if (pattern.cap(0).isEmpty())
                    break;
                QStringList caps = pattern.capturedTexts();
                QString matched = text.mid(pos, caps.at(0).size());
                QString repl = replacement;
                for (int i = 1; i < caps.size(); ++i)
                    repl.replace("\\" + QString::number(i), caps.at(i));
                for (int i = 0; i < repl.size(); ++i) {
                    if (repl.at(i) == '&' && (i == 0 || repl.at(i - 1) != '\\')) {
                        repl.replace(i, 1, caps.at(0));
                        i += caps.at(0).size();
                    }
                }
                text = text.left(pos) + repl + text.mid(pos + matched.size());
                pos += matched.size();
                if (!global)
                    break;
            }
            if (text != origText)
                setLineContents(line, text);
        }
        endEditBlock();
    } else if (reSet.indexIn(cmd) != -1) { // :set
        showBlackMessage(QString());
        QString arg = reSet.cap(2);
        SavedAction *act = theEmacsModeSettings()->item(arg);
        if (arg.isEmpty()) {
            theEmacsModeSetting(SettingsDialog)->trigger(QVariant());
        } else if (act && act->value().type() == QVariant::Bool) {
            // boolean config to be switched on
            bool oldValue = act->value().toBool();
            if (oldValue == false)
                act->setValue(true);
            else if (oldValue == true)
                {} // nothing to do
        } else if (act) {
            // non-boolean to show
            showBlackMessage(arg + '=' + act->value().toString());
        } else if (arg.startsWith("no")
                && (act = theEmacsModeSettings()->item(arg.mid(2)))) {
            // boolean config to be switched off
            bool oldValue = act->value().toBool();
            if (oldValue == true)
                act->setValue(false);
            else if (oldValue == false)
                {} // nothing to do
        } else if (arg.contains('=')) {
            // non-boolean config to set
            int p = arg.indexOf('=');
            act = theEmacsModeSettings()->item(arg.left(p));
            if (act)
                act->setValue(arg.mid(p + 1));
        } else {
            showRedMessage(EmacsModeHandler::tr("E512: Unknown option: ") + arg);
        }
        updateMiniBuffer();
    } else if (reHistory.indexIn(cmd) != -1) { // :history
        QString arg = reSet.cap(3);
        if (arg.isEmpty()) {
            QString info;
            info += "#  command history\n";
            int i = 0;
            foreach (const QString &item, m_commandHistory) {
                ++i;
                info += QString("%1 %2\n").arg(i, -8).arg(item);
            }
            emit q->extraInformationChanged(info);
        } else {
            notImplementedYet();
        }
        updateMiniBuffer();
    } else {
        passUnknownExCommand(cmd);
    }
}

void EmacsModeHandler::Private::passUnknownExCommand(const QString &cmd)
{
    emit q->handleExCommandRequested(cmd);
}

static void vimPatternToQtPattern(QString *needle, QTextDocument::FindFlags *flags)
{
    // FIXME: Rough mapping of a common case
    if (needle->startsWith("\\<") && needle->endsWith("\\>"))
        (*flags) |= QTextDocument::FindWholeWords;
    needle->replace("\\<", ""); // start of word
    needle->replace("\\>", ""); // end of word
    //qDebug() << "NEEDLE " << needle0 << needle;
}

void EmacsModeHandler::Private::search(const QString &needle0, bool forward)
{
    showBlackMessage((forward ? '/' : '?') + needle0);
    CursorPosition origPosition = cursorPosition();
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
    if (!forward)
        flags |= QTextDocument::FindBackward;

    QString needle = needle0;
    vimPatternToQtPattern(&needle, &flags);

    if (forward)
        m_tc.movePosition(Right, MoveAnchor, 1);

    int oldLine = cursorLineInDocument() - cursorLineOnScreen();

    EDITOR(setTextCursor(m_tc));
    if (EDITOR(find(needle, flags))) {
        m_tc = EDITOR(textCursor());
        m_tc.setPosition(m_tc.anchor());
        // making this unconditional feels better, but is not "vim like"
        if (oldLine != cursorLineInDocument() - cursorLineOnScreen())
            scrollToLineInDocument(cursorLineInDocument() - linesOnScreen() / 2);
        highlightMatches(needle);
    } else {
        m_tc.setPosition(forward ? 0 : lastPositionInDocument());
        EDITOR(setTextCursor(m_tc));
        if (EDITOR(find(needle, flags))) {
            m_tc = EDITOR(textCursor());
            m_tc.setPosition(m_tc.anchor());
            if (oldLine != cursorLineInDocument() - cursorLineOnScreen())
                scrollToLineInDocument(cursorLineInDocument() - linesOnScreen() / 2);
            if (forward)
                showRedMessage(EmacsModeHandler::tr("search hit BOTTOM, continuing at TOP"));
            else
                showRedMessage(EmacsModeHandler::tr("search hit TOP, continuing at BOTTOM"));
            highlightMatches(needle);
        } else {
            highlightMatches(QString());
            setCursorPosition(origPosition);
            showRedMessage(EmacsModeHandler::tr("Pattern not found: ") + needle);
        }
    }
}

void EmacsModeHandler::Private::highlightMatches(const QString &needle0)
{
    if (!hasConfig(ConfigHlSearch))
        return;
    if (needle0 == m_oldNeedle)
        return;
    m_oldNeedle = needle0;
    m_searchSelections.clear();

    if (!needle0.isEmpty()) {
        QTextCursor tc = m_tc;
        tc.movePosition(StartOfDocument, MoveAnchor);

        QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
        QString needle = needle0;
        vimPatternToQtPattern(&needle, &flags);


        EDITOR(setTextCursor(tc));
        while (EDITOR(find(needle, flags))) {
            tc = EDITOR(textCursor());
            QTextEdit::ExtraSelection sel;
            sel.cursor = tc;
            sel.format = tc.blockCharFormat();
            sel.format.setBackground(QColor(177, 177, 0));
            m_searchSelections.append(sel);
            tc.movePosition(Right, MoveAnchor);
            EDITOR(setTextCursor(tc));
        }
    }
    updateSelection();
}

void EmacsModeHandler::Private::moveToFirstNonBlankOnLine()
{
    QTextDocument *doc = m_tc.document();
    const QTextBlock &block = m_tc.block();
    int firstPos = block.position();
    for (int i = firstPos, n = firstPos + block.length(); i < n; ++i) {
        if (!doc->characterAt(i).isSpace()) {
            setPosition(i);
            return;
        }
    }
    setPosition(block.position());
}

void EmacsModeHandler::Private::indentRegion(QChar typedChar)
{
    //int savedPos = anchor();
    int beginLine = lineForPosition(m_tc.anchor());
    int endLine = lineForPosition(m_tc.position());
    if (beginLine > endLine)
        qSwap(beginLine, endLine);

    int amount = 0;
    emit q->indentRegion(&amount, beginLine, endLine, typedChar);

    setPosition(firstPositionInLine(beginLine));
    moveToFirstNonBlankOnLine();
    setTargetColumn();
    setDotCommand("%1==", endLine - beginLine + 1);
}

void EmacsModeHandler::Private::shiftRegionRight(int repeat)
{
    int beginLine = lineForPosition(anchor());
    int endLine = lineForPosition(position());
    if (beginLine > endLine)
        qSwap(beginLine, endLine);
    int len = config(ConfigShiftWidth).toInt() * repeat;
    QString indent(len, ' ');
    int firstPos = firstPositionInLine(beginLine);

    beginEditBlock(firstPos);
    for (int line = beginLine; line <= endLine; ++line) {
        setPosition(firstPositionInLine(line));
        m_tc.insertText(indent);
    }
    endEditBlock();

    setPosition(firstPos);
    moveToFirstNonBlankOnLine();
    setTargetColumn();
    setDotCommand("%1>>", endLine - beginLine + 1);
}

void EmacsModeHandler::Private::shiftRegionLeft(int repeat)
{
    int beginLine = lineForPosition(anchor());
    int endLine = lineForPosition(position());
    if (beginLine > endLine)
        qSwap(beginLine, endLine);
    int shift = config(ConfigShiftWidth).toInt() * repeat;
    int tab = config(ConfigTabStop).toInt();
    int firstPos = firstPositionInLine(beginLine);

    beginEditBlock(firstPos);
    for (int line = beginLine; line <= endLine; ++line) {
        int pos = firstPositionInLine(line);
        setPosition(pos);
        setAnchor(pos);
        QString text = m_tc.block().text();
        int amount = 0;
        int i = 0;
        for (; i < text.size() && amount <= shift; ++i) {
            if (text.at(i) == ' ')
                amount++;
            else if (text.at(i) == '\t')
                amount += tab; // FIXME: take position into consideration
            else
                break;
        }
        setPosition(pos + i);
        text = selectedText();
        removeSelectedText();
        setPosition(pos);
    }
    endEditBlock();

    setPosition(firstPos);
    moveToFirstNonBlankOnLine();
    setTargetColumn();
    setDotCommand("%1<<", endLine - beginLine + 1);
}

void EmacsModeHandler::Private::moveToTargetColumn()
{
    const QTextBlock &block = m_tc.block();
    int col = m_tc.position() - block.position();
    if (col == m_targetColumn)
        return;
    //qDebug() << "CORRECTING COLUMN FROM: " << col << "TO" << m_targetColumn;
    if (m_targetColumn == -1 || block.length() <= m_targetColumn)
        m_tc.setPosition(block.position() + block.length() - 1, MoveAnchor);
    else
        m_tc.setPosition(block.position() + m_targetColumn, MoveAnchor);
}

/* if simple is given:
 *  class 0: spaces
 *  class 1: non-spaces
 * else
 *  class 0: spaces
 *  class 1: non-space-or-letter-or-number
 *  class 2: letter-or-number
 */
static int charClass(QChar c, bool simple)
{
    if (simple)
        return c.isSpace() ? 0 : 1;
    if (c.isLetterOrNumber() || c.unicode() == '_')
        return 2;
    return c.isSpace() ? 0 : 1;
}

void EmacsModeHandler::Private::moveToWordBoundary(bool simple, bool forward)
{
    int repeat = count();
    QTextDocument *doc = m_tc.document();
    int n = forward ? lastPositionInDocument() : 0;
    int lastClass = -1;
    while (true) {
        QChar c = doc->characterAt(m_tc.position() + (forward ? 1 : -1));
        //qDebug() << "EXAMINING: " << c << " AT " << position();
        int thisClass = charClass(c, simple);
        if (thisClass != lastClass && lastClass != 0)
            --repeat;
        if (repeat == -1)
            break;
        lastClass = thisClass;
        if (m_tc.position() == n)
            break;
        forward ? moveRight() : moveLeft();
    }
    setTargetColumn();
}

void EmacsModeHandler::Private::handleFfTt(int key)
{
    // m_subsubmode \in { 'f', 'F', 't', 'T' }
    bool forward = m_subsubdata == 'f' || m_subsubdata == 't';
    int repeat = count();
    QTextDocument *doc = m_tc.document();
    QTextBlock block = m_tc.block();
    int n = block.position();
    if (forward)
        n += block.length();
    int pos = m_tc.position();
    while (true) {
        pos += forward ? 1 : -1;
        if (pos == n)
            break;
        int uc = doc->characterAt(pos).unicode();
        if (uc == ParagraphSeparator)
            break;
        if (uc == key)
            --repeat;
        if (repeat == 0) {
            if (m_subsubdata == 't')
                --pos;
            else if (m_subsubdata == 'T')
                ++pos;

            if (forward)
                m_tc.movePosition(Right, KeepAnchor, pos - m_tc.position());
            else
                m_tc.movePosition(Left, KeepAnchor, m_tc.position() - pos);
            break;
        }
    }
    setTargetColumn();
}

void EmacsModeHandler::Private::moveToNextWord(bool simple)
{
    // FIXME: 'w' should stop on empty lines, too
    int repeat = count();
    int n = lastPositionInDocument();
    int lastClass = charClass(characterAtCursor(), simple);
    while (true) {
        QChar c = characterAtCursor();
        int thisClass = charClass(c, simple);
        if (thisClass != lastClass && thisClass != 0)
            --repeat;
        if (repeat == 0)
            break;
        lastClass = thisClass;
        moveRight();
        if (m_tc.position() == n)
            break;
    }
    setTargetColumn();
}

void EmacsModeHandler::Private::moveToMatchingParanthesis()
{
    bool moved = false;
    bool forward = false;

    emit q->moveToMatchingParenthesis(&moved, &forward, &m_tc);

    if (moved && forward) {
       if (m_submode == NoSubMode || m_submode == ZSubMode || m_submode == CapitalZSubMode || m_submode == RegisterSubMode)
            m_tc.movePosition(Left, KeepAnchor, 1);
    }
    setTargetColumn();
}

int EmacsModeHandler::Private::cursorLineOnScreen() const
{
    if (!editor())
        return 0;
    QRect rect = EDITOR(cursorRect());
    return rect.y() / rect.height();
}

int EmacsModeHandler::Private::linesOnScreen() const
{
    if (!editor())
        return 1;
    QRect rect = EDITOR(cursorRect());
    return EDITOR(height()) / rect.height();
}

int EmacsModeHandler::Private::columnsOnScreen() const
{
    if (!editor())
        return 1;
    QRect rect = EDITOR(cursorRect());
    // qDebug() << "WID: " << EDITOR(width()) << "RECT: " << rect;
    return EDITOR(width()) / rect.width();
}

int EmacsModeHandler::Private::cursorLineInDocument() const
{
    return m_tc.block().blockNumber();
}

int EmacsModeHandler::Private::cursorColumnInDocument() const
{
    return m_tc.position() - m_tc.block().position();
}

int EmacsModeHandler::Private::linesInDocument() const
{
    return m_tc.isNull() ? 0 : m_tc.document()->blockCount();
}

void EmacsModeHandler::Private::scrollToLineInDocument(int line)
{
    // FIXME: works only for QPlainTextEdit
    QScrollBar *scrollBar = EDITOR(verticalScrollBar());
    //qDebug() << "SCROLL: " << scrollBar->value() << line;
    scrollBar->setValue(line);
    //QTC_ASSERT(firstVisibleLineInDocument() == line, /**/);
}

int EmacsModeHandler::Private::firstVisibleLineInDocument() const
{
    QScrollBar *scrollBar = EDITOR(verticalScrollBar());
    if (0 && scrollBar->value() != cursorLineInDocument() - cursorLineOnScreen()) {
        qDebug() << "SCROLLBAR: " << scrollBar->value()
            << "CURSORLINE IN DOC" << cursorLineInDocument()
            << "CURSORLINE ON SCREEN" << cursorLineOnScreen();
    }
    //return scrollBar->value();
    return cursorLineInDocument() - cursorLineOnScreen();
}

void EmacsModeHandler::Private::scrollUp(int count)
{
    scrollToLineInDocument(cursorLineInDocument() - cursorLineOnScreen() - count);
}

int EmacsModeHandler::Private::lastPositionInDocument() const
{
    QTextBlock block = m_tc.document()->lastBlock();
    return block.position() + block.length() - 1;
}

QString EmacsModeHandler::Private::lastSearchString() const
{
     return m_searchHistory.empty() ? QString() : m_searchHistory.back();
}

QString EmacsModeHandler::Private::text(const Range &range) const
{
    if (range.rangemode == RangeCharMode) {
        QTextCursor tc = m_tc;
        tc.setPosition(range.beginPos, MoveAnchor);
        tc.setPosition(range.endPos, KeepAnchor);
        return tc.selection().toPlainText();
    }
    if (range.rangemode == RangeLineMode) {
        QTextCursor tc = m_tc;
        int firstPos = firstPositionInLine(lineForPosition(range.beginPos));
        int lastLine = lineForPosition(range.endPos);
        int lastPos = lastLine == m_tc.document()->lastBlock().blockNumber() + 1
            ? lastPositionInDocument() : firstPositionInLine(lastLine + 1);
        tc.setPosition(firstPos, MoveAnchor);
        tc.setPosition(lastPos, KeepAnchor);
        return tc.selection().toPlainText();
    }
    // FIXME: Performance?
    int beginLine = lineForPosition(range.beginPos);
    int endLine = lineForPosition(range.endPos);
    int beginColumn = 0;
    int endColumn = INT_MAX;
    if (range.rangemode == RangeBlockMode) {
        int column1 = range.beginPos - firstPositionInLine(beginLine);
        int column2 = range.endPos - firstPositionInLine(endLine);
        beginColumn = qMin(column1, column2);
        endColumn = qMax(column1, column2);
        qDebug() << "COLS: " << beginColumn << endColumn;
    }
    int len = endColumn - beginColumn + 1;
    QString contents;
    QTextBlock block = m_tc.document()->findBlockByNumber(beginLine - 1);
    for (int i = beginLine; i <= endLine && block.isValid(); ++i) {
        QString line = block.text();
        if (range.rangemode == RangeBlockMode) {
            line = line.mid(beginColumn, len);
            if (line.size() < len)
                line += QString(len - line.size(), QChar(' '));
        }
        contents += line;
        if (!contents.endsWith('\n'))
            contents += '\n';
        block = block.next();
    }
    //qDebug() << "SELECTED: " << contents;
    return contents;
}

void EmacsModeHandler::Private::yankSelectedText()
{
    Range range(anchor(), position());
    range.rangemode = m_rangemode;
    yankText(range, m_register);
}

void EmacsModeHandler::Private::yankText(const Range &range, int toregister)
{
    Register &reg = m_registers[toregister];
    reg.contents = text(range);
    reg.rangemode = range.rangemode;
    //qDebug() << "YANKED: " << reg.contents;
}

void EmacsModeHandler::Private::removeSelectedText()
{
    Range range(anchor(), position());
    range.rangemode = m_rangemode;
    removeText(range);
}

void EmacsModeHandler::Private::removeText(const Range &range)
{
    QTextCursor tc = m_tc;
    switch (range.rangemode) {
        case RangeCharMode: {
            tc.setPosition(range.beginPos, MoveAnchor);
            tc.setPosition(range.endPos, KeepAnchor);
            fixMarks(range.beginPos, tc.selectionStart() - tc.selectionEnd());
            tc.removeSelectedText();
            return;
        }
        case RangeLineMode: {
            tc.setPosition(range.beginPos, MoveAnchor);
            tc.movePosition(StartOfLine, MoveAnchor);
            tc.setPosition(range.endPos, KeepAnchor);
            tc.movePosition(EndOfLine, KeepAnchor);
            tc.movePosition(Right, KeepAnchor, 1);
            fixMarks(range.beginPos, tc.selectionStart() - tc.selectionEnd());
            tc.removeSelectedText();
            return;
        }
        case RangeBlockAndTailMode: 
        case RangeBlockMode: {
            int beginLine = lineForPosition(range.beginPos);
            int endLine = lineForPosition(range.endPos);
            int column1 = range.beginPos - firstPositionInLine(beginLine);
            int column2 = range.endPos - firstPositionInLine(endLine);
            int beginColumn = qMin(column1, column2);
            int endColumn = qMax(column1, column2);
            if (range.rangemode == RangeBlockAndTailMode)
                endColumn = INT_MAX - 1;
            QTextBlock block = m_tc.document()->findBlockByNumber(endLine - 1);
            beginEditBlock(range.beginPos);
            for (int i = beginLine; i <= endLine && block.isValid(); ++i) {
                int bCol = qMin(beginColumn, block.length() - 1);
                int eCol = qMin(endColumn + 1, block.length() - 1);
                tc.setPosition(block.position() + bCol, MoveAnchor);
                tc.setPosition(block.position() + eCol, KeepAnchor);
                fixMarks(block.position() + bCol,
                         tc.selectionStart() - tc.selectionEnd());
                tc.removeSelectedText();
                block = block.previous();
            }
            endEditBlock();
        }
    }
}

void EmacsModeHandler::Private::pasteText(bool afterCursor)
{
    const QString text = m_registers[m_register].contents;
    const QStringList lines = text.split(QChar('\n'));
    switch (m_registers[m_register].rangemode) {
        case RangeCharMode: {
            m_targetColumn = 0;
            for (int i = count(); --i >= 0; ) {
                if (afterCursor && rightDist() > 0)
                    moveRight();
                fixMarks(position(), text.length());
                m_tc.insertText(text);
                moveLeft();
            }
            break;
        }
        case RangeLineMode: {
            moveToStartOfLine();
            m_targetColumn = 0;
            for (int i = count(); --i >= 0; ) {
                if (afterCursor)
                    moveDown();
                fixMarks(position(), text.length());
                m_tc.insertText(text);
                moveUp(lines.size() - 1);
            }
            moveToFirstNonBlankOnLine();
            break;
        }
        case RangeBlockAndTailMode:
        case RangeBlockMode: {
            beginEditBlock();
            QTextBlock block = m_tc.block();
            if (afterCursor)
                moveRight();
            QTextCursor tc = m_tc;
            const int col = tc.position() - block.position();
            //for (int i = lines.size(); --i >= 0; ) {
            for (int i = 0; i < lines.size(); ++i) {
                const QString line = lines.at(i);
                tc.movePosition(StartOfLine, MoveAnchor);
                if (col >= block.length()) {
                    tc.movePosition(EndOfLine, MoveAnchor);
                    fixMarks(position(), col - line.size() + 1);
                    tc.insertText(QString(col - line.size() + 1, QChar(' ')));
                } else {
                    tc.movePosition(Right, MoveAnchor, col - 1 + afterCursor);
                }
                qDebug() << "INSERT " << line << " AT " << tc.position()
                    << "COL: " << col;
                fixMarks(position(), line.length());
                tc.insertText(line);
                tc.movePosition(StartOfLine, MoveAnchor);
                tc.movePosition(Down, MoveAnchor, 1);
                if (tc.position() >= lastPositionInDocument() - 1) {
                    fixMarks(position(), 1);
                    tc.insertText(QString(QChar('\n')));
                    tc.movePosition(Up, MoveAnchor, 1);
                }
                block = block.next();
            }
            moveLeft();
            endEditBlock();
            break;
        }
    }
}

//FIXME: This needs to called after undo/insert
void EmacsModeHandler::Private::fixMarks(int positionAction, int positionChange)
{
    QHashIterator<int, int> i(m_marks);
    while (i.hasNext()) {
        i.next();
        if (i.value() >= positionAction) {
            if (i.value() + positionChange > 0)
                m_marks[i.key()] = i.value() + positionChange;
            else
                m_marks.remove(i.key());
        }
    }
}

QString EmacsModeHandler::Private::lineContents(int line) const
{
    return m_tc.document()->findBlockByNumber(line - 1).text();
}

void EmacsModeHandler::Private::setLineContents(int line, const QString &contents) const
{
    QTextBlock block = m_tc.document()->findBlockByNumber(line - 1);
    QTextCursor tc = m_tc;
    tc.setPosition(block.position());
    tc.setPosition(block.position() + block.length() - 1, KeepAnchor);
    tc.removeSelectedText();
    tc.insertText(contents);
}

int EmacsModeHandler::Private::firstPositionInLine(int line) const
{
    return m_tc.document()->findBlockByNumber(line - 1).position();
}

int EmacsModeHandler::Private::lastPositionInLine(int line) const
{
    QTextBlock block = m_tc.document()->findBlockByNumber(line - 1);
    return block.position() + block.length() - 1;
}

int EmacsModeHandler::Private::lineForPosition(int pos) const
{
    QTextCursor tc = m_tc;
    tc.setPosition(pos);
    return tc.block().blockNumber() + 1;
}

void EmacsModeHandler::Private::enterVisualMode(VisualMode visualMode)
{
    setAnchor();
    m_visualMode = visualMode;
    m_marks['<'] = m_tc.position();
    m_marks['>'] = m_tc.position();
    updateMiniBuffer();
    updateSelection();
}

void EmacsModeHandler::Private::leaveVisualMode()
{
    m_visualMode = NoVisualMode;
    updateMiniBuffer();
    updateSelection();
}

QWidget *EmacsModeHandler::Private::editor() const
{
    return m_textedit
        ? static_cast<QWidget *>(m_textedit)
        : static_cast<QWidget *>(m_plaintextedit);
}

void EmacsModeHandler::Private::undo()
{
    //qDebug() << " CURSOR POS: " << m_undoCursorPosition;
    int current = m_tc.document()->availableUndoSteps();
    //endEditBlock();
    EDITOR(undo());
    //beginEditBlock();
    int rev = m_tc.document()->availableUndoSteps();
    if (current == rev)
        showBlackMessage(EmacsModeHandler::tr("Already at oldest change"));
    else
        showBlackMessage(QString());

    if (m_undoCursorPosition.contains(rev))
        m_tc.setPosition(m_undoCursorPosition[rev]);
}

void EmacsModeHandler::Private::redo()
{
    int current = m_tc.document()->availableUndoSteps();
    //endEditBlock();
    EDITOR(redo());
    //beginEditBlock();
    int rev = m_tc.document()->availableUndoSteps();
    if (rev == current)
        showBlackMessage(EmacsModeHandler::tr("Already at newest change"));
    else
        showBlackMessage(QString());

    if (m_undoCursorPosition.contains(rev))
        m_tc.setPosition(m_undoCursorPosition[rev]);
}

void EmacsModeHandler::Private::enterInsertMode()
{
    EDITOR(setCursorWidth(m_cursorWidth));
    EDITOR(setOverwriteMode(false));
    m_mode = InsertMode;
    m_lastInsertion.clear();
}

void EmacsModeHandler::Private::enterCommandMode()
{
    EDITOR(setCursorWidth(m_cursorWidth));
    EDITOR(setOverwriteMode(true));
    m_mode = CommandMode;
}

void EmacsModeHandler::Private::enterExMode()
{
    EDITOR(setCursorWidth(0));
    EDITOR(setOverwriteMode(false));
    m_mode = ExMode;
}

void EmacsModeHandler::Private::recordJump()
{
    m_jumpListUndo.append(cursorPosition());
    m_jumpListRedo.clear();
    UNDO_DEBUG("jumps: " << m_jumpListUndo);
}

void EmacsModeHandler::Private::recordNewUndo()
{
    //endEditBlock();
    UNDO_DEBUG("---- BREAK ----");
    //beginEditBlock();
}

Indentation EmacsModeHandler::Private::indentation(const QString &line) const
{
    int ts = config(ConfigTabStop).toInt();
    int physical = 0;
    int logical = 0;
    int n = line.size();
    while (physical < n) {
        QChar c = line.at(physical);
        if (c == QLatin1Char(' '))
            ++logical;
        else if (c == QLatin1Char('\t'))
            logical += ts - logical % ts;
        else
            break;
        ++physical;
    }
    return Indentation(physical, logical);
}

QString EmacsModeHandler::Private::tabExpand(int n) const
{
    int ts = config(ConfigTabStop).toInt();
    if (hasConfig(ConfigExpandTab) || ts < 1)
        return QString(n, QLatin1Char(' '));
    return QString(n / ts, QLatin1Char('\t'))
         + QString(n % ts, QLatin1Char(' '));
}

void EmacsModeHandler::Private::insertAutomaticIndentation(bool goingDown)
{
    if (!hasConfig(ConfigAutoIndent))
        return;
    QTextBlock block = goingDown ? m_tc.block().previous() : m_tc.block().next();
    QString text = block.text();
    int pos = 0;
    int n = text.size();
    while (pos < n && text.at(pos).isSpace())
        ++pos;
    text.truncate(pos);
    // FIXME: handle 'smartindent' and 'cindent'
    m_tc.insertText(text);
    m_justAutoIndented = text.size();
}

bool EmacsModeHandler::Private::removeAutomaticIndentation()
{
    if (!hasConfig(ConfigAutoIndent) || m_justAutoIndented == 0)
        return false;
    m_tc.movePosition(StartOfLine, KeepAnchor);
    m_tc.removeSelectedText();
    m_lastInsertion.chop(m_justAutoIndented);
    m_justAutoIndented = 0;
    return true;
}

void EmacsModeHandler::Private::handleStartOfLine()
{
    if (hasConfig(ConfigStartOfLine))
        moveToFirstNonBlankOnLine();
}

void EmacsModeHandler::Private::replay(const QString &command, int n)
{
    //qDebug() << "REPLAY: " << command;
    m_inReplay = true;
    for (int i = n; --i >= 0; ) {
        foreach (QChar c, command) {
            //qDebug() << "  REPLAY: " << QString(c);
            handleKey(c.unicode(), c.unicode(), QString(c));
        }
    }
    m_inReplay = false;
}


///////////////////////////////////////////////////////////////////////
//
// EmacsModeHandler
//
///////////////////////////////////////////////////////////////////////

EmacsModeHandler::EmacsModeHandler(QWidget *widget, QObject *parent)
    : QObject(parent), d(new Private(this, widget))
{}

EmacsModeHandler::~EmacsModeHandler()
{
    delete d;
}

bool EmacsModeHandler::eventFilter(QObject *ob, QEvent *ev)
{
    bool active = theEmacsModeSetting(ConfigUseEmacsMode)->value().toBool();

    if (active && ev->type() == QEvent::KeyPress && ob == d->editor()) {
        QKeyEvent *kev = static_cast<QKeyEvent *>(ev);
        KEY_DEBUG("KEYPRESS" << kev->key());
        EventResult res = d->handleEvent(kev);
        // returning false core the app see it
        //KEY_DEBUG("HANDLED CODE:" << res);
        //return res != EventPassedToCore;
        //return true;
        return res == EventHandled;
    }

    if (active && ev->type() == QEvent::ShortcutOverride && ob == d->editor()) {
        QKeyEvent *kev = static_cast<QKeyEvent *>(ev);
        if (d->wantsOverride(kev)) {


            KEY_DEBUG("OVERRIDING SHORTCUT" << kev->key());
            ev->accept(); // accepting means "don't run the shortcuts"
            return true;
        }
        KEY_DEBUG("NO SHORTCUT OVERRIDE" << kev->key());
        return true;
    }

    return QObject::eventFilter(ob, ev);
}

void EmacsModeHandler::installEventFilter()
{
    d->installEventFilter();
}

void EmacsModeHandler::setupWidget()
{
    d->setupWidget();
}

void EmacsModeHandler::restoreWidget()
{
    d->restoreWidget();
}

void EmacsModeHandler::handleCommand(const QString &cmd)
{
    d->handleCommand(cmd);
}

void EmacsModeHandler::setCurrentFileName(const QString &fileName)
{
   d->m_currentFileName = fileName;
}

void EmacsModeHandler::showBlackMessage(const QString &msg)
{
   d->showBlackMessage(msg);
}

void EmacsModeHandler::showRedMessage(const QString &msg)
{
   d->showRedMessage(msg);
}


QWidget *EmacsModeHandler::widget()
{
    return d->editor();
}

// Test only
int EmacsModeHandler::physicalIndentation(const QString &line) const
{
    Indentation ind = d->indentation(line);
    return ind.physical;
}

int EmacsModeHandler::logicalIndentation(const QString &line) const
{
    Indentation ind = d->indentation(line);
    return ind.logical;
}

QString EmacsModeHandler::tabExpand(int n) const
{
    return d->tabExpand(n);
}


} // namespace Internal
} // namespace EmacsMode
