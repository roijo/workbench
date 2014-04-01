
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

#include <algorithm>
#include <iostream>
#include <iterator>
#include <stack>
#include <vector>

#include "AlgorithmNodesInsideBorder.h"

#include "AlgorithmException.h"
#include "Border.h"
#include "BorderFile.h"
#include "CaretAssert.h"
#include "CaretOMP.h"
#include "CaretLogger.h"
#include "CiftiBrainordinateLabelFile.h"
#include "CiftiBrainordinateScalarFile.h"
#include "CiftiFile.h"
#include "GeodesicHelper.h"
#include "LabelFile.h"
#include "MathFunctions.h"
#include "MetricFile.h"
#include "SurfaceFile.h"
#include "SurfaceProjectedItem.h"
#include "TopologyHelper.h"

using namespace caret;

/*
 * Turning on this debug flag will produce a "debug borders"
 * showing the unconnected and connected paths.  If the GUI
 * is used, these "debug borders" will be added to the
 * "brain" and are viewable.
 */
static const bool DEBUG_FLAG = true;

AString AlgorithmNodesInsideBorder::getCommandSwitch()
{
    return "-border-to-rois";//maybe the command should be in a separate file, and the nodes inside border code should be a helper class?
}

AString AlgorithmNodesInsideBorder::getShortDescription()
{
    return "MAKE METRIC ROIS FROM BORDERS";
}

OperationParameters* AlgorithmNodesInsideBorder::getParameters()
{
    OperationParameters* ret = new OperationParameters();
    
    ret->addSurfaceParameter(1, "surface", "the surface the borders are drawn on");
    
    ret->addBorderParameter(2, "border-file", "the border file");
    
    ret->addMetricOutputParameter(3, "metric-out", "the output metric file");
    
    OptionalParameter* borderOpt = ret->createOptionalParameter(4, "-border", "create ROI for only one border");
    borderOpt->addStringParameter(1, "name", "the name of the border");
    
    ret->createOptionalParameter(5, "-inverse", "use inverse selection (outside border)");
    
    ret->setHelpText(
        AString("By default, draws ROIs inside all borders in the border file, as separate metric columns.")
    );
    return ret;
}

void AlgorithmNodesInsideBorder::useParameters(OperationParameters* myParams, ProgressObject* myProgObj)
{
    SurfaceFile* mySurf = myParams->getSurface(1);
    BorderFile* myBorderFile = myParams->getBorder(2);
    MetricFile* myMetricOut = myParams->getOutputMetric(3);
    OptionalParameter* borderOpt = myParams->getOptionalParameter(4);
    bool inverse = myParams->getOptionalParameter(5)->m_present;
    if (mySurf->getStructure() == StructureEnum::INVALID) throw AlgorithmException("surface file needs a valid structure to find the right borders in the file");
    //TODO: check that border file is valid on this surface file
    if (borderOpt->m_present)
    {
        AString findName = borderOpt->getString(1);
        int numBorders = myBorderFile->getNumberOfBorders();
        int borderIndx = -1;
        for (int i = 0; i < numBorders; ++i)
        {
            if (myBorderFile->getBorder(i)->getName() == findName && myBorderFile->getBorder(i)->getStructure() == mySurf->getStructure())
            {
                borderIndx = i;
                break;
            }
        }
        if (borderIndx == -1) throw AlgorithmException("border name not found for surface structure");
        myMetricOut->setNumberOfNodesAndColumns(mySurf->getNumberOfNodes(), 1);
        myMetricOut->setStructure(mySurf->getStructure());
        myMetricOut->setColumnName(0, findName);
        AlgorithmNodesInsideBorder(myProgObj, mySurf, myBorderFile->getBorder(borderIndx), inverse, 0, 1.0f, myMetricOut);
    } else {
        int numBorders = myBorderFile->getNumberOfBorders();
        vector<int> borderSelect;
        for (int i = 0; i < numBorders; ++i)
        {
            const Border* thisBorder = myBorderFile->getBorder(i);
            if (thisBorder->getStructure() != mySurf->getStructure()) continue;
            borderSelect.push_back(i);
        }
        int numSelect = (int)borderSelect.size();
        if (numSelect < 1) throw AlgorithmException("no borders match the structure of the surface");
        myMetricOut->setNumberOfNodesAndColumns(mySurf->getNumberOfNodes(), numSelect);
        myMetricOut->setStructure(mySurf->getStructure());
#pragma omp CARET_PARFOR schedule(dynamic)
        for (int i = 0; i < numSelect; ++i)
        {
            myMetricOut->setColumnName(i, myBorderFile->getBorder(borderSelect[i])->getName());
            AlgorithmNodesInsideBorder(myProgObj, mySurf, myBorderFile->getBorder(borderSelect[i]), inverse, i, 1.0f, myMetricOut);
        }
    }
}

/**
 * \class caret::AlgorithmNodesInsideBorder 
 * \brief Assign attribute values to nodes within a closed border.
 *
 * Identify nodes within a closed border and assign metric or
 * label values to those nodes.
 */

/**
 * Constructor.
 * 
 * @param myProgObj
 *    
 * @param surfaceFile
 *    Surface file for nodes inside border.
 * @param border
 *    Border for which nodes inside are found.
 * @param assignToMetricMapIndex
 *    Map index in metric file to which assigments are made for
 *    nodes inside the border.
 * @param assignMetricValue
 *    Metric value assigned to nodes within the border.
 * @param metricFileInOut
 *    Metric file that has map set with nodes inside border.
 */
AlgorithmNodesInsideBorder::AlgorithmNodesInsideBorder(ProgressObject* myProgObj, 
                                                       const SurfaceFile* surfaceFile,
                                                       const Border* border, 
                                                       const bool isInverseSelection,
                                                       const int32_t assignToMetricMapIndex,
                                                       const float assignMetricValue,
                                                       MetricFile* metricFileInOut)
: AbstractAlgorithm(myProgObj),
m_surfaceFile(surfaceFile)
{
    CaretAssert(surfaceFile);
    CaretAssert(border);
    CaretAssert(metricFileInOut);
    m_debugBorderFile = NULL;
    
    if (surfaceFile->getNumberOfNodes() != metricFileInOut->getNumberOfNodes()) throw AlgorithmException("metric file must be initialized to same number of nodes");//a method that requires this really shouldn't be public
    
    this->isInverseSelection = isInverseSelection;
    
    std::vector<int32_t> nodesInsideBorder;
    this->findNodesInsideBorder(surfaceFile,
                                border,
                                nodesInsideBorder);
    
    const int32_t numberOfNodesInsideBorder = static_cast<int32_t>(nodesInsideBorder.size());
    for (int32_t i = 0; i < numberOfNodesInsideBorder; i++) {
        const int32_t nodeNumber = nodesInsideBorder[i];
        metricFileInOut->setValue(nodeNumber,
                                  assignToMetricMapIndex,
                                  assignMetricValue);
    }
}

