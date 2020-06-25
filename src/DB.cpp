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

int DB::execSQL(const char sql[]){
    char* error;
    int exitcode = sqlite3_exec(db, sql, NULL, NULL, &error);
    if(exitcode != SQLITE_OK) {
        if(error != NULL){
            fprintf(stderr, "Error executing SQL: %s\n", error);
            sqlite3_free(error);
            return 1;
        }
    } else {
      fprintf(stderr, "Successfully executed SQL statement: %.36s...\n", sql);
    }
    return 0;
}

void DB::initialiseTables(){
    const char dirIndex[] = "CREATE TABLE IF NOT EXISTS dirIndex ("
        "PATH       TEXT    NOT NULL,"
        "RECURSIVE  BOOLEAN NOT NULL    DEFAULT FALSE);";
    const char fileIndex[] = "CREATE TABLE IF NOT EXISTS fileIndex ("
        "PATH       TEXT    NOT NULL," 
        "MODTIME    INTEGER NOT NULL,"
        "HASH       TEXT);";
    execSQL(dirIndex);
    execSQL(fileIndex);
}