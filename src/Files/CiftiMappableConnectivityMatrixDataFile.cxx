
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

#define __CIFTI_MAPPABLE_CONNECTIVITY_MATRIX_DATA_FILE_DECLARE__
#include "CiftiMappableConnectivityMatrixDataFile.h"
#undef __CIFTI_MAPPABLE_CONNECTIVITY_MATRIX_DATA_FILE_DECLARE__

#include "CaretAssert.h"
#include "CiftiFile.h"
#include "CaretLogger.h"
#include "ConnectivityDataLoaded.h"
#include "EventManager.h"
#include "EventProgressUpdate.h"
#include "SceneClass.h"
#include "SceneClassAssistant.h"

using namespace caret;


    
/**
 * \class caret::CiftiMappableConnectivityMatrixDataFile 
 * \brief Data file for Cifti Connectivity Matrix Files.
 * \ingroup Files
 */

/**
 * Constructor.
 */
CiftiMappableConnectivityMatrixDataFile::CiftiMappableConnectivityMatrixDataFile(const DataFileTypeEnum::Enum dataFileType)
: CiftiMappableDataFile(dataFileType)
{
    m_connectivityDataLoaded = new ConnectivityDataLoaded();
    
    clearPrivate();

    m_sceneAssistant = new SceneClassAssistant();
    m_sceneAssistant->add("m_connectivityDataLoaded",
                          "ConnectivityDataLoaded",
                          m_connectivityDataLoaded);
    m_sceneAssistant->add("m_dataLoadingEnabled",
                           &m_dataLoadingEnabled);
}

/**
 * Destructor.
 */
CiftiMappableConnectivityMatrixDataFile::~CiftiMappableConnectivityMatrixDataFile()
{
    clearPrivate();
    
    delete m_connectivityDataLoaded;
    delete m_sceneAssistant;
}

/**
 * Clear the contents of the file.
 */
void
CiftiMappableConnectivityMatrixDataFile::clear()
{
    CiftiMappableDataFile::clear();
    clearPrivate();
}

/**
 * Clear the contents of the file.
 * Note that "clear()" is virtual and cannot be called from destructor.
 */
void
CiftiMappableConnectivityMatrixDataFile::clearPrivate()
{
    m_loadedRowData.clear();
    m_rowLoadedTextForMapName = "";
    m_rowLoadedText = "";
    m_dataLoadingEnabled = true;
    m_connectivityDataLoaded->reset();
}

/**
 * @return Pointer to the information about last loaded connectivity data.
 */
const ConnectivityDataLoaded*
CiftiMappableConnectivityMatrixDataFile::getConnectivityDataLoaded() const
{
    return m_connectivityDataLoaded;
}

/**
 * Get the nodes for the parcel for the given structure that corresponds
 * to the last selected row.
 *
 * @param parcelNodesOut
 *    Ouput containing the node indices.
 * @param structure
 *    The surface structure.
 */
bool
CiftiMappableConnectivityMatrixDataFile::getParcelNodesElementForSelectedParcel(std::set<int64_t> &parcelNodesOut,
                                                                                const StructureEnum::Enum &structure) const
{
    bool validFlag = false;
    
    int64_t rowIndex = -1;
    m_connectivityDataLoaded->getRowLoading(rowIndex);
    if (rowIndex >= 0) {
        validFlag = CiftiMappableDataFile::getParcelNodesElementForSelectedParcel(parcelNodesOut,
                                                           structure,
                                                           rowIndex);
    }
    
    return validFlag;
}

/**
 * @return Is this file empty?
 */
bool
CiftiMappableConnectivityMatrixDataFile::isEmpty() const
{
    if (CiftiMappableDataFile::isEmpty()) {
        return true;
    }
    
    return false;
}

/**
 * @return Is loading of data enabled.  Note that if
 * disabled, any previously loaded data is NOT removed
 * so that it can still be displayed but not updated.
 */
bool
CiftiMappableConnectivityMatrixDataFile::isMapDataLoadingEnabled(const int32_t /*mapIndex*/) const
{
    return m_dataLoadingEnabled;
}

/**
 * Set loading of data enabled.  Note that if
 * disabled, any previously loaded data is NOT removed
 * so that it can still be displayed but not updated.
 *
 * @param dataLoadingEnabled
 *   New data loading enabled status.
 */
