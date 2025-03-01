
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

#include <cmath>
#include <set>

#define __BROWSER_TAB_CONTENT_DECLARE__
#include "BrowserTabContent.h"
#undef __BROWSER_TAB_CONTENT_DECLARE__

#include "BorderFile.h"
#include "Brain.h"
#include "BrainOpenGLViewportContent.h"
#include "BrainStructure.h"
#include "CaretAssert.h"
#include "CaretDataFileSelectionModel.h"
#include "CaretLogger.h"
#include "ChartData.h"
#include "ChartMatrixDisplayProperties.h"
#include "ChartableMatrixInterface.h"
#include "ChartModelDataSeries.h"
#include "ClippingPlaneGroup.h"
#include "GroupAndNameHierarchyGroup.h"
#include "GroupAndNameHierarchyModel.h"
#include "GroupAndNameHierarchyName.h"
#include "DisplayPropertiesBorders.h"
#include "DisplayPropertiesFoci.h"
#include "EventCaretMappableDataFileMapsViewedInOverlays.h"
#include "EventIdentificationHighlightLocation.h"
#include "EventModelGetAll.h"
#include "EventManager.h"
#include "FociFile.h"
#include "IdentificationManager.h"
#include "LabelFile.h"
#include "MathFunctions.h"
#include "Matrix4x4.h"
#include "ModelChart.h"
#include "ModelSurface.h"
#include "ModelSurfaceMontage.h"
#include "ModelSurfaceSelector.h"
#include "ModelTransform.h"
#include "ModelVolume.h"
#include "ModelWholeBrain.h"
#include "Overlay.h"
#include "OverlaySet.h"
#include "SceneAttributes.h"
#include "SceneClass.h"
#include "SceneClassAssistant.h"
#include "Surface.h"
#include "SurfaceMontageConfigurationCerebellar.h"
#include "SurfaceMontageConfigurationCerebral.h"
#include "SurfaceMontageConfigurationFlatMaps.h"
#include "SurfaceSelectionModel.h"
#include "StructureEnum.h"
#include "VolumeFile.h"
#include "ViewingTransformations.h"
#include "ViewingTransformationsCerebellum.h"
#include "ViewingTransformationsVolume.h"
#include "VolumeSliceSettings.h"
#include "VolumeSurfaceOutlineModel.h"
#include "VolumeSurfaceOutlineSetModel.h"
#include "WholeBrainSurfaceSettings.h"

using namespace caret;

/**
 * Constructor.
 * @param tabNumber
 *    Number for this tab.
 */
BrowserTabContent::BrowserTabContent(const int32_t tabNumber)
: CaretObject()
{
    s_allBrowserTabContent.insert(this);
    
    m_tabNumber = tabNumber;
    m_surfaceModelSelector = new ModelSurfaceSelector();
    m_selectedModelType = ModelTypeEnum::MODEL_TYPE_INVALID;
    m_volumeModel = NULL;
    m_wholeBrainModel = NULL;
    m_surfaceMontageModel = NULL;
    m_chartModel = NULL;
    m_guiName = "";
    m_userName = "";
    m_volumeSurfaceOutlineSetModel = new VolumeSurfaceOutlineSetModel();
    m_yokingGroup = YokingGroupEnum::YOKING_GROUP_OFF;
    m_identificationUpdatesVolumeSlices = false;

    m_cerebellumViewingTransformation  = new ViewingTransformationsCerebellum();
    m_flatSurfaceViewingTransformation = new ViewingTransformations();
    m_viewingTransformation            = new ViewingTransformations();
    m_volumeSliceViewingTransformation = new ViewingTransformationsVolume();
    
    m_wholeBrainSurfaceSettings        = new WholeBrainSurfaceSettings();
    
    m_obliqueVolumeRotationMatrix = new Matrix4x4();
    
    leftView();

    m_volumeSliceSettings = new VolumeSliceSettings();
    
    m_clippingPlaneGroup = new ClippingPlaneGroup();
    
    m_sceneClassAssistant = new SceneClassAssistant();
    m_sceneClassAssistant->add("m_tabNumber", 
                               &m_tabNumber);
    m_sceneClassAssistant->add("m_userName",
                               &m_userName);
    m_sceneClassAssistant->add<ModelTypeEnum, ModelTypeEnum::Enum>("m_selectedModelType",
                                                                   &m_selectedModelType);
    m_sceneClassAssistant->add("m_surfaceModelSelector",
                               "ModelSurfaceSelector",
                               m_surfaceModelSelector);
    m_sceneClassAssistant->add("m_volumeSurfaceOutlineSetModel",
                               "VolumeSurfaceOutlineSetModel",
                               m_volumeSurfaceOutlineSetModel);
    
    m_sceneClassAssistant->add("m_clippingPlaneGroup",
                               "ClippingPlaneGroup",
                               m_clippingPlaneGroup);
    
    m_sceneClassAssistant->add("m_cerebellumViewingTransformation",
                               "ViewingTransformations",
                               m_cerebellumViewingTransformation);
    
    m_sceneClassAssistant->add("m_flatSurfaceViewingTransformation",
                               "ViewingTransformations",
                               m_flatSurfaceViewingTransformation);
    m_sceneClassAssistant->add("m_viewingTransformation",
                               "ViewingTransformations",
                               m_viewingTransformation);
    
    m_sceneClassAssistant->add("m_volumeSliceViewingTransformation",
                               "ViewingTransformations",
                               m_volumeSliceViewingTransformation);
    
    m_sceneClassAssistant->add("m_volumeSliceSettings",
                               "VolumeSliceSettings",
                               m_volumeSliceSettings);
    
    m_sceneClassAssistant->add("m_wholeBrainSurfaceSettings",
                               "WholeBrainSurfaceSettings",
                               m_wholeBrainSurfaceSettings);

    m_sceneClassAssistant->add("m_identificationUpdatesVolumeSlices",
                               &m_identificationUpdatesVolumeSlices);
    
    m_sceneClassAssistant->add<YokingGroupEnum, YokingGroupEnum::Enum>("m_yokingGroup",
                                                                   &m_yokingGroup);
    
    EventManager::get()->addEventListener(this,
                                          EventTypeEnum::EVENT_IDENTIFICATION_HIGHLIGHT_LOCATION);
    EventManager::get()->addEventListener(this,
                                          EventTypeEnum::EVENT_CARET_MAPPABLE_DATA_FILE_MAPS_VIEWED_IN_OVERLAYS);
}

/**
 * Destructor.
 */
BrowserTabContent::~BrowserTabContent()
{
    EventManager::get()->removeAllEventsFromListener(this);
 
    s_allBrowserTabContent.erase(this);
    
    delete m_clippingPlaneGroup;
    delete m_flatSurfaceViewingTransformation;
    delete m_cerebellumViewingTransformation;
    delete m_viewingTransformation;
    delete m_volumeSliceViewingTransformation;
    delete m_obliqueVolumeRotationMatrix;
    
    delete m_surfaceModelSelector;
    m_surfaceModelSelector = NULL;
    
    delete m_volumeSurfaceOutlineSetModel;
    m_volumeSurfaceOutlineSetModel = NULL;
    
    delete m_volumeSliceSettings;
    
    delete m_wholeBrainSurfaceSettings;
    
    delete m_sceneClassAssistant;
    m_sceneClassAssistant = NULL;
}

/**
 * Clone the contents of the given browser tab.
 * @param tabToClone
 *    Tab whose contents is cloned.
 */
void 
BrowserTabContent::cloneBrowserTabContent(BrowserTabContent* tabToClone)
{
    CaretAssert(tabToClone);
    m_surfaceModelSelector->setSelectedStructure(tabToClone->m_surfaceModelSelector->getSelectedStructure());
    m_surfaceModelSelector->setSelectedSurfaceModel(tabToClone->m_surfaceModelSelector->getSelectedSurfaceModel());

    m_selectedModelType = tabToClone->m_selectedModelType;

    /*
     *
     */
    EventModelGetAll allModelsEvent;
    EventManager::get()->sendEvent(allModelsEvent.getPointer());
    std::vector<Model*> allModels = allModelsEvent.getModels();
    for (std::vector<Model*>::iterator modelIter = allModels.begin();
         modelIter != allModels.end();
         modelIter++) {
        Model* model = *modelIter;
        model->copyTabContent(tabToClone->m_tabNumber,
                              m_tabNumber);
    }
    
    *m_clippingPlaneGroup = *tabToClone->m_clippingPlaneGroup;
    
    m_yokingGroup = tabToClone->m_yokingGroup;
    
    *m_cerebellumViewingTransformation = *tabToClone->m_cerebellumViewingTransformation;
    *m_flatSurfaceViewingTransformation = *tabToClone->m_flatSurfaceViewingTransformation;
    *m_viewingTransformation = *tabToClone->m_viewingTransformation;
    *m_volumeSliceViewingTransformation = *tabToClone->m_volumeSliceViewingTransformation;
    *m_volumeSliceSettings = *tabToClone->m_volumeSliceSettings;
    *m_wholeBrainSurfaceSettings = *tabToClone->m_wholeBrainSurfaceSettings;
    
    *m_obliqueVolumeRotationMatrix = *tabToClone->m_obliqueVolumeRotationMatrix;
    
    Model* model = getModelForDisplay();
    
    if (model != NULL) {
        Brain* brain = model->getBrain();
        
        brain->copyDisplayProperties(tabToClone->getTabNumber(),
                                     getTabNumber());
        
        const int32_t numberOfBrainStructures = brain->getNumberOfBrainStructures();
        for (int32_t i = 0; i < numberOfBrainStructures; i++) {
            BrainStructure* bs = brain->getBrainStructure(i);
            const int32_t numLabelFiles = bs->getNumberOfLabelFiles();
            for (int32_t j = 0; j < numLabelFiles; j++) {
                LabelFile* labelFile = bs->getLabelFile(j);
                labelFile->getGroupAndNameHierarchyModel()->copySelections(tabToClone->getTabNumber(),
                                                                           getTabNumber());
            }
        }

        const int32_t numBorderFiles = brain->getNumberOfBorderFiles();
        for (int32_t i = 0; i < numBorderFiles; i++) {
            BorderFile* bf = brain->getBorderFile(i);
            bf->getGroupAndNameHierarchyModel()->copySelections(tabToClone->getTabNumber(),
                                                                getTabNumber());
        }
        
        const int32_t numFociFiles = brain->getNumberOfFociFiles();
        for (int32_t i = 0; i < numFociFiles; i++) {
            FociFile* ff = brain->getFociFile(i);
            ff->getGroupAndNameHierarchyModel()->copySelections(tabToClone->getTabNumber(),
                                                                getTabNumber());
        }
    }
    
    m_volumeSurfaceOutlineSetModel->copyVolumeSurfaceOutlineSetModel(tabToClone->getVolumeSurfaceOutlineSet());
}

/**
 * Get the name of this browser tab.
 * Name priority is (1) name set by user, (2) name set by
 * user-interface, and (3) the default name.
 *
 * @return  Name of this tab.
 */
AString 
BrowserTabContent::getName() const
{
    AString s = "(" + AString::number(m_tabNumber + 1) + ") ";
    
    if (m_userName.isEmpty() == false) {
        s += m_userName;
    }
    else {
        const Model* displayedModel =
            getModelForDisplay();
        if (displayedModel != NULL) {
            const AString name = displayedModel->getNameForBrowserTab();
            s += name;
        }
    }
    
    return s;
}

/**
 * Set the user name of this tab.  The user name
 * overrides the default naming.
 *
 * @param userName
 *    User name for tab.
 */
void 
BrowserTabContent::setUserName(const AString& userName)
{
    m_userName = userName;
}

/**
 * @return The user name.
 */
AString 
BrowserTabContent::getUserName() const
{
    return m_userName;
}

/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString 
BrowserTabContent::toString() const
{
    PlainTextStringBuilder tb;
    getDescriptionOfContent(tb);
    return tb.getText();
}

/**
 * Get a text description of the window's content.
 *
 * @param descriptionOut
 *    Description of the window's content.
 */
