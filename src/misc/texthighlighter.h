#ifndef TEXT_HIGHLIGHTER_H
#define TEXT_HIGHLIGHTER_H



#include <QSyntaxHighlighter>
#include <QVector>
#include <QRegExp>
#include <QTextCharFormat>

class Highlighter : public QSyntaxHighlighter
{
        Q_OBJECT

    public:
        Highlighter(QTextDocument * parent = 0);

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
