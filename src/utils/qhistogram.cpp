#include "qhistogram.h"

//~ QHistogram::QHistogram()
//~ {
    //~ this->setObjectName("QHistogram");
//~ }

void QHistogram::historize(QString label, MiniArray<float> * data, int n_bins, int stride, int offset, float lim_low_x, float lim_high_x, float lim_low_y, float lim_high_y, bool auto_lim_low_x,  bool auto_lim_high_x, bool auto_lim_low_y,  bool auto_lim_high_y, bool log_x, bool log_y)
{
    if (data->size() > 0.2e9) return;
    
    // Decide x limits
    float tmp_x_max = std::numeric_limits<float>::min();
    float tmp_x_min = std::numeric_limits<float>::max();
    
    if (auto_lim_low_x || auto_lim_high_x)
    {
        for (size_t i = 0; i < data->size(); i++)
        {
            if (data->at(i) > tmp_x_max) tmp_x_max = data->at(i);
            if (data->at(i) < tmp_x_min) tmp_x_min = data->at(i);
        }
        if (auto_lim_low_x)
        {
            lim_low_x = tmp_x_min;
        }
        if (auto_lim_high_x)
        {
            lim_high_x = tmp_x_max;
        }
    }
    // Populate the bins
    bins.reserve(n_bins);
    for (int i = 0; i < bins.size(); i++)
    {
        bins[i] = 0;
    }
    int bin;
    if (log_x)
    {
        if (lim_low_x <= 1) lim_low_x = 1;
        for (size_t i = 0; i < data->size(); i++)
        {
            if (data->at(i) <= 1+lim_low_x) continue;
            bin = (int) (log10f(data->at(i) - lim_low_x)/log10f(lim_high_x - lim_low_x) * (float) n_bins);
            if (bin >= n_bins) bin = n_bins - 1;
            if (bin < 0)
            { 
                
                qDebug() << bin << " " << data->at(i) << " " << lim_low_x << " " << lim_high_x;
                bin = 0;
            }
            bins[bin] += 1.0;
        }
        lim_low_x = log10f(lim_low_x);
        lim_high_x = log10f(lim_high_x);
    }
    else
    {
        for (size_t i = 0; i < data->size(); i++)
        {
            bin = (data->at(i) - lim_low_x)/(lim_high_x - lim_low_x) * n_bins;
            if (bin >= n_bins) bin = n_bins - 1;
            bins[bin] += 1.0;
        }
    }
    if (log_y)
    {
        if (lim_low_y <= 1) lim_low_y = 1;
        
        for (int i = 0; i < (int)  bins.size(); i++)
        {
            if (bins[i] < 1) bins[i] = 1;
            
            bins[i] = log10f(bins[i]);
        }
        lim_low_y = log10f(lim_low_y);
        lim_high_y = log10f(lim_high_y);
    }
    // Decide y limits
    float tmp_y_max = std::numeric_limits<float>::min();
    float tmp_y_min = std::numeric_limits<float>::max();
    
    if (auto_lim_low_y || auto_lim_high_y)
    {
        for (int i = 0; i < (int) bins.size(); i++)
        {
            if (bins[i] > tmp_y_max) tmp_y_max = bins[i];
            if (bins[i] < tmp_y_min) tmp_y_min = bins[i];
        }
        if (auto_lim_low_y)
        {
            lim_low_y = tmp_y_min;
        }
        if (auto_lim_high_y)
        {
            lim_high_y = tmp_y_max;
        }
    } 
    this->lim_low_x = lim_low_x;
    this->lim_low_y = lim_low_y;
    this->lim_high_x = lim_high_x;
    this->lim_high_y = lim_high_y;
    this->log_x = log_x;
    this->log_y = log_y;
    this->label = label;
    update();
}

