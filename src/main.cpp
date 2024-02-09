#include <iostream>
#include <fstream>
#include <cstring>
#include "WARCReader.h"
#include "WARCRecord.h"
#include "WARCException.h"
#include "CSVWriter.h"
#include "ValueParsers.h"
#include "PublicSuffix.h"
#include "PocoUri.h"
#include "tclap/CmdLine.h"

void writeCSVHeader(CSV::Writer&);
void processWARC(std::istream&, CSV::Writer&, const PublicSuffix&, int);

constexpr int noVerbosity = 0;
constexpr int lowVerbosity = 1;
constexpr int medVerbosity = 2;
constexpr int highVerbosity = 3;

int main(int argc, char** argv) {
    std::istream* input;
    std::ostream* output;

    TCLAP::CmdLine cmd("meta-extractor", ' ', "0.1");
    TCLAP::ValueArg<std::string> inFile("i", "input", "Input file", false, "", "in-file", cmd);
    TCLAP::ValueArg<std::string> outFile("o", "output", "Output file", false, "", "out-file", cmd);
    TCLAP::ValueArg<std::string> suffixFile("s", "public-suffixes", "Public suffix list file: https://publicsuffix.org/list/", true, "", "suffix-file", cmd);
    TCLAP::SwitchArg csvHeader("", "print-header", "Output includes csv header", cmd, false);
    TCLAP::MultiSwitchArg verbosity("v", "verbose", "Verbose Output", cmd, noVerbosity);
    try {
        cmd.parse(argc, argv);
    } catch(TCLAP::ArgException &e) {
        std::cerr << "command line parsing error: " << e.what() << std::endl;
        return 1;
    }

    std::ifstream inFileStream;
    if(inFile.isSet()) {
        inFileStream.open(inFile.getValue(), std::ifstream::binary);
        if(!inFileStream.good()) {
            std::cerr << "input file not found" << std::endl;
            return 2;
        }
        input = &inFileStream;
    } else {
        input = &std::cin;
    }

    std::ofstream outFileStream;
    if(outFile.isSet()) {
        outFileStream.open(outFile.getValue(), std::ifstream::binary);
        if(!outFileStream.good()) {
            std::cerr << "output file not found" << std::endl;
            return 3;
        }
        output = &outFileStream;
    } else {
        output = &std::cout;
    }

    std::ifstream suffixFileStream(suffixFile.getValue());
    if(!suffixFileStream.good()) {
        std::cerr << "public suffix list file not found" << std::endl;
        return 4;
    }
    PublicSuffix suffixes(suffixFileStream);

    try {
        CSV::Writer csv(*output);

        if(csvHeader.getValue()) {
            writeCSVHeader(csv);
        }
        processWARC(*input, csv, suffixes, verbosity.getValue());
    } catch(const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 10;
    }

    return 0;
}

void writeCSVHeader(CSV::Writer& csv) {
    csv
        << "id"                  // string : warc record id
        << "url"                 // string : <full url>
        << "https"               // bool   : http[s]
        << "hostname"            // string : amazon.co.uk
        << "tld"                 // string : uk
        << "public suffix"       // string : co.uk
        << "path depth"          // uint8  : number of slashes in path + 1
        << "pathsegment length"; // uint16 : length of path + query + fragment

    csv.next();
}

void processWARC(std::istream& input, CSV::Writer& writer, const PublicSuffix& suffixes, int verbosity) {
    WARC::Reader reader(input);
    WARC::Record<void> record;
    uint32_t countProcessed {0}, countIgnored {0};

    while(reader.read(record)) {
        if (true) {
            ++countProcessed;

            Poco::URI url(record.headers.at("WARC-Target-URI"));
            writer << record.id
                   << record.headers.at("WARC-Target-URI")
                   << std::to_string(url.getScheme() == "https")
                   << url.getHost()
                   << Value::extractTld(url.getHost())
                   << suffixes.getSuffix(url.getHost())
                   << std::to_string(Value::extractPathDepth(url.getPath()))
                   << std::to_string((uint16_t) url.getPathEtc().size());

            writer.next();
        } else {
            ++countIgnored;
        }

        if (verbosity >= highVerbosity) {
            std::cerr << record.id << ", " << record.date << ", "
                      << record.length << " bytes, "
                      << record.headers.at("Content-Type") << std::endl;
        }
    }

    if(verbosity >= lowVerbosity) {
        std::cerr << countProcessed << " records processed, "
                  << countIgnored << " records ignored because of Content-Type"
                  << std::endl;
    }
}