AlgorithmNodesInsideBorder::AlgorithmNodesInsideBorder(ProgressObject* myProgObj, 
                                                       const SurfaceFile* surfaceFile,
                                                       const Border* border, 
                                                       const bool isInverseSelection,
                                                       const int32_t assignToLabelMapIndex,
                                                       const int32_t assignLabelKey,
                                                       LabelFile* labelFileInOut)
: AbstractAlgorithm(myProgObj),
m_surfaceFile(surfaceFile)
{
    CaretAssert(surfaceFile);
    CaretAssert(border);
    CaretAssert(labelFileInOut);
    m_debugBorderFile = NULL;
    
    if (surfaceFile->getNumberOfNodes() != labelFileInOut->getNumberOfNodes()) throw AlgorithmException("metric file must be initialized to same number of nodes");//a method that requires this really shouldn't be public
    
    this->isInverseSelection = isInverseSelection;
    
    std::vector<int32_t> nodesInsideBorder;
    this->findNodesInsideBorder(surfaceFile,
                                border,
                                nodesInsideBorder);
    
    const int32_t numberOfNodesInsideBorder = static_cast<int32_t>(nodesInsideBorder.size());
    for (int32_t i = 0; i < numberOfNodesInsideBorder; i++) {
        const int32_t nodeNumber = nodesInsideBorder[i];
        labelFileInOut->setLabelKey(nodeNumber,
                                    assignToLabelMapIndex,
                                    assignLabelKey);
    }
}

/**
 * Constructor.
 *
 * @param myProgObj
 *
 * @param surfaceFile
 *    Surface file for nodes inside border.
 * @param border
 *    Border for which nodes inside are found.
 * @param isInverseSelection
 *    Invert the selection.
 * @param assignToCiftiScalarMapIndex
 *    Map index in cifti scalar file to which assigments are made for
 *    nodes inside the border.
 * @param assignScalarValue
 *    Scalar value assigned to nodes within the border.
 * @param ciftiScalarFileInOut
 *    CIFTI scalar file that has map set with nodes inside border.
 */
AlgorithmNodesInsideBorder::AlgorithmNodesInsideBorder(ProgressObject* myProgObj,
                           const SurfaceFile* surfaceFile,
                           const Border* border,
                           const bool isInverseSelection,
                           const int32_t assignToCiftiScalarMapIndex,
                           const float assignScalarValue,
                           CiftiBrainordinateScalarFile* ciftiScalarFileInOut)
: AbstractAlgorithm(myProgObj),
m_surfaceFile(surfaceFile)
{
    CaretAssert(surfaceFile);
    CaretAssert(border);
    CaretAssert(ciftiScalarFileInOut);
    m_debugBorderFile = NULL;
    
    this->isInverseSelection = isInverseSelection;
    
    std::vector<int32_t> nodesInsideBorder;
    this->findNodesInsideBorder(surfaceFile,
                                border,
                                nodesInsideBorder);
    
    std::vector<float> surfaceMapData(surfaceFile->getNumberOfNodes(),
                                      0.0);
    
    const int32_t numberOfNodesInsideBorder = static_cast<int32_t>(nodesInsideBorder.size());
    for (int32_t i = 0; i < numberOfNodesInsideBorder; i++) {
        const int32_t nodeIndex = nodesInsideBorder[i];
        CaretAssertVectorIndex(surfaceMapData, nodeIndex);
        surfaceMapData[nodeIndex] = assignScalarValue;
    }
    ciftiScalarFileInOut->setMapDataForSurface(assignToCiftiScalarMapIndex,
                                               surfaceFile->getStructure(),
                                               surfaceMapData);
}

/**
 * Constructor.
 *
 * @param myProgObj
 *
 * @param surfaceFile
 *    Surface file for nodes inside border.
 * @param border
 *    Border for which nodes inside are found.
 * @param isInverseSelection
 *    Invert the selection.
 * @param assignToCiftiScalarMapIndex
 *    Map index in cifti scalar file to which assigments are made for
 *    nodes inside the border.
 * @param assignScalarValue
 *    Scalar value assigned to nodes within the border.
 * @param ciftiScalarFileInOut
 *    CIFTI scalar file that has map set with nodes inside border.
 */
AlgorithmNodesInsideBorder::AlgorithmNodesInsideBorder(ProgressObject* myProgObj,
                                                       const SurfaceFile* surfaceFile,
                                                       const Border* border,
                                                       const bool isInverseSelection,
                                                       const int32_t assignToCiftiLabelMapIndex,
                                                       const int32_t assignLabelKey,
                                                       CiftiBrainordinateLabelFile* ciftiLabelFileInOut)
: AbstractAlgorithm(myProgObj),
m_surfaceFile(surfaceFile)
{
    CaretAssert(surfaceFile);
    CaretAssert(border);
    CaretAssert(ciftiLabelFileInOut);
    m_debugBorderFile = NULL;
    
    this->isInverseSelection = isInverseSelection;
    
    std::vector<int32_t> nodesInsideBorder;
    this->findNodesInsideBorder(surfaceFile,
                                border,
                                nodesInsideBorder);
    
    std::vector<float> surfaceMapData(surfaceFile->getNumberOfNodes(),
                                      0.0);
    
    const int32_t numberOfNodesInsideBorder = static_cast<int32_t>(nodesInsideBorder.size());
    for (int32_t i = 0; i < numberOfNodesInsideBorder; i++) {
        const int32_t nodeIndex = nodesInsideBorder[i];
        CaretAssertVectorIndex(surfaceMapData, nodeIndex);
        surfaceMapData[nodeIndex] = assignLabelKey;
    }
    ciftiLabelFileInOut->setMapDataForSurface(assignToCiftiLabelMapIndex,
                                               surfaceFile->getStructure(),
                                               surfaceMapData);
    
}

/**
 * Destructor.
 */
AlgorithmNodesInsideBorder::~AlgorithmNodesInsideBorder()
{
    if (m_debugBorderFile != NULL) {
        delete m_debugBorderFile;
    }
}


/**
 * @return Border file containing borders used for debugging algorithm.
 *         WILL BE NULL if no debug borders were created.
 */
const BorderFile*
AlgorithmNodesInsideBorder::getDebugBorderFile()
{
    return m_debugBorderFile;
}

/**
 * Add a border to the debug border file.
 *
 * @param border
 *    Border to add to debug border file.
 */
void
AlgorithmNodesInsideBorder::addDebugBorder(Border* border)
{
    if (m_debugBorderFile == NULL) {
        m_debugBorderFile = new BorderFile();
        m_debugBorderFile->setFileName("DEBUG_"
                                       + m_borderName
                                       + "."
                                       + DataFileTypeEnum::toFileExtension(DataFileTypeEnum::BORDER));
    }
    
    m_debugBorderFile->addBorder(border);
}

/**
 * Find nodes inside the border.
 *
 * @param surfaceFile
 *    Surface file for nodes inside border.
 * @param border
 *    Border for which nodes inside are found.
 * @param nodesInsideBorderOut
 *    Vector into which nodes inside border are loaded.
 */
