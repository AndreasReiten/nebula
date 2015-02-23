#include "texthighlighter.h"

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <cmath>
//#include <cerrno>
#include <vector>
#include <limits>

#include <QStringList>
#include <QWidget>
#include <QPainter>

#include <QHash>
#include <QChar>
#include <QString>

Highlighter::Highlighter(QTextDocument *parent): QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Colors
    QColor keyword_color(106, 27, 224, 255);
    QColor warning_color(255, 40, 40, 255);
    QColor error_color(255, 70, 70, 255);
//    QColor stamp_color(40, 255, 20, 255);
    QColor class_color(106, 27, 224, 255);
    QColor singlecomment_color(195, 83, 20, 255);
    QColor multicomment_color(195, 83, 20, 255);
    QColor number_color(40, 140, 255, 255);
    QColor quote_color(40, 100, 20, 255);
    QColor paranthesis_color(255, 93, 255, 255);
    QColor function_color(150, 70, 224, 255);


    /* KEYWORDS */
    keywordFormat.setForeground(keyword_color);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b"
        << "\\bdouble\\b" << "\\benum\\b" << "\\bexplicit\\b"
        << "\\bfriend\\b" << "\\binline\\b" << "\\bint\\b"
        << "\\blong\\b" << "\\bnamespace\\b" << "\\boperator\\b"
        << "\\bprivate\\b" << "\\bprotected\\b" << "\\bpublic\\b"
        << "\\bshort\\b" << "\\bsignals\\b" << "\\bsigned\\b"
        << "\\bslots\\b" << "\\bstatic\\b" << "\\bstruct\\b"
        << "\\btemplate\\b" << "\\btypedef\\b" << "\\btypename\\b"
        << "\\bunion\\b" << "\\bunsigned\\b" << "\\bvirtual\\b"
        << "\\bvoid\\b" << "\\bvolatile\\b";
    foreach (const QString &pattern, keywordPatterns)
    {
        rule.pattern = QRegExp(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    /* WARNINGS */
    warningFormat.setForeground(warning_color);
    rule.pattern = QRegExp("Warning:");
    rule.format = warningFormat;
    highlightingRules.append(rule);

    /* ERRORS */
    errorFormat.setForeground(error_color);
    rule.pattern = QRegExp("(?:\\S+)?Error:");
    rule.format = errorFormat;
    highlightingRules.append(rule);

    /* CLASSES */
    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(class_color);
    rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);

    /* SINGLE LINE COMMENTS */
    singleLineCommentFormat.setForeground(singlecomment_color);
    rule.pattern = QRegExp("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    /* MULT LINE COMMENTS */
    multiLineCommentFormat.setForeground(multicomment_color);

    /* NUMBERS */
    numberFormat.setForeground(number_color);
    rule.pattern = QRegExp("(?:\\d*\\.\\d+)|(?:\\d+)");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    /* QUOTATIONS */
    quotationFormat.setForeground(quote_color); //QColor(255, 230, 171, 255) Unique
    rule.pattern = QRegExp("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    /* PARANTHESES */
    paranthesisFormat.setForeground(paranthesis_color);
    rule.pattern = QRegExp("[()\\[\\]]");
    rule.format = paranthesisFormat;
    highlightingRules.append(rule);

    /* FUNCTIONS */
    functionFormat.setForeground(function_color);//QColor(106, 27, 224, 255) Epic
    rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegExp("/\\*");
    commentEndExpression = QRegExp("\\*/");
}

void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
    startIndex = commentStartExpression.indexIn(text);

    while (startIndex >= 0)
    {
        int endIndex = commentEndExpression.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1)
        {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex
            + commentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
    }
}