void
BrowserTabContent::getDescriptionOfContent(PlainTextStringBuilder& descriptionOut) const
{
    const int32_t tabIndex = getTabNumber();
    
    descriptionOut.addLine("Browser Tab "
                   + AString::number(tabIndex + 1)
                   + ": ");
    
    descriptionOut.pushIndentation();
    
    const Model* model = getModelForDisplay();
    
    if (model != NULL) {
        bool chartFlag      = false;
        bool surfaceFlag    = false;
        bool surfaceMontageFlag = false;
        bool wholeBrainFlag = false;
        bool volumeFlag     = false;
        switch (model->getModelType()) {
            case ModelTypeEnum::MODEL_TYPE_CHART:
                chartFlag = true;
                break;
            case ModelTypeEnum::MODEL_TYPE_INVALID:
                break;
            case ModelTypeEnum::MODEL_TYPE_SURFACE:
                surfaceFlag = true;
                break;
            case ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE:
                surfaceMontageFlag = true;
                break;
            case ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES:
                volumeFlag = true;
                break;
            case ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN:
                wholeBrainFlag = true;
                break;
        }
        
        if (chartFlag) {
            model->getDescriptionOfContent(tabIndex,
                                           descriptionOut);
        }
        else if (volumeFlag) {
            descriptionOut.addLine("Volume Slice View");
        }
        else if (wholeBrainFlag) {
            descriptionOut.addLine("All View");
            descriptionOut.pushIndentation();
            
            if (isWholeBrainCerebellumEnabled()) {
                const Surface* cerebellumSurface = m_wholeBrainModel->getSelectedSurface(StructureEnum::CEREBELLUM,
                                                                                         tabIndex);
                if (cerebellumSurface != NULL) {
                    cerebellumSurface->getDescriptionOfContent(descriptionOut);
                }
            }
            if (isWholeBrainLeftEnabled()) {
                const Surface* leftSurface = m_wholeBrainModel->getSelectedSurface(StructureEnum::CORTEX_LEFT,
                                                                                   tabIndex);
                if (leftSurface != NULL) {
                    leftSurface->getDescriptionOfContent(descriptionOut);
                }
            }
            if (isWholeBrainRightEnabled()) {
                const Surface* rightSurface = m_wholeBrainModel->getSelectedSurface(StructureEnum::CORTEX_RIGHT,
                                                                                    tabIndex);
                if (rightSurface != NULL) {
                    rightSurface->getDescriptionOfContent(descriptionOut);
                }
            }
            
            descriptionOut.popIndentation();
        }
        else if (surfaceFlag) {
            model->getDescriptionOfContent(tabIndex,
                                           descriptionOut);
        }
        else if (surfaceMontageFlag) {
            model->getDescriptionOfContent(tabIndex,
                                           descriptionOut);
        }
        
        if (wholeBrainFlag
            || volumeFlag) {
            descriptionOut.pushIndentation();
            m_volumeSliceSettings->getDescriptionOfContent(model->getModelType(),
                                                           descriptionOut);
            descriptionOut.popIndentation();
        }
        
        if ( ! chartFlag) {
            getOverlaySet()->getDescriptionOfContent(descriptionOut);
        }
    }
    
    descriptionOut.popIndentation();
}


/**
 * Get the selected model type.
 * 
 * @return The selected model type.
 */   
ModelTypeEnum::Enum 
BrowserTabContent::getSelectedModelType() const
{
    return m_selectedModelType;
}

/**
 * Set the selected model type.
 *
 * @param selectedModelType
 *    New selected model type. 
 */   
void 
BrowserTabContent::setSelectedModelType(ModelTypeEnum::Enum selectedModelType)
{
    m_selectedModelType = selectedModelType;
}

/**
 * Get the model for DISPLAY.
 * 
 * @return  Pointer to displayed model or NULL
 *          if none are available.
 */   
Model* 
BrowserTabContent::getModelForDisplay()
{
    Model* mdc = NULL;
    
    switch (m_selectedModelType) {
        case ModelTypeEnum::MODEL_TYPE_INVALID:
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE:
            mdc = m_surfaceModelSelector->getSelectedSurfaceModel();
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE:
            mdc = m_surfaceMontageModel;
            break;
        case ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES:
            mdc = m_volumeModel;
            break;
        case ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN:
            mdc = m_wholeBrainModel;
            break;
        case ModelTypeEnum::MODEL_TYPE_CHART:
            mdc = m_chartModel;
            break;
    }
    
    return mdc;
}

/**
 * Get the model model for DISPLAY.
 * 
 * @return  Pointer to displayed model or NULL
 *          if none are available.
 */   
const Model* 
BrowserTabContent::getModelForDisplay() const
{
    Model* mdc = NULL;
    
    switch (m_selectedModelType) {
        case ModelTypeEnum::MODEL_TYPE_INVALID:
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE:
            mdc = m_surfaceModelSelector->getSelectedSurfaceModel();
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE:
            mdc = m_surfaceMontageModel;
            break;
        case ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES:
            mdc = m_volumeModel;
            break;
        case ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN:
            mdc = m_wholeBrainModel;
            break;
        case ModelTypeEnum::MODEL_TYPE_CHART:
            mdc = m_chartModel;
            break;
    }
    
    return mdc;
}

/**
 * Get the displayed chart model.
 *
 * @return  Pointer to displayed chart model or
 *          NULL if the displayed model is NOT a
 *          chart.
 */
ModelChart*
BrowserTabContent::getDisplayedChartModel()
{
    ModelChart* mc = dynamic_cast<ModelChart*>(getModelForDisplay());
    return mc;
}

/**
 * Get the displayed chart model.
 *
 * @return  Pointer to displayed chart model or
 *          NULL if the displayed model is NOT a
 *          chart.
 */
const ModelChart*
BrowserTabContent::getDisplayedChartModel() const
{
    const ModelChart* mc = dynamic_cast<const ModelChart*>(getModelForDisplay());
    return mc;
}

/**
 * Get the displayed surface model.
 * 
 * @return  Pointer to displayed surface model or 
 *          NULL if the displayed model is NOT a 
 *          surface.
 */   
ModelSurface* 
BrowserTabContent::getDisplayedSurfaceModel()
{
    ModelSurface* mdcs =
        dynamic_cast<ModelSurface*>(getModelForDisplay());
    return mdcs;
}

/**
 * Get the displayed surface model.
 * 
 * @return  Pointer to displayed surface model or 
 *          NULL if the displayed model is NOT a 
 *          surface.
 */   
const ModelSurface* 
BrowserTabContent::getDisplayedSurfaceModel() const
{
    const ModelSurface* mdcs =
    dynamic_cast<const ModelSurface*>(getModelForDisplay());
    return mdcs;
}

/**
 * Get the displayed volume model.
 * 
 * @return  Pointer to displayed volume model or 
 *          NULL if the displayed model is NOT a 
 *          volume.
 */   
ModelVolume* 
BrowserTabContent::getDisplayedVolumeModel()
{
    ModelVolume* mdcv =
        dynamic_cast<ModelVolume*>(getModelForDisplay());
    return mdcv;
}

/**
 * @return True if the displayed model is a cerebellum surface.
 */
bool
BrowserTabContent::isCerebellumDisplayed() const
{
    const ModelSurface* surfaceModel = getDisplayedSurfaceModel();
    if (surfaceModel != NULL) {
        if (surfaceModel->getSurface()->getStructure() == StructureEnum::CEREBELLUM) {
            return true;
        }
    }
    
    const ModelSurfaceMontage* montageModel = getDisplayedSurfaceMontageModel();
    if (montageModel != NULL) {
        if (montageModel->getSelectedConfigurationType(getTabNumber()) == SurfaceMontageConfigurationTypeEnum::CEREBELLAR_CORTEX_CONFIGURATION) {
            return true;
        }
    }
    
    return false;
}

/**
 * @return True if the displayed model is a flat surface.
 */
bool
BrowserTabContent::isFlatSurfaceDisplayed() const
{
    const ModelSurface* surfaceModel = getDisplayedSurfaceModel();
    if (surfaceModel != NULL) {
        if (surfaceModel->getSurface()->getSurfaceType() == SurfaceTypeEnum::FLAT) {
            return true;
        }
    }
    
    const ModelSurfaceMontage* montageModel = getDisplayedSurfaceMontageModel();
    if (montageModel != NULL) {
        if (montageModel->getSelectedConfigurationType(getTabNumber()) == SurfaceMontageConfigurationTypeEnum::FLAT_CONFIGURATION) {
            return true;
        }
    }
    
    return false;
}

/**
 * @return True if the displayed model is a chart
 */
bool
BrowserTabContent::isChartDisplayed() const
{
    const ModelChart* chartModel = getDisplayedChartModel();
    if (chartModel != NULL) {
        return true;
    }
    
    return false;
}

/**
 * @return Is the displayed model a volume slice model?
 */
bool
BrowserTabContent::isVolumeSlicesDisplayed() const
{
    const ModelVolume* mdcv = dynamic_cast<const ModelVolume*>(getModelForDisplay());
    
    const bool volumeFlag = (mdcv != NULL);
    return volumeFlag;
}

/**
 * @return Is the displayed model the whole brain model (ALL)?
 */
bool
BrowserTabContent::isWholeBrainDisplayed() const
{
    const ModelWholeBrain* mwb = dynamic_cast<const ModelWholeBrain*>(getModelForDisplay());
    const bool wholeBrainFlag = (mwb != NULL);
    return wholeBrainFlag;
}

/**
 * Get the displayed whole brain model.
 * 
 * @return  Pointer to displayed whole brain model or 
 *          NULL if the displayed model is NOT a 
 *          whole brain.
 */   
ModelWholeBrain* 
BrowserTabContent::getDisplayedWholeBrainModel()
{
    ModelWholeBrain* mdcwb =
        dynamic_cast<ModelWholeBrain*>(getModelForDisplay());
    return mdcwb;

}

/**
 * @return Pointer to displayed surface montage model
 * or NULL if the displayed model is not a surface
 * montage model.
 */
ModelSurfaceMontage* 
BrowserTabContent::getDisplayedSurfaceMontageModel()
{
    ModelSurfaceMontage* mdcsm =
    dynamic_cast<ModelSurfaceMontage*>(getModelForDisplay());
    return mdcsm;
}

/**
 * @return Pointer to displayed surface montage model
 * or NULL if the displayed model is not a surface
 * montage model.
 */
const ModelSurfaceMontage*
BrowserTabContent::getDisplayedSurfaceMontageModel() const
{
    const ModelSurfaceMontage* mdcsm =
    dynamic_cast<const ModelSurfaceMontage*>(getModelForDisplay());
    return mdcsm;
}

/**
 * Get all of the available surface models.
 * 
 * @return Vector containing all surface models.
 */   
const std::vector<ModelSurface*> 
BrowserTabContent::getAllSurfaceModels() const
{
    return m_allSurfaceModels;
}

/**
 * @return The surface model selector used to 
 * select a surface and structure.
 */
ModelSurfaceSelector* 
BrowserTabContent::getSurfaceModelSelector()
{
    return m_surfaceModelSelector;
}

/**
 * Get the overlay assignments for this tab.
 * 
 * @return  Overlay assignments for this tab or NULL if no valid model.
 */
OverlaySet* 
BrowserTabContent::getOverlaySet()
{
    Model* model = getModelForDisplay();
    if (model != NULL) {
       return model->getOverlaySet(m_tabNumber);
    }
    return NULL;
}

/**
 * Get the overlay assignments for this tab.
 *
 * @return  Overlay assignments for this tab or NULL if no valid model.
 */
const OverlaySet*
BrowserTabContent::getOverlaySet() const
{
    const Model* model = getModelForDisplay();
    if (model != NULL) {
        return model->getOverlaySet(m_tabNumber);
    }
    return NULL;
}

/**
 * Get the tab number for this content.
 * 
 * @return  Tab number.
 */
int32_t 
BrowserTabContent::getTabNumber() const
{
    return m_tabNumber;
}

/**
 * Update the selected models.
 */
void 
BrowserTabContent::update(const std::vector<Model*> models)
{
    m_surfaceModelSelector->updateSelector(models);
    
    const int32_t numModels = static_cast<int32_t>(models.size());
    
    ModelVolume* previousVolumeModel = m_volumeModel;
    
    m_allSurfaceModels.clear();
    m_surfaceModelSelector->getSelectableSurfaceModels(m_allSurfaceModels);
    m_volumeModel = NULL;
    m_wholeBrainModel = NULL;
    m_surfaceMontageModel = NULL;
    m_chartModel = NULL;
    
    for (int i = 0; i < numModels; i++) {
        Model* mdc = models[i];
        
        ModelSurface* mdcs = dynamic_cast<ModelSurface*>(mdc);
        ModelVolume* mdcv = dynamic_cast<ModelVolume*>(mdc);
        ModelWholeBrain* mdcwb = dynamic_cast<ModelWholeBrain*>(mdc);
        ModelSurfaceMontage* mdcsm = dynamic_cast<ModelSurfaceMontage*>(mdc);
        ModelChart* mdch = dynamic_cast<ModelChart*>(mdc);
        
        if (mdcs != NULL) {
            /* nothing to do since the surface model selector handles surfaces */
        }
        else if (mdcv != NULL) {
            CaretAssertMessage((m_volumeModel == NULL), "There is more than one volume model.");
            m_volumeModel = mdcv;
        }
        else if (mdcwb != NULL) {
            CaretAssertMessage((m_wholeBrainModel == NULL), "There is more than one whole brain model.");
            m_wholeBrainModel = mdcwb;
        }
        else if (mdcsm != NULL) {
            CaretAssertMessage((m_surfaceMontageModel == NULL), "There is more than one surface montage model.");
            m_surfaceMontageModel = mdcsm;
        }
        else if (mdch != NULL) {
            CaretAssertMessage((m_chartModel == NULL), "There is more than one surface chart model.");
            m_chartModel = mdch;
        }
        else {
            CaretAssertMessage(0, (AString("Unknown type of brain model.") + mdc->getNameForGUI(true)));
        }
    }
    
    switch (m_selectedModelType) {
        case ModelTypeEnum::MODEL_TYPE_INVALID:
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE:
            if (m_surfaceModelSelector->getSelectedSurfaceModel() == NULL) {
                m_selectedModelType = ModelTypeEnum::MODEL_TYPE_INVALID;
            }
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE:
            if (m_surfaceMontageModel == NULL) {
                m_selectedModelType = ModelTypeEnum::MODEL_TYPE_INVALID;
            }
            break;
        case ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES:
            if (m_volumeModel == NULL) {
                m_selectedModelType = ModelTypeEnum::MODEL_TYPE_INVALID;
            }
            break;
        case ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN:
            if (m_wholeBrainModel == NULL) {
                m_selectedModelType = ModelTypeEnum::MODEL_TYPE_INVALID;
            }
            break;
        case ModelTypeEnum::MODEL_TYPE_CHART:
            if (m_chartModel == NULL) {
                m_selectedModelType = ModelTypeEnum::MODEL_TYPE_INVALID;
            }
            break;
    }
    
    if (m_selectedModelType == ModelTypeEnum::MODEL_TYPE_INVALID) {
        if (m_surfaceModelSelector->getSelectedSurfaceModel() != NULL) {
            m_selectedModelType = ModelTypeEnum::MODEL_TYPE_SURFACE;
        }
        else if (m_volumeModel != NULL) {
            m_selectedModelType = ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES;
        }
        else if (m_wholeBrainModel != NULL) {
            m_selectedModelType = ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN;
        }
        else if (m_surfaceMontageModel != NULL) {
            m_selectedModelType = ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE;
        }
        else if (m_chartModel != NULL) {
            m_selectedModelType = ModelTypeEnum::MODEL_TYPE_CHART;
        }
    }
    
    if (m_volumeModel != NULL) {
        if (m_volumeModel != previousVolumeModel) {
        
            VolumeMappableInterface* underlayVolume = m_volumeModel->getOverlaySet(m_tabNumber)->getUnderlayVolume();
            if (underlayVolume != NULL) {
                /*
                 * Set montage slice spacing based upon slices
                 * in the Z-axis.
                 */
                std::vector<int64_t> dimensions;
                underlayVolume->getDimensions(dimensions);
                
                if (dimensions.size() >= 3) {
                    const int32_t dimZ = dimensions[2];
                    if (dimZ > 0) {
                        const int32_t maxZ = static_cast<int32_t>(dimZ * 0.90);
                        const int32_t minZ = static_cast<int32_t>(dimZ * 0.10);
                        const int32_t sliceRange = (maxZ - minZ);
                        int32_t sliceSpacing = 1;
                        if (sliceRange > 0) {
                            const int32_t numSlicesViewed = (m_volumeSliceSettings->getMontageNumberOfRows()
                                                             * m_volumeSliceSettings->getMontageNumberOfColumns());
                            sliceSpacing = (sliceRange / numSlicesViewed);
                        }
                        m_volumeSliceSettings->setMontageSliceSpacing(sliceSpacing);
                    }
                }
            }
        }
    }
}

