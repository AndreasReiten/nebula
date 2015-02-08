#include "selection.h"

Selection::Selection()
{
    p_integral = 0;
    p_weighted_x = 0;
    p_weighted_y = 0;
    p_isMouseSelected = false;
}

Selection::Selection(const Selection & other)
{
    this->setRect(other.x(),other.y(),other.width(),other.height());
    p_integral = other.integral();
    p_weighted_x = other.weighted_x();
    p_weighted_y = other.weighted_y();
    p_isMouseSelected = other.selected();
}
Selection::~Selection()
{

}

Matrix<int> Selection::lrtb()
{
    Matrix<int> buf(1,4);
    buf[0] = this->left();
    buf[1] = this->left() + this->width();
    buf[2] = this->top();
    buf[3] = this->top() + this->height();
    return buf;
}

double Selection::integral() const
{
    return p_integral;
}

double Selection::average() const
{
    return p_integral/(double)(this->width()*this->height());
}

double Selection::weighted_x() const
{
    return p_weighted_x;
}
double Selection::weighted_y() const
{
    return p_weighted_y;
}

void Selection::setSum(double value)
{
    p_integral = value;
}
void Selection::setWeightedX(double value)
{
    p_weighted_x = value;
}
void Selection::setWeightedY(double value)
{
    p_weighted_y = value;
}

void Selection::setSelected(bool value)
{
    p_isMouseSelected = value;
}

void Selection::restrictToRect(QRect rect)
{
//    qDebug() << "Restrict to:" << rect;

//    qDebug() << "UnRestricted:" << *this;

    // First ensure the size is ok
    if (this->width() > rect.width()) this->setWidth(rect.width());
    if (this->height() > rect.height()) this->setHeight(rect.height());

    // Then move the rectangle so it lies within the bounding rect
    if (this->left() < rect.left()) this->moveLeft(rect.left());
    if (this->right() > rect.right()) this->moveRight(rect.right());
    if (this->top() < rect.top()) this->moveTop(rect.top());
    if (this->bottom() > rect.bottom()) this->moveBottom(rect.bottom());

//    qDebug() << "Restricted:" << *this;
}

bool Selection::selected() const
{
    return p_isMouseSelected;
}

Selection& Selection::operator = (QRect other)
{
    this->setRect(other.x(),other.y(),other.width(),other.height());

//    qDebug() << "Used rect copy";
    return * this;
}

Selection& Selection::operator = (Selection other)
{
    this->setRect(other.x(),other.y(),other.width(),other.height());

    p_integral = other.integral();
    p_weighted_x = other.weighted_x();
    p_weighted_y = other.weighted_y();
    p_isMouseSelected = other.selected();


//    qDebug() << p_isMouseSelected << other.selected();
    return * this;
}

QDebug operator<<(QDebug dbg, const Selection &selection)
{
    QRect tmp;
    tmp.setRect(selection.x(),selection.y(),selection.width(),selection.height());

    dbg.nospace() << "Selection()" << tmp << selection.integral();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const Selection &selection)
{
    QRect tmp;
    tmp.setRect(selection.x(), selection.y(),selection.width(), selection.height());
    
    out << tmp << selection.integral() << selection.weighted_x() << selection.weighted_y() << selection.selected();

    return out;
}

QDataStream &operator>>(QDataStream &in, Selection &selection)
{
    QRect tmp;
    double integral;
    double weighted_x;
    double weighted_y;
    bool selected;

    in >> tmp >> integral >> weighted_x >> weighted_y >> selected;
    selection = tmp;
    selection.setSum(integral);
    selection.setWeightedX(weighted_x);
    selection.setWeightedY(weighted_y);
    selection.setSelected(selected);

    return in;
}
