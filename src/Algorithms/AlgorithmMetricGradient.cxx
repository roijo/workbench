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

#include "AlgorithmMetricGradient.h"
#include "AlgorithmMetricSmoothing.h"
#include "AlgorithmException.h"
#include "CaretOMP.h"
#include "CaretLogger.h"
#include "FloatMatrix.h"
#include "MathFunctions.h"
#include "MetricFile.h"
#include "PaletteColorMapping.h"
#include "SurfaceFile.h"
#include "TopologyHelper.h"

using namespace caret;
using namespace std;

AString AlgorithmMetricGradient::getCommandSwitch()
{
    return "-metric-gradient";
}

AString AlgorithmMetricGradient::getShortDescription()
{
    return "SURFACE GRADIENT OF A METRIC FILE";
}

OperationParameters* AlgorithmMetricGradient::getParameters()
{
    OperationParameters* ret = new OperationParameters();
    ret->addSurfaceParameter(1, "surface", "the surface to compute the gradient on");
    
    ret->addMetricParameter(2, "metric-in", "the metric to compute the gradient of");
    
    ret->addMetricOutputParameter(3, "metric-out", "the magnitude of the gradient");
    
    OptionalParameter* presmooth = ret->createOptionalParameter(4, "-presmooth", "smooth the metric before computing the gradient");
    presmooth->addDoubleParameter(1, "kernel", "the sigma for the gaussian smoothing kernel, in mm");
    
    OptionalParameter* roiOption = ret->createOptionalParameter(5, "-roi", "select a region of interest to take the gradient of");
    roiOption->addMetricParameter(1, "roi-metric", "the area to take the gradient within, as a metric");
    roiOption->createOptionalParameter(2, "-match-columns", "for each input column, use the corresponding column from the roi");
    
    OptionalParameter* vecOut = ret->createOptionalParameter(6, "-vectors", "output vectors");
    vecOut->addMetricOutputParameter(1, "vector-metric-out", "the vectors as a metric file");
    
    OptionalParameter* columnSelect = ret->createOptionalParameter(7, "-column", "select a single column to compute the gradient of");
    columnSelect->addStringParameter(1, "column", "the column number or name");
    
    ret->createOptionalParameter(8, "-average-normals", "average the normals of each vertex with its neighbors before using them to compute the gradient");
    //that option has no parameters to take, so don't store the return value
    ret->setHelpText(
        AString("At each vertex, the immediate neighbors are unfolded onto a plane tangent to the surface at the vertex (specifically, perpendicular to the normal).  ") +
        "The gradient is computed using a regression between the unfolded positions of the vertices and their values.  " +
        "The gradient is then given by the slopes of the regression, and reconstructed as a 3D gradient vector.  " +
        "By default, takes the gradient of all columns, with no presmoothing, across the whole surface, without averaging the normals of the surface among neighbors.  " +
        "Specifying an ROI will restrict the gradient to only use data from where the ROI metric is positive, and output zeros anywhere the ROI metric is not positive.\n\n" +
        "By default, the first column of the roi metric is used for all input columns.  " +
        "When -match-columns is specified to the -roi option, the input and roi metrics must have the same number of columns, " +
        "and for each input column's index, the same column index is used in the roi metric.  " +
        "If the -match-columns option to -roi is used while the -column option is also used, the number of columns of the roi metric must match the input metric, " +
        "and it will use the roi column with the index of the selected input column.\n\n" +
        "The vector output metric is organized such that the X, Y, and Z components from a single input column are consecutive columns."
    );
    return ret;
}

