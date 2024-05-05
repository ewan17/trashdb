typedef enum Log_Level {
    DEBUG,
    INFO,
    ERROR,
} Log_Level;

typedef struct Logger Logger;

/**
 * Creates a Logger struct that contains function pointers to execute logging.
 * 
 * @todo    Make logger input more abstract to handle any case of logging, not just error messages
 * 
 * @param   logger  logger struct
 *
 * @return  Logger  
*/
int init_logger(Logger **logger);

/**
 * Constructs a log message that gets passed to a buffer.
 * 
 * @note    Only pass in the logger struct that is calling this function
 * 
 * @param   logger      the struct that is calling the log function  
 * @param   log_level   the log level that this message represents   
 * @param   message     the log message
 * 
*/
// void log(Logger *logger, Log_Level const * const log_level, char const * const message);

