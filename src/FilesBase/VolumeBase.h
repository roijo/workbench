
#ifndef __VOLUME_BASE_H__
#define __VOLUME_BASE_H__

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

#include "stdint.h"
#include <vector>
#include "CaretAssert.h"
#include "CaretPointer.h"
#include "VolumeMappableInterface.h"
#include "VolumeSpace.h"

namespace caret {

    struct AbstractVolumeExtension
    {
        enum ExtensionType
        {
            NIFTI
        };
        std::vector<char> m_bytes;
        virtual ExtensionType getType() const = 0;
        virtual ~AbstractVolumeExtension();
    };
    
    struct AbstractHeader
    {
        enum HeaderType
        {
            NIFTI
        };
        virtual HeaderType getType() const = 0;
        virtual ~AbstractHeader();
    };
    
    class VolumeBase : public VolumeMappableInterface
    {
    protected:
        VolumeSpace m_volSpace;
        float* m_data;
        int64_t m_dataSize;
        int64_t m_dimensions[5];//store internally as 4d+component
        std::vector<int64_t> m_origDims;//but keep track of the original dimensions
        float*** m_indexRef;//some magic to avoid multiply during getVoxel/setVoxel
        int64_t* m_jMult;//some magic for fast getIndex/getValue/setValue
        int64_t* m_kMult;
        int64_t* m_bMult;
        int64_t* m_cMult;
        
        void freeIndexing();
        void setupIndexing();//sets up the magic
        bool m_ModifiedFlag;
        
        VolumeBase();
        VolumeBase(const std::vector<int64_t>& dimensionsIn, const std::vector<std::vector<float> >& indexToSpace, const int64_t numComponents = 1);
        //convenience method for unsigned
        VolumeBase(const std::vector<uint64_t>& dimensionsIn, const std::vector<std::vector<float> >& indexToSpace, const uint64_t numComponents = 1);
    public:
        void clear();
        virtual ~VolumeBase();

        ///there isn't much VolumeFile can do to restrict access to extensions, so just have them public
        std::vector<CaretPointer<AbstractVolumeExtension> > m_extensions;
        
        ///ditto for header
        CaretPointer<AbstractHeader> m_header;
        
        ///recreates the volume file storage with new size and spacing
        void reinitialize(const std::vector<int64_t>& dimensionsIn, const std::vector<std::vector<float> >& indexToSpace, const int64_t numComponents = 1);
        void reinitialize(const std::vector<uint64_t>& dimensionsIn, const std::vector<std::vector<float> >& indexToSpace, const uint64_t numComponents = 1);
        
        ///get the spacing info
        inline const std::vector<std::vector<float> >& getSform() const {
            return m_volSpace.getSform();
        }
        
        void setVolumeSpace(const std::vector<std::vector<float> >& indexToSpace);

        ///get the originally specified dimensions vector
        inline const std::vector<int64_t>& getOriginalDimensions() const {
            return m_origDims;
        }
        
        inline const int64_t& getNumberOfComponents() const {
            return m_dimensions[4];
        }
        
        ///translates extraspatial indices into a (flat) brick index
        int64_t getBrickIndexFromNonSpatialIndexes(const std::vector<int64_t>& extraInds) const;
        
        ///translates a (flat) brick index into the original extraspatial indices
        std::vector<int64_t> getNonSpatialIndexesFromBrickIndex(const int64_t& brickIndex) const;
        
        ///returns true if volume space is not skew, and each axis and index is separate
        bool isPlumb() const;

        ///returns orientation, spacing, and center (spacing/center can be negative, spacing/center is LPI rearranged to ijk (first dimension uses first element), will assert false if isOblique is true)
        void getOrientAndSpacingForPlumb(VolumeSpace::OrientTypes* orientOut, float* spacingOut, float* centerOut) const;
        
        ///get just orientation, even for non-plumb volumes
        void getOrientation(VolumeSpace::OrientTypes orientOut[3]) const;
        
        ///reorient this volume
        void reorient(const VolumeSpace::OrientTypes newOrient[3]);

        //not to worry, simple passthrough convenience functions like these get partially optimized to the main one by even -O1, and completely optimized together by -O2 or -O3

        ///returns coordinate triplet of an index triplet
        void indexToSpace(const int64_t* indexIn, float* coordOut) const;
        ///returns three coordinates of an index triplet
        void indexToSpace(const int64_t* indexIn, float& coordOut1, float& coordOut2, float& coordOut3) const;

