#include "text_highlighter.h"

Highlighter::Highlighter(QTextDocument *parent): QSyntaxHighlighter(parent)
{
    HighlightingRule rule;
    
    /* KEYWORDS */
    keywordFormat.setForeground(QBrush(QColor(106, 27, 224, 255)));
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
    warningFormat.setForeground(QBrush(QColor(255, 40, 40, 255)));
    rule.pattern = QRegExp("Warning:");
    rule.format = warningFormat;
    highlightingRules.append(rule);

    /* ERRORS */
    errorFormat.setForeground(QBrush(QColor(255, 70, 70, 255)));
    rule.pattern = QRegExp("(?:\\S+)?Error:");
    rule.format = errorFormat;
    highlightingRules.append(rule);
    
    /* STAMPS */
    stampFormat.setForeground(QBrush(QColor(40, 255, 20, 255)));
    //~ rule.pattern = QRegExp("(?:[Script])?(?:[Set])?(?:[Read])?(?:[Project])?(?:[Voxelize])?");
    rule.pattern = QRegExp("\\[\\S+\\]");
    rule.format = stampFormat;
    highlightingRules.append(rule);
    
    /* CLASSES */
    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(QBrush(QColor(106, 27, 224, 255)));
    rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);
    
    /* SINGLE LINE COMMENTS */
    singleLineCommentFormat.setForeground(QBrush(QColor(195, 83, 20, 255)));
    rule.pattern = QRegExp("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
    
    /* MULT LINE COMMENTS */
    multiLineCommentFormat.setForeground(QBrush(QColor(195, 83, 20, 255)));
    
    /* NUMBERS */
    numberFormat.setForeground(QBrush(QColor(40, 140, 255, 255)));
    rule.pattern = QRegExp("(?:\\d*\\.\\d+)|(?:\\d+)");
    rule.format = numberFormat;
    highlightingRules.append(rule);
    
    /* QUOTATIONS */
    quotationFormat.setForeground(QBrush(QColor(255, 230, 171, 255)));
    rule.pattern = QRegExp("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    /* PARANTHESES */
    paranthesisFormat.setForeground(QBrush(QColor(255, 93, 60, 255)));
    rule.pattern = QRegExp("[()]");
    rule.format = paranthesisFormat;
    highlightingRules.append(rule);
    
    /* FUNCTIONS */
    functionFormat.setForeground(QColor(106, 27, 224, 255));
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
