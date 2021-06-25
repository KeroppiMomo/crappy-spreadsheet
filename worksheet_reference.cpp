#include "worksheet_reference.h"
#include <type_traits>

#define HALF_REFERENCE_RELATION_OP_IMPLEMENTATION(op) \
template<typename T> \
bool worksheet_reference::half_reference<T>::operator op(const T& other) const noexcept { \
    return number op other.number; \
} \
static_assert(true, "require semi-colon after macro, https://stackoverflow.com/a/59153563/10845353")

HALF_REFERENCE_RELATION_OP_IMPLEMENTATION(==);
template<typename T>
bool worksheet_reference::half_reference<T>::operator !=(const T& other) const noexcept {
    return !(*this == other);
}
HALF_REFERENCE_RELATION_OP_IMPLEMENTATION(<);
HALF_REFERENCE_RELATION_OP_IMPLEMENTATION(>);
HALF_REFERENCE_RELATION_OP_IMPLEMENTATION(<=);
HALF_REFERENCE_RELATION_OP_IMPLEMENTATION(>=);

template<typename T>
T worksheet_reference::half_reference<T>::operator +(int offset) const noexcept {
    return T(number + offset);
}
template<typename T>
T& worksheet_reference::half_reference<T>::operator +=(int offset) noexcept {
    number += offset;
    return static_cast<T&>(*this);
}
template<typename T>
T& worksheet_reference::half_reference<T>::operator ++() noexcept {
    return *this += 1;
}
template<typename T>
T worksheet_reference::half_reference<T>::operator -(int offset) const noexcept {
    return *this + (-1);
}
template<typename T>
T& worksheet_reference::half_reference<T>::operator -=(int offset) noexcept {
    return *this += 1;
}
template<typename T>
T& worksheet_reference::half_reference<T>::operator --() noexcept {
    return *this -= 1;
}

template class worksheet_reference::half_reference<worksheet_reference::row_reference>;
template class worksheet_reference::half_reference<worksheet_reference::col_reference>;

std::string worksheet_reference::row_reference::to_code() const noexcept {
    return std::to_string(number + 1);
}
worksheet_reference::row_reference worksheet_reference::row_reference::from_code(std::string code) noexcept(false) {
    if (code.length() == 0) throw std::invalid_argument("Empty code");
    size_t parsed;
    int no = std::stoi(code, &parsed) - 1;
    if (parsed != code.length()) throw std::invalid_argument("Parsed length is not the total length");
    if (no < 0) throw std::out_of_range("Row index smaller than 1");
    return row_reference(no);
}
std::string worksheet_reference::col_reference::to_code() const noexcept {
    // https://stackoverflow.com/a/15366979/10845353
    std::string res = "";
    int no = number;
    no++;
    do {
        int c = (no - 1) % 26;
        res = std::string(1, (char)(c + 'A')) + res;
        no = (no - c)/26;
    } while (no > 0);
    return res;
}
worksheet_reference::col_reference worksheet_reference::col_reference::from_code(std::string code) noexcept(false) {
    if (code.length() == 0) throw std::invalid_argument("Empty code");
    int no = 0;
    for (int i=0; i<code.length(); ++i) {
        if (code[i] >= 'a' && code[i] <= 'z') code[i] += 'A' - 'a';
        if (!(code[i] >= 'A' && code[i] <= 'Z')) throw std::invalid_argument("Expected letter");
        no = no * 26 + (code[i] - 'A' + 1);
    }
    return worksheet_reference::col_reference(no-1);
}
std::string worksheet_reference::cell_reference::to_code() const noexcept {
    return col.to_code() + row.to_code();
}
worksheet_reference::cell_reference worksheet_reference::cell_reference::from_code(std::string code) noexcept(false) {
    if (code.length() == 0) throw std::invalid_argument("Empty code");
    if (!((code[0] >= 'A' && code[0] <= 'Z') || (code[0] >= 'a' && code[0] <= 'z')))
        throw std::invalid_argument("Missing column");

    int splitI = -1;
    for (int i=0; i<code.length(); ++i) {
        if ((code[i] >= 'A' && code[i] <= 'Z') || (code[i] >= 'a' && code[i] <= 'z')) {
            continue;
        } else {
            splitI = i;
            break;
        }
    }
    if (splitI == -1)
        throw std::invalid_argument("Missing row");

    return cell_reference(row_reference::from_code(code.substr(splitI)), col_reference::from_code(code.substr(0, splitI)));
}
bool worksheet_reference::cell_reference::operator==(const cell_reference& other) const noexcept {
    return row == other.row && col == other.col;
}
bool worksheet_reference::cell_reference::operator!=(const cell_reference& other) const noexcept {
    return !(*this == other);
}