void
CiftiMappableConnectivityMatrixDataFile::setMapDataLoadingEnabled(const int32_t /*mapIndex*/,
                                                          const bool dataLoadingEnabled)
{
    m_dataLoadingEnabled = dataLoadingEnabled;
}

/**
 * Get the data for the given map index.
 *
 * @param mapIndex
 *     Index of the map.
 * @param dataOut
 *     A vector that will contain the data for the map upon exit.
 */
void
CiftiMappableConnectivityMatrixDataFile::getMapData(const int32_t /*mapIndex*/,
                                  std::vector<float>& dataOut) const
{
    //int nCols = m_ciftiInterface->getNumberOfColumns();
    //dataOut.resize(nCols);
    //m_ciftiInterface->getColumn(&dataOut[0],mapIndex);
    dataOut = m_loadedRowData;
}

/**
 * Get the index of a row when loading data for a surface node.
 * @param structure
 *    Structure of the surface.
 * @param surfaceNumberOfNodes
 *    Number of nodes in the surface.
 * @param nodeIndex
 *    Index of the node.
 * @return
 *    Index of row corresponding to node or -1 if no row in the
 *    matrix corresponds to the node.
 */
int64_t
CiftiMappableConnectivityMatrixDataFile::getRowIndexForNodeWhenLoading(const StructureEnum::Enum structure,
                                                               const int64_t surfaceNumberOfNodes,
                                                               const int64_t nodeIndex) 
{
    if (m_ciftiFile == NULL) {
        return -1;
    }
    
    int64_t rowIndex = -1;
    
    const CiftiXML& ciftiXML = m_ciftiFile->getCiftiXML();
    
    switch (ciftiXML.getMappingType(CiftiXML::ALONG_COLUMN))
    {
        case CiftiMappingType::BRAIN_MODELS:
            if (ciftiXML.getBrainModelsMap(CiftiXML::ALONG_COLUMN).getSurfaceNumberOfNodes(structure) != surfaceNumberOfNodes) return -1;
            break;
        case CiftiMappingType::PARCELS:
            if (ciftiXML.getParcelsMap(CiftiXML::ALONG_COLUMN).getSurfaceNumberOfNodes(structure) != surfaceNumberOfNodes) return -1;
            break;
        default:
            return -1;
    }
    /*
     * Get the mapping type
     */
    const CiftiMappingType::MappingType rowMappingType = ciftiXML.getMappingType(CiftiXML::ALONG_COLUMN);
    
    switch (rowMappingType) {
        case CiftiMappingType::BRAIN_MODELS:
            rowIndex = ciftiXML.getBrainModelsMap(CiftiXML::ALONG_COLUMN).getIndexForNode(nodeIndex, structure);
            break;
        case CiftiMappingType::PARCELS:
            rowIndex = ciftiXML.getParcelsMap(CiftiXML::ALONG_COLUMN).getIndexForNode(nodeIndex, structure);
            break;
        case CIFTI_INDEX_TYPE_SCALARS:
            break;
        case CIFTI_INDEX_TYPE_TIME_POINTS:
            break;
        default:
            CaretAssert(0);
            CaretLogSevere("Invalid row mapping type for connectivity file "
                        + DataFileTypeEnum::toName(getDataFileType()));
            break;
    }
    return rowIndex;
}


/**
 * Get the index of a row when loading data for a voxel at a coordinate.
 * @param xyz
 *    Coordinate of the voxel.
 * @return
 *    Index of row corresponding to voxel or negative if no row in the
 *    matrix corresponds to the voxel.
 */
int64_t
CiftiMappableConnectivityMatrixDataFile::getRowIndexForVoxelAtCoordinateWhenLoading(const float xyz[3])
{
    if (m_ciftiFile == NULL) {
        return -1;
    }
    int64_t ijk[3];
    enclosingVoxel(xyz[0], xyz[1], xyz[2], ijk[0], ijk[1], ijk[2]);
    return getRowIndexForVoxelIndexWhenLoading(ijk);
}

/**
 * Get the index of a row when loading data for a voxel index.
 *
 * @param ijk
 *    Indices of the voxel.
 * @return
 *    Index of row corresponding to voxel or negative if no row in the
 *    matrix corresponds to the voxel.
 */
