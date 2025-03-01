
/*LICENSE_START*/
/*
 *  Copyright (C) 2014 Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#include <cmath>

#define __BRAIN_BROWSER_WINDOW_TOOL_BAR_SLICE_SELECTION_DECLARE__
#include "BrainBrowserWindowToolBarSliceSelection.h"
#undef __BRAIN_BROWSER_WINDOW_TOOL_BAR_SLICE_SELECTION_DECLARE__

#include <QAction>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QToolButton>

#include "BrainBrowserWindowToolBar.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "EnumComboBoxTemplate.h"
#include "GuiManager.h"
#include "ModelVolume.h"
#include "ModelWholeBrain.h"
#include "VolumeFile.h"
#include "VolumeSliceProjectionTypeEnum.h"
#include "WuQFactory.h"
#include "WuQWidgetObjectGroup.h"
#include "WuQtUtilities.h"

using namespace caret;


    
/**
 * \class caret::BrainBrowserWindowToolBarSliceSelection 
 * \brief Toolbar component for selection of volume slices.
 * \ingroup GuiQt
 */

/**
 * Constructor.
 */
BrainBrowserWindowToolBarSliceSelection::BrainBrowserWindowToolBarSliceSelection(BrainBrowserWindowToolBar* parentToolBar)
: BrainBrowserWindowToolBarComponent(parentToolBar),
m_parentToolBar(parentToolBar)
{
    QAction* volumeIndicesOriginToolButtonAction = WuQtUtilities::createAction("O\nR\nI\nG\nI\nN",
                                                                               "Set the slice indices to the origin, \n"
                                                                               "stereotaxic coordinate (0, 0, 0)",
                                                                               this,
                                                                               this,
                                                                               SLOT(volumeIndicesOriginActionTriggered()));
    QToolButton* volumeIndicesOriginToolButton = new QToolButton;
    volumeIndicesOriginToolButton->setDefaultAction(volumeIndicesOriginToolButtonAction);
    
    QLabel* parasagittalLabel = new QLabel("P:");
    QLabel* coronalLabel = new QLabel("C:");
    QLabel* axialLabel = new QLabel("A:");
    
    m_volumeIndicesParasagittalCheckBox = new QCheckBox(" ");
    WuQtUtilities::setToolTipAndStatusTip(m_volumeIndicesParasagittalCheckBox,
                                          "Enable/Disable display of PARASAGITTAL slice");
    QObject::connect(m_volumeIndicesParasagittalCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(volumeIndicesParasagittalCheckBoxStateChanged(int)));
    
    m_volumeIndicesCoronalCheckBox = new QCheckBox(" ");
    WuQtUtilities::setToolTipAndStatusTip(m_volumeIndicesCoronalCheckBox,
                                          "Enable/Disable display of CORONAL slice");
    QObject::connect(m_volumeIndicesCoronalCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(volumeIndicesCoronalCheckBoxStateChanged(int)));
    
    m_volumeIndicesAxialCheckBox = new QCheckBox(" ");
    WuQtUtilities::setToolTipAndStatusTip(m_volumeIndicesAxialCheckBox,
                                          "Enable/Disable display of AXIAL slice");
    
    QObject::connect(m_volumeIndicesAxialCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(volumeIndicesAxialCheckBoxStateChanged(int)));
    
    const int sliceIndexSpinBoxWidth = 55;
    const int sliceCoordinateSpinBoxWidth = 60;
    
    m_volumeIndicesParasagittalSpinBox = WuQFactory::newSpinBox();
    m_volumeIndicesParasagittalSpinBox->setFixedWidth(sliceIndexSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(m_volumeIndicesParasagittalSpinBox,
                                          "Change the selected PARASAGITTAL slice");
    QObject::connect(m_volumeIndicesParasagittalSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(volumeIndicesParasagittalSpinBoxValueChanged(int)));
    
    m_volumeIndicesCoronalSpinBox = WuQFactory::newSpinBox();
    m_volumeIndicesCoronalSpinBox->setFixedWidth(sliceIndexSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(m_volumeIndicesCoronalSpinBox,
                                          "Change the selected CORONAL slice");
    QObject::connect(m_volumeIndicesCoronalSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(volumeIndicesCoronalSpinBoxValueChanged(int)));
    
    m_volumeIndicesAxialSpinBox = WuQFactory::newSpinBox();
    m_volumeIndicesAxialSpinBox->setFixedWidth(sliceIndexSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(m_volumeIndicesAxialSpinBox,
                                          "Change the selected AXIAL slice");
    QObject::connect(m_volumeIndicesAxialSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(volumeIndicesAxialSpinBoxValueChanged(int)));
    
    m_volumeIndicesXcoordSpinBox = WuQFactory::newDoubleSpinBox();
    m_volumeIndicesXcoordSpinBox->setDecimals(1);
    m_volumeIndicesXcoordSpinBox->setFixedWidth(sliceCoordinateSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(m_volumeIndicesXcoordSpinBox,
                                          "Adjust coordinate to select PARASAGITTAL slice");
    QObject::connect(m_volumeIndicesXcoordSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(volumeIndicesXcoordSpinBoxValueChanged(double)));
    
    m_volumeIndicesYcoordSpinBox = WuQFactory::newDoubleSpinBox();
    m_volumeIndicesYcoordSpinBox->setDecimals(1);
    m_volumeIndicesYcoordSpinBox->setFixedWidth(sliceCoordinateSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(m_volumeIndicesYcoordSpinBox,
                                          "Adjust coordinate to select CORONAL slice");
    QObject::connect(m_volumeIndicesYcoordSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(volumeIndicesYcoordSpinBoxValueChanged(double)));
    
    m_volumeIndicesZcoordSpinBox = WuQFactory::newDoubleSpinBox();
    m_volumeIndicesZcoordSpinBox->setDecimals(1);
    m_volumeIndicesZcoordSpinBox->setFixedWidth(sliceCoordinateSpinBoxWidth);
    WuQtUtilities::setToolTipAndStatusTip(m_volumeIndicesZcoordSpinBox,
                                          "Adjust coordinate to select AXIAL slice");
    QObject::connect(m_volumeIndicesZcoordSpinBox, SIGNAL(valueChanged(double)),
                     this, SLOT(volumeIndicesZcoordSpinBoxValueChanged(double)));
    
    m_volumeSliceProjectionTypeEnumComboBox = new EnumComboBoxTemplate(this);
    m_volumeSliceProjectionTypeEnumComboBox->setup<VolumeSliceProjectionTypeEnum,VolumeSliceProjectionTypeEnum::Enum>();
    m_volumeSliceProjectionTypeEnumComboBox->getComboBox()->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    QObject::connect(m_volumeSliceProjectionTypeEnumComboBox, SIGNAL(itemActivated()),
                     this, SLOT(volumeSliceProjectionTypeEnumComboBoxItemActivated()));
    WuQtUtilities::setToolTipAndStatusTip(m_volumeSliceProjectionTypeEnumComboBox->getWidget(),
                                          "Chooses viewing orientation (oblique or orthogonal)");
    
    QGridLayout* gridLayout = new QGridLayout(this);
    WuQtUtilities::setLayoutSpacingAndMargins(gridLayout, 0, 0);
    gridLayout->addWidget(m_volumeIndicesParasagittalCheckBox, 0, 0);
    gridLayout->addWidget(parasagittalLabel, 0, 1);
    gridLayout->addWidget(m_volumeIndicesCoronalCheckBox, 1, 0);
    gridLayout->addWidget(coronalLabel, 1, 1);
    gridLayout->addWidget(m_volumeIndicesAxialCheckBox, 2, 0);
    gridLayout->addWidget(axialLabel, 2, 1);

    gridLayout->addWidget(m_volumeIndicesParasagittalSpinBox, 0, 2);
    gridLayout->addWidget(m_volumeIndicesCoronalSpinBox, 1, 2);
    gridLayout->addWidget(m_volumeIndicesAxialSpinBox, 2, 2);

    gridLayout->addWidget(m_volumeIndicesXcoordSpinBox, 0, 3);
    gridLayout->addWidget(m_volumeIndicesYcoordSpinBox, 1, 3);
    gridLayout->addWidget(m_volumeIndicesZcoordSpinBox, 2, 3);

    gridLayout->addWidget(m_volumeSliceProjectionTypeEnumComboBox->getWidget(), 3, 1, 1, 4, Qt::AlignHCenter);

    gridLayout->addWidget(volumeIndicesOriginToolButton, 0, 4, 3, 1);
    
    m_volumeIndicesWidgetGroup = new WuQWidgetObjectGroup(this);
    m_volumeIndicesWidgetGroup->add(volumeIndicesOriginToolButtonAction);
    m_volumeIndicesWidgetGroup->add(m_volumeIndicesParasagittalCheckBox);
    m_volumeIndicesWidgetGroup->add(m_volumeIndicesParasagittalSpinBox);
    m_volumeIndicesWidgetGroup->add(m_volumeIndicesCoronalCheckBox);
    m_volumeIndicesWidgetGroup->add(m_volumeIndicesCoronalSpinBox);
    m_volumeIndicesWidgetGroup->add(m_volumeIndicesAxialCheckBox);
    m_volumeIndicesWidgetGroup->add(m_volumeIndicesAxialSpinBox);
    m_volumeIndicesWidgetGroup->add(m_volumeIndicesXcoordSpinBox);
    m_volumeIndicesWidgetGroup->add(m_volumeIndicesYcoordSpinBox);
    m_volumeIndicesWidgetGroup->add(m_volumeIndicesZcoordSpinBox);
}

/**
 * Destructor.
 */
BrainBrowserWindowToolBarSliceSelection::~BrainBrowserWindowToolBarSliceSelection()
{
}

/**
 * Update the surface montage options widget.
 *
 * @param browserTabContent
 *   The active model display controller (may be NULL).
 */
void
BrainBrowserWindowToolBarSliceSelection::updateContent(BrowserTabContent* browserTabContent)
{
    m_volumeIndicesWidgetGroup->blockAllSignals(true);
    
    const int32_t tabIndex = browserTabContent->getTabNumber();
    
    VolumeMappableInterface* vf = NULL;
    ModelVolume* volumeModel = browserTabContent->getDisplayedVolumeModel();
    if (volumeModel != NULL) {
        if (m_parentToolBar->getDisplayedModel() == volumeModel) {
            vf = volumeModel->getUnderlayVolumeFile(tabIndex);
            m_volumeIndicesAxialCheckBox->setVisible(false);
            m_volumeIndicesCoronalCheckBox->setVisible(false);
            m_volumeIndicesParasagittalCheckBox->setVisible(false);
        }
    }
    
    ModelWholeBrain* wholeBrainModel = browserTabContent->getDisplayedWholeBrainModel();
    if (wholeBrainModel != NULL) {
        if (m_parentToolBar->getDisplayedModel() == wholeBrainModel) {
            vf = wholeBrainModel->getUnderlayVolumeFile(tabIndex);
            m_volumeIndicesAxialCheckBox->setVisible(true);
            m_volumeIndicesCoronalCheckBox->setVisible(true);
            m_volumeIndicesParasagittalCheckBox->setVisible(true);
        }
    }
    
    if (vf != NULL) {
        m_volumeIndicesAxialCheckBox->setChecked(browserTabContent->isSliceAxialEnabled());
        m_volumeIndicesCoronalCheckBox->setChecked(browserTabContent->isSliceCoronalEnabled());
        m_volumeIndicesParasagittalCheckBox->setChecked(browserTabContent->isSliceParasagittalEnabled());
    }
    
    m_volumeSliceProjectionTypeEnumComboBox->setSelectedItem<VolumeSliceProjectionTypeEnum,VolumeSliceProjectionTypeEnum::Enum>(browserTabContent->getSliceProjectionType());
    
    this->updateSliceIndicesAndCoordinatesRanges();
    
    m_volumeIndicesWidgetGroup->blockAllSignals(false);
}

/*
 * Set the values/minimums/maximums for volume slice indices and coordinate spin controls.
 */
void
BrainBrowserWindowToolBarSliceSelection::updateSliceIndicesAndCoordinatesRanges()
{
    const bool blockedStatus = m_volumeIndicesWidgetGroup->signalsBlocked();
    m_volumeIndicesWidgetGroup->blockAllSignals(true);
    
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const int32_t tabIndex = btc->getTabNumber();
    
    VolumeMappableInterface* vf = NULL;
    ModelVolume* volumeModel = btc->getDisplayedVolumeModel();
    if (volumeModel != NULL) {
        vf = volumeModel->getUnderlayVolumeFile(tabIndex);
    }
    
    ModelWholeBrain* wholeBrainModel = btc->getDisplayedWholeBrainModel();
    if (wholeBrainModel != NULL) {
        vf = wholeBrainModel->getUnderlayVolumeFile(tabIndex);
    }
    
    if (vf != NULL) {
        m_volumeIndicesAxialSpinBox->setEnabled(true);
        m_volumeIndicesCoronalSpinBox->setEnabled(true);
        m_volumeIndicesParasagittalSpinBox->setEnabled(true);
        
        std::vector<int64_t> dimensions;
        vf->getDimensions(dimensions);
        
        /*
         * Setup minimum and maximum slices for each dimension.
         * Range is unlimited when Yoked.
         */
        int minAxialDim = 0;
        int minCoronalDim = 0;
        int minParasagittalDim = 0;
        int maxAxialDim = (dimensions[2] > 0) ? (dimensions[2] - 1) : 0;
        int maxCoronalDim = (dimensions[1] > 0) ? (dimensions[1] - 1) : 0;
        int maxParasagittalDim = (dimensions[0] > 0) ? (dimensions[0] - 1) : 0;
        
        m_volumeIndicesAxialSpinBox->setRange(minAxialDim,
                                                  maxAxialDim);
        m_volumeIndicesCoronalSpinBox->setRange(minCoronalDim,
                                                    maxCoronalDim);
        m_volumeIndicesParasagittalSpinBox->setRange(minParasagittalDim,
                                                         maxParasagittalDim);
        
        
        /*
         * Setup minimum and maximum coordinates for each dimension.
         * Range is unlimited when Yoked.
         */
        int64_t slicesZero[3] = { 0, 0, 0 };
        float sliceZeroCoords[3];
        vf->indexToSpace(slicesZero,
                         sliceZeroCoords);
        int64_t slicesMax[3] = { maxParasagittalDim, maxCoronalDim, maxAxialDim };
        float sliceMaxCoords[3];
        vf->indexToSpace(slicesMax,
                         sliceMaxCoords);
        
        m_volumeIndicesXcoordSpinBox->setMinimum(std::min(sliceZeroCoords[0],
                                                              sliceMaxCoords[0]));
        m_volumeIndicesYcoordSpinBox->setMinimum(std::min(sliceZeroCoords[1],
                                                              sliceMaxCoords[1]));
        m_volumeIndicesZcoordSpinBox->setMinimum(std::min(sliceZeroCoords[2],
                                                              sliceMaxCoords[2]));
        
        m_volumeIndicesXcoordSpinBox->setMaximum(std::max(sliceZeroCoords[0],
                                                              sliceMaxCoords[0]));
        m_volumeIndicesYcoordSpinBox->setMaximum(std::max(sliceZeroCoords[1],
                                                              sliceMaxCoords[1]));
        m_volumeIndicesZcoordSpinBox->setMaximum(std::max(sliceZeroCoords[2],
                                                              sliceMaxCoords[2]));
        
        int64_t slicesOne[3] = { 1, 1, 1 };
        float slicesOneCoords[3];
        vf->indexToSpace(slicesOne,
                         slicesOneCoords);
        const float dx = std::fabs(slicesOneCoords[0] - sliceZeroCoords[0]);
        const float dy = std::fabs(slicesOneCoords[1] - sliceZeroCoords[1]);
        const float dz = std::fabs(slicesOneCoords[2] - sliceZeroCoords[2]);
        m_volumeIndicesXcoordSpinBox->setSingleStep(dx);
        m_volumeIndicesYcoordSpinBox->setSingleStep(dy);
        m_volumeIndicesZcoordSpinBox->setSingleStep(dz);
        
        m_volumeIndicesAxialSpinBox->setValue(btc->getSliceIndexAxial(vf));
        m_volumeIndicesCoronalSpinBox->setValue(btc->getSliceIndexCoronal(vf));
        m_volumeIndicesParasagittalSpinBox->setValue(btc->getSliceIndexParasagittal(vf));
        
        int64_t slices[3] = {
            btc->getSliceIndexParasagittal(vf),
            btc->getSliceIndexCoronal(vf),
            btc->getSliceIndexAxial(vf)
        };
        float sliceCoords[3] = { 0.0, 0.0, 0.0 };
        if (vf != NULL) {
            vf->indexToSpace(slices,
                             sliceCoords);
        }
        m_volumeIndicesXcoordSpinBox->setValue(btc->getSliceCoordinateParasagittal());
        m_volumeIndicesYcoordSpinBox->setValue(btc->getSliceCoordinateCoronal());
        m_volumeIndicesZcoordSpinBox->setValue(btc->getSliceCoordinateAxial());
        
    }
    
    m_volumeIndicesWidgetGroup->blockAllSignals(blockedStatus);
}

/**
 * Called when volume indices ORIGIN tool button is pressed.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesOriginActionTriggered()
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->setSlicesToOrigin();
    
    updateContent(btc);
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume indices parasagittal check box is toggled.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesParasagittalCheckBoxStateChanged(int /*state*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->setSliceParasagittalEnabled(m_volumeIndicesParasagittalCheckBox->isChecked());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume indices coronal check box is toggled.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesCoronalCheckBoxStateChanged(int /*state*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->setSliceCoronalEnabled(m_volumeIndicesCoronalCheckBox->isChecked());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume indices axial check box is toggled.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesAxialCheckBoxStateChanged(int /*state*/)
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    btc->setSliceAxialEnabled(m_volumeIndicesAxialCheckBox->isChecked());
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when volume indices parasagittal spin box value is changed.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesParasagittalSpinBoxValueChanged(int /*i*/)
{
    this->readVolumeSliceIndicesAndUpdateSliceCoordinates();
}

/**
 * Called when volume indices coronal spin box value is changed.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesCoronalSpinBoxValueChanged(int /*i*/)
{
    this->readVolumeSliceIndicesAndUpdateSliceCoordinates();
}

/**
 * Called when volume indices axial spin box value is changed.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesAxialSpinBoxValueChanged(int /*i*/)
{
    this->readVolumeSliceIndicesAndUpdateSliceCoordinates();
}

/**
 * Called when X stereotaxic coordinate is changed.
 * @param d
 *    New value.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesXcoordSpinBoxValueChanged(double /*d*/)
{
    this->readVolumeSliceCoordinatesAndUpdateSliceIndices();
}

/**
 * Called when Y stereotaxic coordinate is changed.
 * @param d
 *    New value.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesYcoordSpinBoxValueChanged(double /*d*/)
{
    this->readVolumeSliceCoordinatesAndUpdateSliceIndices();
}

