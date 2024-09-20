# Crappy Spreadsheet
A terminal-based spreadsheet with very basic formula support.

This is basically an excuse to learn ANSI, C++, and syntax analysis.

## Demo
- The spreadsheet adjusts to the initial terminal window size. Redraw by pressing `<C-l>`.
- Press `h`, `j`, `k`, `l` to move the cursor (like in Vim).
- Press `i` to edit a cell, then `<Enter>` to confirm or `<Esc>` to discard the change.
- To enter a formula, start with `=` followed by an expression.
- Single cell references (e.g. `A1`) are supported.
- Currently supports `integer`, `text`, `boolean` and `error` as the "primative" data types.
- Available operators: `+`, `-`, `*`, `/`, `&`, `=`, `<>`, `<`, `>`, `<=`, `>=`
- Available functions: `SUM`, `IF`

https://github.com/user-attachments/assets/426b711d-59c1-489b-9ecc-4ab137e7d481

## Technical Details
In case you somehow want to run this...

- Make sure [Boost](https://www.boost.org/) is installed, and
  modify the Makefile to include the correct path to Boost.
- Run `make` to compile and execute the binary with `./compilation`.
