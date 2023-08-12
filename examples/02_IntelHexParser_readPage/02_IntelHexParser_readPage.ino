#include <SPIFFS.h>
#include <IntelHexParser.h>

/**
 * Example for using the ESP-IntelHex-Parser library to read and decode data from a hex file in blocks of pages with defined page size.
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
    const uint8_t pageBufferSize = 128;

    // create class instance with pointer to the file instace of the hex file to be parsed
    IntelHexParser hexParser(&hexFile, pageBufferSize);
    // create buffer to store the data of a page
    // For Atmega MCUs like the Atmega32A this is 128 bytes
    byte pageBuffer[pageBufferSize];

    // populate the pageBuffer with bytes from the hex file
    // if the hex file contains less data bytes than page buffer size, the buffer will be padded with 0xFF
    // we loop here until no more page data can be read from the hex file
    while (hexParser.getNextPage(pageBuffer))
    {
        // do whatever your want with the page data stored in the buffer
        Serial.println("Page Data:");
        for (uint8_t i = 0; i < pageBufferSize; i++)
        {
            Serial.printf("0x%X ", pageBuffer[i]);
        }
        Serial.println("\n--------------");
    }

    // once you are done, close the file instance
    hexFile.close();
}

void loop()
{
    // do whatever you want here
}