/**
 * Is the chart model selection valid?
 *
 * @return bool indicating validity.
 */
bool
BrowserTabContent::isChartModelValid() const
{
    bool valid = (m_chartModel != NULL);
    return valid;
}

/**
 * Is the surface model selection valid?
 *
 * @return bool indicating validity.
 */
bool 
BrowserTabContent::isSurfaceModelValid() const
{
    bool valid = (m_allSurfaceModels.empty() == false);
    return valid;
}

/**
 * Is the volume model selection valid?
 *
 * @return bool indicating validity.
 */
bool 
BrowserTabContent::isVolumeSliceModelValid() const
{
    bool valid = (m_volumeModel != NULL);
    return valid;
}

/**
 * Is the whole brain model selection valid?
 *
 * @return bool indicating validity.
 */
bool 
BrowserTabContent::isWholeBrainModelValid() const
{
    bool valid = (m_wholeBrainModel != NULL);
    return valid;
}

/**
 * Is the surface montage model selection valid?
 *
 * @return bool indicating validity.
 */
bool 
BrowserTabContent::isSurfaceMontageModelValid() const
{
    bool valid = (m_surfaceMontageModel != NULL);
    return valid;
}

/**
 * Receive an event.
 * 
 * @param event
 *     The event that the receive can respond to.
 */
void
BrowserTabContent::receiveEvent(Event* event)
{
    if (event->getEventType() == EventTypeEnum::EVENT_IDENTIFICATION_HIGHLIGHT_LOCATION) {
        EventIdentificationHighlightLocation* idLocationEvent =
        dynamic_cast<EventIdentificationHighlightLocation*>(event);
        CaretAssert(idLocationEvent);

        Model* model = getModelForDisplay();
        if (model == NULL) {
            return;
        }

        if (isIdentificationUpdatesVolumeSlices()) {
            const float* highlighXYZ = idLocationEvent->getXYZ();
            for (int32_t windowTabNumber = 0;
                 windowTabNumber < BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS;
                 windowTabNumber++) {
                
                float volumeSliceXYZ[3] = {
                    highlighXYZ[0],
                    highlighXYZ[1],
                    highlighXYZ[2]
                };
                
                /*
                 * If othogonal/montage viewing, do not alter the slice
                 * coordinate in the axis being viewed
                 */
                if (getDisplayedVolumeModel() != NULL) {
                    bool keepSliceCoordinateForSelectedAxis = false;
                    switch (m_volumeSliceSettings->getSliceProjectionType()) {
                        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_ORTHOGONAL:
                            break;
                        case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_OBLIQUE:
                            keepSliceCoordinateForSelectedAxis = true;
                            break;
                    }
                    switch (m_volumeSliceSettings->getSliceDrawingType()) {
                        case VolumeSliceDrawingTypeEnum::VOLUME_SLICE_DRAW_MONTAGE:
                            keepSliceCoordinateForSelectedAxis = true;
                            break;
                        case VolumeSliceDrawingTypeEnum::VOLUME_SLICE_DRAW_SINGLE:
                            break;
                    }

                    if (keepSliceCoordinateForSelectedAxis) {
                        switch (getSliceViewPlane()) {
                            case VolumeSliceViewPlaneEnum::ALL:
                                volumeSliceXYZ[0] = getSliceCoordinateParasagittal();
                                volumeSliceXYZ[1] = getSliceCoordinateCoronal();
                                volumeSliceXYZ[2] = getSliceCoordinateAxial();
                                break;
                            case VolumeSliceViewPlaneEnum::PARASAGITTAL:
                                volumeSliceXYZ[0] = getSliceCoordinateParasagittal();
                                break;
                            case VolumeSliceViewPlaneEnum::CORONAL:
                                volumeSliceXYZ[1] = getSliceCoordinateCoronal();
                                break;
                            case VolumeSliceViewPlaneEnum::AXIAL:
                                volumeSliceXYZ[2] = getSliceCoordinateAxial();
                                break;
                        }
                    }
//                    switch (m_volumeSliceSettings->getSliceViewMode()) {
//                        case VolumeSliceViewModeEnum::MONTAGE:
//                        case VolumeSliceViewModeEnum::OBLIQUE:
//                            switch (getSliceViewPlane()) {
//                                case VolumeSliceViewPlaneEnum::ALL:
//                                    break;
//                                case VolumeSliceViewPlaneEnum::PARASAGITTAL:
//                                    volumeSliceXYZ[0] = getSliceCoordinateParasagittal();
//                                    break;
//                                case VolumeSliceViewPlaneEnum::CORONAL:
//                                    volumeSliceXYZ[1] = getSliceCoordinateCoronal();
//                                    break;
//                                case VolumeSliceViewPlaneEnum::AXIAL:
//                                    volumeSliceXYZ[2] = getSliceCoordinateAxial();
//                                    break;
//                            }
//                            break;
//                        case VolumeSliceViewModeEnum::ORTHOGONAL:
//                            break;
//                    }
                }
                
                m_volumeSliceSettings->selectSlicesAtCoordinate(volumeSliceXYZ);
            }
        }
        
        idLocationEvent->setEventProcessed();
    }
    else if (event->getEventType() == EventTypeEnum::EVENT_CARET_MAPPABLE_DATA_FILE_MAPS_VIEWED_IN_OVERLAYS) {
        EventCaretMappableDataFileMapsViewedInOverlays* mapOverlayEvent  =
        dynamic_cast<EventCaretMappableDataFileMapsViewedInOverlays*>(event);
        CaretAssert(mapOverlayEvent);
        
        OverlaySet* overlaySet = getOverlaySet();
        const int32_t numOverlays = overlaySet->getNumberOfDisplayedOverlays();
        for (int32_t i = (numOverlays - 1); i >= 0; i--) {
            Overlay* overlay = overlaySet->getOverlay(i);
            if (overlay->isEnabled()) {
                CaretMappableDataFile* mapFile;
                int32_t mapFileIndex;
                overlay->getSelectionData(mapFile, mapFileIndex);
                if (mapFile != NULL) {
                    if (mapFile == mapOverlayEvent->getCaretMappableDataFile()) {
                        if ((mapFileIndex >= 0)
                            && (mapFileIndex < mapFile->getNumberOfMaps())) {
                            mapOverlayEvent->addMapIndex(mapFileIndex);
                        }
                    }
                }
            }
        }
    }
}

/**
 * Get the map files for which a palette should be displayed in the 
 * graphcis window.  Note that the order of map files and indices starts
 * with the bottom most layer and ends with the top most overlay.
 *
 * @param mapFiles
 *    Outut Map files that should have a palette displayed in the graphics window.
 * @param mapIndices
 *    Output Indices of the maps in the mapFiles.
 */
void 
BrowserTabContent::getDisplayedPaletteMapFiles(std::vector<CaretMappableDataFile*>& mapFiles,
                                               std::vector<int32_t>& mapIndices)
{
    mapFiles.clear();
    mapIndices.clear();
    
    OverlaySet* overlaySet = getOverlaySet();
    const int32_t numOverlays = overlaySet->getNumberOfDisplayedOverlays();
    for (int32_t i = (numOverlays - 1); i >= 0; i--) {
        Overlay* overlay = overlaySet->getOverlay(i);
        if (overlay->isEnabled()) {
            if (overlay->isPaletteDisplayEnabled()) {
                CaretMappableDataFile* mapFile;
                int32_t mapFileIndex;
                overlay->getSelectionData(mapFile, mapFileIndex);
                if (mapFile != NULL) {
                    if (mapFile->isMappedWithPalette()) {
                        if ((mapFileIndex >= 0) 
                            && (mapFileIndex < mapFile->getNumberOfMaps())) {
                            mapFiles.push_back(mapFile);
                            mapIndices.push_back(mapFileIndex);
                        }
                    }
                }
            }
        }
    }
}

/**
 * @return The volume surface outline model for this tab.
 */
VolumeSurfaceOutlineSetModel* 
BrowserTabContent::getVolumeSurfaceOutlineSet()
{
    return m_volumeSurfaceOutlineSetModel;
}

/**
 * @return The volume surface outline model for this tab.
 */
const VolumeSurfaceOutlineSetModel* 
BrowserTabContent::getVolumeSurfaceOutlineSet() const
{
    return m_volumeSurfaceOutlineSetModel;
}

/**
 * Get the data files displayed in this tab.
 * @param displayedDataFilesOut
 *    Displayed data file are loaded into this parameter.
 */