int64_t
CiftiMappableConnectivityMatrixDataFile::getRowIndexForVoxelIndexWhenLoading(const int64_t ijk[3])
{  
    if (m_ciftiFile == NULL) {
        return -1;
    }
    
    int64_t rowIndex = -1;
    
    const CiftiXML& ciftiXML = m_ciftiFile->getCiftiXML();
    const CiftiMappingType::MappingType rowMappingType = ciftiXML.getMappingType(CiftiXML::ALONG_COLUMN);
    
    /*
     * Get the mapping type
     */
    if (indexValid(ijk[0], ijk[1], ijk[2])) {
        switch (rowMappingType) {
            case CIFTI_INDEX_TYPE_BRAIN_MODELS:
                rowIndex = ciftiXML.getBrainModelsMap(CiftiXML::ALONG_COLUMN).getIndexForVoxel(ijk);
                break;
            case CIFTI_INDEX_TYPE_PARCELS:
                rowIndex = ciftiXML.getParcelsMap(CiftiXML::ALONG_COLUMN).getIndexForVoxel(ijk);
                break;
            default:
                CaretAssert(0);
                CaretLogSevere("Invalid row mapping type for connectivity file "
                            + DataFileTypeEnum::toName(getDataFileType()));
                break;
        }
    }
    
    return rowIndex;
}

/**
 * Set the loaded row data to zeros.
 */
void
CiftiMappableConnectivityMatrixDataFile::setLoadedRowDataToAllZeros()
{
    if (m_loadedRowData.empty() == false){
        std::fill(m_loadedRowData.begin(),
                  m_loadedRowData.end(),
                  0.0);
    }
    if (m_mapContent.empty() == false) {
        m_mapContent[0]->invalidateColoring();
    }
    m_connectivityDataLoaded->reset();
    m_rowLoadedText.clear();
    m_rowLoadedTextForMapName.clear();
}

/**
 * Load the given row from the file even if the file is disabled.
 *
 * NOTE: Afterwards, it will be necessary to update this file's color mapping
 * with updateScalarColoringForMap().
 *
 *
 * @param rowIndex
 *    Index of row that is loaded.
 * @throw DataFileException
 *    If an error occurs.
 */
void
CiftiMappableConnectivityMatrixDataFile::loadDataForRowIndex(const int64_t rowIndex) throw (DataFileException)
{
    setLoadedRowDataToAllZeros();
    
    const int64_t dataCount = m_ciftiFile->getNumberOfColumns();
    if (dataCount > 0) {
        m_rowLoadedTextForMapName = ("Row: "
                                        + AString::number(rowIndex));
        
        m_rowLoadedText = ("Row_"
                            + AString::number(rowIndex));
        CaretAssert((rowIndex >= 0) && (rowIndex < m_ciftiFile->getNumberOfRows()));
        m_loadedRowData.resize(dataCount);
        
        m_ciftiFile->getRow(&m_loadedRowData[0],
                                    rowIndex);
        
        CaretLogFine("Read row " + AString::number(rowIndex));
        m_connectivityDataLoaded->setRowLoading(rowIndex);
    }
    else {
        throw DataFileException("Row "
                                + AString::number(rowIndex)
                                + " is invalid or contains no data.");
    }
    
    
    CaretAssertVectorIndex(m_mapContent, 0);
    m_mapContent[0]->invalidateColoring();

}

/**
 * Load connectivity data for the surface's node.
 *
 * @param mapIndex
 *    Index of map.
 * @param surfaceNumberOfNodes
 *    Number of nodes in surface.
 * @param structure
 *    Surface's structure.
 * @param nodeIndex
 *    Index of node number.
 * @return
 *    Index of row that was loaded or -1 if no data was loaded.
 * @throw
 *    DataFileException if there is an error.
 */
