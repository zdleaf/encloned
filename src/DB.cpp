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
      fprintf(stderr, "Successfully executed SQL statement\n");
    }
}

void DB::initialiseTables(){
    const char sql[] = "CREATE TABLE IF NOT EXISTS WatchDirs (PATH TEXT NOT NULL);";
    execSQL(sql);
}