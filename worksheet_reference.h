#ifndef __INCLUDE_WORKSHEET_REFERENCE_
#define __INCLUDE_WORKSHEET_REFERENCE_

#include <string>

/**
 * Define reference objects in `worksheet`.
 *
 * This class is created just to have forward declaration working.
 */
class worksheet_reference {
    public:
    /**
     * An abstract struct of a reference in a worksheet.
     */
    struct reference {
        /**
         * Convert the reference into text using reference type-specific naming conventions.
         */
        virtual std::string to_code() const noexcept = 0;
    };
    /**
     * An abstract struct of a reference with a single index value.
     *
     * This class uses CRTP, so its children class should have the template type set to itself.
     */
    template<typename T>
    struct half_reference: reference {
        /// Index represented by the reference.
        int number;
        /**
         * Convert the reference into text using reference type-specific naming conventions.
         */
        virtual std::string to_code() const noexcept = 0;
        /**
         * Construct a `half_reference` with its index number.
         *
         * @param no Index represented by the reference.
         */
        half_reference(int no) noexcept: number(no) {}

        bool operator ==(const T& other) const noexcept;
        bool operator !=(const T& other) const noexcept;
        bool operator <(const T& other) const noexcept;
        bool operator >(const T& other) const noexcept;
        bool operator <=(const T& other) const noexcept;
        bool operator >=(const T& other) const noexcept;
        T operator +(int offset) const noexcept;
        T& operator +=(int offset) noexcept;
        T& operator ++() noexcept;
        T operator -(int offset) const noexcept;
        T& operator -=(int offset) noexcept;
        T& operator --() noexcept;
    };
    /**
     * A reference to a row of a worksheet.
     */
    struct row_reference: half_reference<row_reference> {
        /**
         * Construct a `row_reference` with its row index.
         *
         * @param no Row index.
         */
        row_reference(int no) noexcept : half_reference(no) {}
        /**
         * Convert the row reference into conventional one-based format.
         */
        std::string to_code() const noexcept override;
        /**
         * Construct a `row_reference` from its code.
         *
         * @param code Code representation of the reference to be converted.
         * @throws std::invalid_argument Thrown if the code is empty or cannot be parsed into an
         *     integer.
         * @throws std::out_of_range Thrown if the parsed integer cannot be
         *     represented by `int`, or the one-based row index is non-positive.
         */
        static row_reference from_code(std::string code) noexcept(false);

        using half_reference<row_reference>::operator ==;
        using half_reference<row_reference>::operator !=;
        using half_reference<row_reference>::operator <;
        using half_reference<row_reference>::operator >;
        using half_reference<row_reference>::operator <=;
        using half_reference<row_reference>::operator >=;
        using half_reference<row_reference>::operator +;
        using half_reference<row_reference>::operator +=;
        using half_reference<row_reference>::operator ++;
        using half_reference<row_reference>::operator -;
        using half_reference<row_reference>::operator -=;
        using half_reference<row_reference>::operator --;
    };
    struct col_reference: half_reference<col_reference> {
        /**
         * Construct a `col_reference` with its column index.
         *
         * @param no Column index.
         */
        col_reference(int no): half_reference(no) {}
        /**
         * Convert column row reference into conventional capital letter format.
         */
        std::string to_code() const noexcept override;
        /**
         * Construct a `col_reference` from its code.
         *
         * @param code Code representation of the reference to be converted.
         * @throws std::invalid_argument Thrown if the code is empty or contains a character
         *     which is not a letter.
         */
        static col_reference from_code(std::string code) noexcept(false);

        using half_reference<col_reference>::operator ==;
        using half_reference<col_reference>::operator !=;
        using half_reference<col_reference>::operator <;
        using half_reference<col_reference>::operator >;
        using half_reference<col_reference>::operator <=;
        using half_reference<col_reference>::operator >=;
        using half_reference<col_reference>::operator +;
        using half_reference<col_reference>::operator +=;
        using half_reference<col_reference>::operator ++;
        using half_reference<col_reference>::operator -;
        using half_reference<col_reference>::operator -=;
        using half_reference<col_reference>::operator --;
    };
    /**
     * A reference to a worksheet cell, a combination of a `row_reference` and `col_reference`.
     */
    struct cell_reference: reference {
        /// The row reference part of the cell.
        row_reference row;
        /// The column reference part of the cell.
        col_reference col;

        /**
         * Construct a `cell_reference` with row and column references.
         *
         * @param r Row reference part.
         * @param c Column reference part.
         */
        cell_reference(row_reference r, col_reference c) noexcept: row(r), col(c) {}
        /**
         * Construct a `cell_reference` with row and column indices.
         *
         * @param r Row index.
         * @param c Column index.
         */
        cell_reference(int r, int c) noexcept: row(row_reference(r)), col(col_reference(c)) {}

        /**
         * Convert the cell reference into the conventional "row-column"
         * format, for example, A4.
         */
        std::string to_code() const noexcept override;
        /**
         * Construct a  cell_reference` from its code.
         *
         * @param code Code representation of the reference to be converted.
         * @throws std::invalid_argument Thrown if column and/or row part is empty,
         *     the column part contains a character which is not a letter, or the row
         *     part cannot be converted into an integer.
         * @throws std::out_of_range Thrown if the parsed integer of the row part
         *     cannot be represented by `int`, or the one-based row index is
         *     non-positive.
         */
        static cell_reference from_code(std::string code) noexcept(false);

        bool operator==(const cell_reference& other) const noexcept;
        bool operator!=(const cell_reference& other) const noexcept;
    };
};

#endif
