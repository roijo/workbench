
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


#include <memory>

#include <QTextStream>

#define __FOCI_FILE_DECLARE__
#include "FociFile.h"
#undef __FOCI_FILE_DECLARE__

#include "CaretAssert.h"
#include "CaretLogger.h"
#include "GroupAndNameHierarchyModel.h"
#include "FileAdapter.h"
#include "FociFileSaxReader.h"
#include "Focus.h"
#include "GiftiLabel.h"
#include "GiftiLabelTable.h"
#include "GiftiMetaData.h"
#include "SurfaceProjectedItem.h"
#include "XmlAttributes.h"
#include "XmlSaxParser.h"
#include "XmlWriter.h"

using namespace caret;


    
/**
 * \class caret::FociFile 
 * \brief A foci (plural of focus) file.
 */

/**
 * Constructor.
 */
FociFile::FociFile()
: CaretDataFile(DataFileTypeEnum::FOCI)
{
    initializeFociFile();
}

/**
 * Destructor.
 */
FociFile::~FociFile()
{
    delete m_nameColorTable;
    delete m_classColorTable;
    delete m_metadata;
    
    for (std::vector<Focus*>::iterator iter = m_foci.begin();
         iter != m_foci.end();
         iter++) {
        delete *iter;
    }
    m_foci.clear();
    
    delete m_classNameHierarchy;
}

/**
 * Copy constructor.
 * @param obj
 *    Object that is copied.
 */
FociFile::FociFile(const FociFile& obj)
: CaretDataFile(obj)
{
    initializeFociFile();
    copyHelperFociFile(obj);
}

/**
 * Assignment operator.
 * @param obj
 *    Data copied from obj to this.
 * @return 
 *    Reference to this object.
 */
FociFile&
FociFile::operator=(const FociFile& obj)
{
    if (this != &obj) {
        clear();
        CaretDataFile::operator=(obj);
        copyHelperFociFile(obj);
    }
    return *this;    
}

void 
FociFile::initializeFociFile()
{
    m_classColorTable = new GiftiLabelTable();
    m_nameColorTable  = new GiftiLabelTable();
    m_classNameHierarchy = new GroupAndNameHierarchyModel();
    m_metadata = new GiftiMetaData();
    m_forceUpdateOfGroupAndNameHierarchy = true;
}


/**
 * Helps with copying an object of this type.
 * @param ff
 *    Object that is copied.
 */
void 
FociFile::copyHelperFociFile(const FociFile& ff)
{
    *m_classColorTable = *ff.m_classColorTable;
    *m_nameColorTable  = *ff.m_nameColorTable;
    if (m_classNameHierarchy != NULL) {
        delete m_classNameHierarchy;
    }
    m_classNameHierarchy = new GroupAndNameHierarchyModel();
    
    *m_metadata = *ff.m_metadata;
    
    const int32_t numFoci = getNumberOfFoci();
    for (int32_t i = 0; i < numFoci; i++) {
        m_foci.push_back(new Focus(*ff.getFocus(i)));
    }
    
    m_forceUpdateOfGroupAndNameHierarchy = true;
    
    setModified();
}

/**
 * @return Is this foci file empty (contains zero focuss)?
 */
bool 
FociFile::isEmpty() const
{
    return m_foci.empty();
}

/**
 * @return The structure for this file.
 */
StructureEnum::Enum 
FociFile::getStructure() const
{
    return StructureEnum::ALL;
}

/**
 * Set the structure for this file.
 * @param structure
 *   New structure for this file.
 */
void 
FociFile::setStructure(const StructureEnum::Enum /*structure*/)
{
    // does nothing since focuss apply to all structures
}

/**
 * @return Get access to the file's metadata.
 */
GiftiMetaData* 
FociFile::getFileMetaData()
{
    return m_metadata;
}

/**
 * @return Get access to unmodifiable file's metadata.
 */
const GiftiMetaData* 
FociFile::getFileMetaData() const
{
    return m_metadata;
}