void 
BrowserTabContent::getFilesDisplayedInTab(std::vector<CaretDataFile*>& displayedDataFilesOut)
{
    displayedDataFilesOut.clear();
 
    Model* model = getModelForDisplay();
    if (model == NULL) {
        return;
    }
    
    std::set<CaretDataFile*> displayedDataFiles;
    
    const int32_t tabIndex = getTabNumber();
    switch (getSelectedModelType()) {
        case ModelTypeEnum::MODEL_TYPE_INVALID:
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE:
        {
            ModelSurface* ms = getDisplayedSurfaceModel();
            displayedDataFiles.insert(ms->getSurface());
        }
            break;
        case ModelTypeEnum::MODEL_TYPE_SURFACE_MONTAGE:
        {
            ModelSurfaceMontage* msm = getDisplayedSurfaceMontageModel();
            switch (msm->getSelectedConfigurationType(tabIndex)) {
                case SurfaceMontageConfigurationTypeEnum::CEREBELLAR_CORTEX_CONFIGURATION:
                {
                    SurfaceMontageConfigurationCerebellar* smcc = msm->getCerebellarConfiguration(tabIndex);
                    if (smcc->isFirstSurfaceEnabled()) {
                        displayedDataFiles.insert(smcc->getFirstSurfaceSelectionModel()->getSurface());
                    }
                    if (smcc->isSecondSurfaceEnabled()) {
                        displayedDataFiles.insert(smcc->getSecondSurfaceSelectionModel()->getSurface());
                    }
                }
                    break;
                case SurfaceMontageConfigurationTypeEnum::CEREBRAL_CORTEX_CONFIGURATION:
                {
                    SurfaceMontageConfigurationCerebral* smcc = msm->getCerebralConfiguration(tabIndex);
                    if (smcc->isFirstSurfaceEnabled()) {
                        if (smcc->isLeftEnabled()) {
                            displayedDataFiles.insert(smcc->getLeftFirstSurfaceSelectionModel()->getSurface());
                        }
                        if (smcc->isRightEnabled()) {
                            displayedDataFiles.insert(smcc->getRightFirstSurfaceSelectionModel()->getSurface());
                        }
                    }
                    if (smcc->isSecondSurfaceEnabled()) {
                        if (smcc->isLeftEnabled()) {
                            displayedDataFiles.insert(smcc->getLeftSecondSurfaceSelectionModel()->getSurface());
                        }
                        if (smcc->isRightEnabled()) {
                            displayedDataFiles.insert(smcc->getRightSecondSurfaceSelectionModel()->getSurface());
                        }
                    }
                }
                    break;
                case SurfaceMontageConfigurationTypeEnum::FLAT_CONFIGURATION:
                {
                    SurfaceMontageConfigurationFlatMaps* smcfm = msm->getFlatMapsConfiguration(tabIndex);
                    if (smcfm->isLeftEnabled()) {
                        displayedDataFiles.insert(smcfm->getLeftSurfaceSelectionModel()->getSurface());
                    }
                    if (smcfm->isRightEnabled()) {
                        displayedDataFiles.insert(smcfm->getRightSurfaceSelectionModel()->getSurface());
                    }
                    if (smcfm->isCerebellumEnabled()) {
                        displayedDataFiles.insert(smcfm->getCerebellumSurfaceSelectionModel()->getSurface());
                    }
                }
                    break;
            }
        }
            break;
        case ModelTypeEnum::MODEL_TYPE_VOLUME_SLICES:
        {
            const int32_t numOutlines = m_volumeSurfaceOutlineSetModel->getNumberOfDislayedVolumeSurfaceOutlines();
            for (int32_t i = 0; i < numOutlines; i++) {
                VolumeSurfaceOutlineModel* model = m_volumeSurfaceOutlineSetModel->getVolumeSurfaceOutlineModel(i);
                if (model->isDisplayed()) {
                    displayedDataFiles.insert(model->getSurface());
                }
            }
        }
            break;
        case ModelTypeEnum::MODEL_TYPE_WHOLE_BRAIN:
        {
            ModelWholeBrain* wbm = getDisplayedWholeBrainModel();
            if (isWholeBrainLeftEnabled()) {
                displayedDataFiles.insert(wbm->getSelectedSurface(StructureEnum::CORTEX_LEFT, tabIndex));
            }
            if (isWholeBrainRightEnabled()) {
                displayedDataFiles.insert(wbm->getSelectedSurface(StructureEnum::CORTEX_RIGHT, tabIndex));
            }
            if (isWholeBrainCerebellumEnabled()) {
                displayedDataFiles.insert(wbm->getSelectedSurface(StructureEnum::CEREBELLUM, tabIndex));
            }
        }
            break;
        case ModelTypeEnum::MODEL_TYPE_CHART:
            break;
    }

    /*
     * Check overlay files
     */
    OverlaySet* overlaySet = model->getOverlaySet(tabIndex);
    const int32_t numOverlays = overlaySet->getNumberOfDisplayedOverlays();
    for (int32_t i = 0; i < numOverlays; i++) {
        Overlay* overlay = overlaySet->getOverlay(i);
        if (overlay->isEnabled()) {
            CaretMappableDataFile* overlayDataFile = NULL;
            int32_t mapIndex;
            overlay->getSelectionData(overlayDataFile,
                                      mapIndex);
            displayedDataFiles.insert(overlayDataFile);
        }
    }
    
    
    /*
     * Check border files
     */
    Brain* brain = model->getBrain();
    const DisplayPropertiesBorders* borderDisplayProperties = brain->getDisplayPropertiesBorders();
    const DisplayGroupEnum::Enum borderDisplayGroup = borderDisplayProperties->getDisplayGroupForTab(tabIndex);
    if (borderDisplayProperties->isDisplayed(borderDisplayGroup,
                                             tabIndex)) {
        const int32_t numBorderFiles = brain->getNumberOfBorderFiles();
        for (int32_t i = 0; i < numBorderFiles; i++) {
            BorderFile* borderFile = brain->getBorderFile(i);
            const GroupAndNameHierarchyModel* classAndNameSelection = borderFile->getGroupAndNameHierarchyModel();
            if (classAndNameSelection->isSelected(borderDisplayGroup,
                                                  tabIndex)) {
                displayedDataFilesOut.push_back(borderFile);
            }
        }
    }

    /*
     * Check foci files
     */
    const DisplayPropertiesFoci* fociDisplayProperties = brain->getDisplayPropertiesFoci();
    const DisplayGroupEnum::Enum fociDisplayGroup = fociDisplayProperties->getDisplayGroupForTab(tabIndex);
    
    if (fociDisplayProperties->isDisplayed(fociDisplayGroup,
                                           tabIndex)) {
        const int32_t numFociFiles = brain->getNumberOfFociFiles();
        for (int32_t i = 0; i < numFociFiles; i++) {
            FociFile* fociFile = brain->getFociFile(i);
            
            const GroupAndNameHierarchyModel* classAndNameSelection = fociFile->getGroupAndNameHierarchyModel();
            if (classAndNameSelection->isSelected(fociDisplayGroup,
                                                  tabIndex)) {
                displayedDataFilesOut.push_back(fociFile);
            }
        }
    }
    
    /*
     * Might be NULLs so filter them out and return the results
     */
    for (std::set<CaretDataFile*>::iterator iter = displayedDataFiles.begin();
         iter != displayedDataFiles.end();
         iter++) {
        CaretDataFile* cdf = *iter;
        if (cdf != NULL) {
            displayedDataFilesOut.push_back(cdf);
        }
    }
}

ViewingTransformations*
BrowserTabContent::getViewingTransformation()
{
    if (isVolumeSlicesDisplayed()) {
        return m_volumeSliceViewingTransformation;
    }
    else if (isFlatSurfaceDisplayed()) {
        return m_flatSurfaceViewingTransformation;
    }
    else if (isCerebellumDisplayed()) {
        return m_cerebellumViewingTransformation;
    }
    return m_viewingTransformation;
}

const ViewingTransformations*
BrowserTabContent::getViewingTransformation() const
{
    if (isVolumeSlicesDisplayed()) {
        return m_volumeSliceViewingTransformation;
    }
    else if (isFlatSurfaceDisplayed()) {
        return m_flatSurfaceViewingTransformation;
    }
    else if (isCerebellumDisplayed()) {
        return m_cerebellumViewingTransformation;
    }
    return m_viewingTransformation;
}


/**
 * @return The viewing translation.
 */
const float*
BrowserTabContent::getTranslation() const
{
    return getViewingTransformation()->getTranslation();
}

/**
 * Get the viewing translation.
 *
 * @param translationOut
 *    Translation values output.
 */
void
BrowserTabContent::getTranslation(float translationOut[3]) const
{
    return getViewingTransformation()->getTranslation(translationOut);
}

/**
 * Set the viewing translation.
 *
 * @param translation
 *    New translation values.
 */
void
BrowserTabContent::setTranslation( const float translation[3])
{
    getViewingTransformation()->setTranslation(translation);
    updateYokedBrowserTabs();
}

/**
 * Set the viewing translation.
 *
 * @param translationX
 *    New translation X-value.
 * @param translationY
 *    New translation Y-value.
 * @param translationZ
 *    New translation Z-value.
 */
void
BrowserTabContent::setTranslation(const float translationX,
                                  const float translationY,
                                  const float translationZ)
{
    getViewingTransformation()->setTranslation(translationX,
                                               translationY,
                                               translationZ);
    updateYokedBrowserTabs();
}

/**
 * @return The viewing scaling.
 */
float
BrowserTabContent::getScaling() const
{
    return getViewingTransformation()->getScaling();
}

/**
 * Set the viewing scaling.
 * @param scaling
 *    New value for scaling.
 */
void
BrowserTabContent::setScaling(const float scaling)
{
    return getViewingTransformation()->setScaling(scaling);
    updateYokedBrowserTabs();
}

/**
 * @return The rotation matrix.
 */
Matrix4x4
BrowserTabContent::getRotationMatrix() const
{
    return getViewingTransformation()->getRotationMatrix();
}

/**
 * Set the rotation matrix.
 *
 * @param rotationMatrix
 *    The new rotation matrix.
 */
void
BrowserTabContent::setRotationMatrix(const Matrix4x4& rotationMatrix)
{
    getViewingTransformation()->setRotationMatrix(rotationMatrix);
    updateYokedBrowserTabs();
}

/**
 * @return The oblique volume rotation matrix.
 */
Matrix4x4
BrowserTabContent::getObliqueVolumeRotationMatrix() const
{
    return *m_obliqueVolumeRotationMatrix;
}

/**
 * Set the oblique rotation matrix.
 *
 * @param obliqueRotationMatrix
 *    The new oblique rotation matrix.
 */
void
BrowserTabContent::setObliqueVolumeRotationMatrix(const Matrix4x4& obliqueRotationMatrix)
{
    *m_obliqueVolumeRotationMatrix = obliqueRotationMatrix;
    
    updateYokedBrowserTabs();
}

/**
 * Reset the view to the default view.
 */
void
BrowserTabContent::resetView()
{
    getViewingTransformation()->resetView();
    if (isVolumeSlicesDisplayed()) {
        m_obliqueVolumeRotationMatrix->identity();
    }
    updateYokedBrowserTabs();
}

/**
 * Set to a right side view.
 */
void
BrowserTabContent::rightView()
{
    if (isVolumeSlicesDisplayed()) {
        return;
    }
    getViewingTransformation()->rightView();
    updateYokedBrowserTabs();
}

/**
 * set to a left side view.
 */
void
BrowserTabContent::leftView()
{
    if (isVolumeSlicesDisplayed()) {
        return;
    }
    getViewingTransformation()->leftView();
    updateYokedBrowserTabs();
}

/**
 * set to a anterior view.
 */
void
BrowserTabContent::anteriorView()
{
    if (isVolumeSlicesDisplayed()) {
        return;
    }
    getViewingTransformation()->anteriorView();
    updateYokedBrowserTabs();
}

/**
 * set to a posterior view.
 */
void
BrowserTabContent::posteriorView()
{
    if (isVolumeSlicesDisplayed()) {
        return;
    }
    getViewingTransformation()->posteriorView();
    updateYokedBrowserTabs();
}

/**
 * set to a dorsal view.
 */
void
BrowserTabContent::dorsalView()
{
    if (isVolumeSlicesDisplayed()) {
        return;
    }
    getViewingTransformation()->dorsalView();
    updateYokedBrowserTabs();
}

/**
 * set to a ventral view.
 */
void
BrowserTabContent::ventralView()
{
    if (isVolumeSlicesDisplayed()) {
        return;
    }
    getViewingTransformation()->ventralView();
    updateYokedBrowserTabs();
}

/*
 * @return The slice view plane for the given viewport coordinate.
 * If ALL is returned, is indicates that the given viewport coordinate
 * is in the bottom left region in which volume slices are not displayed.
 *
 * @param viewport
 *   The viewport.
 * @param mousePressX
 *   X Location of the mouse press.
 * @param mousePressY
 *   Y Location of the mouse press.
 */
VolumeSliceViewPlaneEnum::Enum
BrowserTabContent::getSliceViewPlaneForVolumeAllSliceView(const int32_t viewport[4],
                                                          const int32_t mousePressX,
                                                          const int32_t mousePressY,
                                                          int32_t sliceViewportOut[4]) const
{
    VolumeSliceViewPlaneEnum::Enum view = VolumeSliceViewPlaneEnum::ALL;
    
    const int32_t halfWidth  = viewport[2] / 2;
    const int32_t halfHeight = viewport[3] / 2;
    const int32_t viewportMousePressedX = mousePressX - viewport[0];
    const int32_t viewportMousePressedY = mousePressY - viewport[1];
    bool isRight  = false;
    bool isTop = false;
    if (viewportMousePressedX > halfWidth) {
        isRight = true;
    }
    if (viewportMousePressedY > halfHeight) {
        isTop = true;
    }
    
    /*
     * Top Right is Coronal
     * Top Left is Parasagittal
     * Bottom Right is Axial
     * Bottom Left is Empty or Surfaces
     */
    if (isTop) {
        if (isRight) {
            view = VolumeSliceViewPlaneEnum::CORONAL;
        }
        else {
            view = VolumeSliceViewPlaneEnum::PARASAGITTAL;
        }
    }
    else {
        if (isRight) {
            view = VolumeSliceViewPlaneEnum::AXIAL;
        }
        else {
            
        }
    }

    sliceViewportOut[0] = viewport[0];
    sliceViewportOut[1] = viewport[1];
    sliceViewportOut[2] = halfWidth;
    sliceViewportOut[3] = halfHeight;
    
    switch (view) {
        case VolumeSliceViewPlaneEnum::ALL:
            sliceViewportOut[2] = viewport[2];
            sliceViewportOut[3] = viewport[3];
            break;
        case VolumeSliceViewPlaneEnum::AXIAL:
            sliceViewportOut[0] = halfWidth;
            break;
        case VolumeSliceViewPlaneEnum::CORONAL:
            sliceViewportOut[0] = halfWidth;
            sliceViewportOut[1] = halfHeight;
            break;
        case VolumeSliceViewPlaneEnum::PARASAGITTAL:
            sliceViewportOut[1] = halfHeight;
            break;
    }
    return view;
}

/**
 * Apply mouse rotation to the displayed model.
 *
 * @param mousePressX
 *    X coordinate of where mouse was pressed.
 * @param mousePressY
 *    X coordinate of where mouse was pressed.
 * @param mouseX
 *    X coordinate of mouse.
 * @param mouseY
 *    Y coordinate of mouse.
 * @param mouseDeltaX
 *    Change in mouse X coordinate.
 * @param mouseDeltaY
 *    Change in mouse Y coordinate.
 */
