#ifndef __BORDER_FILE__H_
#define __BORDER_FILE__H_

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


#include "CaretDataFile.h"
#include "DisplayGroupEnum.h"

namespace caret {

    class Border;
    class BorderPointFromSearch;
    class GroupAndNameHierarchyModel;
    class GiftiLabelTable;
    class SurfaceFile;
    class SurfaceProjectedItem;
    
    class BorderFile : public CaretDataFile {
        
    public:
        BorderFile();
        
        virtual ~BorderFile();
        
        BorderFile(const BorderFile& obj);

        BorderFile& operator=(const BorderFile& obj);
        
        virtual void addToDataFileContentInformation(DataFileContentInformation& dataFileInformation);
        
        StructureEnum::Enum getStructure() const;
        
        void setStructure(const StructureEnum::Enum structure);
        
        GiftiMetaData* getFileMetaData();
        
        const GiftiMetaData* getFileMetaData() const;
        
        void readFile(const AString& filename) throw (DataFileException);
        
        void writeFile(const AString& filename) throw (DataFileException);
        
        void clear();
        
        bool isEmpty() const;
        
        int32_t getNumberOfBorders() const;
        
        Border* getBorder(const int32_t indx);
        
        const Border* getBorder(const int32_t indx) const;
        
        void findAllBordersWithEndPointNearSegmentFirstPoint(const DisplayGroupEnum::Enum displayGroup,
                                               const int32_t browserTabIndex,
                                               const SurfaceFile* surfaceFile,
                                               const Border* borderSegment,
                                               const float maximumDistance,
                                               std::vector<BorderPointFromSearch>& borderPointsOut) const;
        
        
        void findAllBordersWithPointsNearBothSegmentEndPoints(const DisplayGroupEnum::Enum displayGroup,
                                                 const int32_t browserTabIndex,
                                                 const SurfaceFile* surfaceFile,
                                                 const Border* borderSegment,
                                                 const float maximumDistance,
                                                 std::vector<BorderPointFromSearch>& borderPointsOut) const;
        
        void addBorder(Border* border);
        
        void removeBorder(const int32_t indx);
        
        void removeBorder(Border* border);
        
        bool isBorderDisplayed(const DisplayGroupEnum::Enum displayGroup,
                               const int32_t browserTabIndex,
                               const Border* border);
        
        GiftiLabelTable* getClassColorTable();
        
        const GiftiLabelTable* getClassColorTable() const;
        
        GiftiLabelTable* getNameColorTable();
        
        const GiftiLabelTable* getNameColorTable() const;
        
        void createNameAndClassColorTables(const GiftiLabelTable* oldColorTable);
        
        GroupAndNameHierarchyModel* getGroupAndNameHierarchyModel();
        
        const GroupAndNameHierarchyModel* getGroupAndNameHierarchyModel() const;
        
        QStringList getAllBorderNamesSorted() const;
        
        static int32_t getFileVersion();
        
        static AString getFileVersionAsString();
        
        void exportToCaret5Format(const std::vector<SurfaceFile*>& surfaceFiles,
                                  const AString& outputCaret5FilesPrefix) throw (DataFileException);
        
        /** XML Tag for BorderFile element */
        static const AString XML_TAG_BORDER_FILE;
        
        /** XML Tag for Version attribute */
        static const AString XML_ATTRIBUTE_VERSION;
        
        
        /** XML Tag for Name Color Table */
        static const AString XML_TAG_NAME_COLOR_TABLE;
        
        /** XML Tag for Class Color Table */
        static const AString XML_TAG_CLASS_COLOR_TABLE;
        
        virtual bool isModified() const;
        
        virtual void clearModified();
        
        void invalidateAllAssignedColors();
        
    private:
        void copyHelperBorderFile(const BorderFile& obj);
        
        void initializeBorderFile();
        
        GiftiMetaData* m_metadata;
        
        std::vector<Border*> m_borders;
        
        /** Holds colors assigned to classes */
        GiftiLabelTable* m_classColorTable;
        
        /** Holds colors assigned to names */
        GiftiLabelTable* m_nameColorTable;
        
        /** Holds class and name hierarchy used for display selection */
        mutable GroupAndNameHierarchyModel* m_classNameHierarchy;
        
        /** force an update of the class and name hierarchy */
        bool m_forceUpdateOfGroupAndNameHierarchy;
        
        /** Version of this BorderFile */
        static const int32_t s_borderFileVersion;
    };
    
#ifdef __BORDER_FILE_DECLARE__
    const AString BorderFile::XML_TAG_BORDER_FILE = "BorderFile";
    const AString BorderFile::XML_ATTRIBUTE_VERSION = "Version";
    const AString BorderFile::XML_TAG_NAME_COLOR_TABLE = "BorderNameColorTable";
    const AString BorderFile::XML_TAG_CLASS_COLOR_TABLE = "BorderClassColorTable";
    
    const int32_t BorderFile::s_borderFileVersion = 2;
#endif // __BORDER_FILE_DECLARE__

} // namespace
#endif  //__BORDER_FILE__H_