void 
AlgorithmNodesInsideBorder::findNodesInsideBorder(const SurfaceFile* surfaceFile,
                                                  const Border* border,
                                                  std::vector<int32_t>& nodesInsideBorderOut)
{
    CaretAssert(surfaceFile);
    CaretAssert(border);
    
    m_borderName = border->getName();
    nodesInsideBorderOut.clear();
    
    /*
     * Move border points to the nearest nodes.
     */
    std::vector<int32_t> unconnectedNodesPath;
    moveBorderPointsToNearestNodes(border,
                                   unconnectedNodesPath);

    if (DEBUG_FLAG) {
        Border* b = Border::newInstanceFromSurfaceNodes(("DEBUG_UNCONNECTED_PATH_"
                                                         + m_borderName),
                                                        surfaceFile,
                                                        unconnectedNodesPath);
        addDebugBorder(b);
    }
    
    /*
     * Convert the unconnected nodes path into a connected nodes path
     */
    std::vector<int32_t> connectedNodesPath;
    createConnectedNodesPath(surfaceFile,
                             unconnectedNodesPath,
                             connectedNodesPath);
    
    int32_t numberOfNodesInConnectedPath = static_cast<int32_t>(connectedNodesPath.size());
    if (numberOfNodesInConnectedPath < 4) {
        throw AlgorithmException("Connected path is too small "
                                 "as it consists of fewer than four vertices.");
    }
    
    if (DEBUG_FLAG) {
        Border* b = Border::newInstanceFromSurfaceNodes(("DEBUG_CONNECTED_PATH_"
                                                         + m_borderName),
                                                        surfaceFile,
                                                        connectedNodesPath);
        addDebugBorder(b);
    }
    
    /*
     * Find nodes that are OUTSIDE the connected path
     */
    std::vector<int32_t> nodesOutsidePath;
    findNodesOutsideOfConnectedPath(connectedNodesPath,
                                    nodesOutsidePath);

    /*
     * Identify nodes INSIDE the connected path
     */
    const int32_t numberOfSurfaceNodes = m_surfaceFile->getNumberOfNodes();
    std::vector<bool> nodeInsideRegionFlags(numberOfSurfaceNodes,
                                            true);
    
    const int32_t numberOfNodesOutsidePath = static_cast<int32_t>(nodesOutsidePath.size());
    for (int32_t i = 0; i < numberOfNodesOutsidePath; i++) {
        const int32_t nodeIndex = nodesOutsidePath[i];
        CaretAssertVectorIndex(nodeInsideRegionFlags, nodeIndex);
        nodeInsideRegionFlags[nodeIndex] = false;
    }
    
    /*
     * Handle inverse selection or possibility outside selection was
     * accidentally inside selection
     */
    bool doInverseFlag = isInverseSelection;
    
    const int32_t halfNumberOfSurfaceNodes = numberOfSurfaceNodes / 2;
    if (numberOfNodesOutsidePath < halfNumberOfSurfaceNodes) {
        doInverseFlag = ( ! doInverseFlag);
    }
    
    const CaretPointer<TopologyHelper> th = m_surfaceFile->getTopologyHelper();
    /*
     * Identify nodes
     */
    for (int32_t i = 0; i < numberOfSurfaceNodes; i++) {
        bool insideFlag = nodeInsideRegionFlags[i];
        if (doInverseFlag) {
            insideFlag = ( ! insideFlag);
        }

        if (th->getNodeHasNeighbors(i)) {
            if (std::find(connectedNodesPath.begin(),
                          connectedNodesPath.end(),
                          i) != connectedNodesPath.end()) {
                insideFlag = false;
            }
        }
        else {
            insideFlag = false;
        }

        if (insideFlag) {
            nodesInsideBorderOut.push_back(i);
        }
    }
}

/**
 * Move the points in the given border to the nearest nodes.
 * The output path is "cleaned" so that it does not contain 
 * any consecutive points.
 *
 * @param border
 *    The input border.
 * @param nodeIndicesFollowingBorder
 *    Output containing nodes nearest border points.
 */
void
AlgorithmNodesInsideBorder::moveBorderPointsToNearestNodes(const Border* border,
                                                           std::vector<int32_t>& nodeIndicesFollowingBorder)
{
    nodeIndicesFollowingBorder.clear();
    
    /*
     * Move border points to the nearest nodes.
     */
    const int32_t originalNumberOfBorderPoints = border->getNumberOfPoints();
    for (int32_t i = 0; i < originalNumberOfBorderPoints; i++) {
        const SurfaceProjectedItem* spi = border->getPoint(i);
        float xyz[3];
        if (spi->getProjectedPosition(*m_surfaceFile, xyz, true)) {
            const int32_t nearestNode = m_surfaceFile->closestNode(xyz);
            if (nearestNode >= 0) {
                nodeIndicesFollowingBorder.push_back(nearestNode);
            }
        }
    }
    cleanNodePath(nodeIndicesFollowingBorder);
    
    /*
     * Make sure first and last nodes are not the same
     */
    int32_t numberOfPointsInBorder = static_cast<int32_t>(nodeIndicesFollowingBorder.size());
    if (nodeIndicesFollowingBorder.size() < 4) {
        throw AlgorithmException("Border is too small.  "
                                 "When moved to nearest vertices, border consists of four or fewer vertices.");
    }
    if (nodeIndicesFollowingBorder[0] == nodeIndicesFollowingBorder[numberOfPointsInBorder - 1]) {
        nodeIndicesFollowingBorder.resize(numberOfPointsInBorder - 1);
    }
}


/**
 * Given an path consisting of unconnected nodes, create a path
 * that connectects the nodes.
 *
 * @param surfaceFile
 *    Surface whose topology is used for finding the path.
 * @param unconnectedNodesPath
 *    Input consisting of the unconnected nodes path.
 * @param connectedNodesPathOut
 *    Output connected nodes path.
 */