void
BrowserTabContent::applyMouseRotation(BrainOpenGLViewportContent* viewportContent,
                                      const int32_t mousePressX,
                                      const int32_t mousePressY,
                                      const int32_t mouseX,
                                      const int32_t mouseY,
                                      const int32_t mouseDeltaX,
                                      const int32_t mouseDeltaY)
{
    if (isVolumeSlicesDisplayed()) {
        switch (getSliceProjectionType()) {
            case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_OBLIQUE:
            {
                int viewport[4];
                viewportContent->getModelViewport(viewport);
                VolumeSliceViewPlaneEnum::Enum slicePlane = this->getSliceViewPlane();
                int sliceViewport[4] = {
                    viewport[0],
                    viewport[1],
                    viewport[2],
                    viewport[3]
                };
                if (slicePlane == VolumeSliceViewPlaneEnum::ALL) {
                    slicePlane = getSliceViewPlaneForVolumeAllSliceView(viewport,
                                                                        mousePressX,
                                                                        mousePressY,
                                                                        sliceViewport);
                }
                
                Matrix4x4 rotationMatrix = getObliqueVolumeRotationMatrix();
                
                if (slicePlane == VolumeSliceViewPlaneEnum::ALL) {
                    rotationMatrix.rotateX(-mouseDeltaY);
                    rotationMatrix.rotateY(mouseDeltaX);
                }
                else {
                    if ((mouseDeltaX != 0)
                        || (mouseDeltaY != 0)) {
                        
                        const int previousMouseX = mouseX - mouseDeltaX;
                        const int previousMouseY = mouseY - mouseDeltaY;
                        
                        /*
                         * Need to account for the quadrants!!!!
                         */
                        const float viewportCenter[3] = {
                            sliceViewport[0] + sliceViewport[2] / 2,
                            sliceViewport[1] + sliceViewport[3] / 2,
                            0.0
                        };
                        
                        const float oldPos[3] = {
                            previousMouseX,
                            previousMouseY,
                            0.0
                        };
                        
                        const float newPos[3] = {
                            mouseX,
                            mouseY,
                            0.0
                        };
                        
                        /*
                         * Compute normal vector from viewport center to
                         * old mouse position to new mouse position.
                         * If normal-Z is positive, mouse has been moved
                         * in a counter clockwise motion relative to center.
                         * If normal-Z is negative, mouse has moved clockwise.
                         */
                        float normalDirection[3];
                        MathFunctions::normalVectorDirection(viewportCenter,
                                                             oldPos,
                                                             newPos,
                                                             normalDirection);
                        bool isClockwise = false;
                        bool isCounterClockwise = false;
                        if (normalDirection[2] > 0.0) {
                            isCounterClockwise = true;
                        }
                        else if (normalDirection[2] < 0.0) {
                            isClockwise = true;
                        }
                        
                        if (isClockwise
                            || isCounterClockwise) {
                            const float mouseDelta = std::sqrt(static_cast<float>((mouseDeltaX * mouseDeltaX)
                                                                                  + (mouseDeltaY * mouseDeltaY)));
                            
                            switch (slicePlane) {
                                case VolumeSliceViewPlaneEnum::ALL:
                                {
                                    CaretAssert(0);
                                }
                                    break;
                                case VolumeSliceViewPlaneEnum::AXIAL:
                                {
                                    Matrix4x4 rotation;
                                    if (isClockwise) {
                                        rotation.rotateZ(mouseDelta);
                                    }
                                    else if (isCounterClockwise) {
                                        rotation.rotateZ(-mouseDelta);
                                    }
                                    rotationMatrix.premultiply(rotation);
                                }
                                    break;
                                case VolumeSliceViewPlaneEnum::CORONAL:
                                {
                                    Matrix4x4 rotation;
                                    if (isClockwise) {
                                        rotation.rotateY(-mouseDelta);
                                    }
                                    else if (isCounterClockwise) {
                                        rotation.rotateY(mouseDelta);
                                    }
                                    rotationMatrix.premultiply(rotation);
                                }
                                    break;
                                case VolumeSliceViewPlaneEnum::PARASAGITTAL:
                                {
                                    Matrix4x4 rotation;
                                    if (isClockwise) {
                                        rotation.rotateX(-mouseDelta);
                                    }
                                    else if (isCounterClockwise) {
                                        rotation.rotateX(mouseDelta);
                                    }
                                    rotationMatrix.premultiply(rotation);
                                }
                                    break;
                            }
                        }
                    }
                }
                
                setObliqueVolumeRotationMatrix(rotationMatrix);
            }
                break;
            case VolumeSliceProjectionTypeEnum::VOLUME_SLICE_PROJECTION_ORTHOGONAL:
                break;
                /* Orthogonal olume slices are not rotated */
                break;
        }
    }
    else if (isChartDisplayed()) {
        /* no rotation for chart */
    }
    else if (isCerebellumDisplayed()) {
        const float screenDX = mouseDeltaX;
        const float screenDY = mouseDeltaY;
        
        float rotateDX = 0.0;
        float rotateDY = 0.0;
        float rotateDZ = 0.0;
        
        ModelSurfaceMontage* montageModel = getDisplayedSurfaceMontageModel();
        if (montageModel != NULL) {
            std::vector<const SurfaceMontageViewport*> montageViewports;
            montageModel->getSurfaceMontageViewportsForTransformation(getTabNumber(),
                                                                      montageViewports);
            
            bool foundMontageViewportFlag = false;
            
            const int32_t numViewports = static_cast<int32_t>(montageViewports.size());
            for (int32_t ivp = 0; ivp < numViewports; ivp++) {
                const SurfaceMontageViewport* smv = montageViewports[ivp];
                
                if (smv->isInside(mousePressX,
                                  mousePressY)) {
                    switch (smv->getProjectionViewType()) {
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_ANTERIOR:
                            rotateDX =  screenDY;
                            rotateDZ =  screenDX;
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_DORSAL:
                            rotateDX = -screenDY;
                            rotateDY =  screenDX;
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_POSTERIOR:
                            rotateDX =  -screenDY;
                            rotateDZ =   screenDX;
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_VENTRAL:
                            rotateDX = -screenDY;
                            rotateDY = -screenDX;
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_FLAT_SURFACE:
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_LATERAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_MEDIAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_FLAT_SURFACE:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_LATERAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_MEDIAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_FLAT_SURFACE:
                            break;
                    }
                }
                if (foundMontageViewportFlag) {
                    break;
                }
            }
        }
        else {
            rotateDX = -screenDY;
            rotateDY =  screenDX;
        }
        
        Matrix4x4 rotationMatrix = m_cerebellumViewingTransformation->getRotationMatrix();
        rotationMatrix.rotateX(rotateDX);
        rotationMatrix.rotateY(rotateDY);
        rotationMatrix.rotateZ(rotateDZ);
        m_cerebellumViewingTransformation->setRotationMatrix(rotationMatrix);
    }
    else {
        ViewingTransformations* viewingTransform = getViewingTransformation();
        if (getProjectionViewType() == ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_LATERAL) {
            Matrix4x4 rotationMatrix = viewingTransform->getRotationMatrix();
            rotationMatrix.rotateX(-mouseDeltaY);
            rotationMatrix.rotateY(-mouseDeltaX);
            viewingTransform->setRotationMatrix(rotationMatrix);
        }
        else {
            float dx = mouseDeltaX;
            float dy = mouseDeltaY;
            
            ModelSurfaceMontage* montageModel = getDisplayedSurfaceMontageModel();
            if (montageModel != NULL) {
                std::vector<const SurfaceMontageViewport*> montageViewports;
                montageModel->getSurfaceMontageViewportsForTransformation(getTabNumber(),
                                                                          montageViewports);
                
                bool isValid = false;
                bool isFlat  = false;
                bool isLeft = false;
                bool isLateral = true;
                const int32_t numViewports = static_cast<int32_t>(montageViewports.size());
                for (int32_t ivp = 0; ivp < numViewports; ivp++) {
                    const SurfaceMontageViewport* smv = montageViewports[ivp];
                    if (smv->isInside(mousePressX,
                                     mousePressY)) {
                        switch (smv->getProjectionViewType()) {
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_ANTERIOR:
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_DORSAL:
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_POSTERIOR:
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_VENTRAL:
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_FLAT_SURFACE:
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_LATERAL:
                                isLeft = true;
                                isLateral = true;
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_MEDIAL:
                                isLeft = true;
                                isLateral = false;
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_FLAT_SURFACE:
                                isLeft = true;
                                isFlat = true;
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_LATERAL:
                                isLeft = false;
                                isLateral = true;
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_MEDIAL:
                                isLeft = false;
                                isLateral = false;
                                break;
                            case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_FLAT_SURFACE:
                                isLeft = false;
                                isFlat = true;
                                break;
                        }
                        isValid = true;
                    }
                    
                    if (isValid) {
                        break;
                    }
                }
                
                if (isFlat) {
                    /*
                     * No rotation.
                     */
                    dx = 0.0;
                    dy = 0.0;
                }
                else if (isValid) {
                    if (isLeft == false) {
                        dx = -dx;
                    }
                    if (isLateral == false) {
                        dy = -dy;
                    }
                }
            }
            
            Matrix4x4 rotationMatrix = viewingTransform->getRotationMatrix();
            rotationMatrix.rotateX(-dy);
            rotationMatrix.rotateY(dx);
            viewingTransform->setRotationMatrix(rotationMatrix);
        }
    }
    updateYokedBrowserTabs();
}

/**
 * Apply mouse scaling to the displayed model.
 *
 * @param mouseDX
 *    Change in mouse X coordinate.
 * @param mouseDY
 *    Change in mouse Y coordinate.
 */
void
BrowserTabContent::applyMouseScaling(const int32_t /*mouseDX*/,
                                     const int32_t mouseDY)
{
    if (isChartDisplayed()) {
        ModelChart* modelChart = getDisplayedChartModel();
        CaretAssert(modelChart);
        
        if (modelChart->getSelectedChartDataType(m_tabNumber) == ChartDataTypeEnum::CHART_DATA_TYPE_MATRIX) {
            CaretDataFileSelectionModel* matrixSelectionModel = modelChart->getChartableMatrixFileSelectionModel(m_tabNumber);
            if (matrixSelectionModel != NULL) {
                ChartableMatrixInterface* chartableInterface = matrixSelectionModel->getSelectedFileOfType<ChartableMatrixInterface>();
                if (chartableInterface != NULL) {
                    ChartMatrixDisplayProperties* matrixProperties = chartableInterface->getChartMatrixDisplayProperties(m_tabNumber);
                    matrixProperties->setScaleMode(ChartMatrixScaleModeEnum::CHART_MATRIX_SCALE_MANUAL);
                    float scaling = matrixProperties->getViewZooming();
                    if (mouseDY != 0.0) {
                        scaling *= (1.0f + (mouseDY * 0.01));
                    }
                    if (scaling < 0.01) {
                        scaling = 0.01;
                    }
                    matrixProperties->setViewZooming(scaling);
                }
            }
        }        
    }
    else {
        float scaling = getViewingTransformation()->getScaling();
        if (mouseDY != 0.0) {
            scaling *= (1.0f + (mouseDY * 0.01));
        }
        if (scaling < 0.01) {
            scaling = 0.01;
        }
        getViewingTransformation()->setScaling(scaling);
    }
    updateYokedBrowserTabs();
}

/**
 * Apply mouse translation to the displayed model.
 *
 * @param viewportContent
 *    Content of the viewport.
 * @param mousePressX
 *    X coordinate of where mouse was pressed.
 * @param mousePressY
 *    X coordinate of where mouse was pressed.
 * @param mouseDX
 *    Change in mouse X coordinate.
 * @param mouseDY
 *    Change in mouse Y coordinate.
 */