        ///returns coordinate triplet of a floating point index triplet
        void indexToSpace(const float* indexIn, float* coordOut) const;
        ///returns coordinate triplet of three floating point indexes
        void indexToSpace(const float& indexIn1, const float& indexIn2, const float& indexIn3, float* coordOut) const;
        ///returns three coordinates of a floating point index triplet
        void indexToSpace(const float* indexIn, float& coordOut1, float& coordOut2, float& coordOut3) const;
        ///returns three coordinates of three floating point indexes
        void indexToSpace(const float& indexIn1, const float& indexIn2, const float& indexIn3, float& coordOut1, float& coordOut2, float& coordOut3) const;

        ///returns floating point index triplet of a given coordinate triplet
        void spaceToIndex(const float* coordIn, float* indexOut) const;
        ///returns floating point index triplet of three given coordinates
        void spaceToIndex(const float& coordIn1, const float& coordIn2, const float& coordIn3, float* indexOut) const;
        ///returns three floating point indexes of a given coordinate triplet
        void spaceToIndex(const float* coordIn, float& indexOut1, float& indexOut2, float& indexOut3) const;
        ///returns three floating point indexes of three given coordinates
        void spaceToIndex(const float& coordIn1, const float& coordIn2, const float& coordIn3, float& indexOut1, float& indexOut2, float& indexOut3) const;

        ///returns integer index triplet of voxel whose center is closest to the coordinate triplet
        void enclosingVoxel(const float* coordIn, int64_t* indexOut) const;
        ///returns integer index triplet of voxel whose center is closest to the three coordinates
        void enclosingVoxel(const float& coordIn1, const float& coordIn2, const float& coordIn3, int64_t* indexOut) const;
        ///returns integer indexes of voxel whose center is closest to the coordinate triplet
        void enclosingVoxel(const float* coordIn, int64_t& indexOut1, int64_t& indexOut2, int64_t& indexOut3) const;
        ///returns integer indexes of voxel whose center is closest to the three coordinates
        void enclosingVoxel(const float& coordIn1, const float& coordIn2, const float& coordIn3, int64_t& indexOut1, int64_t& indexOut2, int64_t& indexOut3) const;
        
        inline const VolumeSpace& getVolumeSpace() const { return m_volSpace; }

        ///get a value at an index triplet and optionally timepoint
        inline const float& getValue(const int64_t* indexIn, const int64_t brickIndex = 0, const int64_t component = 0) const
        {
            return getValue(indexIn[0], indexIn[1], indexIn[2], brickIndex, component);
        }
        
        ///get a value at three indexes and optionally timepoint
        inline const float& getValue(const int64_t& indexIn1, const int64_t& indexIn2, const int64_t& indexIn3, const int64_t brickIndex = 0, const int64_t component = 0) const
        {
            CaretAssert(indexValid(indexIn1, indexIn2, indexIn3, brickIndex, component));//assert so release version isn't slowed by checking
            if (m_indexRef != NULL)
            {
                return m_indexRef[component][brickIndex][indexIn1 + m_jMult[indexIn2] + m_kMult[indexIn3]];
            } else {
                return m_data[getIndex(indexIn1, indexIn2, indexIn3, brickIndex, component)];
            }
        }

        /**
         * Get the value of the voxel containing the given coordinate.
         *
         * @param coordinateIn
         *    The 3D coordinate
         * @param validOut
         *    If not NULL, will indicate if the coordinate (and hence the
         *    returned value) is valid.
         * @param mapIndex
         *    Index of map.
         * @param component
         *    Voxel component.
         * @return
         *    Value of voxel containing the given coordinate.
         */
        virtual inline float getVoxelValue(const float* coordinateIn,
                                    bool* validOut = NULL,
                                    const int64_t mapIndex = 0,
                                    const int64_t component = 0) const
        {
            return getVoxelValue(coordinateIn[0],
                                 coordinateIn[1],
                                 coordinateIn[2],
                                 validOut,
                                 mapIndex,
                                 component);
        }
        
        /**
         * Get the value of the voxel containing the given coordinate.
         *
         * @param coordinateX
         *    The X coordinate
         * @param coordinateY
         *    The Y coordinate
         * @param coordinateZ
         *    The Z coordinate
         * @param validOut
         *    If not NULL, will indicate if the coordinate (and hence the
         *    returned value) is valid.
         * @param mapIndex
         *    Index of map.
         * @param component
         *    Voxel component.
         * @return
         *    Value of voxel containing the given coordinate.
         */
        virtual inline float getVoxelValue(const float coordinateX,
                                    const float coordinateY,
                                    const float coordinateZ,
                                    bool* validOut = NULL,
                                    const int64_t mapIndex = 0,
                                    const int64_t component = 0) const
        {
            if (validOut != NULL) {
                *validOut = false;
            }
            
            int64_t voxelI, voxelJ, voxelK;
            enclosingVoxel(coordinateX,
                           coordinateY,
                           coordinateZ,
                           voxelI,
                           voxelJ,
                           voxelK);
            if (indexValid(voxelI,
                           voxelJ,
                           voxelK,
                           mapIndex,
                           component)) {
                if (validOut != NULL) {
                    *validOut = true;
                }
                return getValue(voxelI, voxelJ, voxelK, mapIndex, component);
            }
            
            return 0.0;
        }
        