void
AlgorithmNodesInsideBorder::createConnectedNodesPath(const SurfaceFile* surfaceFile,
                                                     const std::vector<int32_t>& unconnectedNodesPath,
                                                     std::vector<int32_t>& connectedNodesPathOut)
{
    connectedNodesPathOut.clear();
    
    /*
     * Geodesic helper for surface
     */
    CaretPointer<GeodesicHelper> geodesicHelper = surfaceFile->getGeodesicHelper();
    
    /*
     * Get the topology helper for the surface with neighbors sorted.
     */
    CaretPointer<TopologyHelper> th = surfaceFile->getTopologyHelper(true);
    
    /*
     * Find connected path along node neighbors
     */
    const int32_t numberOfNodesInUnconnectedPath = static_cast<int32_t>(unconnectedNodesPath.size());
    for (int32_t i = 0; i < numberOfNodesInUnconnectedPath; i++) {
        const int node = unconnectedNodesPath[i];
        connectedNodesPathOut.push_back(node);
        
        int nextNode = -1;
        const bool lastNodeFlag = (i >= (numberOfNodesInUnconnectedPath - 1));
        if (lastNodeFlag) {
            nextNode = unconnectedNodesPath[0];
        }
        else {
            nextNode = unconnectedNodesPath[i + 1];
        }
        
        
        if (node != nextNode) {
            bool doGeodesicSearch = true;
            
            /*
             * See if next node is an immediate neighbor of the current node.
             */
            const std::vector<int32_t> neighbors = th->getNodeNeighbors(node);
            if (std::find(neighbors.begin(),
                          neighbors.end(),
                          nextNode) != neighbors.end()) {
                connectedNodesPathOut.push_back(nextNode);
                doGeodesicSearch = false;
            }
            
            if (doGeodesicSearch) {
                /*
                 * Find the geodesic path from node to the next node
                 */
                std::vector<float> distances;
                std::vector<int32_t> parentNodes;
                geodesicHelper->getGeoFromNode(node,
                                               distances,
                                               parentNodes,
                                               false);
                
                bool doneFlag = false;
                std::vector<int32_t> pathFromNextNodeToNode;
                int32_t pathNode = nextNode;
                while (doneFlag == false) {
                    int32_t nextPathNode = parentNodes[pathNode];
                    if (nextPathNode >= 0) {
                        if (nextPathNode == node) {
                            doneFlag = true;
                        }
                        else {
                            pathFromNextNodeToNode.push_back(nextPathNode);
                        }
                    }
                    else {
                        doneFlag = true;
                    }
                    
                    if (doneFlag == false) {
                        pathNode = nextPathNode;
                    }
                }
                
                const int32_t numberOfPathNodes = static_cast<int32_t>(pathFromNextNodeToNode.size());
                for (int32_t i = (numberOfPathNodes - 1); i >= 0; i--) {
                    connectedNodesPathOut.push_back(pathFromNextNodeToNode[i]);
                }
            }
        }
    }
    
    /*
     * Remove duplicates.
     */
    this->cleanNodePath(connectedNodesPathOut);
    
    /*
     * Valid that the path nodes are connected.
     */
    this->validateConnectedNodesPath(surfaceFile,
                                     connectedNodesPathOut);
    
}

/**
 * Find the nodes inside the unconnected path on the surface.
 *
 * @param surfaceFile
 *    Surface file for nodes inside connected path.
 * @param unconnectedNodesPath
 *    Unconnected path for which nodes inside are found.
 * @param nodesEnclosedByPathOut
 *    Nodes enclosed by the path.
 */
void
AlgorithmNodesInsideBorder::findNodesEnclosedByUnconnectedPath(const SurfaceFile* surfaceFile,
                                                               const std::vector<int32_t>& unconnectedNodesPath,
                                                               std::vector<int32_t>& nodesEnclosedByPathOut)
{
    nodesEnclosedByPathOut.clear();
    
    /*
     * Find the nodes enclosed by the unconnected path
     * assuming the path is oriented counter-clockwise
     */
    std::vector<int32_t> connectedNodesPath;
    std::vector<int32_t> nodesEnclosedByPath;
    findNodesEnclosedByUnconnectedPathCCW(surfaceFile,
                                          unconnectedNodesPath,
                                          connectedNodesPath,
                                          nodesEnclosedByPath);
    
    
    bool doInverseFlag = this->isInverseSelection;
    
    /*
     * This should NEVER happen, but, apparently, it does.
     */
    const int32_t halfNumberOfSurfaceNodes = surfaceFile->getNumberOfNodes() / 2;
    if (static_cast<int32_t>(nodesEnclosedByPath.size()) > halfNumberOfSurfaceNodes) {
        CaretLogFine(m_borderName
                       + ("FAILURE to correctly find nodes inside path. Number of nodes "
                          "inside path is greater than half number of nodes in surface. "
                          "Flipping result but it may not be correct around the border nodes."));

        /*
         * Flip the inverse flag 
         */
        doInverseFlag = ! doInverseFlag;
    }
    
    /*
     * Get the topology helper for the surface with neighbors sorted.
     */
    CaretPointer<TopologyHelper> th = surfaceFile->getTopologyHelper(true);
    
    /*
     * Use a boolean flag to indicate if a node is inside or outside path
     */
    const int32_t numberOfNodes = surfaceFile->getNumberOfNodes();
    std::vector<bool> insideROI(numberOfNodes,
                                false);
    for (std::vector<int32_t>::iterator iter = nodesEnclosedByPath.begin();
         iter != nodesEnclosedByPath.end();
         iter++) {
        const int32_t nodeIndex = *iter;
        CaretAssertVectorIndex(insideROI, nodeIndex);
        insideROI[nodeIndex] = true;
    }
    
    /*
     * Perform inverse and also exlude any nodes that are part of the path
     */
    for (int32_t i = 0; i < numberOfNodes; i++) {
        bool insideFlag = insideROI[i];
        if (doInverseFlag) {
            insideFlag = ( ! insideFlag);
        }
        
        if (th->getNodeHasNeighbors(i)) {
            if (std::find(connectedNodesPath.begin(),
                          connectedNodesPath.end(),
                          i) != connectedNodesPath.end()) {
                insideFlag = false;
            }
        }
        else {
            insideFlag = false;
        }
        
        if (insideFlag) {
            nodesEnclosedByPathOut.push_back(i);
        }
    }
}

/**
 * Find the nodes inside the unconnected path on the surface.
 *
 * @param surfaceFile
 *    Surface file for nodes inside connected path.
 * @param unconnectedNodesPath
 *    Unconnected path for which nodes inside are found.
 * @param connectedNodesPathOut
 *    Connected path formed from unconnected path.
 * @param nodesEnclosedByPathOut
 *    Nodes enclosed by the path.
 */
void
AlgorithmNodesInsideBorder::findNodesEnclosedByUnconnectedPathCCW(const SurfaceFile* surfaceFile,
                                                                  const std::vector<int32_t>& unconnectedNodesPath,
                                                                  std::vector<int32_t>& connectedNodesPathOut,
                                                                  std::vector<int32_t>& nodesEnclosedByPathOut)
{
    connectedNodesPathOut.clear();
    nodesEnclosedByPathOut.clear();
    
    std::cout << "UNCONNECTED: ";
    isNodePathSelfIntersecting(unconnectedNodesPath);
    
    /*
     * Convert the unconnected nodes path into a connected nodes path
     */
    std::vector<int32_t> connectedNodesPath;
    createConnectedNodesPath(surfaceFile,
                             unconnectedNodesPath,
                             connectedNodesPath);
    
    int32_t numberOfNodesInConnectedPath = static_cast<int32_t>(connectedNodesPath.size());
    if (numberOfNodesInConnectedPath < 4) {
        throw AlgorithmException("Connected path is too small "
                                 "as it consists of fewer than four vertices.");
    }
    
    if (DEBUG_FLAG) {
        Border* b = Border::newInstanceFromSurfaceNodes(("DEBUG_CONNECTED_PATH_"
                                                         + m_borderName),
                                                        surfaceFile,
                                                        connectedNodesPath);
        addDebugBorder(b);
    }

    std::cout << "CONNECTED: ";
    isNodePathSelfIntersecting(connectedNodesPath);
    
    /*
     * Determine the nodes inside the connected path
     */
    this->findNodesEnclosedByConnectedNodesPathCounterClockwise(surfaceFile,
                                                connectedNodesPath,
                                                nodesEnclosedByPathOut);
    
    connectedNodesPathOut = connectedNodesPath;
}