/**
 * Called when Z stereotaxic coordinate is changed.
 * @param d
 *    New value.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeIndicesZcoordSpinBoxValueChanged(double /*d*/)
{
    this->readVolumeSliceCoordinatesAndUpdateSliceIndices();
}

/**
 * Read the slice indices and update the slice coordinates.
 */
void
BrainBrowserWindowToolBarSliceSelection::readVolumeSliceIndicesAndUpdateSliceCoordinates()
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const int32_t tabIndex = btc->getTabNumber();
    
    VolumeMappableInterface* underlayVolumeFile = NULL;
    ModelWholeBrain* wholeBrainModel = btc->getDisplayedWholeBrainModel();
    if (wholeBrainModel != NULL) {
        if (m_parentToolBar->getDisplayedModel() == wholeBrainModel) {
            underlayVolumeFile = wholeBrainModel->getUnderlayVolumeFile(tabIndex);
        }
    }
    
    ModelVolume* volumeModel = btc->getDisplayedVolumeModel();
    if (volumeModel != NULL) {
        if (m_parentToolBar->getDisplayedModel() == volumeModel) {
            underlayVolumeFile = volumeModel->getUnderlayVolumeFile(tabIndex);
        }
    }
    
    if (underlayVolumeFile != NULL) {
        const int64_t parasagittalSlice = m_volumeIndicesParasagittalSpinBox->value();
        const int64_t coronalSlice      = m_volumeIndicesCoronalSpinBox->value();
        const int64_t axialSlice        = m_volumeIndicesAxialSpinBox->value();
        btc->setSliceIndexAxial(underlayVolumeFile, axialSlice);
        btc->setSliceIndexCoronal(underlayVolumeFile, coronalSlice);
        btc->setSliceIndexParasagittal(underlayVolumeFile, parasagittalSlice);
    }
    
    this->updateSliceIndicesAndCoordinatesRanges();
    
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Read the slice coordinates and convert to slices indices and then
 * update the displayed slices.
 */
