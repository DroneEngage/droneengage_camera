#define _ERROR_CONSOLE_BOLD_TEXT_    "\033[1;31m"  // Turn text on console red
#define _ERROR_CONSOLE_TEXT_         "\033[0;31m"  // Turn text on console red
#define _SUCCESS_CONSOLE_BOLD_TEXT_  "\033[1;32m"  // Green
#define _SUCCESS_CONSOLE_TEXT_       "\033[0;32m"  // Green
#define _INFO_CONSOLE_TEXT           "\033[0;33m"  // Yellow
#define _INFO_BOLD_CONSOLE_TEXT      "\033[1;33m"  // Yellow
#define _LOG_CONSOLE_TEXT            "\033[0;34m"  // Blue
#define _LOG_CONSOLE_TEXT_BOLD_      "\033[1;34m"  // Blue
#define _TEXT_BOLD_HIGHTLITED_       "\033[1;37m"  // WHITE
#define _BOLD_CONSOLE_TEXT_          "\033[1m" // Restore normal console colour
#define _NORMAL_CONSOLE_TEXT_        "\033[0m" // Restore normal console colour

#define __FULL_DEBUG__   _LOG_CONSOLE_TEXT << __FILE__ << "." << _SUCCESS_CONSOLE_TEXT_ <<  __FUNCTION__  << "." << __LINE__ << ": " << _NORMAL_CONSOLE_TEXT_ 