/**
 * Find the nodes OUTSIDE the given connected path.
 *
 * @param connectedNodesPath
 *    Connected path for which nodes inside are found.
 * @param nodesOutsidePathOut
 *    Vector into which nodes OUTSIDE connected path are loaded. 
 */
void
AlgorithmNodesInsideBorder::findNodesOutsideOfConnectedPath(const std::vector<int32_t>& connectedNodesPath,
                                                             std::vector<int32_t>& nodesOutsidePathOut)
{
    nodesOutsidePathOut.clear();

    /*
     * Track nodes that are found inside and/or have been visited.
     */
    const int32_t numberOfNodes = m_surfaceFile->getNumberOfNodes();
    std::vector<NodeInsideBorderStatus> nodeSearchStatus(numberOfNodes,
                                                         NODE_UNVISITED);
    std::vector<bool> insideBorderFlag(numberOfNodes, false);
    
    /*
     * Mark all nodes in connected path as visited.
     */
    const int32_t numberOfNodesInConnectedPath = static_cast<int32_t>(connectedNodesPath.size());
    for (int32_t i = 0; i < numberOfNodesInConnectedPath; i++) {
        CaretAssertVectorIndex(nodeSearchStatus, connectedNodesPath[i]);
        nodeSearchStatus[connectedNodesPath[i]] = NODE_VISITED;
    }
    
    /*
     * Get the topology helper for the surface with neighbors sorted.
     */
    CaretPointer<TopologyHelper> th = m_surfaceFile->getTopologyHelper(true);
    
    int32_t startNode = findNodeFurthestFromConnectedPathCenterOfGravity(th,
                                                                         connectedNodesPath,
                                                                         nodeSearchStatus);
    
    if (startNode < 0) {
        throw AlgorithmException("Failed to find vertex that is not inside of the connected path.");
    }
    
    /*
     * Mark the starting node as 'inside'.
     */
    CaretAssertVectorIndex(insideBorderFlag, startNode);
    insideBorderFlag[startNode] = true;
    
    /*
     * Use a stack to help with search.
     */
    std::stack<int32_t> stack;
    stack.push(startNode);
    
    /*
     * Search until no more to search.
     */
    while (stack.empty() == false) {
        const int32_t nodeNumber = stack.top();
        stack.pop();
        
        /*
         * Has node been visited?
         */
        CaretAssertVectorIndex(nodeSearchStatus, nodeNumber);
        if (nodeSearchStatus[nodeNumber] == NODE_UNVISITED) {
            nodeSearchStatus[nodeNumber] = NODE_VISITED;
            
            /*
             * Set node as inside
             */
            CaretAssertVectorIndex(insideBorderFlag, nodeNumber);
            insideBorderFlag[nodeNumber] = true;
            
            /*
             * Get neighbors of this node
             */
            int numNeighbors = 0;
            const int* neighbors = th->getNodeNeighbors(nodeNumber, numNeighbors);
            
            /*
             * add neighbors to search
             */
            for (int i = 0; i < numNeighbors; i++) {
                const int neighborNode = neighbors[i];
                CaretAssertVectorIndex(nodeSearchStatus, neighborNode);
                if (nodeSearchStatus[neighborNode] != NODE_VISITED) {
                    stack.push(neighborNode);
                }
            }
        }
    }
    
    /*
     * Return nodes inside the path
     */
    int32_t insideCount = 0;
    for (int32_t i = 0; i < numberOfNodes; i++) {
        CaretAssertVectorIndex(insideBorderFlag, i);
        if (insideBorderFlag[i]) {
            nodesOutsidePathOut.push_back(i);
            insideCount++;
        }
    }

    std::cout << "Final inside count: " << insideCount << std::endl;
    std::cout << "nodesOutsidePathOut.size()=" << nodesOutsidePathOut.size() << std::endl;
}


/**
 * Find the nodes inside the connected path on the surface assuming the
 * path is counter-clockwise around the region.
 *
 * @param surfaceFile
 *    Surface file for nodes inside connected path.
 * @param connectedNodesPath
 *    Connected path for which nodes inside are found.
 * @param nodesInsidePathOut
 *    Vector into which nodes inside connected path are loaded.
 */
