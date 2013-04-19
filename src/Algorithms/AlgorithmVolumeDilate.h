#ifndef __ALGORITHM_VOLUME_DILATE_H__
#define __ALGORITHM_VOLUME_DILATE_H__

/*LICENSE_START*/
/*
 *  Copyright 1995-2002 Washington University School of Medicine
 *
 *  http://brainmap.wustl.edu
 *
 *  This file is part of CARET.
 *
 *  CARET is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CARET is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CARET; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "AbstractAlgorithm.h"

namespace caret {
    
    class AlgorithmVolumeDilate : public AbstractAlgorithm
    {
        AlgorithmVolumeDilate();
        struct VoxTriple
        {
            int m_ijk[3];
            VoxTriple(int ijk[3])
            {
                m_ijk[0] = ijk[0];
                m_ijk[1] = ijk[1];
                m_ijk[2] = ijk[2];
            }
        };
    protected:
        static float getSubAlgorithmWeight();
        static float getAlgorithmInternalWeight();
    public:
        enum Method
        {
            NEAREST,
            WEIGHTED
        };
        AlgorithmVolumeDilate(ProgressObject* myProgObj, const VolumeFile* volIn, const float& distance, const Method& myMethod,
                              VolumeFile* volOut, const VolumeFile* badRoi, const int& subvol = -1);
        static OperationParameters* getParameters();
        static void useParameters(OperationParameters* myParams, ProgressObject* myProgObj);
        static AString getCommandSwitch();
        static AString getShortDescription();
    private:
        void dilateFrame(const VolumeFile* volIn, const int& insubvol, const int& component, VolumeFile* volOut, const int& outsubvol, const VolumeFile* badRoi,
                         const Method& myMethod, const std::vector<int>& stencil, const std::vector<float>& stenWeights);
    };

    typedef TemplateAutoOperation<AlgorithmVolumeDilate> AutoAlgorithmVolumeDilate;

}

#endif //__ALGORITHM_VOLUME_DILATE_H__