void
BrowserTabContent::applyMouseTranslation(BrainOpenGLViewportContent* viewportContent,
                                         const int32_t mousePressX,
                                         const int32_t mousePressY,
                                         const int32_t mouseDX,
                                         const int32_t mouseDY)
{
    const int tabIndex = getTabNumber();
    
    if (isVolumeSlicesDisplayed()) {
        const float volumeSliceScaling = m_volumeSliceViewingTransformation->getScaling();
        ModelVolume* modelVolume = getDisplayedVolumeModel();
        VolumeMappableInterface* vf = modelVolume->getUnderlayVolumeFile(tabIndex);
        BoundingBox mybox;
        vf->getVoxelSpaceBoundingBox(mybox);
        float cubesize = std::max(std::max(mybox.getDifferenceX(), mybox.getDifferenceY()), mybox.getDifferenceZ());//factor volume bounding box into slowdown for zoomed in
        float slowdown = 0.005f * cubesize / volumeSliceScaling;//when zoomed in, make the movements slower to match - still changes based on viewport currently
        
        float dx = 0.0;
        float dy = 0.0;
        float dz = 0.0;
        switch (this->getSliceViewPlane())
        {
            case VolumeSliceViewPlaneEnum::ALL:
            {
                int viewport[4];
                viewportContent->getModelViewport(viewport);
                const int32_t halfWidth  = viewport[2] / 2;
                const int32_t halfHeight = viewport[3] / 2;
                const int32_t viewportMousePressedX = mousePressX - viewport[0];
                const int32_t viewportMousePressedY = mousePressY - viewport[1];
                bool isRight  = false;
                bool isTop = false;
                if (viewportMousePressedX > halfWidth) {
                    isRight = true;
                }
                if (viewportMousePressedY > halfHeight) {
                    isTop = true;
                }
                //CaretLogInfo("right: " + AString::fromBool(isRight) + " top: " + AString::fromBool(isTop));
                if (isTop)
                {
                    if (isRight)//coronal
                    {
                        dx = mouseDX * slowdown;
                        dz = mouseDY * slowdown;
                    } else {//parasaggital
                        dy = -mouseDX * slowdown;
                        dz = mouseDY * slowdown;
                    }
                } else {
                    if (isRight)//axial
                    {
                        dx = mouseDX * slowdown;
                        dy = mouseDY * slowdown;
                    }//bottom left has no slice
                }
                break;
            }
            case VolumeSliceViewPlaneEnum::AXIAL:
                dx = mouseDX * slowdown;
                dy = mouseDY * slowdown;
                break;
            case VolumeSliceViewPlaneEnum::CORONAL:
                dx = mouseDX * slowdown;
                dz = mouseDY * slowdown;
                break;
            case VolumeSliceViewPlaneEnum::PARASAGITTAL:
                dy = -mouseDX * slowdown;
                dz = mouseDY * slowdown;
                break;
        }
        
        float translation[3];
        m_volumeSliceViewingTransformation->getTranslation(translation);
        translation[0] += dx;
        translation[1] += dy;
        translation[2] += dz;
        m_volumeSliceViewingTransformation->setTranslation(translation);
    }
    else if (isChartDisplayed()) {
        ModelChart* modelChart = getDisplayedChartModel();
        CaretAssert(modelChart);
        
        if (modelChart->getSelectedChartDataType(m_tabNumber) == ChartDataTypeEnum::CHART_DATA_TYPE_MATRIX) {
            CaretDataFileSelectionModel* matrixSelectionModel = modelChart->getChartableMatrixFileSelectionModel(m_tabNumber);
            if (matrixSelectionModel != NULL) {
                ChartableMatrixInterface* chartableInterface = matrixSelectionModel->getSelectedFileOfType<ChartableMatrixInterface>();
                if (chartableInterface != NULL) {
                    ChartMatrixDisplayProperties* matrixProperties = chartableInterface->getChartMatrixDisplayProperties(m_tabNumber);
                    matrixProperties->setScaleMode(ChartMatrixScaleModeEnum::CHART_MATRIX_SCALE_MANUAL);
                    float translation[2];
                    matrixProperties->getViewPanning(translation);
                    translation[0] += mouseDX;
                    translation[1] += mouseDY;
                    matrixProperties->setViewPanning(translation);
                }
            }
        }
    }
    else if (isCerebellumDisplayed()) {
        const float screenDX = mouseDX;
        const float screenDY = mouseDY;
        
        float translateDX = 0.0;
        float translateDY = 0.0;
        float translateDZ = 0.0;
        
        ModelSurfaceMontage* montageModel = getDisplayedSurfaceMontageModel();
        if (montageModel != NULL) {
            std::vector<const SurfaceMontageViewport*> montageViewports;
            montageModel->getSurfaceMontageViewportsForTransformation(getTabNumber(),
                                                                      montageViewports);
            
            bool foundMontageViewportFlag = false;
            
            const int32_t numViewports = static_cast<int32_t>(montageViewports.size());
            for (int32_t ivp = 0; ivp < numViewports; ivp++) {
                const SurfaceMontageViewport* smv = montageViewports[ivp];
                
                if (smv->isInside(mousePressX,
                                  mousePressY)) {
                    switch (smv->getProjectionViewType()) {
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_ANTERIOR:
                            translateDX =  screenDX;
                            translateDY =  screenDY;
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_DORSAL:
                            translateDX = screenDX;
                            translateDY = screenDY;
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_POSTERIOR:
                            translateDX = screenDX;
                            translateDY = screenDY;
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_VENTRAL:
                            translateDX =  screenDX;
                            translateDY =  screenDY;
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_FLAT_SURFACE:
                            translateDX = screenDX;
                            translateDY = screenDY;
                            foundMontageViewportFlag = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_LATERAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_MEDIAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_FLAT_SURFACE:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_LATERAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_MEDIAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_FLAT_SURFACE:
                            break;
                    }
                }
                if (foundMontageViewportFlag) {
                    break;
                }
            }
        }
        else {
            translateDX = screenDX;
            translateDY = screenDY;
        }

        float translation[3];
        m_cerebellumViewingTransformation->getTranslation(translation);
        translation[0] += translateDX;
        translation[1] += translateDY;
        translation[2] += translateDZ;
        m_cerebellumViewingTransformation->setTranslation(translation);
    }
    else {
        float dx = mouseDX;
        float dy = mouseDY;
        
        ModelSurfaceMontage* montageModel = getDisplayedSurfaceMontageModel();
        
        
        if (montageModel != NULL) {
            std::vector<const SurfaceMontageViewport*> montageViewports;
            montageModel->getSurfaceMontageViewportsForTransformation(getTabNumber(),
                                                                      montageViewports);
            
            bool isValid = false;
            bool isLeft = true;
            bool isLateral = false;
            const int32_t numViewports = static_cast<int32_t>(montageViewports.size());
            for (int32_t ivp = 0; ivp < numViewports; ivp++) {
                const SurfaceMontageViewport* smv = montageViewports[ivp];
                if (smv->isInside(mousePressX,
                                 mousePressY)) {
                    switch (smv->getProjectionViewType()) {
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_ANTERIOR:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_DORSAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_POSTERIOR:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_VENTRAL:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_FLAT_SURFACE:
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_LATERAL:
                            isLeft = true;
                            isLateral = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_MEDIAL:
                            isLeft = true;
                            isLateral = false;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_FLAT_SURFACE:
                            isLeft = true;
                            isLateral = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_LATERAL:
                            isLeft = false;
                            isLateral = true;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_MEDIAL:
                            isLeft = false;
                            isLateral = false;
                            break;
                        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_FLAT_SURFACE:
                            isLeft = false;
                            isLateral = true;
                            break;
                    }
                    isValid = true;
                }
                
                if (isValid) {
                    break;
                }
            }
            
            if (isValid) {
                if (isLeft == false) {
                    dx = -dx;
                }
                if (isLateral == false) {
                    dx = -dx;
                }
                
                float translation[3];
                getViewingTransformation()->getTranslation(translation);
                translation[0] += dx;
                translation[1] += dy;
                getViewingTransformation()->setTranslation(translation);
            }
        }
        else {
            if (getProjectionViewType() == ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_LATERAL) {
                dx = -dx;
            }
            else if (getProjectionViewType() == ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_FLAT_SURFACE) {
                dx = -dx;
            }
            float translation[3];
            getViewingTransformation()->getTranslation(translation);
            translation[0] += dx;
            translation[1] += dy;
            getViewingTransformation()->setTranslation(translation);
        }
    }
    updateYokedBrowserTabs();
}

/**
 * Get the transformations for drawing a model.
 *
 * @param projectionViewType
 *    Type of projection view
 * @param translationOut
 *    Translation
 * @param rotationMatrixOut
 *    OpenGL rotation matrix.
 * @param scalingOut
 *    Scaling.
 */
void
BrowserTabContent::getTransformationsForOpenGLDrawing(const ProjectionViewTypeEnum::Enum projectionViewType,
                                                      float translationOut[3],
                                                      double rotationMatrixOut[16],
                                                      float& scalingOut) const
{
    /*
     * Check for volume slice viewing
     */
    if (isVolumeSlicesDisplayed()) {
        m_volumeSliceViewingTransformation->getTranslation(translationOut);
        
        Matrix4x4 rotationMatrix = m_volumeSliceViewingTransformation->getRotationMatrix();
        rotationMatrix.getMatrixForOpenGL(rotationMatrixOut);
        
        scalingOut = m_volumeSliceViewingTransformation->getScaling();
        
        return;
    }
    
    /*
     * Surfaces may need a modification to the rotation matrix
     * dependent upon the projection view type.
     */
    
    Matrix4x4 rotationMatrix = getViewingTransformation()->getRotationMatrix();
    getViewingTransformation()->getTranslation(translationOut);
    scalingOut = getViewingTransformation()->getScaling();
    
    double rotationX, rotationY, rotationZ;
    rotationMatrix.getRotation(rotationX,
                               rotationY,
                               rotationZ);
    const double rotationFlippedX = -rotationX;
    const double rotationFlippedY = 180.0 - rotationY;
    
    switch (projectionViewType) {
        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_ANTERIOR:
        {
            Matrix4x4 matrixOut = rotationMatrix;
            
            Matrix4x4 anteriorMatrix;
            anteriorMatrix.setRotation(90.0, 0.0, -180.0);
            
            matrixOut.postmultiply(anteriorMatrix);
            matrixOut.getMatrixForOpenGL(rotationMatrixOut);
            return;
        }
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_DORSAL:
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_POSTERIOR:
        {
            Matrix4x4 matrixOut = rotationMatrix;
            
            Matrix4x4 posteriorMatrix;
            posteriorMatrix.setRotation(-90.0, 0.0, 0.0);
            
            matrixOut.postmultiply(posteriorMatrix);
            matrixOut.getMatrixForOpenGL(rotationMatrixOut);
            return;
        }
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_VENTRAL:
        {
            Matrix4x4 matrixOut = rotationMatrix;
            
            Matrix4x4 ventralMatrix;
            ventralMatrix.setRotation(0.0, 180.0, 180.0);
            
            matrixOut.postmultiply(ventralMatrix);
            matrixOut.getMatrixForOpenGL(rotationMatrixOut);
            return;
        }
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_CEREBELLUM_FLAT_SURFACE:
            rotationX =     0.0;
            rotationY =     0.0;
            rotationZ =     0.0;
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_LATERAL:
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_MEDIAL:
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_FLAT_SURFACE:
            rotationX =     0.0;
            rotationY =     0.0;
            rotationZ =     0.0;
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_LATERAL:
            rotationX = rotationFlippedX;
            rotationY = rotationFlippedY;
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_MEDIAL:
            rotationX = rotationFlippedX;
            rotationY = rotationFlippedY;
            break;
        case ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_FLAT_SURFACE:
            rotationX =   0.0;
            rotationY = 180.0;
            rotationZ =   0.0;
            break;
    }
    
    Matrix4x4 matrix;
    matrix.setRotation(rotationX,
                       rotationY,
                       rotationZ);
    matrix.getMatrixForOpenGL(rotationMatrixOut);
}

/**
 * Place the transformations for the given window tab into
 * the model transform.
 * @param windowTabNumber
 *    Tab number for transformations.
 * @param modelTransform
 *    Model transform into which transformations are loaded.
 */
void
BrowserTabContent::getTransformationsInModelTransform(ModelTransform& modelTransform) const
{
    modelTransform.setTranslation(getTranslation());
    
    const Matrix4x4 rotMatrix = getRotationMatrix();
    float m[4][4];
    rotMatrix.getMatrix(m);
    modelTransform.setRotation(m);
    
    const Matrix4x4 obliqueRotationMatrix = getObliqueVolumeRotationMatrix();
    float mob[4][4];
    obliqueRotationMatrix.getMatrix(mob);
    modelTransform.setObliqueRotation(mob);
    
    modelTransform.setScaling(getScaling());
}

/**
 * Apply the transformations to the browser tab.
 * @param modelTransform
 *    Model transform into which transformations are retrieved.
 */
void
BrowserTabContent::setTransformationsFromModelTransform(const ModelTransform& modelTransform)
{
    float translation[3];
    modelTransform.getTranslation(translation);
    const float tx = translation[0];
    const float ty = translation[1];
    const float tz = translation[2];
    
    setTranslation(tx, ty, tz);

    float m[4][4];
    modelTransform.getRotation(m);
    
    Matrix4x4 rotationMatrix;
    rotationMatrix.setMatrix(m);
    setRotationMatrix(rotationMatrix);

    float mob[4][4];
    modelTransform.getObliqueRotation(mob);
    Matrix4x4 obliqueRotationMatrix;
    obliqueRotationMatrix.setMatrix(mob);
    setObliqueVolumeRotationMatrix(obliqueRotationMatrix);

    const float scale = modelTransform.getScaling();
    setScaling(scale);
    updateYokedBrowserTabs();
}


/**
 * Create a scene for an instance of a class.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @param instanceName
 *    Name of the class' instance.
 *
 * @return Pointer to SceneClass object representing the state of
 *    this object.  Under some circumstances a NULL pointer may be
 *    returned.  Caller will take ownership of returned object.
 */