void QHistogram::historize(QString label, QList<file> data, int n_bins, float lim_low_x, float lim_high_x, float lim_low_y, float lim_high_y, bool auto_lim_low_x,  bool auto_lim_high_x, bool auto_lim_low_y,  bool auto_lim_high_y, bool log_x, bool log_y)
{
    
    //qDebug("Decide x limits");
    // Decide x limits
    float tmp_x_max = std::numeric_limits<float>::min();
    float tmp_x_min = std::numeric_limits<float>::max();
    
    //~ std::cout << "m1" << std::endl;
    if (auto_lim_low_x || auto_lim_high_x)
    {
        for (int i = 0; i < (int) data.size(); i++)
        {
            for (int j = 0; j < (int) data[i].intensity.size(); j++)
            {
                if (data[i].intensity[j] > tmp_x_max) tmp_x_max = data[i].intensity[j];
                if (data[i].intensity[j] < tmp_x_min) tmp_x_min = data[i].intensity[j];
            }
        }
        if (auto_lim_low_x)
        {
            lim_low_x = tmp_x_min;
        }
        if (auto_lim_high_x)
        {
            lim_high_x = tmp_x_max;
        }
    }
    //~ std::cout << "m2" << std::endl;
    //qDebug("Populate the bins");
    // Populate the bins
    bins.reserve(n_bins);
    for (int i = 0; i < bins.size(); i++)
    {
        bins[i] = 0;
    }
    
    int bin;
    if (log_x)
    {
        if (lim_low_x <= 1) lim_low_x = 1;
        
        for (int i = 0; i < (int) data.size(); i++)
        {
            for (int j = 0; j < (int) data[i].intensity.size(); j++)
            {
                if (data[i].intensity[j] <= 1+lim_low_x) continue;
                bin = (int) (log10f(data[i].intensity[j] - lim_low_x)/log10f(lim_high_x - lim_low_x) * (float) n_bins);
                if (bin >= n_bins) bin = n_bins - 1;
                bins[bin] += 1.0;
                
            }
        }
        
        lim_low_x = log10f(lim_low_x);
        lim_high_x = log10f(lim_high_x);
    }
    else
    {
        for (int i = 0; i < (int) data.size(); i++)
        {
            for (int j = 0; j < (int) data[i].intensity.size(); j++)
            {
                bin = (data[i].intensity[j] - lim_low_x)/(lim_high_x - lim_low_x) * n_bins;
                if (bin >= n_bins) bin = n_bins - 1;
                bins[bin] += 1.0;
            }
        }
    }
    //~ std::cout << "m3" << std::endl;
    if (log_y)
    {
        if (lim_low_y <= 1) lim_low_y = 1;
        
        for (int i = 0; i < (int)  bins.size(); i++)
        {
            if (bins[i] < 1) bins[i] = 1;
            bins[i] = log10f(bins[i]);
            //std::cout << bins[i] << std::endl;
        }
        lim_low_y = log10f(lim_low_y);
        lim_high_y = log10f(lim_high_y);
    }
    
    //qDebug("Decide y limits");
    // Decide y limits
    float tmp_y_max = std::numeric_limits<float>::min();
    float tmp_y_min = std::numeric_limits<float>::max();
    //~ std::cout << "m4" << std::endl;
    if (auto_lim_low_y || auto_lim_high_y)
    {
        for (int i = 0; i < (int) bins.size(); i++)
        {
            if (bins[i] > tmp_y_max) tmp_y_max = bins[i];
            if (bins[i] < tmp_y_min) tmp_y_min = bins[i];
        }
        if (auto_lim_low_y)
        {
            lim_low_y = tmp_y_min;
        }
        if (auto_lim_high_y)
        {
            lim_high_y = tmp_y_max;
        }
    } 
    
    //qDebug("Update");
    
    /*std::cout << "lim_low_x " << lim_low_x << std::endl; 
    std::cout << "lim_high_x " << lim_high_x << std::endl; 
    std::cout << "lim_low_y " << lim_low_y << std::endl; 
    std::cout << "lim_high_y " << lim_high_y << std::endl; */
    
    
    this->lim_low_x = lim_low_x;
    this->lim_low_y = lim_low_y;
    this->lim_high_x = lim_high_x;
    this->lim_high_y = lim_high_y;
    this->log_x = log_x;
    this->log_y = log_y;
    this->label = label;
    
    update();
    //~ std::cout << "m5" << std::endl;
}