void
AlgorithmNodesInsideBorder::findNodesEnclosedByConnectedNodesPathCounterClockwise(const SurfaceFile* surfaceFile,
                                                                                  const std::vector<int32_t>& connectedNodesPath,
                                                                                  std::vector<int32_t>& nodesInsidePathOut)
{
    nodesInsidePathOut.clear();
    
    /*
     * Track nodes that are found inside and/or have been visited.
     */
    const int32_t numberOfNodes = surfaceFile->getNumberOfNodes();
    std::vector<NodeInsideBorderStatus> nodeSearchStatus(numberOfNodes,
                                                         NODE_UNVISITED);
    std::vector<bool> insideBorderFlag(numberOfNodes, false);
    
    /*
     * Mark all nodes in connected path as boundary.
     */
    const int32_t numberOfNodesInConnectedPath = static_cast<int32_t>(connectedNodesPath.size());
    for (int32_t i = 0; i < numberOfNodesInConnectedPath; i++) {
        CaretAssertVectorIndex(nodeSearchStatus, connectedNodesPath[i]);
        nodeSearchStatus[connectedNodesPath[i]] = NODE_BOUNDARY;
    }
    
    /*
     * Get the topology helper for the surface with neighbors sorted.
     */
    CaretPointer<TopologyHelper> th = surfaceFile->getTopologyHelper(true);
    
    /*
     * Multiple iterations may be required to find the nodes inside the border.
     * Very skinny borders may "close" that results in only a part of the
     * region inside the border from being identified.  This can be 
     * detected by finding any boundary nodes that remain.
     */
    int32_t loopIterationCounter = 0;
    bool doSearchFlag = true;
    while (doSearchFlag) {
        /*
         * Using three nodes, find a node that is 'on the left'
         * assuming the path is oriented counter-clockwise.
         */
//        int32_t startNode = findUnvisitedNodeInsideConnectedPath(th,
//                                                                 connectedNodesPath,
//                                                                 nodeSearchStatus);
        
        
        
        
        
        for (int32_t i = 0; i < numberOfNodesInConnectedPath; i++) {
            CaretAssertVectorIndex(nodeSearchStatus, connectedNodesPath[i]);
            nodeSearchStatus[connectedNodesPath[i]] = NODE_VISITED;
        }
        int32_t startNode = findNodeFurthestFromConnectedPathCenterOfGravity(th,
                                                                     connectedNodesPath,
                                                                     nodeSearchStatus);
        
        
        
        
        if (startNode >= 0) {
            std::cout << "Start node: " << startNode
            << " loop: " << loopIterationCounter << std::endl;
        }
    
        if (startNode >= 0) {
            /*
             * Mark the starting node as 'inside'.
             */
            CaretAssertVectorIndex(insideBorderFlag, startNode);
            insideBorderFlag[startNode] = true;
            
            /*
             * Use a stack to help with search.
             */
            std::stack<int32_t> stack;
            stack.push(startNode);
            
            /*
             * Search until no more to search.
             */
            while (stack.empty() == false) {
                const int32_t nodeNumber = stack.top();
                stack.pop();
                
                /*
                 * Has node been visited?
                 */
                CaretAssertVectorIndex(nodeSearchStatus, nodeNumber);
                if (nodeSearchStatus[nodeNumber] == NODE_BOUNDARY) {
                    /*
                     * Boundary nodes are not included inside the region
                     * and its 'status' is changed from BOUNDARY to 
                     * VISITED to indicate that it has been encountered.
                     * This assists with very skinny borders where
                     * opposite sides of a border may be adjacent.
                     */
                    nodeSearchStatus[nodeNumber] = NODE_VISITED;
                }
                else if (nodeSearchStatus[nodeNumber] == NODE_UNVISITED) {
                    nodeSearchStatus[nodeNumber] = NODE_VISITED;
                    
                    /*
                     * Set node as inside
                     */
                    CaretAssertVectorIndex(insideBorderFlag, nodeNumber);
                    insideBorderFlag[nodeNumber] = true;
                    
                    /*
                     * Get neighbors of this node
                     */
                    int numNeighbors = 0;
                    const int* neighbors = th->getNodeNeighbors(nodeNumber, numNeighbors);
                    
                    /*
                     * add neighbors to search
                     */
                    for (int i = 0; i < numNeighbors; i++) {
                        const int neighborNode = neighbors[i];
                        CaretAssertVectorIndex(nodeSearchStatus, neighborNode);
                        if (nodeSearchStatus[neighborNode] != NODE_VISITED) {
                            stack.push(neighborNode);
                        }
                    }
                }
            }
            
            /*
             * Count the nodes that still have a BOUNDARY status which 
             * may indicate a narrow skinny border with a multiple 
             * closed regions.
             */
            int32_t boundaryCount = 0;
            for (int32_t i = 0; i < numberOfNodes; i++) {
                CaretAssertVectorIndex(nodeSearchStatus, i);
                if (nodeSearchStatus[i] == NODE_BOUNDARY) {
                    boundaryCount++;
                }
            }
            std::cout << "Loop " << loopIterationCounter << " boundary nodes " << std::endl;
            if (boundaryCount < 3) { 
                doSearchFlag = false;
            }
        }
        else {
            /*
             * No "start" node found.
             * If NOT first iteration, this is acceptable.
             */
            if (loopIterationCounter > 0) {
                doSearchFlag = false;
            }
            else {
                throw AlgorithmException("Failed to find vertex that is not inside of the connected path.");
            }
        }
        
        int32_t insideCount = 0;
        for (int32_t iNode = 0; iNode < numberOfNodes; iNode++) {
            CaretAssertVectorIndex(insideBorderFlag, iNode);
            if (insideBorderFlag[iNode]) {
                insideCount++;
            }
        }
        std::cout << "Loop " << loopIterationCounter << " inside count: " << insideCount << std::endl;
        
        loopIterationCounter++;
    }
    
    int32_t boundaryCount = 0;
    for (int32_t i = 0; i < numberOfNodes; i++) {
        CaretAssertVectorIndex(nodeSearchStatus, i);
        if (nodeSearchStatus[i] == NODE_BOUNDARY) {
            boundaryCount++;
        }
    }
    CaretLogSevere(AString::number(boundaryCount)
                   + " boundary nodes remaining after "
                   + AString::number(loopIterationCounter)
                   + " loop iterations.");
    
    /*
     * Return nodes inside the path
     */
    for (int32_t i = 0; i < numberOfNodes; i++) {
        CaretAssertVectorIndex(insideBorderFlag, i);
        if (insideBorderFlag[i]) {
            nodesInsidePathOut.push_back(i);
        }
    }
    
    int32_t insideCount = 0;
    for (int32_t iNode = 0; iNode < numberOfNodes; iNode++) {
        CaretAssertVectorIndex(insideBorderFlag, iNode);
        if (insideBorderFlag[iNode]) {
            insideCount++;
        }
    }
    std::cout << "Final inside count: " << insideCount << std::endl;
    std::cout << "nodesInsidePathOut.size()=" << nodesInsidePathOut.size() << std::endl;
}

/**
 * Find an unvisited node that is FURTHEST from the center-of-gravity of
 * the ROI
 */
int32_t
AlgorithmNodesInsideBorder::findNodeFurthestFromConnectedPathCenterOfGravity(const TopologyHelper* topologyHelper,
                                                         const std::vector<int32_t>& connectedNodesPath,
                                                         std::vector<NodeInsideBorderStatus>& nodeSearchStatus)
{
    double sumX = 0.0;
    double sumY = 0.0;
    double sumZ = 0.0;
    double sumCount = 0.0;
    
    const int32_t numberOfNodesInConnectedPath = static_cast<int32_t>(connectedNodesPath.size());
    
    for (int32_t i = 1; i < (numberOfNodesInConnectedPath - 1); i++) {
        const float* xyz = m_surfaceFile->getCoordinate(connectedNodesPath[i]);
        sumX += xyz[0];
        sumY += xyz[1];
        sumZ += xyz[2];
        sumCount++;
    }
    
    if (sumCount >= 1.0) {
        const float cog[3] = {
            sumX / sumCount,
            sumY / sumCount,
            sumZ / sumCount
        };
        
        float maxDist = -1.0;
        int32_t maxDistNodeIndex = -1;
        
        const int32_t numberOfNodes = m_surfaceFile->getNumberOfNodes();
        for (int32_t i = 0; i < numberOfNodes; i++) {
            if (nodeSearchStatus[i] == NODE_UNVISITED) {
                if (topologyHelper->getNodeHasNeighbors(i)) {
                    const float* coordXYZ = m_surfaceFile->getCoordinate(i);
                    const float distSQ = MathFunctions::distanceSquared3D(cog,
                                                                          coordXYZ);
                    if (distSQ > maxDist) {
                        maxDist = distSQ;
                        maxDistNodeIndex = i;
                    }
                }
            }
        }
        
        return maxDistNodeIndex;
    }
    
    return -1;
}


/**
 * Find an 'unvisited' node inside the connected path.
 * The connected path is assumed to be oriented in a counter-clockwise orientation.
 * So, find three consecutive boundary (connected path nodes) and then
 * find a node 'to the left' of these consecutive boundary nodes.
 *
 * @param topologyHelper
 *     Topology helper used for examining a node's neighbors.
 * @param connectedNodesPath
 *     The connected path that forms the region's boundary.
 * @param nodeSearchStatus
 *     Search status of each node.
 * @return
 *     Index of an 'unvisited' node inside the connected path or a negative
 *     value if no node was found.
 */