void AlgorithmMetricGradient::useParameters(OperationParameters* myParams, ProgressObject* myProgObj)
{
    SurfaceFile* mySurf = myParams->getSurface(1);
    MetricFile* myMetricIn = myParams->getMetric(2);
    MetricFile* myMetricOut = myParams->getOutputMetric(3);
    float myPresmooth = -1.0f;//negative or zero means no smoothing
    OptionalParameter* presmooth = myParams->getOptionalParameter(4);
    if (presmooth->m_present)
    {
        myPresmooth = (float)presmooth->getDouble(1);
    }
    MetricFile* myRoi = NULL;
    bool matchRoiColumns = false;
    OptionalParameter* roiOption = myParams->getOptionalParameter(5);
    if (roiOption->m_present)
    {
        myRoi = roiOption->getMetric(1);
        matchRoiColumns = roiOption->getOptionalParameter(2)->m_present;
    }
    MetricFile* myVectorsOut = NULL;
    OptionalParameter* vecOut = myParams->getOptionalParameter(6);
    if (vecOut->m_present)
    {
        myVectorsOut = vecOut->getOutputMetric(1);
    }
    int32_t myColumn = -1;
    OptionalParameter* columnSelect = myParams->getOptionalParameter(7);
    if (columnSelect->m_present)
    {
        myColumn = (int)myMetricIn->getMapIndexFromNameOrNumber(columnSelect->getString(1));
        if (myColumn < 0)
        {
            throw AlgorithmException("invalid column specified");
        }
    }
    OptionalParameter* avgNormals = myParams->getOptionalParameter(8);
    bool myAvgNormals = avgNormals->m_present;
    AlgorithmMetricGradient(myProgObj, mySurf, myMetricIn, myMetricOut, myVectorsOut, myPresmooth, myRoi, myAvgNormals, myColumn, matchRoiColumns);//executes the algorithm
}