void QHistogram::historize(QString label, MiniArray<float> *  data_x, MiniArray<float> *  data_y, int n_bins, int stride_x, int stride_y, int offset_x, int offset_y, float lim_low_x, float lim_high_x, float lim_low_y, float lim_high_y, bool auto_lim_low_x,  bool auto_lim_high_x, bool auto_lim_low_y,  bool auto_lim_high_y, bool log_x, bool log_y)
{
    if (data_x->size() > 0.2e9) return;
    
    // Decide x limits
    //~ qDebug("Decide x limits");
    float tmp_x_max = std::numeric_limits<float>::min();
    float tmp_x_min = std::numeric_limits<float>::max();
    
    if (auto_lim_low_x || auto_lim_high_x)
    {
        for (int i = 0; i < (int) data_x->size()/stride_x; i++)
        {
            if (data_x->at(i*stride_x+offset_x) > tmp_x_max) tmp_x_max = data_x->at(i*stride_x+offset_x);
            if (data_x->at(i*stride_x+offset_x) < tmp_x_min) tmp_x_min = data_x->at(i*stride_x+offset_x);
        }
        if (auto_lim_low_x)
        {
            lim_low_x = tmp_x_min;
        }
        if (auto_lim_high_x)
        {
            lim_high_x = tmp_x_max;
        }
    }
    
    // Populate the bins
    bins.reserve(n_bins);
    for (int i = 0; i < bins.size(); i++)
    {
        bins[i] = 0;
    }
    
    int bin;
    for (size_t i = 0; i <  data_x->size()/stride_x; i++)
    {
        //~ qDebug("Populate the bins A");
        bin = (data_x->at(i*stride_x+offset_x) - lim_low_x)/(lim_high_x - lim_low_x) * n_bins;
        //~ qDebug("Populate the bins B");
        if (bin >= n_bins) bin = n_bins - 1;
        if (bin < 0) continue;
        //~ qDebug("Populate the bins C");
        //~ std::cout << bin << " " << i*stride_y+offset_y << " " << data_x[i*stride_x+offset_x] << " " << lim_high_x << " " << lim_low_x<< std::endl;
        bins[bin] += data_y->at(i*stride_y+offset_y);
        //~ qDebug("Populate the bins D");
    }
    
    //~ qDebug("Logarize y");
    if (log_y)
    {
        if (lim_low_y <= 1) lim_low_y = 1;
        
        for (int i = 0; i < (int)  bins.size(); i++)
        {
            if (bins[i] < 1) bins[i] = 1;
            
            bins[i] = log10f(bins[i]);
            
        }
        lim_low_y = log10f(lim_low_y);
        lim_high_y = log10f(lim_high_y);
    }
    
    /*for (int i = 0; i < (int)  bins.size(); i++)
    {
        std::cout << bins[i] << std::endl;
    }*/
    
    // Decide y limits
    //~ qDebug("Decide y limits");
    float tmp_y_max = std::numeric_limits<float>::min();
    float tmp_y_min = std::numeric_limits<float>::max();
    
    if (auto_lim_low_y || auto_lim_high_y)
    {
        for (int i = 0; i < bins.size(); i++)
        {
            if (bins[i] > tmp_y_max) tmp_y_max = bins[i];
            if (bins[i] < tmp_y_min) tmp_y_min = bins[i];
        }
        if (auto_lim_low_y)
        {
            lim_low_y = tmp_y_min;
        }
        if (auto_lim_high_y)
        {
            lim_high_y = tmp_y_max;
        }
    } 
    
    this->lim_low_x = lim_low_x;
    this->lim_low_y = lim_low_y;
    this->lim_high_x = lim_high_x;
    this->lim_high_y = lim_high_y;
    this->log_x = log_x;
    this->log_y = log_y;
    this->label = label;
    //~ qDebug("Update");
    update();
}

