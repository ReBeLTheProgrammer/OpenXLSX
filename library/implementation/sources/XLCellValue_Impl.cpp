//
// Created by KBA012 on 16-12-2017.
//

#include <cassert>
#include <pugixml.hpp>

#include "XLCellValue_Impl.hpp"
#include "XLCell_Impl.hpp"
#include "XLWorksheet_Impl.hpp"

using namespace OpenXLSX;
using namespace std;

/**
 * @details The constructor sets the m_parentCell to the value of the input parameter. The m_parentCell member variable
 * is a reference to the parent XLCell object, in order to avoid null objects.
 * @pre The parent input parameter is a valid XLCell object.
 * @post A valid XLCellValue object has been constructed.
 */
Impl::XLCellValue::XLCellValue(const XLCell& parent) noexcept
        : m_cellNode(parent.CellNode()),
          m_sharedStrings(parent.Worksheet()->ParentDoc().Workbook()->SharedStrings()) {
}

/**
 * @details The copy constructor and copy assignment operator works differently for XLCellValue objects.
 * The copy constructor creates an exact copy of the object, with the same parent XLCell object. The copy
 * assignment operator only copies the underlying cell value and type attribute to the target object.
 * @pre Both the lhs and rhs are valid objects.
 * @post Successful assignment to the target object.
 */
Impl::XLCellValue& Impl::XLCellValue::operator=(const Impl::XLCellValue& other) {

    if (&other != this) {

        if (!m_cellNode.attribute("t")) m_cellNode.append_attribute("t");
        if (!m_cellNode.child("v")) m_cellNode.append_child("v");

        m_cellNode.child("v").set_value(!other.m_cellNode.child("v") ? "" : other.m_cellNode.child("v").text().get());
        m_cellNode.child("v").attribute("xml:space").set_value(other.m_cellNode.child("v").attribute("xml:space").value());
        m_cellNode.attribute("t").set_value(!other.m_cellNode.attribute("t") ? "" : other.m_cellNode.attribute("t").value());
    }

    return *this;
}

/**
 * @brief
 * @param other
 * @return
 * @todo Currently, the move constructor is identical to the copy constructor. Ensure that this is the correct behaviour.
 */
Impl::XLCellValue& Impl::XLCellValue::operator=(Impl::XLCellValue&& other) noexcept {
    if (&other != this) {

        if (!m_cellNode.attribute("t")) m_cellNode.append_attribute("t");
        if (!m_cellNode.child("v")) m_cellNode.append_child("v");

        m_cellNode.child("v").set_value(!other.m_cellNode.child("v") ? "" : other.m_cellNode.child("v").text().get());
        m_cellNode.child("v").attribute("xml:space").set_value(other.m_cellNode.child("v").attribute("xml:space").value());
        m_cellNode.attribute("t").set_value(!other.m_cellNode.attribute("t") ? "" : other.m_cellNode.attribute("t").value());
    }

    return *this;
}

/**
 * @details The assignment operator taking a std::string object as a parameter, calls the corresponding Set method
 * and returns the resulting object.
 * @pre The XLCellValue object and the stringValue parameter are both valid.
 * @post The underlying cell xml data has been set to the value of the input parameter and the type attribute has been
 * set to 'str'
 */
Impl::XLCellValue& Impl::XLCellValue::operator=(const std::string& stringValue) {

    Set(stringValue);
    return *this;
}

/**
 * @details The assignment operator taking a char* string as a parameter, calls the corresponding Set method
 * and returns the resulting object.
 * @pre The XLCellValue object and the stringValue parameter are both valid.
 * @post The underlying cell xml data has been set to the value of the input parameter and the type attribute has been
 * set to 'str'
 */
Impl::XLCellValue& Impl::XLCellValue::operator=(const char* stringValue) {

    Set(stringValue);
    return *this;
}

/**
 * @details If the value type is already a String, the value will be set to the new value. Otherwise, the m_value
 * member variable will be set to an XLString object with the given value.
 * @pre The XLCellValue object and the stringValue parameter are both valid.
 * @post The underlying cell xml data has been set to the value of the input parameter and the type attribute has been
 * set to 'str'
 */
void Impl::XLCellValue::Set(const string& stringValue) {

    Set(stringValue.c_str());
}

/**
 * @details Converts the char* parameter to a std::string and calls the corresponding Set method.
 * @pre The XLCellValue object and the stringValue parameter are both valid.
 * @post The underlying cell xml data has been set to the value of the input parameter and the type attribute has been
 * set to 'str'
 */
void Impl::XLCellValue::Set(const char* stringValue) {

    if (!m_cellNode.attribute("t")) m_cellNode.append_attribute("t");
    if (!m_cellNode.child("v")) m_cellNode.append_child("v");

    m_cellNode.attribute("t").set_value("str");
    m_cellNode.child("v").text().set(stringValue);
    m_cellNode.attribute("xml:space").set_value("preserve");
}

/**
 * @details Deletes the value node and type attribute if they exists.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The value node and the type attribute in the underlying xml data has been deleted.
 */
void Impl::XLCellValue::Clear() {

    m_cellNode.remove_child(m_cellNode.child("v"));
    m_cellNode.remove_attribute(m_cellNode.attribute("t"));
}

/**
 * @details Return the cell value as a string, by calling the AsString method of the m_value member variable.
 * @pre The m_value member variable is valid.
 * @post The current object, and any associated objects, are unchanged.
 */