/**
 * Clear the focus file.
 */
void 
FociFile::clear()
{
    CaretDataFile::clear();
    m_classNameHierarchy->clear();
    m_classColorTable->clear();
    m_nameColorTable->clear();
    m_metadata->clear();
    const int32_t numFoci = getNumberOfFoci();
    for (int32_t i = 0; i < numFoci; i++) {
        delete m_foci[i];
    }
    m_foci.clear();
}

/**
 * @return the number of foci.
 */
int32_t 
FociFile::getNumberOfFoci() const
{
    return m_foci.size();
}

/**
 * Get the focus at the given index.
 * @param indx
 *   Index of the focus.
 * @return
 *   focus at the given index.
 */
Focus* 
FociFile::getFocus(const int32_t indx)
{
    CaretAssertVectorIndex(m_foci, indx);
    return m_foci[indx];
}

/**
 * Get the focus at the given index.
 * @param indx
 *   Index of the focus.
 * @return
 *   focus at the given index.
 */
const Focus* 
FociFile::getFocus(const int32_t indx) const
{
    CaretAssertVectorIndex(m_foci, indx);
    return m_foci[indx];
}

/**
 * Add a focus.  NOTE: This focus file
 * takes ownership of the 'focus' and 
 * will handle deleting it.  After calling 
 * this method, the caller must never
 * do anything with the focus that was passed
 * to this method.
 *
 * @param focus
 *    Focus added to this focus file.
 */