int64_t
CiftiMappableConnectivityMatrixDataFile::loadMapDataForSurfaceNode(const int32_t /*mapIndex*/,
                                                           const int32_t surfaceNumberOfNodes,
                                                           const StructureEnum::Enum structure,
                                                           const int32_t nodeIndex) throw (DataFileException)
{
    if (m_ciftiFile == NULL) {
        setLoadedRowDataToAllZeros();
        return -1;
    }
  
    
    /*
     * Loading of data disabled?
     */
    if (m_dataLoadingEnabled == false) {
        return -1;
    }
    
    setLoadedRowDataToAllZeros();
    
    int64_t rowIndex = -1;
    
    try {
        bool dataWasLoaded = false;
        
        rowIndex = getRowIndexForNodeWhenLoading(structure,
                                                 surfaceNumberOfNodes,
                                                 nodeIndex);
        
        if (rowIndex >= 0) {
            const int64_t dataCount = m_ciftiFile->getNumberOfColumns();
            if (dataCount > 0) {
                m_rowLoadedTextForMapName = ("Row: "
                                         + AString::number(rowIndex)
                                         + ", Node Index: "
                                         + AString::number(nodeIndex)
                                         + ", Structure: "
                                         + StructureEnum::toName(structure));

                m_rowLoadedText = ("Row_"
                                   + AString::number(rowIndex)
                                   + "_Node_Index_"
                                   + AString::number(nodeIndex)
                                   + "_Structure_"
                                   + StructureEnum::toGuiName(structure));
                CaretAssert((rowIndex >= 0) && (rowIndex < m_ciftiFile->getNumberOfRows()));
                m_loadedRowData.resize(dataCount);
                m_ciftiFile->getRow(&m_loadedRowData[0],
                                         rowIndex);
                
                CaretLogFine("Read row for node " + AString::number(nodeIndex));
                
                m_connectivityDataLoaded->setSurfaceNodeLoading(structure,
                                                                surfaceNumberOfNodes,
                                                                nodeIndex,
                                                                rowIndex);
                dataWasLoaded = true;
            }
        }
        
        if (dataWasLoaded == false) {
            CaretLogFine("FAILED to read row for node " + AString::number(nodeIndex));
            m_connectivityDataLoaded->reset();
        }
    }
    catch (DataFileException& e) {
        m_connectivityDataLoaded->reset();
        throw e;
    }
    
    CaretAssertVectorIndex(m_mapContent, 0);
    m_mapContent[0]->invalidateColoring();
    
    return rowIndex;
}


/**
 * Load connectivity data for the given map index.
 *
 * @param mapIndex
 *    Index of map.
 * @param surfaceNumberOfNodes
 *    Number of nodes in surface.
 * @param structure
 *    Surface's structure.
 * @param nodeIndex
 *    Index of node number.
 * @return
 *    Index of row that was loaded or -1 if no data was loaded.
 * @throw
 *    DataFileException if there is an error.
 */
bool 
CiftiMappableConnectivityMatrixDataFile::loadMapData(const int32_t selectionIndex) throw (DataFileException)
{
    if (m_ciftiFile == NULL) {
        return false;
    }
    
    /*
     * Loading of data disabled?
     */
    if (m_dataLoadingEnabled == false) {
        return false;
    }
    
    
    
    bool dataWasLoaded = false;
    
    if (selectionIndex >= 0) {
        const int64_t dataCount = m_ciftiFile->getNumberOfColumns();
        if (dataCount > 0) {
            m_rowLoadedTextForMapName = ("Row: "
                                        + AString::number(selectionIndex+1)                                         
                                        );

            m_rowLoadedText = ("Row_"
                                + AString::number(selectionIndex+1)
                                );
            CaretAssert((selectionIndex >= 0) && (selectionIndex < m_ciftiFile->getNumberOfRows()));
            m_loadedRowData.resize(dataCount);
            m_ciftiFile->getRow(&m_loadedRowData[0],
                                        selectionIndex);
            
            CaretLogFine("Read row " + AString::number(selectionIndex+1));
            
            dataWasLoaded = true;
        }
    }
    
    if (dataWasLoaded == false) {
        CaretLogFine("FAILED to read row " + AString::number(selectionIndex+1));
    }
    
    CaretAssertVectorIndex(m_mapContent, 0);
    m_mapContent[0]->invalidateColoring();
    
    return true;
}

/**
 * Load connectivity data for the surface's nodes and then average the data.
 *
 * @param mapIndex
 *    Index of map.
 * @param surfaceFile
 *    Surface file used for structure.
 * @param surfaceNumberOfNodes
 *    Number of nodes in surface.
 * @param structure
 *    Surface's structure.
 * @param nodeIndices
 *    Indices of nodes.
 * @throw
 *    DataFileException if there is an error.
 */