        ///get a frame (const)
        const float* getFrame(const int64_t brickIndex = 0, const int64_t component = 0) const;
        
        ///set a value at an index triplet and optionally timepoint
        inline void setValue(const float& valueIn, const int64_t* indexIn, const int64_t brickIndex = 0, const int64_t component = 0)
        {
            setValue(valueIn, indexIn[0], indexIn[1], indexIn[2], brickIndex, component);
        }
        
        ///set a value at an index triplet and optionally timepoint
        inline void setValue(const float& valueIn, const int64_t& indexIn1, const int64_t& indexIn2, const int64_t& indexIn3, const int64_t brickIndex = 0, const int64_t component = 0)
        {
            CaretAssert(indexValid(indexIn1, indexIn2, indexIn3, brickIndex, component));//assert so release version isn't slowed by checking
            if (m_indexRef != NULL)
            {
                m_indexRef[component][brickIndex][indexIn1 + m_jMult[indexIn2] + m_kMult[indexIn3]] = valueIn;
            } else {
                m_data[getIndex(indexIn1, indexIn2, indexIn3, brickIndex, component)] = valueIn;
            }
            setModified();
        }
        
        /// set every voxel to the given value
        void setValueAllVoxels(const float value);
        
        ///set a frame
        void setFrame(const float* frameIn, const int64_t brickIndex = 0, const int64_t component = 0);

        ///gets dimensions as a vector of 5 integers, 3 spatial, time, components
        void getDimensions(std::vector<int64_t>& dimOut) const;
        ///gets dimensions
        void getDimensions(int64_t& dimOut1, int64_t& dimOut2, int64_t& dimOut3, int64_t& dimTimeOut, int64_t& numComponents) const;
        ///gets dimensions
        std::vector<int64_t> getDimensions() const;

        ///gets index into data array for three indexes plus time index
        inline int64_t getIndex(const int64_t& indexIn1, const int64_t& indexIn2, const int64_t& indexIn3, const int64_t brickIndex = 0, const int64_t component = 0) const
        {
            CaretAssert(indexValid(indexIn1, indexIn2, indexIn3, brickIndex, component));
            if (m_indexRef != NULL)
            {
                //TRICK: use pointer math and the indexing arrays to get the index if it is set up
                return (m_indexRef[component][brickIndex] + indexIn1 + m_jMult[indexIn2] + m_kMult[indexIn3]) - m_data;
            } else {//otherwise, calculate via precalculated multiply arrays
                return indexIn1 + m_jMult[indexIn2] + m_kMult[indexIn3] + m_bMult[brickIndex] + m_cMult[component];
            }
        }
        
        inline int64_t getIndex(const int64_t* indexIn, const int64_t brickIndex = 0, const int64_t component = 0) const
        {
            return getIndex(indexIn[0], indexIn[1], indexIn[2], brickIndex, component);
        }

        inline bool indexValid(const int64_t* indexIn, const int64_t brickIndex = 0, const int64_t component = 0) const
        {
            return indexValid(indexIn[0], indexIn[1], indexIn[2], brickIndex, component);
        }

        ///checks if an index is within array dimensions
        inline bool indexValid(const int64_t& indexIn1, const int64_t& indexIn2, const int64_t& indexIn3, const int64_t brickIndex = 0, const int64_t component = 0) const
        {//inlined so that getValue and setValue can get optimized out entirely
            if (indexIn1 < 0 || indexIn1 >= m_dimensions[0]) return false;
            if (indexIn2 < 0 || indexIn2 >= m_dimensions[1]) return false;
            if (indexIn3 < 0 || indexIn3 >= m_dimensions[2]) return false;
            if (brickIndex < 0 || brickIndex >= m_dimensions[3]) return false;
            if (component < 0 || component >= m_dimensions[4]) return false;
            return true;
        }

        virtual void setModified();//virtual because we need the functions that change voxels in this class to call the setModified in VolumeFile if it really is a VolumeFile (which it always is)
        virtual void clearModified();
        virtual bool isModified() const;
        
        bool isEmpty() const;
        
    };

}

#endif //__VOLUME_BASE_H__
