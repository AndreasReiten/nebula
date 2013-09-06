#ifndef TEXT_HIGHLIGHTER_H
#define TEXT_HIGHLIGHTER_H

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <cmath>
#include <cerrno>
#include <vector>
#include <limits>

#include <QStringList>
#include <QObject>
#include <QWidget>
#include <QPainter>
#include <QSyntaxHighlighter>
#include <QHash>
#include <QChar>
#include <QString>
#include <QTextCharFormat>

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

    public:
        Highlighter(QTextDocument *parent = 0);

    protected:
        void highlightBlock(const QString &text);

    private:
        struct HighlightingRule
        {
            QRegExp pattern;
            QTextCharFormat format;
        };
        QVector<HighlightingRule> highlightingRules;

        QRegExp commentStartExpression;
        QRegExp commentEndExpression;

        QTextCharFormat keywordFormat;
        QTextCharFormat classFormat;
        QTextCharFormat singleLineCommentFormat;
        QTextCharFormat multiLineCommentFormat;
        QTextCharFormat numberFormat;
        QTextCharFormat quotationFormat;
        QTextCharFormat functionFormat;
        QTextCharFormat warningFormat;
        QTextCharFormat errorFormat;
        QTextCharFormat stampFormat;
        QTextCharFormat paranthesisFormat;
};
#endif