void
CiftiMappableConnectivityMatrixDataFile::loadMapAverageDataForSurfaceNodes(const int32_t /*mapIndex*/,
                                                                   const int32_t surfaceNumberOfNodes,
                                                                   const StructureEnum::Enum structure,
                                                                   const std::vector<int32_t>& nodeIndices) throw (DataFileException)
{
    if (m_ciftiFile == NULL) {
        setLoadedRowDataToAllZeros();
        return;
    }
    
    /*
     * Loading of data disabled?
     */
    if (m_dataLoadingEnabled == false) {
        return;
    }
    
    setLoadedRowDataToAllZeros();
    
    const int32_t numberOfNodeIndices = static_cast<int32_t>(nodeIndices.size());
    if (numberOfNodeIndices <= 0) {
        return;
    }
    
    const bool isDenseMatrix = (getDataFileType() == DataFileTypeEnum::CONNECTIVITY_DENSE);
    const int32_t progressUpdateInterval = 1;
    
    bool dataWasLoaded = false;
    
    const int64_t dataCount = m_ciftiFile->getNumberOfColumns();
    if (dataCount > 0) {
        /*
            * Contains the average row
            */
        std::vector<float> dataAverageVector(dataCount, 0.0);
        float* dataAverage = &dataAverageVector[0];
        
        /*
            * Contains row for a node
            */
        std::vector<float> dataRowVector(dataCount, 0.0);
        float* dataRow = &dataRowVector[0];
        
        int64_t rowSuccessCount = 0;
        
        bool userCancelled = false;
        EventProgressUpdate progressEvent(0,
                                            numberOfNodeIndices,
                                            0,
                                            ("Loading data for "
                                            + QString::number(numberOfNodeIndices)
                                            + " vertices in file ")
                                            + getFileNameNoPath());
        EventManager::get()->sendEvent(progressEvent.getPointer());
        
        /*
            * Read rows for each node
            */
        for (int32_t i = 0; i < numberOfNodeIndices; i++) {
            const int32_t nodeIndex = nodeIndices[i];
            
            if (isDenseMatrix) {
                if ((i % progressUpdateInterval) == 0) {
                    progressEvent.setProgress(i,
                                                "");
                    EventManager::get()->sendEvent(progressEvent.getPointer());
                    if (progressEvent.isCancelled()) {
                        userCancelled = true;
                        break;
                    }
                }
            }
            
            const int64_t rowIndex = getRowIndexForNodeWhenLoading(structure,
                                                                    surfaceNumberOfNodes,
                                                                    nodeIndex);
            
            if (rowIndex >= 0) {
                CaretAssert((rowIndex >= 0) && (rowIndex < m_ciftiFile->getNumberOfRows()));
                m_ciftiFile->getRow(dataRow, rowIndex);
                
                for (int64_t j = 0; j < dataCount; j++) {
                    dataAverage[j] += dataRow[j];
                }
                rowSuccessCount++;
                
                CaretLogFine("Read row for node " + AString::fromNumbers(nodeIndices, ","));
            }
            else {
                CaretLogFine("Failed reading row for node " + AString::number(nodeIndex));
            }
        }
        
        if (userCancelled) {
            m_loadedRowData.clear();
            m_loadedRowData.resize(dataCount, 0.0);
        }
        else if (rowSuccessCount > 0) {
            /*
                * Average the data
                */
            for (int64_t i = 0; i < dataCount; i++) {
                dataAverage[i] /= rowSuccessCount;
            }
            
            m_rowLoadedTextForMapName = ("Structure: "
                                        + StructureEnum::toName(structure)
                                        + ", Averaged Node Count: "
                                        + AString::number(numberOfNodeIndices));
            m_rowLoadedText =  ("Structure_"
                                + StructureEnum::toGuiName(structure)
                                + "_Averaged_Node_Count_"
                                + AString::number(numberOfNodeIndices));
            /*
                * Update the viewed data
                */
            m_loadedRowData = dataAverageVector;
            
            dataWasLoaded = true;
        }
    }
    
    if (dataWasLoaded == false) {
        CaretLogFine("FAILED to read rows for node average" + AString::fromNumbers(nodeIndices, ","));
    }

    CaretAssertVectorIndex(m_mapContent, 0);
    m_mapContent[0]->invalidateColoring();

    if (dataWasLoaded) {
        m_connectivityDataLoaded->setSurfaceAverageNodeLoading(structure,
                                                                surfaceNumberOfNodes,
                                                                nodeIndices);
    }
}

