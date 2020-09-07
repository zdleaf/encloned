#include <encloned/DB.h>

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
      fprintf(stderr, "Successfully executed SQL statement: %.36s...\n", sql);
      //fprintf(stderr, "Successfully executed SQL statement: %s...\n", sql);
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

void DB::backupProgress(int leftToCopy, int totalToCopy){
    cout << "DB: " << totalToCopy-leftToCopy << "/" << totalToCopy << " items backed up" << endl; 
    cout.flush();
}

int DB::backupDB(const char *backupFilename){
  int rc;                     /* Function return code */
  sqlite3 *pFile;             /* Database connection opened on zFilename */
  sqlite3_backup *pBackup;    /* Backup handle used to copy data */

  /* Open the database file identified by zFilename. */
  rc = sqlite3_open(backupFilename, &pFile);
  if( rc==SQLITE_OK ){

    /* Open the sqlite3_backup object used to accomplish the transfer */
    pBackup = sqlite3_backup_init(pFile, "main", db, "main");
    if( pBackup ){

      /* Each iteration of this loop copies 10 database pages from database
      ** pDb to the backup database. If the return value of backup_step()
      ** indicates that there are still further pages to copy, sleep for
      ** 250 ms before repeating. */
      do {
        rc = sqlite3_backup_step(pBackup, 10);
        backupProgress( // update progress function
            sqlite3_backup_remaining(pBackup),
            sqlite3_backup_pagecount(pBackup)
        );
        if( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED ){
          sqlite3_sleep(250);
        }
      } while( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED );

      /* Release resources allocated by backup_init(). */
      (void)sqlite3_backup_finish(pBackup);
    }
    rc = sqlite3_errcode(pFile);
  }
  
  /* Close the database connection opened on database file zFilename
  ** and return the result of this function. */
  (void)sqlite3_close(pFile);
  return rc;
}