std::string Impl::XLCellValue::AsString() const {

    if (strcmp(m_cellNode.attribute("t").value(), "b") == 0) {
        if (strcmp(m_cellNode.child("v").text().get(), "0") == 0)
            return "FALSE";

        return "TRUE";
    }

    if (strcmp(m_cellNode.attribute("t").value(), "s") == 0)
        return m_sharedStrings->GetString(m_cellNode.child("v").text().as_ullong());

    return m_cellNode.child("v").text().get();
}

/**
 * @details Determine the value type, based on the cell type, and return the corresponding XLValueType object.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
Impl::XLValueType Impl::XLCellValue::ValueType() const {

    switch (CellType()) {
        case XLCellType::Empty:
            return XLValueType::Empty;

        case XLCellType::Error:
            return XLValueType::Error;

        case XLCellType::Number: {
            if (DetermineNumberType(m_cellNode.child("v").text().get()) == XLNumberType::Integer)
                return XLValueType::Integer;

            return XLValueType::Float;
        }

        case XLCellType::Boolean:
            return XLValueType::Boolean;

        default:
            return XLValueType::String;
    }
}

/**
 * @details Determine the cell type, based on the contents of the underlying XML file, and returns the corresponding
 * XLCellType object.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
Impl::XLCellType Impl::XLCellValue::CellType() const {

    // ===== If neither a Type attribute or a value node is present, the cell is empty.
    if (!m_cellNode.attribute("t") && !m_cellNode.child("v")) {
        return XLCellType::Empty;
    }

    // ===== If a Type attribute is not present, but a value node is, the cell contains a number.
    if ((!m_cellNode.attribute("t") || *m_cellNode.attribute("t").value() == 'n') && m_cellNode.child("v") != nullptr) {
        return XLCellType::Number;
    }

    // ===== If the cell is of type "s", the cell contains a shared string.
    if (m_cellNode.attribute("t") != nullptr && *m_cellNode.attribute("t").value() == 's') {
        return XLCellType::String;
    }

    // ===== If the cell is of type "inlineStr", the cell contains an inline string.
    if (m_cellNode.attribute("t") != nullptr && strcmp(m_cellNode.attribute("t").value(), "inlineStr") == 0) {
        return XLCellType::String;
    }

    // ===== If the cell is of type "str", the cell contains an ordinary string.
    if (m_cellNode.attribute("t") != nullptr && strcmp(m_cellNode.attribute("t").value(), "str") == 0) {
        return XLCellType::String;
    }

    // ===== If the cell is of type "b", the cell contains a boolean.
    if (m_cellNode.attribute("t") != nullptr && *m_cellNode.attribute("t").value() == 'b') {
        return XLCellType::Boolean;
    }

    // ===== Otherwise, the cell contains an error.
    return XLCellType::Error; //the m_typeAttribute has the ValueAsString "e"
}

/**
 * @details The number type (integer or floating point) is determined simply by identifying whether or not a decimal
 * point is present in the input string. If present, the number type is floating point.
 */
Impl::XLNumberType Impl::XLCellValue::DetermineNumberType(const string& numberString) const {

    if (numberString.find('.') != string::npos || numberString.find("E-") != string::npos || numberString.find("e-")
    != string::npos)
        return XLNumberType::Float;

    return XLNumberType::Integer;
}

void Impl::XLCellValue::SetInteger(int64_t numberValue) {

    if (!m_cellNode.child("v")) m_cellNode.append_child("v");

    m_cellNode.remove_attribute("t");
    m_cellNode.child("v").text().set(numberValue);
    m_cellNode.child("v").attribute("xml:space").set_value("default");
}

void Impl::XLCellValue::SetBoolean(bool numberValue) {

    if (!m_cellNode.attribute("t")) m_cellNode.append_attribute("t");
    if (!m_cellNode.child("v")) m_cellNode.append_child("v");

    m_cellNode.attribute("t").set_value("b");
    m_cellNode.child("v").text().set(numberValue ? 1 : 0);
    m_cellNode.child("v").attribute("xml:space").set_value("default");
}

void Impl::XLCellValue::SetFloat(double numberValue) {

    if (!m_cellNode.child("v")) m_cellNode.append_child("v");

    m_cellNode.remove_attribute("t");
    m_cellNode.child("v").text().set(numberValue);
    m_cellNode.child("v").attribute("xml:space").set_value("default");
}

int64_t Impl::XLCellValue::GetInteger() const {

    if (ValueType() != XLValueType::Integer)
        throw XLException("Cell value is not Integer");
    return m_cellNode.child("v").text().as_llong();
}

bool Impl::XLCellValue::GetBoolean() const {

    if (ValueType() != XLValueType::Boolean)
        throw XLException("Cell value is not Boolean");
    return m_cellNode.child("v").text().as_bool();
}

long double Impl::XLCellValue::GetFloat() const {

    if (ValueType() != XLValueType::Float)
        throw XLException("Cell value is not Float");
    return m_cellNode.child("v").text().as_double();
}

const char* Impl::XLCellValue::GetString() const {

    if (ValueType() != XLValueType::String)
        throw XLException("Cell value is not String");
    if (strcmp(m_cellNode.attribute("t").value(),"str") == 0) // ordinary string
        return m_cellNode.child("v").text().get();
    if (strcmp(m_cellNode.attribute("t").value(),"s") == 0) // shared string
        return m_sharedStrings->GetString(m_cellNode.child("v").text().as_ullong());

    throw XLException("Unknown string type");
}
