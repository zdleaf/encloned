#include <enclone/DB.h>

DB::DB(){ // constructor
    openDB();
    initialiseTables();
}

DB::~DB(){ // destructor
    closeDB();
}

void DB::openDB(){
    int exitcode = sqlite3_open(DATABASE_LOCATION, &db);
    if(exitcode) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
      fprintf(stderr, "Opened database successfully\n");
      fs::permissions(DATABASE_LOCATION, fs::perms::owner_read | fs::perms::owner_write);
    }

    int validDBcheck = execSQL("PRAGMA schema_version;");
    if(validDBcheck != 0){
        cout << "DB: ERROR - unable to open index.db - it does not appear to be a valid database. Please check and resolve, or remove to create a new index" << endl;
        throw std::runtime_error("Invalid database file");
    } else {
        fprintf(stderr, "Database is valid\n");
    }
}

void DB::closeDB(){
    sqlite3_close(db);
}

sqlite3* DB::getDbPtr(){
    return db;
}

const char* DB::getDbLocation(){
    return DATABASE_LOCATION;
}

int DB::execSQL(const char sql[]){
    std::scoped_lock<std::mutex> guard(mtx);
    char* error;
    int exitcode = sqlite3_exec(db, sql, NULL, NULL, &error);
    if(exitcode != SQLITE_OK) {
        if(error != NULL){
            fprintf(stderr, "%d Error executing SQL: %s\nSQL:%s", exitcode, error, sql);
            sqlite3_free(error);
            return 1;
        }
    } else {
      //fprintf(stderr, "Successfully executed SQL statement: %.36s...\n", sql);
      fprintf(stderr, "Successfully executed SQL statement: %s...\n", sql);
    }
    return 0;
}

void DB::initialiseTables(){
    const char dirIndex[] = "CREATE TABLE IF NOT EXISTS dirIndex ("
        "PATH       TEXT    NOT NULL    UNIQUE,"
        "RECURSIVE  BOOLEAN NOT NULL    DEFAULT FALSE);";

    const char fileIndex[] = "CREATE TABLE IF NOT EXISTS fileIndex ("
        "PATH       TEXT    NOT NULL," 
        "MODTIME    INTEGER NOT NULL,"
        "PATHHASH       TEXT,"
        "FILEHASH       TEXT,"
        "LOCALEXISTS    BOOLEAN,"
        "REMOTEEXISTS   BOOLEAN);";

    const char indexBackup[] = "CREATE TABLE IF NOT EXISTS indexBackup ("
        "PATH       TEXT    NOT NULL    UNIQUE,"
        "IDXNAME    TEXT    NOT NULL,"
        "MODTIME    INTEGER);";

    execSQL(dirIndex);
    execSQL(fileIndex);
    execSQL(indexBackup);
}