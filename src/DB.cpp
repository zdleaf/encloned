#include <enclone/DB.h>

DB::DB(){
    openDB();
}

void DB::openDB(){
    int exitcode = sqlite3_open("index.db", &db);
    if(exitcode) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
      fprintf(stderr, "Opened database successfully\n");
    }
}

void DB::closeDB(){
    sqlite3_close(db);
}