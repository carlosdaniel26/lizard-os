# Code and Commit Style Guide

## 1. Code Style

### 1.1 Indentation & Layout
- Use tabs, 4 spaces.
- Use  `{` in the same line just and only if its a struct or an union, anything else like IF statements or loops must have theirs brackets on the next line.
- Maximum 80 columns per line.

### 1.2. Name convention
#### 1.2.1
- Use acronyms when useful/usual, if its not usual, its not useful. For example, use don't use `universal_serial_bus`, prefer to keep the usual `usb`
  
#### 1.2.2 Files
- Use `snake_case`.
- Use acronyms when useful/usual, if its not usual, its not useful.
- Header files might use `#pragma once`
  
#### 1.2.3 Functions
- Use `snake_case`.
- Use acronyms when useful/usual, if its not usual, its not useful.
- Use the module or file prefix to identify where that function comes from or what its related to, for example `int blk_dev_register(BlockDevice *dev)` the prefix `blk_dev` indicates that this function comes from `blk_dev.c`.
- If you feel like the number of arguments is too big, even if it don't exceed the maximum of columns, feel free to use the next line and align it to the arguments column.

#### 1.2.4 Local Variables
- Use `snake_case`.
- Local variables can be shorter but meaningful.

#### 1.2.5 Public Global Variables
- Use `g_snake_case`.
- Declare `extern g_*` on the header file to ensure the callers will use it effortless.

#### 1.2.6 Private Global Variables
- use `static snake_case` to ensure other modules won't acess it recklessly.

#### 1.2.6
- Use `PascalCase ` for `structs` & `unions`.
- Use `UPPER_CASE` for constants and macros.
- Use `snake_case_t` for typedefs of primitive types.

### 1.3 File structure
- <in_progress>.
### 1.4 Comments
- Use `/* */` for multi-line comments

- Comment `complex` algorithms and `non-obvious` code.
### 1.5 Functions & type conventions
- <in_progress>.

## Commit style
- <in_progress>