SceneClass* 
BrowserTabContent::saveToScene(const SceneAttributes* sceneAttributes,
                                const AString& instanceName)
{
    SceneClass* sceneClass = new SceneClass(instanceName,
                                            "BrowserTabContent",
                                            3); // version 3 as of 4/22/2014

    float obliqueMatrix[16];
    m_obliqueVolumeRotationMatrix->getMatrixForOpenGL(obliqueMatrix);
    sceneClass->addFloatArray("m_obliqueVolumeRotationMatrix", obliqueMatrix, 16);
    
    m_sceneClassAssistant->saveMembers(sceneAttributes, 
                                       sceneClass);
    
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
 *     sceneClass for the instance of a class that implements
 *     this interface.  May be NULL for some types of scenes.
 */
void 
BrowserTabContent::restoreFromScene(const SceneAttributes* sceneAttributes,
                              const SceneClass* sceneClass)
{
    if (sceneClass == NULL) {
        return;
    }
    
    m_sceneClassAssistant->restoreMembers(sceneAttributes, 
                                          sceneClass);
    
    m_obliqueVolumeRotationMatrix->identity();
    float obliqueMatrix[16];
    const int32_t numInObliqueArray = sceneClass->getFloatArrayValue("m_obliqueVolumeRotationMatrix", obliqueMatrix, 16);
    if (numInObliqueArray == 16) {
        m_obliqueVolumeRotationMatrix->setMatrixFromOpenGL(obliqueMatrix);
    }
    
    /*
     * In older version of workbench, transformation were stored in the
     * model for each tab, so try to restore them.
     */
    if (sceneClass->getVersionNumber() < 2) {
        float translation[3];
        float scaling;
        float rotationMatrix[4][4];
        
        const Model* model = getModelForDisplay();
        if (model != NULL) {
            const bool valid = model->getOldSceneTransformation(m_tabNumber,
                                                                translation,
                                                                scaling,
                                                                rotationMatrix);
            if (valid) {
                setTranslation(translation);
                setScaling(scaling);
                Matrix4x4 m;
                m.setMatrix(rotationMatrix);
                
                ModelSurface* ms = getDisplayedSurfaceModel();
                if (ms != NULL) {
                    /*
                     * Right hemispheres need rotations changed for
                     * proper viewing.
                     */
                    const StructureEnum::Enum structure = ms->getSurface()->getStructure();
                    if (StructureEnum::isRight(structure)) {
                        double rotationX, rotationY, rotationZ;
                        m.getRotation(rotationX,
                                      rotationY,
                                      rotationZ);
                        //rotationX = -rotationX;
                        //rotationY = 180.0 - rotationY;
                        rotationY = 90 + rotationY;
                        rotationZ = -rotationZ;
                        m.identity();
                        m.setRotation(rotationX,
                                           rotationY,
                                           rotationZ);
                    }
                }
                setRotationMatrix(m);
            }
        }
    }
    
    if (getDisplayedWholeBrainModel() != NULL) {
        /*
         * As of 19sep2013 whole brain and volume slice settings were merged
         * (whole brain slice settings removed).  For compatibility, if a
         * whole brain model is being viewed and whole brain slice settings
         * are found, allow them to override volume slice settings.
         */
        const SceneClass* wholeBrainVolumeSettings = sceneClass->getClass("m_wholeBrainSliceSettings");
        if (wholeBrainVolumeSettings != NULL) {
            VolumeSliceSettings settings;
            settings.restoreFromScene(sceneAttributes, wholeBrainVolumeSettings);
            *m_volumeSliceSettings = settings;
        }
    }
    
    /**
     * Check for now obsolete clipping coordinate array.  If found it is an
     * old scene so update the clipping planes.
     */
    const SceneClassArray* oldClippingCoordinateClassArray = sceneClass->getClassArray("m_clippingCoordinate");
    if (oldClippingCoordinateClassArray != NULL) {
        float clipCoords[3];
        if (sceneClass->getFloatArrayValue("m_clippingCoordinate",
                                           clipCoords,
                                           3) != 3) {
            clipCoords[0] = 0.0;
            clipCoords[1] = 0.0;
            clipCoords[2] = 0.0;
        }
        
        float clipThick[3];
        if (sceneClass->getFloatArrayValue("m_clippingThickness",
                                           clipThick,
                                           3) != 3) {
            clipThick[0] = 0.0;
            clipThick[1] = 0.0;
            clipThick[2] = 0.0;
        }
        
        bool clipEnabled[3];
        if (sceneClass->getBooleanArrayValue("m_clippingEnabled",
                                             clipEnabled,
                                             3) != 3) {
            clipEnabled[0] = false;
            clipEnabled[1] = false;
            clipEnabled[2] = false;
        }
        
        m_clippingPlaneGroup->resetToDefaultValues();
        
        m_clippingPlaneGroup->setXAxisSelected(clipEnabled[0]);
        m_clippingPlaneGroup->setYAxisSelected(clipEnabled[1]);
        m_clippingPlaneGroup->setZAxisSelected(clipEnabled[2]);
        
        m_clippingPlaneGroup->setTranslation(clipCoords);
        m_clippingPlaneGroup->setThickness(clipThick);
    }

    /*
     * In older version of workbench, there was no flat surface
     * viewing transformation as it used the same transformations
     * as other surfaces.
     */
    if (sceneClass->getVersionNumber() < 3) {
        *m_flatSurfaceViewingTransformation = *m_viewingTransformation;
    }
}

/**
 * Get the clipping planes enabled attributes
 *
 * @param xEnabled
 *    X clipping plane enabled.
 * @param yEnabled
 *    Y clipping plane enabled.
 * @param zEnabled
 *    Z clipping plane enabled.
 * @param surfaceEnabled
 *    Surface clipping enabled.
 * @param volumeEnabled
 *    Volume clipping enabled.
 * @param featuresEnabled
 *    Features enabled.
 * @param displayClippingBox
 *    Display the clipping box.
 */
void
BrowserTabContent::getClippingPlaneEnabled(bool& xEnabled,
                                           bool& yEnabled,
                                           bool& zEnabled,
                                           bool& surfaceEnabled,
                                           bool& volumeEnabled,
                                           bool& featuresEnabled) const
{
    xEnabled = m_clippingPlaneGroup->isXAxisSelected();
    yEnabled = m_clippingPlaneGroup->isYAxisSelected();
    zEnabled = m_clippingPlaneGroup->isZAxisSelected();
    
    surfaceEnabled = m_clippingPlaneGroup->isSurfaceSelected();
    volumeEnabled  = m_clippingPlaneGroup->isVolumeSelected();
    featuresEnabled = m_clippingPlaneGroup->isFeaturesSelected();
}

/**
 * Set the clipping planes enabled attributes
 *
 * @param xEnabled
 *    X clipping plane enabled.
 * @param yEnabled
 *    Y clipping plane enabled.
 * @param zEnabled
 *    Z clipping plane enabled.
 * @param surfaceEnabled
 *    Surface clipping enabled.
 * @param volumeEnabled
 *    Volume clipping enabled.
 * @param featuresEnabled
 *    Features enabled.
 * @param displayClippingBox
 *    Display the clipping box.
 */
void
BrowserTabContent::setClippingPlaneEnabled(const bool xEnabled,
                                           const bool yEnabled,
                                           const bool zEnabled,
                                           const bool surfaceEnabled,
                                           const bool volumeEnabled,
                                           const bool featuresEnabled)
{
    m_clippingPlaneGroup->setXAxisSelected(xEnabled);
    m_clippingPlaneGroup->setYAxisSelected(yEnabled);
    m_clippingPlaneGroup->setZAxisSelected(zEnabled);
    
    m_clippingPlaneGroup->setSurfaceSelected(surfaceEnabled);
    m_clippingPlaneGroup->setVolumeSelected(volumeEnabled);
    m_clippingPlaneGroup->setFeaturesSelected(featuresEnabled);

    updateYokedBrowserTabs();
}

/**
 * Get the clipping planes transformations.
 *
 * @param panning
 *    Panning (translation) of the clipping planes.
 * @param rotation
 *    Rotation of clipping planes.
 * @param thickness
 *    Thickness of the clipping planes.
 */
void
BrowserTabContent::getClippingPlaneTransformation(float panning[3],
                                                  float rotation[3],
                                                  float thickness[3],
                                                  bool& displayClippingBox) const
{
    m_clippingPlaneGroup->getTranslation(panning);
    m_clippingPlaneGroup->getRotationAngles(rotation);
    m_clippingPlaneGroup->getThickness(thickness);
    
    displayClippingBox = m_clippingPlaneGroup->isDisplayClippingBoxSelected();
}

/**
 * Set the clipping planes transformations.
 *
 * @param panning
 *    Panning (translation) of the clipping planes.
 * @param rotation
 *    Rotation of clipping planes.
 * @param thickness
 *    Thickness of the clipping planes.
 */
void
BrowserTabContent::setClippingPlaneTransformation(const float panning[3],
                                                  const float rotation[3],
                                                  const float thickness[3],
                                                  const bool displayClippingBox)
{
    m_clippingPlaneGroup->setTranslation(panning);
    m_clippingPlaneGroup->setRotationAngles(rotation);
    m_clippingPlaneGroup->setThickness(thickness);
    
    m_clippingPlaneGroup->setDisplayClippingBoxSelected(displayClippingBox);

    updateYokedBrowserTabs();
}

/**
 * Get the clipping plane group (const method).
 *
 * NOTE: Because of yoking, only a const instance of the clipping plane
 * group is available.  To adjust the clipping planes use the methods
 * in this class so that yoking is properly updated.
 */
const ClippingPlaneGroup*
BrowserTabContent::getClippingPlaneGroup() const
{
    return m_clippingPlaneGroup;
}

/**
 * Reset the clipping plane transformations.
 */
void
BrowserTabContent::resetClippingPlaneTransformation()
{
    m_clippingPlaneGroup->resetTransformation();
    updateYokedBrowserTabs();
}

/**
 * @return the projection view type (view from left/right)
 */
ProjectionViewTypeEnum::Enum
BrowserTabContent::getProjectionViewType() const
{
    ProjectionViewTypeEnum::Enum projectionViewType = ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_LATERAL;
    
    const ModelSurface* modelSurface = getDisplayedSurfaceModel();
    if (modelSurface != NULL) {
        const SurfaceFile* surfaceFile = modelSurface->getSurface();
        if (surfaceFile != NULL) {
            const StructureEnum::Enum structure = surfaceFile->getStructure();
            const SurfaceTypeEnum::Enum surfaceType = surfaceFile->getSurfaceType();
            if (StructureEnum::isRight(structure)) {
                projectionViewType = ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_LATERAL;
                if (surfaceType == SurfaceTypeEnum::FLAT) {
                    projectionViewType = ProjectionViewTypeEnum::PROJECTION_VIEW_RIGHT_FLAT_SURFACE;
                }
            }
            else {
                projectionViewType = ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_LATERAL;
                if (surfaceType == SurfaceTypeEnum::FLAT) {
                    projectionViewType = ProjectionViewTypeEnum::PROJECTION_VIEW_LEFT_FLAT_SURFACE;
                }
            }
        }
    }
    
    return projectionViewType;
}

/**
 * @return The slice view plane.
 *
 */
VolumeSliceViewPlaneEnum::Enum
BrowserTabContent::getSliceViewPlane() const
{
    return m_volumeSliceSettings->getSliceViewPlane();
}

/**
 * Set the slice view plane.
 * @param windowTabNumber
 *    New value for slice plane.
 */
void
BrowserTabContent::setSliceViewPlane(const VolumeSliceViewPlaneEnum::Enum slicePlane)
{
    m_volumeSliceSettings->setSliceViewPlane(slicePlane);
    updateYokedBrowserTabs();
}

/**
 * @return Type of slice drawing (single/montage)
 */
VolumeSliceDrawingTypeEnum::Enum
BrowserTabContent::getSliceDrawingType() const
{
    return m_volumeSliceSettings->getSliceDrawingType();
}

/**
 * Set type of slice drawing (single/montage)
 *
 * @param sliceDrawingType
 *    New value for slice drawing type.
 */
void
BrowserTabContent::setSliceDrawingType(const VolumeSliceDrawingTypeEnum::Enum sliceDrawingType)
{
    m_volumeSliceSettings->setSliceDrawingType(sliceDrawingType);
    updateYokedBrowserTabs();
}

/**
 * @return Type of slice projection (oblique/orthogonal)
 */
VolumeSliceProjectionTypeEnum::Enum
BrowserTabContent::getSliceProjectionType() const
{
    return m_volumeSliceSettings->getSliceProjectionType();
}

/**
 * Set type of slice projection (oblique/orthogonal)
 *
 * @param sliceProjectionType
 *    New value for slice projection type.
 */
void
BrowserTabContent::setSliceProjectionType(const VolumeSliceProjectionTypeEnum::Enum sliceProjectionType)
{
    m_volumeSliceSettings->setSliceProjectionType(sliceProjectionType);
    updateYokedBrowserTabs();
}

/**
 * @return the montage number of columns for the given window tab.
 */
int32_t
BrowserTabContent::getMontageNumberOfColumns() const
{
    return m_volumeSliceSettings->getMontageNumberOfColumns();
}

/**
 * Set the montage number of columns in the given window tab.
 * @param montageNumberOfColumns
 *    New value for montage number of columns
 */
void
BrowserTabContent::setMontageNumberOfColumns(const int32_t montageNumberOfColumns)
{
    m_volumeSliceSettings->setMontageNumberOfColumns(montageNumberOfColumns);
    updateYokedBrowserTabs();
}

/**
 * @return the montage number of rows for the given window tab.
 */
int32_t
BrowserTabContent::getMontageNumberOfRows() const
{
    return m_volumeSliceSettings->getMontageNumberOfRows();
}

/**
 * Set the montage number of rows.
 * @param montageNumberOfRows
 *    New value for montage number of rows
 */
void
BrowserTabContent::setMontageNumberOfRows(const int32_t montageNumberOfRows)
{
    m_volumeSliceSettings->setMontageNumberOfRows(montageNumberOfRows);
    updateYokedBrowserTabs();
}

/**
 * @return the montage slice spacing.
 */
int32_t
BrowserTabContent::getMontageSliceSpacing() const
{
    return m_volumeSliceSettings->getMontageSliceSpacing();
}

/**
 * Set the montage slice spacing.
 * @param montageSliceSpacing
 *    New value for montage slice spacing
 */
void
BrowserTabContent::setMontageSliceSpacing(const int32_t montageSliceSpacing)
{
    m_volumeSliceSettings->setMontageSliceSpacing(montageSliceSpacing);
    updateYokedBrowserTabs();
}

/**
 * Set the selected slices to the origin.
 */
void
BrowserTabContent::setSlicesToOrigin()
{
    selectSlicesAtOrigin();
    updateYokedBrowserTabs();
}

/**
 * Reset the slices.
 */
void
BrowserTabContent::reset()
{
    if (isVolumeSlicesDisplayed()
        || isWholeBrainDisplayed()) {
        m_volumeSliceSettings->reset();
        m_obliqueVolumeRotationMatrix->identity();
    }
    updateYokedBrowserTabs();
}

/**
 * Update the slices coordinates so that they are valid for
 * the given VolumeFile.
 * @param volumeFile
 *   File for which slice coordinates are made valid.
 */
void
BrowserTabContent::updateForVolumeFile(const VolumeMappableInterface* volumeFile)
{
    m_volumeSliceSettings->updateForVolumeFile(volumeFile);
}

/**
 * Set the slice indices so that they are at the origin.
 */
void
BrowserTabContent::selectSlicesAtOrigin()
{
    m_volumeSliceSettings->selectSlicesAtOrigin();
    updateYokedBrowserTabs();
}

/**
 * Set the selected slices to the given coordinate.
 * @param xyz
 *    Coordinate for selected slices.
 */
void
BrowserTabContent::selectSlicesAtCoordinate(const float xyz[3])
{
    m_volumeSliceSettings->selectSlicesAtCoordinate(xyz);
    updateYokedBrowserTabs();
}

/**
 * If true, selected volume slices in tab move to location
 * of the identification operation.
 */
bool
BrowserTabContent::isIdentificationUpdatesVolumeSlices() const
{
    return m_identificationUpdatesVolumeSlices;
}

/**
 * Update selected volume slices in tab move to location
 * of the identification operation.
 *
 * @param status
 *    New status.
 */
void
BrowserTabContent::setIdentificationUpdatesVolumeSlices(const bool status)
{
    m_identificationUpdatesVolumeSlices = status;
    updateYokedBrowserTabs();
}

/**
 * Return the axial slice index.
 * @return
 *   Axial slice index or negative if invalid
 */
int64_t
BrowserTabContent::getSliceIndexAxial(const VolumeMappableInterface* volumeFile) const
{
    return m_volumeSliceSettings->getSliceIndexAxial(volumeFile);
}

/**
 * Set the axial slice index.
 * @param
 *    New value for axial slice index.
 */
void
BrowserTabContent::setSliceIndexAxial(const VolumeMappableInterface* volumeFile,
                                        const int64_t sliceIndexAxial)
{
    m_volumeSliceSettings->setSliceIndexAxial(volumeFile, sliceIndexAxial);
    updateYokedBrowserTabs();
}

/**
 * Return the coronal slice index.
 * @return
 *   Coronal slice index.
 */
int64_t
BrowserTabContent::getSliceIndexCoronal(const VolumeMappableInterface* volumeFile) const
{
    return m_volumeSliceSettings->getSliceIndexCoronal(volumeFile);
}


/**
 * Set the coronal slice index.
 * @param sliceIndexCoronal
 *    New value for coronal slice index.
 */
void
BrowserTabContent::setSliceIndexCoronal(const VolumeMappableInterface* volumeFile,
                                          const int64_t sliceIndexCoronal)
{
    m_volumeSliceSettings->setSliceIndexCoronal(volumeFile, sliceIndexCoronal);
    updateYokedBrowserTabs();
}

/**
 * Return the parasagittal slice index.
 * @return
 *   Parasagittal slice index.
 */
int64_t
BrowserTabContent::getSliceIndexParasagittal(const VolumeMappableInterface* volumeFile) const
{
    return m_volumeSliceSettings->getSliceIndexParasagittal(volumeFile);
}

/**
 * Set the parasagittal slice index.
 * @param sliceIndexParasagittal
 *    New value for parasagittal slice index.
 */
void
BrowserTabContent::setSliceIndexParasagittal(const VolumeMappableInterface* volumeFile,
                                               const int64_t sliceIndexParasagittal)
{
    m_volumeSliceSettings->setSliceIndexParasagittal(volumeFile,
                                                         sliceIndexParasagittal);
    updateYokedBrowserTabs();
}

/**
 * @return Coordinate of axial slice.
 */
float
BrowserTabContent::getSliceCoordinateAxial() const
{
    return m_volumeSliceSettings->getSliceCoordinateAxial();
}

/**
 * Set the coordinate for the axial slice.
 * @param z
 *    Z-coordinate for axial slice.
 */
void
BrowserTabContent::setSliceCoordinateAxial(const float z)
{
    m_volumeSliceSettings->setSliceCoordinateAxial(z);
    updateYokedBrowserTabs();
}

/**
 * @return Coordinate of coronal slice.
 */
float
BrowserTabContent::getSliceCoordinateCoronal() const
{
    return m_volumeSliceSettings->getSliceCoordinateCoronal();
}

/**
 * Set the coordinate for the coronal slice.
 * @param y
 *    Y-coordinate for coronal slice.
 */
void
BrowserTabContent::setSliceCoordinateCoronal(const float y)
{
    m_volumeSliceSettings->setSliceCoordinateCoronal(y);
    updateYokedBrowserTabs();
}

/**
 * @return Coordinate of parasagittal slice.
 */
float
BrowserTabContent::getSliceCoordinateParasagittal() const
{
    return m_volumeSliceSettings->getSliceCoordinateParasagittal();
}

/**
 * Set the coordinate for the parasagittal slice.
 * @param x
 *    X-coordinate for parasagittal slice.
 */
void
BrowserTabContent::setSliceCoordinateParasagittal(const float x)
{
    m_volumeSliceSettings->setSliceCoordinateParasagittal(x);
    updateYokedBrowserTabs();
}

/**
 * Is the parasagittal slice enabled?
 * @return
 *    Enabled status of parasagittal slice.
 */
bool
BrowserTabContent::isSliceParasagittalEnabled() const
{
    return m_volumeSliceSettings->isSliceParasagittalEnabled();
}

/**
 * Set the enabled status of the parasagittal slice.
 * @param sliceEnabledParasagittal
 *    New enabled status.
 */
void
BrowserTabContent::setSliceParasagittalEnabled(const bool sliceEnabledParasagittal)
{
    m_volumeSliceSettings->setSliceParasagittalEnabled(sliceEnabledParasagittal);
    updateYokedBrowserTabs();
}

/**
 * Is the coronal slice enabled?
 * @return
 *    Enabled status of coronal slice.
 */
bool
BrowserTabContent::isSliceCoronalEnabled() const
{
    return m_volumeSliceSettings->isSliceCoronalEnabled();
}

/**
 * Set the enabled status of the coronal slice.
 * @param sliceEnabledCoronal
 *    New enabled status.
 */
void
BrowserTabContent::setSliceCoronalEnabled(const bool sliceEnabledCoronal)
{
    m_volumeSliceSettings->setSliceCoronalEnabled(sliceEnabledCoronal);
    updateYokedBrowserTabs();
}

/**
 * Is the axial slice enabled?
 * @return
 *    Enabled status of axial slice.
 */
bool
BrowserTabContent::isSliceAxialEnabled() const
{
    return m_volumeSliceSettings->isSliceAxialEnabled();
}

/**
 * Set the enabled status of the axial slice.
 * @param sliceEnabledAxial
 *    New enabled status.
 */
void
BrowserTabContent::setSliceAxialEnabled(const bool sliceEnabledAxial)
{
    m_volumeSliceSettings->setSliceAxialEnabled(sliceEnabledAxial);
    updateYokedBrowserTabs();
}



/**
 * @return Enabled status for left cerebral cortex.
 */
bool
BrowserTabContent::isWholeBrainLeftEnabled() const
{
    return m_wholeBrainSurfaceSettings->isLeftEnabled();
}

/**
 * Set the enabled status for the left hemisphere.
 * @param windowTabNumber
 *    Index of window tab.
 * @param enabled
 *    New enabled status.
 */
void
BrowserTabContent::setWholeBrainLeftEnabled(const bool enabled)
{
    m_wholeBrainSurfaceSettings->setLeftEnabled(enabled);
    updateYokedBrowserTabs();
}

/**
 * @return Enabled status for right cerebral cortex.
 */
bool
BrowserTabContent::isWholeBrainRightEnabled() const
{
    return m_wholeBrainSurfaceSettings->isRightEnabled();
}

/**
 * Set the enabled status for the right hemisphere.
 * @param enabled
 *    New enabled status.
 */
void
BrowserTabContent::setWholeBrainRightEnabled(const bool enabled)
{
    m_wholeBrainSurfaceSettings->setRightEnabled(enabled);
    updateYokedBrowserTabs();
}

/**
 * @return Enabled status for cerebellum.
 */
bool
BrowserTabContent::isWholeBrainCerebellumEnabled() const
{
    return m_wholeBrainSurfaceSettings->isCerebellumEnabled();
}

/**
 * Set the enabled status for the cerebellum.
 * @param enabled
 *    New enabled status.
 */
void
BrowserTabContent::setWholeBrainCerebellumEnabled(const bool enabled)
{
    m_wholeBrainSurfaceSettings->setCerebellumEnabled(enabled);
    updateYokedBrowserTabs();
}

/**
 * @return The separation between the left and right surfaces.
 */
float
BrowserTabContent::getWholeBrainLeftRightSeparation() const
{
    return m_wholeBrainSurfaceSettings->getLeftRightSeparation();
}

/**
 * Set the separation between the cerebellum and the left/right surfaces.
 * @param separation
 *     New value for separation.
 */
void
BrowserTabContent::setWholeBrainLeftRightSeparation(const float separation)
{
    m_wholeBrainSurfaceSettings->setLeftRightSeparation(separation);
    updateYokedBrowserTabs();
}

/**
 * @return The separation between the left/right surfaces.
 */
float
BrowserTabContent::getWholeBrainCerebellumSeparation() const
{
    return m_wholeBrainSurfaceSettings->getCerebellumSeparation();
}

/**
 * Set the separation between the cerebellum and the left and right surfaces.
 * @param separation
 *     New value for separation.
 */
void
BrowserTabContent::setWholeBrainCerebellumSeparation(const float separation)
{
    m_wholeBrainSurfaceSettings->setCerebellumSeparation(separation);
    updateYokedBrowserTabs();
}

/**
 * @return Selected yoking group.
 */
YokingGroupEnum::Enum
BrowserTabContent::getYokingGroup() const
{
    return m_yokingGroup;
}

/**
 * Set the selected yoking group.
 *
 * @param yokingGroup
 *    New value for yoking group.
 */
void
BrowserTabContent::setYokingGroup(const YokingGroupEnum::Enum yokingGroup)
{
    m_yokingGroup = yokingGroup;
    
    if (m_yokingGroup == YokingGroupEnum::YOKING_GROUP_OFF) {
        return;
    }
    
    /*
     * Find another browser tab using the same yoking as 'me' and copy
     * yoked data from the other browser tab.
     */
    for (std::set<BrowserTabContent*>::iterator iter = s_allBrowserTabContent.begin();
         iter != s_allBrowserTabContent.end();
         iter++) {
        BrowserTabContent* btc = *iter;
        if (btc != this) {
            if (btc->getYokingGroup() == m_yokingGroup) {
                *m_viewingTransformation = *btc->m_viewingTransformation;
                *m_flatSurfaceViewingTransformation = *btc->m_flatSurfaceViewingTransformation;
                *m_cerebellumViewingTransformation = *btc->m_cerebellumViewingTransformation;
                *m_volumeSliceViewingTransformation = *btc->m_volumeSliceViewingTransformation;
                *m_volumeSliceSettings = *btc->m_volumeSliceSettings;
                *m_obliqueVolumeRotationMatrix = *btc->m_obliqueVolumeRotationMatrix;
                *m_clippingPlaneGroup = *btc->m_clippingPlaneGroup;
                m_identificationUpdatesVolumeSlices = btc->m_identificationUpdatesVolumeSlices;
                break;
            }
        }
    }
}

/**
 * @return Is this browser tab yoked?
 */
bool
BrowserTabContent::isYoked() const
{
    const bool yoked = (m_yokingGroup != YokingGroupEnum::YOKING_GROUP_OFF);
    return yoked;
}

/**
 * Update other browser tabs with yoked data.
 */
void
BrowserTabContent::updateYokedBrowserTabs()
{
    if (m_yokingGroup == YokingGroupEnum::YOKING_GROUP_OFF) {
        return;
    }
    
    /*
     * Copy yoked data from 'me' to all other yoked browser tabs
     */
    for (std::set<BrowserTabContent*>::iterator iter = s_allBrowserTabContent.begin();
         iter != s_allBrowserTabContent.end();
         iter++) {
        BrowserTabContent* btc = *iter;
        if (btc != this) {
            if (btc->getYokingGroup() == m_yokingGroup) {
                *btc->m_viewingTransformation = *m_viewingTransformation;
                *btc->m_flatSurfaceViewingTransformation = *m_flatSurfaceViewingTransformation;
                *btc->m_cerebellumViewingTransformation = *m_cerebellumViewingTransformation;
                *btc->m_volumeSliceViewingTransformation = *m_volumeSliceViewingTransformation;
                *btc->m_volumeSliceSettings = *m_volumeSliceSettings;
                *btc->m_obliqueVolumeRotationMatrix = *m_obliqueVolumeRotationMatrix;
                *btc->m_clippingPlaneGroup = *m_clippingPlaneGroup;
                btc->m_identificationUpdatesVolumeSlices = m_identificationUpdatesVolumeSlices;
            }
        }
    }
}