/**
 * Load data for a voxel at the given coordinate.
 *
 * @param mapIndex
 *    Index of map.
 * @param xyz
 *    Coordinate of voxel.
 * @return 
 *    Index of row that was loaded or -1 if no data was loaded.
 * @throw
 *    DataFileException if there is an error.
 */
int64_t
CiftiMappableConnectivityMatrixDataFile::loadMapDataForVoxelAtCoordinate(const int32_t mapIndex,
                                                                 const float xyz[3]) throw (DataFileException)
{
    CaretAssert(mapIndex == 0);
    
    if (m_ciftiFile == NULL) {
        setLoadedRowDataToAllZeros();
        return -1;
    }
    
    /*
     * Loading of data disabled?
     */
    if (m_dataLoadingEnabled == false) {
        return -1;
    }
    
    setLoadedRowDataToAllZeros();
    
    /*
     * Get content for map.
     */
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    
    /*
     * Loading of data disabled?
     */
    if (m_dataLoadingEnabled == false) {
        return -1;
    }
    
    
    int64_t rowIndex = -1;
    
    bool dataWasLoaded = false;
    
    rowIndex = getRowIndexForVoxelAtCoordinateWhenLoading(xyz);
    
    if (rowIndex >= 0) {
        const int64_t dataCount = m_ciftiFile->getNumberOfColumns();
        if (dataCount > 0) {
            m_loadedRowData.resize(dataCount);
            CaretAssert((rowIndex >= 0) && (rowIndex < m_ciftiFile->getNumberOfRows()));
            m_ciftiFile->getRow(&m_loadedRowData[0], rowIndex);
            
            m_rowLoadedTextForMapName = ("Row: "
                                        + AString::number(rowIndex)
                                        + ", Voxel XYZ: ("
                                        + AString::fromNumbers(xyz, 3, ",")
                                        + ")");
            
            m_rowLoadedText = ("Row_"
                                + AString::number(rowIndex)
                                + "_Voxel_XYZ_"
                                + AString::fromNumbers(xyz, 3, "_").replace('-', 'm'));
            
            CaretLogFine("Read row for voxel " + AString::fromNumbers(xyz, 3, ","));
            
            dataWasLoaded = true;
        }
    }
    
    if (dataWasLoaded == false) {
        CaretLogFine("FAILED to read row for voxel " + AString::fromNumbers(xyz, 3, ","));
    }

    CaretAssertVectorIndex(m_mapContent, mapIndex);
    m_mapContent[mapIndex]->invalidateColoring();
    
    m_connectivityDataLoaded->setVolumeXYZLoading(xyz,
                                                    rowIndex);
    
    return rowIndex;
}

/**
 * Load connectivity data for the voxel indices and then average the data.
 *
 * @param mapIndex
 *    Index of map.
 * @param volumeDimensionIJK
 *    Dimensions of the volume.
 * @param voxelIndices
 *    Indices of voxels.
 * @throw
 *    DataFileException if there is an error.
 */