void
BrainBrowserWindowToolBarSliceSelection::readVolumeSliceCoordinatesAndUpdateSliceIndices()
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const int32_t tabIndex = btc->getTabNumber();
    
    VolumeMappableInterface* underlayVolumeFile = NULL;
    ModelWholeBrain* wholeBrainModel = btc->getDisplayedWholeBrainModel();
    if (wholeBrainModel != NULL) {
        if (m_parentToolBar->getDisplayedModel() == wholeBrainModel) {
            underlayVolumeFile = wholeBrainModel->getUnderlayVolumeFile(tabIndex);
        }
    }
    
    ModelVolume* volumeModel = btc->getDisplayedVolumeModel();
    if (volumeModel != NULL) {
        if (m_parentToolBar->getDisplayedModel() == volumeModel) {
            underlayVolumeFile = volumeModel->getUnderlayVolumeFile(tabIndex);
        }
    }
    
    if (underlayVolumeFile != NULL) {
        float sliceCoords[3] = {
            m_volumeIndicesXcoordSpinBox->value(),
            m_volumeIndicesYcoordSpinBox->value(),
            m_volumeIndicesZcoordSpinBox->value()
        };
        
        btc->selectSlicesAtCoordinate(sliceCoords);
    }
    
    this->updateSliceIndicesAndCoordinatesRanges();
    
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}

/**
 * Called when projection type is changed.
 */
void
BrainBrowserWindowToolBarSliceSelection::volumeSliceProjectionTypeEnumComboBoxItemActivated()
{
    BrowserTabContent* btc = this->getTabContentFromSelectedTab();
    const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType = m_volumeSliceProjectionTypeEnumComboBox->getSelectedItem<VolumeSliceProjectionTypeEnum,VolumeSliceProjectionTypeEnum::Enum>();
    btc->setSliceProjectionType(sliceProjectionType);
    this->updateGraphicsWindow();
    this->updateOtherYokedWindows();
}


