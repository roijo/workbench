
/****************************************************************************
**
** Copyright (C) 1992-2007 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt3Support module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#define __WU_Q_DOUBLE_SLIDER_DECLARE__
#include "WuQDoubleSlider.h"
#undef __WU_Q_DOUBLE_SLIDER_DECLARE__

#include <QSlider>

using namespace caret;


    
/**
 * \class WuQDoubleSlider 
 * \brief A slider for real values.
 *
 * Creates a slider for real values by encapsulating
 * a QSlider that operates only on integer values.
 */

/**
 * Constructor.
 *
 * @param orientation
 *   horizontal or vertical
 * @param parent
 *   optional parent
 */
WuQDoubleSlider::WuQDoubleSlider(Qt::Orientation orientation,
                                 QWidget* parent)
: QObject(parent)
{
    this->slider = new QSlider(orientation,
                               parent);
    this->slider->setRange(0, 1000);
    QObject::connect(this->slider, SIGNAL(valueChanged(int)),
                     this, SLOT(qSliderValueChanged(int)));
    
    this->setRange(-100.0, 100.0);    
    this->setValue(0);
}

/**
 * Destructor.
 */
WuQDoubleSlider::~WuQDoubleSlider()
{
    
}

/**
 * Called when the encapsulated QSlider
 * value is changed.
 * @param value
 *    New value.
 */
void 
WuQDoubleSlider::qSliderValueChanged(int value)
{
    const double dSlider = this->slider->maximum() - this->slider->minimum();
    const double parametricValue = (static_cast<double>(value) - this->slider->minimum()) / dSlider;

    double dRange = this->maximum - this->minimum;
    if (dRange == 0.0) {
        dRange = 1.0;
    }
    this->sliderValue = dRange * parametricValue + this->minimum;
    
    emit valueChanged(this->sliderValue);
}

/**
 * @return  The widget that is enapsulated.
 */
QWidget* 
WuQDoubleSlider::getWidget()
{
    return this->slider;
}

/**
 * Set range of values.
 * @param minValue
 *    New value for minimum.
 * @param maxValue
 *    New value for maximum.
 */
void 
WuQDoubleSlider::setRange(double minValue, double maxValue)
{
    this->minimum = minValue;
    this->maximum = maxValue;
    if (this->minimum > this->maximum) {
        this->maximum = this->minimum;
    }
    this->updateSlider();
}

/**
 * @return  The value of the slider.
 */
double 
WuQDoubleSlider::value() const
{
    return this->sliderValue;
}

/**
 * Set the value for the slider.
 * @param d
 *    New value.
 */
void 
WuQDoubleSlider::setValue(double valueIn)
{
    this->sliderValue = valueIn;
    this->updateSlider();
    
    emit valueChanged(this->sliderValue);
}

/**
 * Update the encapsulated slider.
 */
void 
WuQDoubleSlider::updateSlider()
{
    double dRange = this->maximum - this->minimum;
    if (dRange == 0.0) {
        dRange = 1.0;
    }
    
    if (this->sliderValue > this->maximum) {
        this->sliderValue = this->maximum;
    }
    if (this->sliderValue < this->minimum) {
        this->sliderValue = this->minimum;
    }
    
    const double parametricValue = (this->sliderValue - this->minimum) / dRange;
    
    const int dSlider = this->slider->maximum() - this->slider->minimum();
    const int qSliderValue = static_cast<int>(dSlider * parametricValue) + this->minimum;
    this->slider->blockSignals(true);
    this->slider->setValue(qSliderValue);
    this->slider->blockSignals(false);    
}