bool
CiftiMappableConnectivityMatrixDataFile::loadMapAverageDataForVoxelIndices(const int32_t mapIndex,
                                                                           const int64_t volumeDimensionIJK[3],
                                                                           const std::vector<VoxelIJK>& voxelIndices) throw (DataFileException)
{
    if (mapIndex != 0) { // eliminates compilation warning when compiled for release
        CaretAssert(mapIndex == 0);
    }
    
    if (m_ciftiFile == NULL) {
        setLoadedRowDataToAllZeros();
        return false;
    }
    
    /*
     * Loading of data disabled?
     */
    if (m_dataLoadingEnabled == false) {
        return false;
    }
    
    setLoadedRowDataToAllZeros();
    
    /*
     * Match dimensions
     */
    std::vector<int64_t> volumeDimensions;
    getDimensions(volumeDimensions);
    if (volumeDimensions.size() < 3) {
        return false;
    }
    if ((volumeDimensions[0] != volumeDimensionIJK[0])
        || (volumeDimensions[1] != volumeDimensionIJK[1])
        || (volumeDimensions[2] != volumeDimensionIJK[2])) {
        return false;
    }
    
    /*
     * Get content for map.
     */
    CaretAssertVectorIndex(m_mapContent,
                           mapIndex);
    
    const int64_t dataCount = m_ciftiFile->getNumberOfColumns();
    if (dataCount <= 0) {
        return false;
    }
    
    const int64_t numberOfVoxelIndices = static_cast<int64_t>(voxelIndices.size());
    
    bool userCancelled = false;
    const int32_t progressUpdateInterval = 1;
    EventProgressUpdate progressEvent(0,
                                      numberOfVoxelIndices,
                                      0,
                                      ("Loading data for "
                                       + QString::number(numberOfVoxelIndices)
                                       + " voxels in file ")
                                      + getFileNameNoPath());
    EventManager::get()->sendEvent(progressEvent.getPointer());
    
    std::vector<float> rowData(dataCount);
    std::vector<double> rowSum(dataCount, 0.0);
    
    /*
     * Load and sum the data for all rows
     */
    int64_t numberOfRowsLoaded = 0;
    for (int64_t i = 0; i < numberOfVoxelIndices; i++) {
        if ((i % progressUpdateInterval) == 0) {
            progressEvent.setProgress(i,
                                      "");
            EventManager::get()->sendEvent(progressEvent.getPointer());
            if (progressEvent.isCancelled()) {
                userCancelled = true;
                break;
            }
        }
        
       const VoxelIJK& voxelIJK = voxelIndices[i];
        
        const int64_t rowIndex = getRowIndexForVoxelIndexWhenLoading(voxelIJK.m_ijk);
        if (rowIndex >= 0) {
            CaretAssert((rowIndex >= 0) && (rowIndex < m_ciftiFile->getNumberOfRows()));
            m_ciftiFile->getRow(&rowData[0],
                                     rowIndex);
            
            for (int64_t j = 0; j < dataCount; j++) {
                rowSum[j] += rowData[j];
            }
            
            numberOfRowsLoaded++;
        }
    }
    
    if (userCancelled) {
        m_loadedRowData.clear();
        m_loadedRowData.resize(dataCount, 0.0);
    }
    else if (numberOfRowsLoaded > 0) {
        progressEvent.setProgress(numberOfVoxelIndices - 1,
                                  "Averaging voxel data");
        EventManager::get()->sendEvent(progressEvent.getPointer());
        
        m_loadedRowData.resize(dataCount, 0.0);
        for (int64_t j = 0; j < dataCount; j++) {
            m_loadedRowData[j] = rowSum[j] / numberOfRowsLoaded;
        }
        
        m_rowLoadedTextForMapName = ("Averaged Voxel Count: "
                                     + AString::number(numberOfVoxelIndices));
        
        m_connectivityDataLoaded->setVolumeAverageVoxelLoading(volumeDimensionIJK,
                                                               voxelIndices);
        
        return true;
    }
    
    return false;
}

/**
 * @return Text describing row loaded that uses
 * underscores as separators.
 */
AString
CiftiMappableConnectivityMatrixDataFile::getRowLoadedText() const
{
    return m_rowLoadedText;
}


/**
 * Get the name of the map at the given index.  For connectivity matrix
 * files this always returns a description of the last data row that 
 * was loaded.
 *
 * @param mapIndex
 *    Index of the map.
 * @return
 *    Name of the map.
 */
AString
CiftiMappableConnectivityMatrixDataFile::getMapName(const int32_t /*mapIndex*/) const
{
    return m_rowLoadedTextForMapName;
}

AString
CiftiMappableConnectivityMatrixDataFile::getRowName(const int32_t rowIndex) const
{
    const CiftiXML& xml = m_ciftiFile->getCiftiXML();
    if (xml.getMappingType(CiftiXML::ALONG_COLUMN) != CiftiMappingType::PARCELS) return "";//TSC: this was originally implemented only for parcels, dunno why
    const std::vector<CiftiParcelsMap::Parcel>& plist = xml.getParcelsMap(CiftiXML::ALONG_COLUMN).getParcels();
    CaretAssertVectorIndex(plist, rowIndex);
    return plist[rowIndex].m_name;
}

AString
CiftiMappableConnectivityMatrixDataFile::getColumnName(const int32_t columnIndex) const
{
    const CiftiXML& xml = m_ciftiFile->getCiftiXML();
    if (xml.getMappingType(CiftiXML::ALONG_ROW) != CiftiMappingType::PARCELS) return "";//ditto
    const std::vector<CiftiParcelsMap::Parcel>& plist = xml.getParcelsMap(CiftiXML::ALONG_ROW).getParcels();
    CaretAssertVectorIndex(plist, columnIndex);
    return plist[columnIndex].m_name;
}