int32_t
AlgorithmNodesInsideBorder::findUnvisitedNodeInsideConnectedPath(const TopologyHelper* topologyHelper,
                                                                 const std::vector<int32_t>& connectedNodesPath,
                                                                 std::vector<NodeInsideBorderStatus>& nodeSearchStatus)
{
    const int32_t numberOfNodesInConnectedPath = static_cast<int32_t>(connectedNodesPath.size());
    
    for (int32_t i = 1; i < (numberOfNodesInConnectedPath - 1); i++) {
        const int pathNode     = connectedNodesPath[i];
        CaretAssertVectorIndex(nodeSearchStatus, pathNode);
        const int prevPathNode = connectedNodesPath[i - 1];
        const int nextPathNode = connectedNodesPath[i + 1];
        
        if ((nodeSearchStatus[pathNode] == NODE_BOUNDARY)
            && (nodeSearchStatus[prevPathNode] == NODE_BOUNDARY)
            && (nodeSearchStatus[nextPathNode] == NODE_BOUNDARY)) {
            int numNeighbors = 0;
            const int* neighbors = topologyHelper->getNodeNeighbors(pathNode, numNeighbors);
            if (numNeighbors > 2) {
                /*
                 * Located the next and previous nodes in the pathNode's neighbors
                 */
                int32_t prevIndex = -1;
                int32_t nextIndex = -1;
                for (int32_t j = 0; j < numNeighbors; j++) {
                    if (neighbors[j] == prevPathNode) {
                        prevIndex = j;
                    }
                    else if (neighbors[j] == nextPathNode) {
                        nextIndex = j;
                    }
                }
                
                if ((nextIndex >= 0) && (prevIndex >= 0)) {
                    /*
                     * Neighbors are ALWAYS oriented counter-clockwise.
                     * So, we want to examine nodes that are in the 
                     * neighbors array AFTER 'next' and before 'prev'.
                     */
                    std::vector<int32_t> neighborsOfInterest;
                    if (prevIndex > nextIndex) {
                        /*
                         * Find neighbors in between next and prev
                         */
                        for (int32_t k = (nextIndex + 1); k < prevIndex; k++) {
                            neighborsOfInterest.push_back(neighbors[k]);
                        }
                    }
                    else {
                        /*
                         * When 'prev' is less than 'next' we have
                         * wrapped the array so just want any nodes
                         * greater than 'next' or less than 'prev'.
                         */
                        for (int32_t k = 0; k < numNeighbors; k++) {
                            if ((k > nextIndex)
                                || (k < prevIndex)) {
                                neighborsOfInterest.push_back(neighbors[k]);
                            }
                        }
                    }
                    
                    /*
                     * Examine any nodes that may be inside the region
                     * to see if any remain unvisited.
                     */
                    for (std::vector<int32_t>::iterator iter = neighborsOfInterest.begin();
                         iter != neighborsOfInterest.end();
                         iter++) {
                        const int32_t nodeIndex = *iter;
                        CaretAssertVectorIndex(nodeSearchStatus, nodeIndex);
                        if (nodeSearchStatus[nodeIndex] == NODE_UNVISITED) {
                            const AString msg = ("Path nodes: ("
                                                 + AString::number(prevPathNode)
                                                 + ", "
                                                 + AString::number(pathNode)
                                                 + ", "
                                                 + AString::number(nextPathNode)
                                                 + ") neighbors: ("
                                                 + AString::fromNumbers(neighbors, numNeighbors, ", ")
                                                 + " interior node: "
                                                 + AString::number(nodeIndex));
                            std::cout << qPrintable(msg) << std::endl;
                            
                            /*
                             * FOUND unvisited node inside region so
                             * RETURN IT!
                             */
                            return nodeIndex;
                        }
                    }
                }
                
//                if (insideNodeIndex >= 0) {
//                    if (nodeSearchStatus[insideNodeIndex] != NODE_UNVISITED) {
//                        insideNodeIndex = -1;
//                    }
//                    else if (std::find(connectedNodesPath.begin(),
//                                       connectedNodesPath.end(),
//                                       insideNodeIndex) != connectedNodesPath.end()) {
//                        insideNodeIndex = -1;
//                    }
//                    
//                    if (insideNodeIndex >= 0) {
//                        std::cout << "Start node: " << insideNodeIndex << " from boundary ("
//                        << prevPathNode << ", " << pathNode << ", " << nextPathNode << ")"
//                        << " loop " << loopIterationCounter << std::endl;
//                        break;
//                    }
//                }
            }
        }
    }
    
    return -1;
}


