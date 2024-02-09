#include "WARCRecord.h"

#include <istream>
#include "WARCException.h"

bool WARC::readContent(std::istream& input, WARC::Record<void>& record) {
    input.ignore(record.length);

    return true;
}
