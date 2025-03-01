
/*LICENSE_START*/
/*
 *  Copyright (C) 2014  Washington University School of Medicine
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

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
//#include <QTabWidget>
#include <QToolButton>

#define __LABEL_SELECTION_VIEW_CONTROLLER_DECLARE__
#include "LabelSelectionViewController.h"
#undef __LABEL_SELECTION_VIEW_CONTROLLER_DECLARE__

#include "Brain.h"
#include "BrainOpenGL.h"
#include "BrainStructure.h"
#include "BrowserTabContent.h"
#include "CaretAssert.h"
#include "GroupAndNameHierarchyViewController.h"
#include "DisplayGroupEnumComboBox.h"
#include "DisplayPropertiesLabels.h"
#include "EventGraphicsUpdateAllWindows.h"
#include "EventManager.h"
#include "EventSurfaceColoringInvalidate.h"
#include "EventUserInterfaceUpdate.h"
#include "GuiManager.h"
#include "SceneClass.h"
#include "VolumeFile.h"
#include "WuQDataEntryDialog.h"
#include "WuQTabWidget.h"
#include "WuQtUtilities.h"

using namespace caret;


    
/**
 * \class caret::LabelSelectionViewController 
 * \brief Widget for controlling display of labels
 * \ingroup GuiQt
 *
 * Widget for controlling the display of labels including
 * different display groups.
 */

/**
 * Constructor.
 */
LabelSelectionViewController::LabelSelectionViewController(const int32_t browserWindowIndex,
                                             QWidget* parent)
: QWidget(parent)
{
    m_browserWindowIndex = browserWindowIndex;
    
    QLabel* groupLabel = new QLabel("Group");
    m_labelsDisplayGroupComboBox = new DisplayGroupEnumComboBox(this);
    QObject::connect(m_labelsDisplayGroupComboBox, SIGNAL(displayGroupSelected(const DisplayGroupEnum::Enum)),
                     this, SLOT(labelDisplayGroupSelected(const DisplayGroupEnum::Enum)));
    
    QHBoxLayout* groupLayout = new QHBoxLayout();
    groupLayout->addWidget(groupLabel);
    groupLayout->addWidget(m_labelsDisplayGroupComboBox->getWidget());
    groupLayout->addStretch(); 
    
    QWidget* attributesWidget = this->createAttributesWidget();
    QWidget* selectionWidget = this->createSelectionWidget();
    
    m_tabWidget = new WuQTabWidget(WuQTabWidget::TAB_ALIGN_LEFT,
                                               this);
    m_tabWidget->addTab(attributesWidget, 
                      "Attributes");
    m_tabWidget->addTab(selectionWidget, 
                      "Selection");
    m_tabWidget->setCurrentWidget(attributesWidget);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(groupLayout);
    layout->addWidget(m_tabWidget->getWidget(), 0, Qt::AlignLeft);
    layout->addStretch();
    
    EventManager::get()->addEventListener(this, EventTypeEnum::EVENT_USER_INTERFACE_UPDATE);
    
    LabelSelectionViewController::allLabelSelectionViewControllers.insert(this);
}

/**
 * Destructor.
 */
LabelSelectionViewController::~LabelSelectionViewController()
{
    EventManager::get()->removeAllEventsFromListener(this);
    
    LabelSelectionViewController::allLabelSelectionViewControllers.erase(this);
}


QWidget* 
LabelSelectionViewController::createSelectionWidget()
{
    m_labelClassNameHierarchyViewController = new GroupAndNameHierarchyViewController(m_browserWindowIndex);
    
    return m_labelClassNameHierarchyViewController;
}

/**
 * @return The attributes widget.
 */
QWidget* 
LabelSelectionViewController::createAttributesWidget()
{
    QLabel* drawAsLabel = new QLabel("Draw as: ");
    m_labelDrawingTypeComboBox = new EnumComboBoxTemplate(this);
    QObject::connect(m_labelDrawingTypeComboBox, SIGNAL(itemActivated()),
                     this, SLOT(processAttributesChanges()));
    m_labelDrawingTypeComboBox->setup<LabelDrawingTypeEnum, LabelDrawingTypeEnum::Enum>();
    
    QLabel* outlineLabel = new QLabel("Outline Color");
    m_labelOutlineColorComboBox = new EnumComboBoxTemplate(this);
    m_labelOutlineColorComboBox->setup<CaretColorEnum, CaretColorEnum::Enum>();
    QObject::connect(m_labelOutlineColorComboBox, SIGNAL(itemActivated()),
                     this, SLOT(processAttributesChanges()));
    
    QWidget* gridWidget = new QWidget();
    QGridLayout* gridLayout = new QGridLayout(gridWidget);
    WuQtUtilities::setLayoutSpacingAndMargins(gridLayout, 8, 2);
    int row = gridLayout->rowCount();
    gridLayout->addWidget(drawAsLabel, row, 0);
    gridLayout->addWidget(m_labelDrawingTypeComboBox->getWidget(), row, 1);
    row++;
    gridLayout->addWidget(outlineLabel, row, 0);
    gridLayout->addWidget(m_labelOutlineColorComboBox->getWidget(), row, 1);
    row++;
    gridWidget->setSizePolicy(QSizePolicy::Fixed,
                              QSizePolicy::Fixed);
    
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->addWidget(gridWidget);
    layout->addStretch();
        
    return widget;
}

