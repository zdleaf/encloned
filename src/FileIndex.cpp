#include <enclone/FileIndex.h>

FileIndex::FileIndex(){
    openIndex();
}

void FileIndex::openIndex(){
    int exitcode = sqlite3_open("index.db", &db);
    if(exitcode) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
      fprintf(stderr, "Opened database successfully\n");
    }
}

void FileIndex::closeIndex(){
    sqlite3_close(db);
}