AlgorithmMetricGradient::AlgorithmMetricGradient(ProgressObject* myProgObj,
                                                 SurfaceFile* mySurf,
                                                 const MetricFile* myMetricIn,
                                                 MetricFile* myMetricOut,
                                                 MetricFile* myVectorsOut,
                                                 const float myPresmooth,
                                                 const MetricFile* myRoi,
                                                 const bool myAvgNormals,
                                                 const int32_t myColumn,
                                                 const bool matchRoiColumns) : AbstractAlgorithm(myProgObj)
{
    ProgressObject* smoothProgress = NULL;
    if (myProgObj != NULL && myPresmooth > 0.0f)
    {
        smoothProgress = myProgObj->addAlgorithm(AlgorithmMetricSmoothing::getAlgorithmWeight());
    }
    LevelProgress myProgress(myProgObj);
    int32_t numNodes = mySurf->getNumberOfNodes();
    if (myMetricIn->getNumberOfNodes() != numNodes)
    {//TODO: write these as static functions into an AlgorithmHelper class?
        throw AlgorithmException("metric does not match surface in number of vertices");
    }
    if (myRoi != NULL && myRoi->getNumberOfNodes() != numNodes)
    {
        throw AlgorithmException("roi metric does not match surface in number of vertices");
    }
    int32_t numColumns = myMetricIn->getNumberOfColumns();
    if (myColumn < -1 || myColumn >= numColumns)
    {
        throw AlgorithmException("invalid column number");
    }
    if (myRoi != NULL && matchRoiColumns && numColumns != myRoi->getNumberOfColumns())
    {
        throw AlgorithmException("match roi columns specified, but roi metric has the wrong number of columns");
    }
    const MetricFile* toProcess = myMetricIn;
    MetricFile processTemp;
    int32_t useColumn = myColumn;
    if (myPresmooth > 0.0f)
    {
        AlgorithmMetricSmoothing(smoothProgress, mySurf, myMetricIn, myPresmooth, &processTemp, myRoi, matchRoiColumns, false, myColumn, NULL, MetricSmoothingObject::GEO_GAUSS_AREA);
        toProcess = &processTemp;
        if (myColumn != -1)
        {
            useColumn = 0;
        }
    }
    mySurf->computeNormals(myAvgNormals);
    const float* myNormals = mySurf->getNormalData();
    const float* myCoords = mySurf->getCoordinateData();
    bool haveWarned = false, haveFailed = false;//print warning or failure messages only once
    if (myColumn == -1)
    {
        myMetricOut->setNumberOfNodesAndColumns(numNodes, numColumns);
        myMetricOut->setStructure(mySurf->getStructure());
        float* myVecScratch = NULL;
        if (myVectorsOut != NULL)
        {
            myVectorsOut->setNumberOfNodesAndColumns(numNodes, numColumns * 3);
            myVectorsOut->setStructure(mySurf->getStructure());
            myVecScratch = new float[numNodes * 3];
        }
        float* myScratch = new float[numNodes];
        for (int32_t col = 0; col < numColumns; ++col)
        {
            const float* myRoiColumn = NULL;
            if (myRoi != NULL)
            {
                if (matchRoiColumns)
                {
                    myRoiColumn = myRoi->getValuePointerForColumn(col);
                } else {
                    myRoiColumn = myRoi->getValuePointerForColumn(0);
                }
            }
            const float* myMetricColumn = toProcess->getValuePointerForColumn(col);
            myMetricOut->setColumnName(col, toProcess->getColumnName(col) + ", gradient");
            *(myMetricOut->getPaletteColorMapping(col)) = *(toProcess->getPaletteColorMapping(col));//copy the palette settings
            if (myVectorsOut != NULL)
            {
                myVectorsOut->setColumnName(col * 3, toProcess->getColumnName(col) + ", gradient vector X");
                myVectorsOut->setColumnName(col * 3 + 1, toProcess->getColumnName(col) + ", gradient vector Y");
                myVectorsOut->setColumnName(col * 3 + 2, toProcess->getColumnName(col) + ", gradient vector Z");
            }
#pragma omp CARET_PAR
            {
                float somevec[3], xhat[3], yhat[3];
                float sanity = 0.0f;
                CaretPointer<TopologyHelper> myTopoHelp = mySurf->getTopologyHelper();//this stores and reuses helpers, so it isn't really a problem to call inside the loop
#pragma omp CARET_FOR schedule(dynamic)
                for (int32_t i = 0; i < numNodes; ++i)
                {
                    if (myRoiColumn != NULL && myRoiColumn[i] <= 0.0f)
                    {
                        myScratch[i] = 0.0f;
                        if (myVecScratch != NULL)
                        {
                            myVecScratch[i] = 0.0f;
                            myVecScratch[i + numNodes] = 0.0f;
                            myVecScratch[i + numNodes * 2] = 0.0f;
                        }
                        continue;
                    }
                    int32_t numNeigh;
                    int32_t i3 = i * 3;
                    const int32_t* myNeighbors = myTopoHelp->getNodeNeighbors(i, numNeigh);//one function call isn't that bad, most time spent is floating point math anyway
                    const float* myNormal = myNormals + i3;
                    const float* myCoord = myCoords + i3;
                    float nodeValue = myMetricColumn[i];
                    somevec[2] = 0.0;
                    if (myNormal[0] > myNormal[1])
                    {//generate a vector not parallel to normal
                        somevec[0] = 0.0;
                        somevec[1] = 1.0;
                    } else {
                        somevec[0] = 1.0;
                        somevec[1] = 0.0;
                    }
                    MathFunctions::crossProduct(myNormal, somevec, xhat);
                    MathFunctions::normalizeVector(xhat);
                    MathFunctions::crossProduct(myNormal, xhat, yhat);
                    MathFunctions::normalizeVector(yhat);//xhat, yhat are orthogonal unit vectors describing the coord system with k = surface normal
                    int neighCount = 0;//count within-roi neighbors, not simply surface neighbors
                    if (numNeigh >= 2)
                    {
                        FloatMatrix myRegress = FloatMatrix::zeros(3, 4);
                        for (int32_t j = 0; j < numNeigh; ++j)
                        {
                            int32_t whichNode = myNeighbors[j];
                            int32_t whichNode3 = whichNode * 3;
                            if (myRoiColumn == NULL || myRoiColumn[whichNode] > 0.0f)
                            {
                                ++neighCount;
                                float tempf = myMetricColumn[whichNode] - nodeValue;
                                const float* neighCoord = myCoords + whichNode3;
                                MathFunctions::subtractVectors(neighCoord, myCoord, somevec);
                                float origMag = MathFunctions::vectorLength(somevec);//save the original length
                                float xmag = MathFunctions::dotProduct(xhat, somevec);//dot product to get the direction in 2d
                                float ymag = MathFunctions::dotProduct(yhat, somevec);
                                float mag2d = sqrt(xmag * xmag + ymag * ymag);//get the new magnitude, to divide out
                                xmag *= origMag / mag2d;//normalize the 2d vector and multiply by original length to unfold
                                ymag *= origMag / mag2d;
                                myRegress[0][0] += xmag * xmag;//gather A'A and A'b sums for regression
                                myRegress[0][1] += xmag * ymag;
                                myRegress[0][2] += xmag;
                                myRegress[1][1] += ymag * ymag;
                                myRegress[1][2] += ymag;
                                myRegress[2][2] += 1.0f;
                                myRegress[0][3] += xmag * tempf;
                                myRegress[1][3] += ymag * tempf;
                                myRegress[2][3] += tempf;
                            }
                        }
                        if (neighCount >= 2)
                        {
                            myRegress[1][0] = myRegress[0][1];//complete the symmetric elements
                            myRegress[2][0] = myRegress[0][2];
                            myRegress[2][1] = myRegress[1][2];
                            myRegress[2][2] += 1.0f;//include center (metric and coord differences will be zero, so this is all that is needed)
                            FloatMatrix myRref = myRegress.reducedRowEchelon();
                            somevec[0] = xhat[0] * myRref[0][3] + yhat[0] * myRref[1][3];
                            somevec[1] = xhat[1] * myRref[0][3] + yhat[1] * myRref[1][3];
                            somevec[2] = xhat[2] * myRref[0][3] + yhat[2] * myRref[1][3];//somevec is now our surface gradient
                            sanity = somevec[0] + somevec[1] + somevec[2];
                        }
                    }
                    if (neighCount > 0 && (neighCount < 2 || sanity != sanity))
                    {
                        if (!haveWarned && myRoi == NULL)
                        {//don't issue this warning with an ROI, because it is somewhat expected
                            haveWarned = true;
                            CaretLogWarning("WARNING: gradient calculation found a NaN/inf with regression method for at least vertex " + AString::number(i));
                        }
                        float xgrad = 0.0f, ygrad = 0.0f;
                        for (int32_t j = 0; j < numNeigh; ++j)
                        {
                            int32_t whichNode = myNeighbors[j];
                            int32_t whichNode3 = whichNode * 3;
                            if (myRoiColumn == NULL || myRoiColumn[whichNode] > 0.0f)
                            {
                                float tempf = myMetricColumn[whichNode] - nodeValue;
                                const float* neighCoord = myCoords + whichNode3;
                                MathFunctions::subtractVectors(neighCoord, myCoord, somevec);
                                float origMag = MathFunctions::vectorLength(somevec);//save the original length
                                float xmag = MathFunctions::dotProduct(xhat, somevec);//dot product to get the direction in 2d
                                float ymag = MathFunctions::dotProduct(yhat, somevec);
                                float mag2d = sqrt(xmag * xmag + ymag * ymag);//get the new magnitude, to divide out
                                tempf /= origMag * mag2d;//difference divided by distance gives point estimate of gradient magnitude, also divide by magnitude of 2d vector to normalize it at the same time
                                xgrad += xmag * tempf;//point estimate of gradient magnitude times normalized projected direction gives 2d estimate of gradient
                                ygrad += ymag * tempf;//average point estimates for each neighbor to estimate local gradient
                            }
                        }
                        xgrad /= neighCount;//average
                        ygrad /= neighCount;
                        somevec[0] = xhat[0] * xgrad + yhat[0] * ygrad;//unproject back into 3d
                        somevec[1] = xhat[1] * xgrad + yhat[1] * ygrad;
                        somevec[2] = xhat[2] * xgrad + yhat[2] * ygrad;
                        sanity = somevec[0] + somevec[1] + somevec[2];
                    }
                    if (neighCount <= 0 || sanity != sanity)
                    {
                        if (!haveFailed && myRoiColumn == NULL)
                        {//don't warn with an roi, they can be strange
                            haveFailed = true;
                            CaretLogWarning("Failed to compute gradient for at least vertex " + AString::number(i) +
                            " with standard and fallback methods, outputting ZERO, check your surface for disconnected vertices or other strangeness");
                        }
                        somevec[0] = 0.0f;
                        somevec[1] = 0.0f;
                        somevec[2] = 0.0f;
                    }
                    if (myVecScratch != NULL)
                    {
                        myVecScratch[i] = somevec[0];//split them up far, so that they can be set to columns easily
                        myVecScratch[numNodes + i] = somevec[1];
                        myVecScratch[numNodes * 2 + i] = somevec[2];
                    }
                    myScratch[i] = MathFunctions::vectorLength(somevec);
                }
            }
            if (myVectorsOut != NULL)
            {
                myVectorsOut->setValuesForColumn(col * 3, myVecScratch);
                myVectorsOut->setValuesForColumn(col * 3 + 1, myVecScratch + numNodes);
                myVectorsOut->setValuesForColumn(col * 3 + 2, myVecScratch + (numNodes * 2));
            }
            myMetricOut->setValuesForColumn(col, myScratch);
            myProgress.reportProgress(((float)col + 1) / numColumns);
        }
        delete[] myScratch;
        if (myVecScratch != NULL)
        {
            delete[] myVecScratch;
        }
    } else {
        myMetricOut->setNumberOfNodesAndColumns(numNodes, 1);
        myMetricOut->setStructure(mySurf->getStructure());
        float* myVecScratch = NULL;
        if (myVectorsOut != NULL)
        {
            myVectorsOut->setNumberOfNodesAndColumns(numNodes, 3);
            myVectorsOut->setStructure(mySurf->getStructure());
            myVectorsOut->setColumnName(0, toProcess->getColumnName(useColumn) + ", gradient vector X");
            myVectorsOut->setColumnName(1, toProcess->getColumnName(useColumn) + ", gradient vector Y");
            myVectorsOut->setColumnName(2, toProcess->getColumnName(useColumn) + ", gradient vector Z");
            myVecScratch = new float[numNodes * 3];
        }
        float* myScratch = new float[numNodes];
        const float* myRoiColumn = NULL;
        if (myRoi != NULL)
        {
            if (matchRoiColumns)
            {
                myRoiColumn = myRoi->getValuePointerForColumn(myColumn);//use the ORIGINAL column number, not the one that has been modified due to a presmoothing step that generated a new single column metric
            } else {
                myRoiColumn = myRoi->getValuePointerForColumn(0);
            }
        }
        const float* myMetricColumn = toProcess->getValuePointerForColumn(useColumn);
        myMetricOut->setColumnName(0, toProcess->getColumnName(useColumn) + ", gradient");
        *(myMetricOut->getPaletteColorMapping(0)) = *(toProcess->getPaletteColorMapping(useColumn));//copy the palette settings
#pragma omp CARET_PAR
        {
            float somevec[3], xhat[3], yhat[3];
            float sanity = 0.0f;
            CaretPointer<TopologyHelper> myTopoHelp = mySurf->getTopologyHelper();
#pragma omp CARET_FOR schedule(dynamic)
            for (int32_t i = 0; i < numNodes; ++i)
            {
                if (myRoiColumn != NULL && myRoiColumn[i] <= 0.0f)
                {
                    myScratch[i] = 0.0f;
                    if (myVecScratch != NULL)
                    {
                        myVecScratch[i] = 0.0f;
                        myVecScratch[i + numNodes] = 0.0f;
                        myVecScratch[i + numNodes * 2] = 0.0f;
                    }
                    continue;
                }
                int32_t numNeigh;
                int32_t i3 = i * 3;
                const int32_t* myNeighbors = myTopoHelp->getNodeNeighbors(i, numNeigh);
                const float* myNormal = myNormals + i3;
                const float* myCoord = myCoords + i3;
                float nodeValue = myMetricColumn[i];
                somevec[2] = 0.0;
                if (myNormal[0] > myNormal[1])
                {//generate a vector not parallel to normal
                    somevec[0] = 0.0;
                    somevec[1] = 1.0;
                } else {
                    somevec[0] = 1.0;
                    somevec[1] = 0.0;
                }
                MathFunctions::crossProduct(myNormal, somevec, xhat);
                MathFunctions::normalizeVector(xhat);
                MathFunctions::crossProduct(myNormal, xhat, yhat);
                MathFunctions::normalizeVector(yhat);//xhat, yhat are orthogonal unit vectors describing the coord system with k = surface normal
                int neighCount = 0;//count within-roi neighbors, not simply surface neighbors
                if (numNeigh >= 2)
                {
                    FloatMatrix myRegress = FloatMatrix::zeros(3, 4);
                    for (int32_t j = 0; j < numNeigh; ++j)
                    {
                        int32_t whichNode = myNeighbors[j];
                        int32_t whichNode3 = whichNode * 3;
                        if (myRoiColumn == NULL || myRoiColumn[whichNode] > 0.0f)
                        {
                            ++neighCount;
                            float tempf = myMetricColumn[whichNode] - nodeValue;
                            const float* neighCoord = myCoords + whichNode3;
                            MathFunctions::subtractVectors(neighCoord, myCoord, somevec);
                            float origMag = MathFunctions::vectorLength(somevec);//save the original length
                            float xmag = MathFunctions::dotProduct(xhat, somevec);//dot product to get the direction in 2d
                            float ymag = MathFunctions::dotProduct(yhat, somevec);
                            float mag2d = sqrt(xmag * xmag + ymag * ymag);//get the new magnitude, to divide out
                            xmag *= origMag / mag2d;//normalize the 2d vector and multiply by original length to unfold
                            ymag *= origMag / mag2d;
                            myRegress[0][0] += xmag * xmag;//gather A'A and A'b sums for regression
                            myRegress[0][1] += xmag * ymag;
                            myRegress[0][2] += xmag;
                            myRegress[1][1] += ymag * ymag;
                            myRegress[1][2] += ymag;
                            myRegress[2][2] += 1.0f;
                            myRegress[0][3] += xmag * tempf;
                            myRegress[1][3] += ymag * tempf;
                            myRegress[2][3] += tempf;
                        }
                    }
                    if (neighCount >= 2)
                    {
                        myRegress[1][0] = myRegress[0][1];//complete the symmetric elements
                        myRegress[2][0] = myRegress[0][2];
                        myRegress[2][1] = myRegress[1][2];
                        myRegress[2][2] += 1.0f;//include center (metric and coord differences will be zero, so this is all that is needed)
                        FloatMatrix myRref = myRegress.reducedRowEchelon();
                        somevec[0] = xhat[0] * myRref[0][3] + yhat[0] * myRref[1][3];
                        somevec[1] = xhat[1] * myRref[0][3] + yhat[1] * myRref[1][3];
                        somevec[2] = xhat[2] * myRref[0][3] + yhat[2] * myRref[1][3];//somevec is now our surface gradient
                        sanity = somevec[0] + somevec[1] + somevec[2];
                    }
                }
                if (neighCount > 0 && (neighCount < 2 || sanity != sanity))
                {
                    if (!haveWarned && myRoi == NULL)
                    {//don't issue this warning with an ROI, because it is somewhat expected
                        haveWarned = true;
                        CaretLogWarning("WARNING: gradient calculation found a NaN/inf with regression method for at least vertex " + AString::number(i));
                    }
                    float xgrad = 0.0f, ygrad = 0.0f;
                    for (int32_t j = 0; j < numNeigh; ++j)
                    {
                        int32_t whichNode = myNeighbors[j];
                        int32_t whichNode3 = whichNode * 3;
                        if (myRoiColumn == NULL || myRoiColumn[whichNode] > 0.0f)
                        {
                            float tempf = myMetricColumn[whichNode] - nodeValue;
                            const float* neighCoord = myCoords + whichNode3;
                            MathFunctions::subtractVectors(neighCoord, myCoord, somevec);
                            float origMag = MathFunctions::vectorLength(somevec);//save the original length
                            float xmag = MathFunctions::dotProduct(xhat, somevec);//dot product to get the direction in 2d
                            float ymag = MathFunctions::dotProduct(yhat, somevec);
                            float mag2d = sqrt(xmag * xmag + ymag * ymag);//get the new magnitude, to divide out
                            tempf /= origMag * mag2d;//difference divided by distance gives point estimate of gradient magnitude, also divide by magnitude of 2d vector to normalize it at the same time
                            xgrad += xmag * tempf;//point estimate of gradient magnitude times normalized projected direction gives 2d estimate of gradient
                            ygrad += ymag * tempf;//average point estimates for each neighbor to estimate local gradient
                        }
                    }
                    xgrad /= neighCount;//average
                    ygrad /= neighCount;
                    somevec[0] = xhat[0] * xgrad + yhat[0] * ygrad;//unproject back into 3d
                    somevec[1] = xhat[1] * xgrad + yhat[1] * ygrad;
                    somevec[2] = xhat[2] * xgrad + yhat[2] * ygrad;
                    sanity = somevec[0] + somevec[1] + somevec[2];
                }
                if (neighCount <= 0 || sanity != sanity)
                {
                    if (!haveFailed && myRoiColumn == NULL)
                    {//don't warn with an roi, they can be strange
                        haveFailed = true;
                        CaretLogWarning("Failed to compute gradient for at least vertex " + AString::number(i) +
                            " with standard and fallback methods, outputting ZERO, check your surface for disconnected vertices or other strangeness");
                    }
                    somevec[0] = 0.0f;
                    somevec[1] = 0.0f;
                    somevec[2] = 0.0f;
                }
                if (myVecScratch != NULL)
                {
                    myVecScratch[i] = somevec[0];//split them up far, so that they can be set to columns easily
                    myVecScratch[numNodes + i] = somevec[1];
                    myVecScratch[numNodes * 2 + i] = somevec[2];
                }
                myScratch[i] = MathFunctions::vectorLength(somevec);
            }
        }
        if (myVectorsOut != NULL)
        {
            myVectorsOut->setValuesForColumn(0, myVecScratch);
            myVectorsOut->setValuesForColumn(1, myVecScratch + numNodes);
            myVectorsOut->setValuesForColumn(2, myVecScratch + (numNodes * 2));
        }
        myMetricOut->setValuesForColumn(0, myScratch);
        delete[] myScratch;
        if (myVecScratch != NULL)
        {
            delete[] myVecScratch;
        }
    }
}

float AlgorithmMetricGradient::getAlgorithmInternalWeight()
{
    return 1.0f;//override this if needed, if the progress bar isn't smooth
}

float AlgorithmMetricGradient::getSubAlgorithmWeight()
{
    return AlgorithmMetricSmoothing::getAlgorithmWeight();//if you use a subalgorithm
}
