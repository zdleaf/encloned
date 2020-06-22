#include <enclone/DB.h>

DB::DB(){ // constructor
    openDB();
    initialiseTables();
}

DB::~DB(){ // destructor
    closeDB();
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

void DB::execSQL(const char sql[]){
    char* error;
    int exitcode = sqlite3_exec(db, sql, NULL, NULL, &error);
    if(exitcode != SQLITE_OK) {
        if(error != NULL){
            fprintf(stderr, "Error executing SQL: %s\n", error);
            sqlite3_free(error);
        }
    } else {
      fprintf(stderr, "Successfully executed SQL statement: %.36s...\n", sql);
    }
}

void DB::initialiseTables(){
    const char watchdirs[] = "CREATE TABLE IF NOT EXISTS WatchDirs ("
        "PATH       TEXT    NOT NULL);";
    const char fileidx[] = "CREATE TABLE IF NOT EXISTS FileIndex ("
        "FILENAME   TEXT    NOT NULL," 
        "PATH       TEXT    NOT NULL," 
        "MODTIME    INTEGER NOT NULL,"
        "SIZE       INTEGER,"
        "HASH       TEXT);";
    execSQL(watchdirs);
    execSQL(fileidx);
}