void 
FociFile::addFocus(Focus* focus)
{
    m_foci.push_back(focus);
    const AString name = focus->getName();
    if (name.isEmpty() == false) {
        const int32_t nameColorKey = m_nameColorTable->getLabelKeyFromName(name);
        if (nameColorKey < 0) {
            m_nameColorTable->addLabel(name, 0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    AString className = focus->getClassName();
    if (className.isEmpty() == false) {
        const int32_t classColorKey = m_classColorTable->getLabelKeyFromName(className);
        if (classColorKey < 0) {
            m_classColorTable->addLabel(className, 0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    m_forceUpdateOfGroupAndNameHierarchy = true;
    setModified();
}

/**
 * Remove the focus at the given index.
 * @param indx
 *   Index of focus for removal.
 */
void 
FociFile::removeFocus(const int32_t indx)
{
    CaretAssertVectorIndex(m_foci, indx);
    Focus* focus = getFocus(indx);
    m_foci.erase(m_foci.begin() + indx);
    delete focus;
    m_forceUpdateOfGroupAndNameHierarchy = true;
    setModified();
}

/**
 * Remove the focus.
 * @param focus
 *   Focus that will be removed and DELETED.
 */
void 
FociFile::removeFocus(Focus* focus)
{
    const int32_t numFoci = getNumberOfFoci();
    for (int32_t i = 0;i < numFoci; i++) {
        if (m_foci[i] == focus) {
            removeFocus(i);
            return;
        }
    }
    CaretLogWarning("Attempting to delete focus not in focus file with name: "
                    + focus->getName());
}

/**
 * @return The class and name hierarchy.
 */
GroupAndNameHierarchyModel* 
FociFile::getGroupAndNameHierarchyModel()
{
    m_classNameHierarchy->update(this,
                                     m_forceUpdateOfGroupAndNameHierarchy);
    m_forceUpdateOfGroupAndNameHierarchy = false;
    
    return m_classNameHierarchy;
}

/**
 * @return  The class color table.
 */
GiftiLabelTable* 
FociFile::getClassColorTable()
{
    return m_classColorTable;
}

/**
 * @return  The class color table.
 */
const GiftiLabelTable* 
FociFile::getClassColorTable() const
{
    return m_classColorTable;
}

/**
 * @return  The name color table.
 */
GiftiLabelTable*
FociFile::getNameColorTable()
{
    return m_nameColorTable;
}

/**
 * @return  The name color table.
 */
const GiftiLabelTable*
FociFile::getNameColorTable() const
{
    return m_nameColorTable;
}

/**
 * Version 1 foci files contained one color table for both names
 * and classes.  Newer versions of the foci file keep them in
 * separate tables.
 *
 * @param oldColorTable
 *    Old color table that is split into name and class color tables.
 */
void
FociFile::createNameAndClassColorTables(const GiftiLabelTable* oldColorTable)
{
    CaretAssert(oldColorTable);
    
    m_classColorTable->clear();
    m_nameColorTable->clear();
    
    std::set<QString> nameSet;
    std::set<QString> classSet;
    
    const int numFoci = getNumberOfFoci();
    for (int32_t i = 0; i < numFoci; i++) {
        const Focus* focus = getFocus(i);
        nameSet.insert(focus->getName());
        classSet.insert(focus->getClassName());
    }
    
    for (std::set<QString>::iterator iter = nameSet.begin();
         iter != nameSet.end();
         iter++) {
        const AString colorName = *iter;
        const GiftiLabel* label = oldColorTable->getLabelBestMatching(colorName);
        float rgba[4] = { 0.0, 0.0, 0.0, 1.0 };
        if (label != NULL) {
            label->getColor(rgba);
        }
        m_nameColorTable->addLabel(colorName,
                                   rgba[0],
                                   rgba[1],
                                   rgba[2],
                                   rgba[3]);
    }
    
    for (std::set<QString>::iterator iter = classSet.begin();
         iter != classSet.end();
         iter++) {
        const AString colorName = *iter;
        const GiftiLabel* label = oldColorTable->getLabelBestMatching(colorName);
        float rgba[4] = { 0.0, 0.0, 0.0, 1.0 };
        if (label != NULL) {
            label->getColor(rgba);
        }
        m_classColorTable->addLabel(colorName,
                                   rgba[0],
                                   rgba[1],
                                   rgba[2],
                                   rgba[3]);
    }
}

/**
 * @return The version of the file as a number.
 */
int32_t
FociFile::getFileVersion()
{
    return FociFile::s_fociFileVersion;
}

/**
 * @return The version of the file as a string.
 */
AString 
FociFile::getFileVersionAsString()
{
    return AString::number(FociFile::getFileVersion());
}

/**
 * Invalidate all assigned colors.
 */
void
FociFile::invalidateAllAssignedColors()
{
    const int32_t numFoci = getNumberOfFoci();
    for (int32_t i = 0;i < numFoci; i++) {
        m_foci[i]->setNameRgbaInvalid();
        m_foci[i]->setClassRgbaInvalid();
    }
    m_forceUpdateOfGroupAndNameHierarchy = true;
}

/**
 * Read the data file.
 *
 * @param filename
 *    Name of the data file.
 * @throws DataFileException
 *    If the file was not successfully read.
 */
void 
FociFile::readFile(const AString& filename) throw (DataFileException)
{
    FociFileSaxReader saxReader(this);
    std::auto_ptr<XmlSaxParser> parser(XmlSaxParser::createXmlParser());
    try {
        parser->parseFile(filename, &saxReader);
    }
    catch (const XmlSaxParserException& e) {
        setFileName("");
        
        int lineNum = e.getLineNumber();
        int colNum  = e.getColumnNumber();
        
        AString msg =
        "Parse Error while reading "
        + filename;
        
        if ((lineNum >= 0) && (colNum >= 0)) {
            msg += (" line/col ("
                    + AString::number(e.getLineNumber())
                    + "/"
                    + AString::number(e.getColumnNumber())
                    + ")");
        }
        
        msg += (": " + e.whatString());
        
        DataFileException dfe(msg);
        CaretLogThrowing(dfe);
        throw dfe;
    }
    
    setFileName(filename);
    
    m_classNameHierarchy->update(this,
                                     true);
    m_forceUpdateOfGroupAndNameHierarchy = false;
    m_classNameHierarchy->setAllSelected(true);
    
    CaretLogFiner("CLASS/NAME Table for : "
                  + getFileNameNoPath()
                  + "\n"
                  + m_classNameHierarchy->toString());
    
    clearModified();
}

/**
 * Write the data file.
 *
 * @param filename
 *    Name of the data file.
 * @throws DataFileException
 *    If the file was not successfully written.
 */
void 
FociFile::writeFile(const AString& filename) throw (DataFileException)
{
    setFileName(filename);
    
    try {
        //
        // Format the version string so that it ends with at most one zero
        //
        const AString versionString = FociFile::getFileVersionAsString();
        
        //
        // Open the file
        //
        FileAdapter file;
        AString errorMessage;
        QTextStream* textStream = file.openQTextStreamForWritingFile(getFileName(),
                                                                     errorMessage);
        if (textStream == NULL) {
            throw DataFileException(errorMessage);
        }
        
        //
        // Create the xml writer
        //
        XmlWriter xmlWriter(*textStream);
        
        //
        // Write header info
        //
        xmlWriter.writeStartDocument("1.0");
        
        //
        // Write GIFTI root element
        //
        XmlAttributes attributes;
        
        //attributes.addAttribute("xmlns:xsi",
        //                        "http://www.w3.org/2001/XMLSchema-instance");
        //attributes.addAttribute("xsi:noNamespaceSchemaLocation",
        //                        "http://brainvis.wustl.edu/caret6/xml_schemas/GIFTI_Caret.xsd");
        attributes.addAttribute(FociFile::XML_ATTRIBUTE_VERSION,
                                versionString);
        xmlWriter.writeStartElement(FociFile::XML_TAG_FOCI_FILE,
                                    attributes);
        
        //
        // Write Metadata
        //
        if (m_metadata != NULL) {
            m_metadata->writeAsXML(xmlWriter);
        }
        
        //
        // Write the class color table
        //
        xmlWriter.writeStartElement(XML_TAG_CLASS_COLOR_TABLE);
        m_classColorTable->writeAsXML(xmlWriter);
        xmlWriter.writeEndElement();
        
        //
        // Write the name color table
        //
        xmlWriter.writeStartElement(XML_TAG_NAME_COLOR_TABLE);
        m_nameColorTable->writeAsXML(xmlWriter);
        xmlWriter.writeEndElement();
        
        //
        // Write foci
        //
        const int32_t numFoci = getNumberOfFoci();
        for (int32_t i = 0; i < numFoci; i++) {
            m_foci[i]->writeAsXML(xmlWriter, i);
        }
        
        xmlWriter.writeEndElement();
        xmlWriter.writeEndDocument();
        
        file.close();
        
        clearModified();
    }
    catch (const GiftiException& e) {
        throw DataFileException(e);
    }
    catch (const XmlException& e) {
        throw DataFileException(e);
    }
}

/**
 * @return Is this foci file modified?
 */
bool 
FociFile::isModified() const
{
    if (CaretDataFile::isModified()) {
        return true;
    }
    if (m_metadata->isModified()) {
        return true;
    }
    if (m_classColorTable->isModified()) {
        return true;
    }
    if (m_nameColorTable->isModified()) {
        return true;
    }
    
    /* 
     * Note, these members do not affect modification status:
     * classNameHierarchy 
     */
    
    const int32_t numFoci = getNumberOfFoci();
    for (int32_t i = 0; i < numFoci; i++) {
        if (m_foci[i]->isModified()) {
            return true;
        }
    }
    
    return false;
}

/**
 * Clear the modification status of this foci file.
 */
void 
FociFile::clearModified()
{
    CaretDataFile::clearModified();
    
    m_metadata->clearModified();
    
    m_classColorTable->clearModified();
    m_nameColorTable->clearModified();
    
    const int32_t numFoci = getNumberOfFoci();
    for (int32_t i = 0; i < numFoci; i++) {
        m_foci[i]->clearModified();
    }
}