void QHistogram::paintEvent(QPaintEvent*)
{
    //~ qDebug("Paint");
    QPainter painter(this);
    
    QRect viewPort = rect();
    float xLeft = viewPort.left();
    float xRight = viewPort.right();
    float yTop = viewPort.top();
    float yBottom = viewPort.bottom();
    float width = viewPort.width();
    float height = viewPort.height();

    QPen pen;
    
    
    
    pen.setColor(Qt::black);
    pen.setWidth(1);
    painter.setPen(pen);

    QBrush brush(Qt::SolidPattern);
    brush.setColor(QColor ( 40, 40, 40, 255 ));
    painter.setBrush(brush);

    // ---- Draw background ------------------------------------------------
    //~ qDebug("Draw background");
    painter.drawRect(xLeft, yTop, xRight, yBottom);

    pen.setColor("#555555");
    painter.setPen(pen);
    if( !bins.size() )
    {
        painter.drawText(xLeft+2, yBottom-2, tr("(CURRENTLY NO DATA)"));
        return;
    }
    
    // Find maximum bin height 
    //~ _heightMax = 1;
    //~ for( int i = 0; i < (int) bins.size(); ++i )
    //~ {
        //~ if( bins[i]>_heightMax ) _heightMax = bins[i];
    //~ }
    
    // ---- Draw vertical lines -------------------------------------------------
    //~ qDebug("Draw vertical lines");
    pen.setColor(QColor(70, 70, 70, 255));
    pen.setStyle(Qt::DashDotLine);
    painter.setPen(pen);
    if (1)
    {
        int first_bar = (int) ceil(lim_low_x);
        int last_bar = (int) floor(lim_high_x);
        int num_bars = last_bar - first_bar;
        if (num_bars > 0)
        {
            for(int i = first_bar; i <= last_bar; i++)
            {
                float x = xLeft + width * (i - lim_low_x)/(lim_high_x - lim_low_x);
                //~ std::cout << x << std::endl;
                painter.drawLine(x, yTop+1, x, yBottom-1);
            }
        }
    }
    
    
    // ---- Draw horizontal lines -----------------------------------------------
    //~ qDebug("Draw horizontal lines");
    if (log_y)
    {
        int first_bar = (int) ceil(lim_low_y);
        int last_bar = (int) floor(lim_high_y);
        int num_bars = last_bar - first_bar;
        if (num_bars > 0)
        {
            for(int i = first_bar; i <= last_bar; i++)
            {
                float y = yBottom - height * (i - lim_low_y)/(lim_high_y - lim_low_y);
                //~ std::cout << xLeft << " " << xRight << " " << yBottom << " " << yTop << std::endl;
                painter.drawLine(xLeft+1, y , xRight-1, y);
            }
        }
    }
    
    // ---- QHistogram  ----------------------------------------------------
    
    // Avoid giggling graph: do not update heightmax if variation <5%
    /*if( abs(_heightMax-heightMax)/float(_heightMax) > 0.05f )
        _heightMax = heightMax;
    */
    //~ qDebug("QHistogram");
    QLinearGradient linearGradient(0, 0, 0, height);
    pen.setStyle(Qt::SolidLine);

    float wScale = width/((float)bins.size()); // pixels per bin
    float hScale = height/((float)(lim_high_y - lim_low_y)); // pixels per height unit

    pen.setColor("#00aaee");   painter.setPen(pen);
    linearGradient.setColorAt(1.0, QColor ( 70, 70, 200, 150 ));
    linearGradient.setColorAt(0.0, QColor ( 0, 0, 200, 210 ));
    painter.setBrush(linearGradient);

    pen.setColor("#016790");
    painter.setPen(pen);
    painter.setBrush(linearGradient);

    myPolygon.clear();
    myPolygon << QPoint(xRight, yBottom) << QPoint(xLeft, yBottom);
    for( int i = 0; i < (int) bins.size(); ++i )
    {
        myPolygon << QPoint(xLeft+wScale*i, hScale*(lim_high_y-bins[i]));
        myPolygon << QPoint(xLeft+wScale*(i+1), hScale*(lim_high_y-bins[i]));
    }
    //std::cout << hScale  << " " << wScale << std::endl;
    painter.drawPolygon(myPolygon);
    
    pen.setColor(QColor ( 100, 150, 230, 180 ));
    painter.setPen(pen);
    painter.drawText(xLeft+2, yBottom-2, label);
    //~ qDebug("Done!");
}

