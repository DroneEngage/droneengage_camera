#ifndef CHELPER_H
#define CHELPER_H

#include "../common.h"

namespace de
{
namespace util
{
class CHelper{


    public:

        static void initRandom ()
        {
            srand(time(NULL));
        }

        static std::string getGUID ()
        {

            char strUuid[64];
            
            //sprintf(strUuid, "%x%x-%x-%x-%x-%x%x%x", 
            sprintf(strUuid, "%x%x%x%x%x%x%x%x", 
            rand(), rand(),                 // Generates a 64-bit Hex number
            rand(),                         // Generates a 32-bit Hex number
            ((rand() & 0x0fff) | 0x4000),   // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
            rand() % 0x3fff + 0x8000,       // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
            rand(), rand(), rand());

            return std::string (strUuid);
        }


        static std::string getShortSemiGUID ()
        {

            char strUuid[64];
            
            sprintf(strUuid, "G%x%x%x", 
            rand(), rand(),                 // Generates a 64-bit Hex number
            rand());                         // Generates a 32-bit Hex number
            
            
            return std::string (strUuid);
        }


        static std::string getFileTimeStamp ()
        {
            const time_t now = time(0);
            
            const tm *ltm = localtime(&now);

            std::ostringstream timestamp;  

            timestamp << 1900 + ltm->tm_year << "_" << ltm->tm_mon << "_" << ltm->tm_mday << "_" << ltm->tm_hour << "_" << ltm->tm_min << "_" << ltm->tm_sec;

            return timestamp.str();
            
        }

        //function for converting string to upper
        static std::string stringToUpper(std::string oString){
            for(std::size_t i = 0; i < oString.length(); i++){
            oString[i] = toupper(oString[i]);
            }
            return oString;
        }

};

}
}

#endif