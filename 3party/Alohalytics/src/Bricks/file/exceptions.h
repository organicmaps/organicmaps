#ifndef BRICKS_FILE_EXCEPTIONS_H
#define BRICKS_FILE_EXCEPTIONS_H

#include "../exception.h"

namespace bricks {

// TODO(dkorolev): Add more detailed exceptions for Read/Write/etc.
struct FileException : Exception {};

}  // namespace bricks

#endif  // BRICKS_FILE_EXCEPTIONS_H
