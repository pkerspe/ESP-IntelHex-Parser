#include <SPIFFS.h>
#include <IntelHexParser.h>

/**
 * Example for using the ESP-IntelHex-Parser library to read and decode records line by line from a Intel Hex file stored in SPIFFS.
 * You need to upload a hex file to SPIFFS on your ESP in order to execute this example.
 * You can use the firmware hex file in the data subfolder of this example, it contains a simple blink firmware for an Atmega32A as an example
 */

// upload a .hex file to the SPIFFS of the ESP and adapt the path below to match the path/name of your file
String filePathInSpiffs = "/firmware_1.hex";

void setup()
{
    Serial.begin(115200);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    // check if file exists in SPIFFS
    if (!SPIFFS.exists(filePathInSpiffs))
        Serial.printf("File not found in SPIFFS: %s\n", filePathInSpiffs.c_str());
    return;

    // open file for reading
    File hexFile = SPIFFS.open(filePathInSpiffs, "r");

    if (!hexFile)
    {
        Serial.println("Failed opening file for reading");
        return;
    }

    // Here start the relevant part for using this ESP-IntelHex-Parser library

    // create class instance with pointer to the file instace of the hex file to be parsed
    IntelHexParser hexParser(&hexFile);
    // create buffer to store the data of a record
    // this must be at least as long as the expected amount data per record in the Intel HEX file
    // For Arduino firmware hex files this is in general data 16 bytes, we just use 17 to be save here and to provide room for a termination character
    const uint8_t recordDataBufferSize = 17;
    byte recordDataBuffer[recordDataBufferSize];
    recordDataBuffer[recordDataBufferSize - 1] = '\0';

    // get a parsed record as struct with the needed details and fill the buffer with the actual data:
    // NOTE: you can call getNextRecordToWrite in a loop as long as recordDetails.endOfFileReached == false
    // in this example we only parse only the FIRST LINE in the HEX file
    _recordDetailsStruct recordDetails = hexParser.getNextRecordToWrite(recordDataBuffer, recordDataBufferSize);

    // at this point you can check the error code to see if an error occurred (should be 0 in case of no error) otherwise the buffer should now contain the data from this record (= a line in the hex file)
    if (recordDetails.errorCode == 0)
    {
        Serial.printf("Parsed record in HEX file: address = %i, number of data bytes = %i, end of file reached = %i\n", recordDetails.address, recordDetails.dataLength, recordDetails.endOfFileReached);
    }
    else
    {
        Serial.printf("Failed parsing record in HEX file. Error code is: %i \n", recordDetails.errorCode);
    }

    // once you are done, close the file instance
    hexFile.close();
}

void loop()
{
    // do whatever you want here
}
