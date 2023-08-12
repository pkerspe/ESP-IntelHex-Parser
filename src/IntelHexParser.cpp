#include "IntelHexParser.h"

IntelHexParser::IntelHexParser(File *hexFileToParse, byte pageSize)
{
    this->_hexFileToParse = hexFileToParse;
    this->_pageSize = pageSize;
    this->_parsedCompletely = false;
    this->_recordsParsedCounter = 0;
}

_recordDetailsStruct IntelHexParser::getNextRecordToWrite(byte *recordDataBuffer, byte bufferLength)
{
    _recordDetailsStruct details;
    if (this->_hexFileToParse->available())
    {
        byte maxLineLength = 50;
        byte record[maxLineLength]; // Arduino HEX files should only contain records (lines) with max 44 characters
        String dataLine = this->_hexFileToParse->readStringUntil('\n');
        if (dataLine.length() >= maxLineLength)
        {
            Serial.printf("Error: line length longer than expected! %i expected %i\n", dataLine.length(), maxLineLength);
        }
        if (this->getRecordType(record) == INTEL_HEX_PARSER_RECORD_TYPE_DATA)
        {
            dataLine.getBytes(record, dataLine.length());
            details.address = this->getRecordAddress(record);
            details.dataLength = this->getRecordLength(record);
            if(details.dataLength > bufferLength){
                Serial.println("Given record buffer to small to hold record data bytes");
                details.errorCode = 1;
            } else {
                extractData(record, details.dataLength, recordDataBuffer);
            }
        }
        else if (this->getRecordType(record) == INTEL_HEX_PARSER_RECORD_TYPE_END_OF_FILE)
        {
            details.endOfFileReached = true;
        }
    }
    return details;
}

bool IntelHexParser::getNextPage(byte *pageBuffer)
{
    if (_parsedCompletely == true)
    {
        Serial.println("no more content");
        return false;
    }
    byte pageBufferIndex = 0;
    // check if we have data from previous call that didn't fit in the last page
    if (_overflowBufferIndex > 0)
    {
        for (int i = 0; i < _overflowBufferIndex; i++)
        {
            pageBuffer[pageBufferIndex] = this->_overflowBuffer[i];
            pageBufferIndex++;
        }
        _overflowBufferIndex = 0;
    }

    // read new data from file
    bool bufferFilled = false;
    while (this->_hexFileToParse->available() && pageBufferIndex < 128)
    {
        byte maxLineLength = 50;
        byte record[maxLineLength]; // Arduino HEX files should only contain records with max 44 characters
        String dataLine = this->_hexFileToParse->readStringUntil('\n');
        if (dataLine.length() >= maxLineLength)
        {
            Serial.printf("Error: line length longer than expected! %i expected %i\n", dataLine.length(), maxLineLength);
            break;
        }
        dataLine.getBytes(record, dataLine.length());
        Serial.printf("Read line %u from hex file with length of %i characters\n", this->_recordsParsedCounter, dataLine.length());
        this->_recordsParsedCounter++;
        // check if record is of type data (=0)
        if (getRecordType(record) == INTEL_HEX_PARSER_RECORD_TYPE_DATA)
        {
            uint8_t dataLength = this->getRecordLength(record);
            Serial.printf("Found data record with %i bytes in record #%u\n", dataLength, this->_recordsParsedCounter);
            byte dataBuffer[dataLength];
            extractData(record, dataLength, dataBuffer);
            bool bufferOverflow = false;
            for (int i = 0; i < dataLength; i++)
            {
                if (!bufferFilled && pageBufferIndex < 128)
                {
                    pageBuffer[pageBufferIndex] = dataBuffer[i];
                    pageBufferIndex++;
                }
                else
                {
                    pageBufferIndex = 0;
                    bufferFilled = true;
                    this->_overflowBuffer[_overflowBufferIndex] = dataBuffer[i];
                    this->_overflowBufferIndex++;
                }
            }

            // Serial.printf("Added %i bytes to page buffer (filled: %i) and overflowed by %i bytes\n", pageBufferIndex, bufferFilled, _overflowBufferIndex);
        }
        else if (getRecordType(record) == INTEL_HEX_PARSER_RECORD_TYPE_END_OF_FILE)
        {
            Serial.println("End of file marker found");
            _parsedCompletely = true;

            // last record reached, so read data and fill rest of page data with 0xFF
            if (pageBufferIndex > 0)
            {
                Serial.printf("End of file reached, current buffer index is %i. filling up remaining buffer with  %i pad bytes\n", pageBufferIndex, 128 - pageBufferIndex);
                while (pageBufferIndex < 128)
                {
                    pageBuffer[pageBufferIndex] = 0xFF;
                    pageBufferIndex++;
                }
            }
            else
            {
                return false; // no more data, so we return false to indicate no more page available
            }
        }
        else
        {
            Serial.printf("Unsupported record type: %i\n", getRecordType(record));
        }
    }
    return true;
}

int IntelHexParser::getRecordLength(byte *record)
{
    char buffer[3];
    buffer[0] = record[INTEL_HEX_PARSER_BYTE_COUNT_START_INDEX];
    buffer[1] = record[INTEL_HEX_PARSER_BYTE_COUNT_START_INDEX + 1];
    buffer[2] = '\0';
    return (uint8_t)strtol(buffer, 0, 16);
}

int IntelHexParser::getRecordType(byte *record)
{
    char buffer[3];
    buffer[0] = record[INTEL_HEX_PARSER_RECORD_TYPE_START_INDEX];
    buffer[1] = record[INTEL_HEX_PARSER_RECORD_TYPE_START_INDEX + 1];
    buffer[2] = '\0';
    return strtol(buffer, 0, 16);
}

uint16_t IntelHexParser::getRecordAddress(byte *record)
{
    char buffer[5];
    buffer[0] = record[INTEL_HEX_PARSER_ADDRESS_START_INDEX];
    buffer[1] = record[INTEL_HEX_PARSER_ADDRESS_START_INDEX + 1];
    buffer[2] = record[INTEL_HEX_PARSER_ADDRESS_START_INDEX + 2];
    buffer[3] = record[INTEL_HEX_PARSER_ADDRESS_START_INDEX + 3];
    buffer[4] = '\0';
    return strtol(buffer, 0, 16);
}

void IntelHexParser::extractData(byte *record, int len, byte *dataBuffer)
{
    int end = (len * 2) + INTEL_HEX_PARSER_DATA_START_INDEX; // data is represented in two HEX chars per byte
    char buffer[3];
    buffer[2] = '\0';

    int byteIndex = 0;
    for (int i = INTEL_HEX_PARSER_DATA_START_INDEX; i <= end; i = i + 2)
    {
        // get HEX presentation (two characters) and transform to long (factual can only be int with two HEX characters)
        buffer[0] = record[i];
        buffer[1] = record[i + 1];
        dataBuffer[byteIndex] = (int)strtol(buffer, 0, 16);
        byteIndex++;
    }
}
