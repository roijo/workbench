#ifndef __FIBER_ORIENTATION_TRAJECTORY__H_
#define __FIBER_ORIENTATION_TRAJECTORY__H_

/*LICENSE_START*/
/*
 * Copyright 2012 Washington University,
 * All rights reserved.
 *
 * Connectome DB and Connectome Workbench are part of the integrated Connectome 
 * Informatics Platform.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of Washington University nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR  
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*LICENSE_END*/

#include <vector>
#include <stdint.h>

#include "CaretAssert.h"

namespace caret {

    struct FiberFractions;
    class FiberOrientation;
    
    class FiberOrientationTrajectory {
        
    public:
        FiberOrientationTrajectory(const int64_t fiberOrientationIndex,
                                   const FiberOrientation* fiberOrientation);
        
        virtual ~FiberOrientationTrajectory();
        
        void addFiberFractions(const FiberFractions& fiberFraction);
        
        /**
         * @return the Fiber Orientation.
         */
        inline const FiberOrientation* getFiberOrientation() const {
            return m_fiberOrientation;
        }
        
        /**
         * Get the fiber fraction.
         *
         * @param indx
         *    Index of the fiber fraction.
         * @return 
         *    Fiber fraction at the given index.
         */
        inline const FiberFractions* getFiberFraction() const {
            return m_fiberFraction;
        }
        
        /**
         * @return The fiber orientation index.
         */
        inline int64_t getFiberOrientationIndex() const {
            return m_fiberOrientationIndex;
        }
        
        void finish();
        
    private:
        FiberOrientationTrajectory(const FiberOrientationTrajectory&);

        FiberOrientationTrajectory& operator=(const FiberOrientationTrajectory&);
        
    public:

        // ADD_NEW_METHODS_HERE

    private:
        const int64_t m_fiberOrientationIndex;
        
        const FiberOrientation* m_fiberOrientation;
        
        FiberFractions* m_fiberFraction;
        
        double m_totalCountSum;
        std::vector<double> m_fiberCountsSum;
        double m_distanceSum;
        int64_t m_countForAveraging;
        
        // ADD_NEW_MEMBERS_HERE

        friend class CiftiFiberTrajectoryFile;
    };
    
#ifdef __FIBER_ORIENTATION_TRAJECTORY_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __FIBER_ORIENTATION_TRAJECTORY_DECLARE__

} // namespace
#endif  //__FIBER_ORIENTATION_TRAJECTORY__H_
