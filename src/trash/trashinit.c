

/**
 * Creates a directory where the db files exist.
 * 
 * @param   path    Path where the database file will be stored  
*/
// static int env_file(char *path) {
//     if(path == NULL) {
//         path = DB_DIR;
//     }

//     char dir[MAX_DIR_SIZE];
//     char *path_separator;

//     if(getcwd(dir, MAX_DIR_SIZE) == NULL) {
//         return -1;
//     };
//     path_separator = "/";

//     int i;
//     for(i = strlen(dir); i >= 0; i--) {
//         if(dir[i] == *path_separator) {
//             break;
//         }
//     }
//     if((MAX_DIR_SIZE - strlen(dir)) <= strlen(path)) {
//         return -1;
//     } else {
//         int j;
//         for(j = 0; j < strlen(path); j++) {
//             dir[i+j] = path[j];
//         }
//         dir[i+j] = '\0';
//     }

//     return mkdir(dir);
// };

