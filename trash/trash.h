#ifndef TRASH_H
#define TRASH_H

#include <stdlib.h>

typedef struct Settings {
    const char *path;
    char *port;
    size_t db_size;
    size_t num_dbs;
    size_t num_threads; 
} Settings;



/**
 * Will not support multiple dbs at this time.
 * 
 * @param   Settings    settings struct
 * @note    Returns NULL if cannot start db.
*/
void start_trash(Settings *settings);

void kill_trash(TrashDB *db);

#endif //TRASH_H