/**
 * Save file data from the scene.  For subclasses that need to
 * save to a scene, this method should be overriden.  sceneClass
 * will be valid and any scene data should be added to it.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass to which data members should be added.
 */
void
CiftiMappableConnectivityMatrixDataFile::saveFileDataToScene(const SceneAttributes* sceneAttributes,
                                                             SceneClass* sceneClass)
{
    CiftiMappableDataFile::saveFileDataToScene(sceneAttributes,
                                               sceneClass);
    
    m_sceneAssistant->saveMembers(sceneAttributes, sceneClass);
    
}

/**
 * Restore file data from the scene.  For subclasses that need to
 * restore from a scene, this method should be overridden. The scene class
 * will be valid and any scene data may be obtained from it.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass for the instance of a class that implements
 *     this interface.  Will NEVER be NULL.
 */
void
CiftiMappableConnectivityMatrixDataFile::restoreFileDataFromScene(const SceneAttributes* sceneAttributes,
                                                                  const SceneClass* sceneClass)
{
    CiftiMappableDataFile::restoreFileDataFromScene(sceneAttributes,
                                                    sceneClass);
    
    m_connectivityDataLoaded->reset();
    m_sceneAssistant->restoreMembers(sceneAttributes,
                                     sceneClass);
    
    
    /*
     * Loading of data may be disabled in the scene
     * so temporarily enabled loading and then
     * restore the status.
     */
    const int32_t mapIndex = 0;
    const bool loadingEnabledStatus = isMapDataLoadingEnabled(mapIndex);
    
    setMapDataLoadingEnabled(mapIndex, true);
    
    switch (m_connectivityDataLoaded->getMode()) {
        case ConnectivityDataLoaded::MODE_NONE:
            setLoadedRowDataToAllZeros();
            break;
        case ConnectivityDataLoaded::MODE_ROW:
        {
            int64_t rowIndex;
            m_connectivityDataLoaded->getRowLoading(rowIndex);
            loadDataForRowIndex(rowIndex);
        }
            break;
        case ConnectivityDataLoaded::MODE_SURFACE_NODE:
        {
            StructureEnum::Enum structure;
            int32_t surfaceNumberOfNodes;
            int32_t surfaceNodeIndex;
            int64_t rowIndex;
            m_connectivityDataLoaded->getSurfaceNodeLoading(structure,
                                                            surfaceNumberOfNodes,
                                                            surfaceNodeIndex,
                                                            rowIndex);
            loadMapDataForSurfaceNode(mapIndex,
                                      surfaceNumberOfNodes,
                                      structure,
                                      surfaceNodeIndex);
        }
            break;
        case ConnectivityDataLoaded::MODE_SURFACE_NODE_AVERAGE:
        {
            StructureEnum::Enum structure;
            int32_t surfaceNumberOfNodes;
            std::vector<int32_t> surfaceNodeIndices;
            m_connectivityDataLoaded->getSurfaceAverageNodeLoading(structure,
                                                                   surfaceNumberOfNodes,
                                                                   surfaceNodeIndices);
            loadMapAverageDataForSurfaceNodes(mapIndex,
                                              surfaceNumberOfNodes,
                                              structure,
                                              surfaceNodeIndices);
        }
            break;
        case ConnectivityDataLoaded::MODE_VOXEL_XYZ:
        {
            float volumeXYZ[3];
            int64_t rowIndex;
            m_connectivityDataLoaded->getVolumeXYZLoading(volumeXYZ,
                                                          rowIndex);
            loadMapDataForVoxelAtCoordinate(mapIndex, volumeXYZ);
        }
            break;
        case ConnectivityDataLoaded::MODE_VOXEL_IJK_AVERAGE:
        {
            int64_t volumeDimensionsIJK[3];
            std::vector<VoxelIJK> voxelIndicesIJK;
            m_connectivityDataLoaded->getVolumeAverageVoxelLoading(volumeDimensionsIJK,
                                                                   voxelIndicesIJK);
            loadMapAverageDataForVoxelIndices(mapIndex,
                                              volumeDimensionsIJK,
                                              voxelIndicesIJK);
        }
            break;
    }
    
    setMapDataLoadingEnabled(mapIndex,
                             loadingEnabledStatus);
}