/**
 * Called when a widget on the attributes page has 
 * its value changed.
 */
void 
LabelSelectionViewController::processAttributesChanges()
{
    DisplayPropertiesLabels* dpl = GuiManager::get()->getBrain()->getDisplayPropertiesLabels();
    const LabelDrawingTypeEnum::Enum labelDrawingType = m_labelDrawingTypeComboBox->getSelectedItem<LabelDrawingTypeEnum, LabelDrawingTypeEnum::Enum>();
    const CaretColorEnum::Enum outlineColor = m_labelOutlineColorComboBox->getSelectedItem<CaretColorEnum, CaretColorEnum::Enum>();

    BrowserTabContent* browserTabContent = 
    GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, true);
    if (browserTabContent == NULL) {
        return;
    }
    const int32_t browserTabIndex = browserTabContent->getTabNumber();
    const DisplayGroupEnum::Enum displayGroup = dpl->getDisplayGroupForTab(browserTabIndex);

    dpl->setDrawingType(displayGroup,
                        browserTabIndex,
                        labelDrawingType);
    dpl->setOutlineColor(displayGroup,
                         browserTabIndex,
                         outlineColor);
    
    EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
    
    processLabelSelectionChanges();
}

/**
 * Called when the label display group combo box is changed.
 */
void 
LabelSelectionViewController::labelDisplayGroupSelected(const DisplayGroupEnum::Enum displayGroup)
{
    /*
     * Update selected display group in model.
     */
    BrowserTabContent* browserTabContent = 
    GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, false);
    if (browserTabContent == NULL) {
        return;
    }
    
    const int32_t browserTabIndex = browserTabContent->getTabNumber();
    Brain* brain = GuiManager::get()->getBrain();
    DisplayPropertiesLabels* dsb = brain->getDisplayPropertiesLabels();
    dsb->setDisplayGroupForTab(browserTabIndex,
                         displayGroup);
    
    /*
     * Since display group has changed, need to update controls
     */
    updateLabelViewController();
    
    /*
     * Apply the changes.
     */
    processLabelSelectionChanges();
}

/**
 * Update the label selection widget.
 */
void 
LabelSelectionViewController::updateLabelViewController()
{
    setWindowTitle("Labels");
    
    BrowserTabContent* browserTabContent = 
    GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, true);
    if (browserTabContent == NULL) {
        return;
    }
    
    const int32_t browserTabIndex = browserTabContent->getTabNumber();
    Brain* brain = GuiManager::get()->getBrain();
    DisplayPropertiesLabels* dpb = brain->getDisplayPropertiesLabels();
    const DisplayGroupEnum::Enum displayGroup = dpb->getDisplayGroupForTab(browserTabIndex);
    
    m_labelsDisplayGroupComboBox->setSelectedDisplayGroup(dpb->getDisplayGroupForTab(browserTabIndex));
    
    m_labelDrawingTypeComboBox->setSelectedItem<LabelDrawingTypeEnum, LabelDrawingTypeEnum::Enum>(dpb->getDrawingType(displayGroup,
                                                                    browserTabIndex));
    m_labelOutlineColorComboBox->setSelectedItem<CaretColorEnum,CaretColorEnum::Enum>(dpb->getOutlineColor(displayGroup, browserTabIndex));

    /*
     * Get all of label files.
     */
    std::vector<LabelFile*> allLabelFiles;
    const int numBrainStructures = brain->getNumberOfBrainStructures();
    for (int32_t ibs = 0; ibs < numBrainStructures; ibs++) {
        BrainStructure* brainStructure = brain->getBrainStructure(ibs);
        const int32_t numLabelFiles = brainStructure->getNumberOfLabelFiles();
        for (int32_t ilf = 0; ilf < numLabelFiles; ilf++) {
            allLabelFiles.push_back(brainStructure->getLabelFile(ilf));
        }
    }
    
    /*
     * Get all CIFTI label files
     */
    std::vector<CiftiBrainordinateLabelFile*> allCiftiLabelFiles;
    const int32_t numCiftiLabelFiles = brain->getNumberOfConnectivityDenseLabelFiles();
    for (int32_t iclf = 0; iclf < numCiftiLabelFiles; iclf++) {
        allCiftiLabelFiles.push_back(brain->getConnectivityDenseLabelFile(iclf));
    }
    
    /*
     * Get all Volume Files that are mapped with label tables
     */
    std::vector<VolumeFile*> allVolumeLabelFiles;
    const int32_t numVolumeFiles = brain->getNumberOfVolumeFiles();
    for (int32_t iVol = 0; iVol < numVolumeFiles; iVol++) {
        VolumeFile* vf = brain->getVolumeFile(iVol);
        if (vf->isMappedWithLabelTable()) {
            allVolumeLabelFiles.push_back(vf);
        }
    }
    
    /*
     * Update the class/name hierarchy
     */
    m_labelClassNameHierarchyViewController->updateContents(allLabelFiles,
                                                            allCiftiLabelFiles,
                                                            allVolumeLabelFiles,
                                                             displayGroup);
}