///**
// * Find the nodes inside the connected path on the surface assuming the
// * path is counter-clockwise around the region.
// *
// * @param surfaceFile
// *    Surface file for nodes inside connected path.
// * @param connectedNodesPath
// *    Connected path for which nodes inside are found.
// * @param nodesInsidePathOut
// *    Vector into which nodes inside connected path are loaded.
// */
//void 
//AlgorithmNodesInsideBorder::findNodesEnclosedByConnectedNodesPathCounterClockwise(const SurfaceFile* surfaceFile,
//                                                          const std::vector<int32_t>& connectedNodesPath,
//                                                          std::vector<int32_t>& nodesInsidePathOut)
//{
//    /*
//     * Get the topology helper for the surface with neighbors sorted.
//     */
//    CaretPointer<TopologyHelper> th = surfaceFile->getTopologyHelper(true);
//    
//    /*
//     * Using three nodes, find a node that is 'on the left'
//     * assuming the path is oriented counter-clockwise.
//     */
//    int32_t startNode = -1;
//    const int32_t numberOfNodesInConnectedPath = static_cast<int32_t>(connectedNodesPath.size());
//    for (int32_t i = 1; i < (numberOfNodesInConnectedPath - 1); i++) {
//        const int prevPathNode = connectedNodesPath[i - 1];
//        const int pathNode     = connectedNodesPath[i];
//        const int nextPathNode = connectedNodesPath[i + 1];
//        
//        int numNeighbors;
//        const int* neighbors = th->getNodeNeighbors(pathNode, numNeighbors);
//        if (numNeighbors > 2) {
//            int32_t prevIndex = -1;
//            int32_t nextIndex = -1;
//            for (int32_t j = 0; j < numNeighbors; j++) {
//                if (neighbors[j] == prevPathNode) {
//                    prevIndex = j;
//                }
//                else if (neighbors[j] == nextPathNode) {
//                    nextIndex = j;
//                }
//            }
//            
//            if ((nextIndex >= 0) && (prevIndex >= 0)) {
//                if (nextIndex >= (numNeighbors - 1)) {
//                    if (prevIndex > 0) {
//                        startNode = neighbors[0];
//                    }
//                }
//                else {
//                    if (prevIndex != (nextIndex + 1)) {
//                        startNode = neighbors[nextIndex + 1];
//                    }
//                }
//            }
//            
//            if (startNode >= 0) {
//                if (std::find(connectedNodesPath.begin(),
//                              connectedNodesPath.end(),
//                              startNode) != connectedNodesPath.end()) {
//                    startNode = -1;
//                }
//                
//                if (startNode >= 0) {
//                    break;
//                }
//            }
//        }
//    }
//    
//    if (startNode < 0) {
//        throw AlgorithmException("Failed to find vertex that is not inside of the connected path.");
//    }
//    
//    /*
//     * Track nodes that are found inside and/or have been visited.
//     */
//    const int32_t numberOfNodes = surfaceFile->getNumberOfNodes();
//    std::vector<NodeInsideBorderStatus> visitedStatus(numberOfNodes,
//                                                      NODE_UNVISITED);
//    std::vector<bool> inside(numberOfNodes, false);
//    
//    /*
//     * Mark all nodes in connected path as visited.
//     */
//    for (int32_t i = 0; i < numberOfNodesInConnectedPath; i++) {
//        visitedStatus[connectedNodesPath[i]] = NODE_BOUNDARY;
//    }
//    
//    /*
//     * Mark the starting node as 'inside'.
//     */
//    inside[startNode] = true;
//    
//    /*
//     * Use a stack to help with search.
//     */
//    std::stack<int32_t> stack;
//    stack.push(startNode);
//    
//    /*
//     * Search until no more to search.
//     */
//    while (stack.empty() == false) {
//        const int32_t nodeNumber = stack.top();
//        stack.pop();
//        
//        /*
//         * Has node been visited?
//         */
//        if (visitedStatus[nodeNumber] == NODE_BOUNDARY) {
//            visitedStatus[nodeNumber] = NODE_VISITED;
//        }
//        else if (visitedStatus[nodeNumber] == NODE_UNVISITED) {
//            visitedStatus[nodeNumber] = NODE_VISITED;
//            
//            /*
//             * Set node as inside
//             */
//            inside[nodeNumber] = true;
//            
//            /*
//             * Get neighbors of this node
//             */
//            int numNeighbors = 0;
//            const int* neighbors = th->getNodeNeighbors(nodeNumber, numNeighbors);
//            
//            /*
//             * add neighbors to search
//             */
//            for (int i = 0; i < numNeighbors; i++) {
//                const int neighborNode = neighbors[i];
//                if (visitedStatus[neighborNode] != NODE_VISITED) {
//                    stack.push(neighborNode);
//                }
//            }
//        }
//    }
//    
//    int32_t boundaryCount = 0;
//    for (int32_t i = 0; i < numberOfNodes; i++) {
//        if (visitedStatus[i] == NODE_BOUNDARY) {
//            boundaryCount++;
//        }
//    }
//    CaretLogSevere(AString::number(boundaryCount)
//                   + " boundary nodes were not visited.");
//    
//    /*
//     * Return nodes inside the path
//     */
//    for (int32_t i = 0; i < numberOfNodes; i++) {
//        if (inside[i]) {
//            nodesInsidePathOut.push_back(i);
//        }
//    }
//}

/**
 * Clean the path by removing any consecutive nodes that are identical.
 *
 * @param nodePath
 *    Path that is cleaned.
 */
void 
AlgorithmNodesInsideBorder::cleanNodePath(std::vector<int32_t>& nodePath)
{
    std::vector<int32_t> path = nodePath;
    nodePath.clear();
    
    /*
     * Unique copy will remove consecutive identical elements
     */
    std::unique_copy(path.begin(),
                     path.end(),
                     back_inserter(nodePath));
}

/**
 * Try to find out if the path intersects itself. that intersect each other.
 *
 * @param nodePath
 *    Path tested for intersection.
 */
bool
AlgorithmNodesInsideBorder::isNodePathSelfIntersecting(const std::vector<int32_t>& nodePath)
{
    const int32_t numNodes = static_cast<int32_t>(nodePath.size());
    
    int32_t positiveCount = 0;
    int32_t negativeCount = 0;
    
    for (int32_t iNode = 0; iNode < numNodes; iNode++) {
        CaretAssertVectorIndex(nodePath, iNode);
        int32_t iPrev = iNode - 1;
        if (iPrev < 0) {
            iPrev = numNodes - 1;
        }
        
        int32_t iNext = iNode + 1;
        if (iNext >= numNodes) {
            iNext = 0;
        }
        
        CaretAssertVectorIndex(nodePath, iPrev);
        const int32_t prevNode = nodePath[iPrev];
        
        CaretAssertVectorIndex(nodePath, iNext);
        const int32_t nextNode = nodePath[iNext];
        
        const float* p1 = m_surfaceFile->getCoordinate(prevNode);
        const float* p2 = m_surfaceFile->getCoordinate(iNode);
        const float* p3 = m_surfaceFile->getCoordinate(nextNode);
        
        float a[3], b[3];
        MathFunctions::subtractVectors(p2, p1, a);
        MathFunctions::subtractVectors(p3, p1, b);
        const float aLength = MathFunctions::vectorLength(a);
        const float bLength = MathFunctions::vectorLength(b);
        if ((aLength > 0)
            && (bLength > 0)) {
            const float dot = MathFunctions::dotProduct(a, b);
            
            if (dot > 0) {
                positiveCount++;
            }
            else if (dot < 0.0) {
                negativeCount++;
            }
        }
    }
    
    std::cout << "Positive angles: " << positiveCount << " Negative angles: " << negativeCount << std::endl;
    
    return false;
}


/**
 * Verify that the connect nodes path is fully connected.
 *
 * @param surfaceFile
 *    Surface file for nodes inside connected path.
 * @param connectedNodesPath
 *    Path that is validated.
 */
void 
AlgorithmNodesInsideBorder::validateConnectedNodesPath(const SurfaceFile* surfaceFile,
                                                       const std::vector<int32_t>& connectedNodesPath)
{
    /*
     * Get the topology helper for the surface with neighbors sorted.
     */
    CaretPointer<TopologyHelper> th = surfaceFile->getTopologyHelper(false);
    
    /*
     * Check the path to see that each pair of nodes are connected.
     */
    const int32_t numberOfNodesInPath = static_cast<int32_t>(connectedNodesPath.size());
    for (int32_t i = 0; i < numberOfNodesInPath; i++) {
        int numNeighbors;
        const int node = connectedNodesPath[i];
        const int* neighbors = th->getNodeNeighbors(node, numNeighbors);
        
        int32_t nextNode = -1;
        if (i >= (numberOfNodesInPath - 1)) {
            nextNode = connectedNodesPath[0];
        }
        else {
            nextNode = connectedNodesPath[i + 1];
        }
        
        if (node != nextNode) {
            bool foundIt = false;
            for (int j = 0; j < numNeighbors; j++) {
                if (neighbors[j] == nextNode) {
                    foundIt = true;
                    break;
                }
            }
            
            if (foundIt == false) {
                throw AlgorithmException("Validation of vertex path along border failed.  Vertex "
                                         + AString::number(node)
                                         + " should be connected to "
                                         + AString::number(nextNode)
                                         + " but it is not.");
            }
        }
    }
}