/**
 * Update other selection toolbox since they should all be the same.
 */
void 
LabelSelectionViewController::updateOtherLabelViewControllers()
{
    for (std::set<LabelSelectionViewController*>::iterator iter = LabelSelectionViewController::allLabelSelectionViewControllers.begin();
         iter != LabelSelectionViewController::allLabelSelectionViewControllers.end();
         iter++) {
        LabelSelectionViewController* bsw = *iter;
        if (bsw != this) {
            bsw->updateLabelViewController();
        }
    }
}

/**
 * Gets called when label selections are changed.
 */
void 
LabelSelectionViewController::processLabelSelectionChanges()
{
    BrowserTabContent* browserTabContent = 
    GuiManager::get()->getBrowserTabContentForBrowserWindow(m_browserWindowIndex, false);
    CaretAssert(browserTabContent);
    const int32_t browserTabIndex = browserTabContent->getTabNumber();
    Brain* brain = GuiManager::get()->getBrain();
    DisplayPropertiesLabels* dsb = brain->getDisplayPropertiesLabels();
    dsb->setDisplayGroupForTab(browserTabIndex, 
                         m_labelsDisplayGroupComboBox->getSelectedDisplayGroup());
    
    
    processSelectionChanges();
}

/**
 * Issue update events after selections are changed.
 */
void 
LabelSelectionViewController::processSelectionChanges()
{
    updateOtherLabelViewControllers();
    EventManager::get()->sendEvent(EventSurfaceColoringInvalidate().getPointer());
    EventManager::get()->sendEvent(EventGraphicsUpdateAllWindows().getPointer());
}

/**
 * Receive events from the event manager.
 * 
 * @param event
 *   Event sent by event manager.
 */
void 
LabelSelectionViewController::receiveEvent(Event* event)
{
    bool doUpdate = false;
    
    if (event->getEventType() == EventTypeEnum::EVENT_USER_INTERFACE_UPDATE) {
        EventUserInterfaceUpdate* uiEvent = dynamic_cast<EventUserInterfaceUpdate*>(event);
        CaretAssert(uiEvent);
        
        if (uiEvent->isUpdateForWindow(m_browserWindowIndex)) {
            if (uiEvent->isToolBoxUpdate()) {
                doUpdate = true;
                uiEvent->setEventProcessed();
            }
        }
    }

    if (doUpdate) {
        updateLabelViewController();
    }
}

/**
 * Create a scene for an instance of a class.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @return Pointer to SceneClass object representing the state of
 *    this object.  Under some circumstances a NULL pointer may be
 *    returned.  Caller will take ownership of returned object.
 */
SceneClass*
LabelSelectionViewController::saveToScene(const SceneAttributes* sceneAttributes,
                                           const AString& instanceName)
{
    SceneClass* sceneClass = new SceneClass(instanceName,
                                            "LabelSelectionViewController",
                                            1);
    sceneClass->addClass(m_tabWidget->saveToScene(sceneAttributes,
                                                  "m_tabWidget"));
    
    return sceneClass;
}

/**
 * Restore the state of an instance of a class.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     SceneClass containing the state that was previously
 *     saved and should be restored.
 */
void
LabelSelectionViewController::restoreFromScene(const SceneAttributes* sceneAttributes,
                                                const SceneClass* sceneClass)
{
    if (sceneClass == NULL) {
        return;
    }
    
    m_tabWidget->restoreFromScene(sceneAttributes,
                                  sceneClass->getClass("m_tabWidget"));
}



