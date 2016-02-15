/**
* $Id$ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief SqliteWrap
*/
#include <string.h>
#include <algorithm> 
#include "sqlite_wrap.h"
#include "sqlite3.h"
#include "ezsync/verify_true.h"
#include "ezsync/log.h"

#ifndef WIN32
#define stricmp strcasecmp
#endif

SqliteWrap::SqliteWrap(const std::string&db_file):_db(0){

    sqlite3* db=0;
    std::string error;
    if(sqlite3_open(db_file.c_str(),&db) != SQLITE_OK){
        if(db){
            const char* e = sqlite3_errmsg(_db);
            error = e?e:"";
            sqlite3_close(db);
            db = 0;
        }
    }
    _db = db;
    VERIFY_TRUE(_db != 0)<<"open sqlite db failed with "<<error;
    create_snapshoot();
}
SqliteWrap::~SqliteWrap(){
    if(_db)sqlite3_close(_db);
}

std::set<std::string> SqliteWrap::get_tables(){
    std::set<std::string> tables;
    std::for_each(_snapshoot.begin(),_snapshoot.end(),[&tables](Snapshoot::reference& it){tables.insert(it.first);});
    return tables;
}

void SqliteWrap::exec(const std::string&sql,Rows& rows){
    char *errmsg= 0;
    int res = sqlite3_exec(_db,sql.c_str(), callback, &rows, &errmsg);
    std::string err = errmsg?errmsg:"";
    if(errmsg)sqlite3_free(errmsg);
    VERIFY_TRUE(res == SQLITE_OK) << "exec " << sql << " failed with " <<  err;
}
void SqliteWrap::exec(const std::string&sql){
    char *errmsg= 0;
    int res = sqlite3_exec(_db,sql.c_str(), 0,0, &errmsg);
    std::string err = errmsg?errmsg:"";
    if(errmsg)sqlite3_free(errmsg);
    VERIFY_TRUE(res == SQLITE_OK) << "exec " << sql << " failed with " <<  err;
}
//TODO: 优化,使用缓存
std::set<std::string> SqliteWrap::get_columns(const std::string&table){
    return _snapshoot[table].columns;
}

//TODO: 优化,使用缓存
std::set<std::string> SqliteWrap::get_pks(const std::string&table){
    return _snapshoot[table].pks;
}
int SqliteWrap::callback(void*p,int count,char**value ,char**name){
    Rows& rows = *(Rows*)p;
    rows.push_back(Row());
    for(int i=0;i<count;i++){
        (rows.back())[name[i]] = value[i]?value[i]:"";
    }
    return SQLITE_OK;
}
bool SqliteWrap::is_table_exist(const std::string& table){
    return _snapshoot.find(table) != _snapshoot.end();
}
void SqliteWrap::create_snapshoot(){

    _snapshoot.clear();
    SqliteWrap::Rows rows;
    exec("select tbl_name from sqlite_master where type = \'table\'",rows);
    for(SqliteWrap::Rows::iterator it = rows.begin();it!=rows.end();it++){
        sqlite3_stmt *stmt =0;
        TableInfo info;
        VERIFY_TRUE(SQLITE_OK == sqlite3_prepare(_db, (std::string("pragma table_info (\'")+(*it)["tbl_name"]+"\')").c_str(), -1, &stmt, 0)) 
            << "pragma table_info failed with "
            << sqlite3_errmsg(_db);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            int count = sqlite3_column_count(stmt);
            std::string name;
            bool is_pk = false;
            for(int i=0;i<count;i++){
                if(stricmp(sqlite3_column_name(stmt,i) ,"name")==0){
                    const char* var = (const char *)sqlite3_column_text(stmt, i);
                    name = var?var:"";
                }
                if(stricmp(sqlite3_column_name(stmt,i) ,"pk")==0) is_pk = (0 != sqlite3_column_int(stmt, i));
            }
            if(is_pk)
                info.pks.insert(name);
            info.columns.insert(name);
        }
        sqlite3_finalize(stmt);
        _snapshoot[(*it)["tbl_name"]] = info;
    }
}
void SqliteWrap::create_snapshoot(const std::string&table){
    SqliteWrap::Rows rows;
    exec("select tbl_name from sqlite_master where type = \'table\' and tbl_name=\'"+table+"\'",rows);
    for(SqliteWrap::Rows::iterator it = rows.begin();it!=rows.end();it++){
        sqlite3_stmt *stmt =0;
        TableInfo info;
        VERIFY_TRUE(SQLITE_OK == sqlite3_prepare(_db, (std::string("pragma table_info (\'")+(*it)["tbl_name"]+"\')").c_str(), -1, &stmt, 0)) 
            << "pragma table_info failed with "
            << sqlite3_errmsg(_db);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            int count = sqlite3_column_count(stmt);
            std::string name;
            bool is_pk = false;
            for(int i=0;i<count;i++){
                if(stricmp(sqlite3_column_name(stmt,i) ,"name")==0){
                    const char* var = (const char *)sqlite3_column_text(stmt, i);
                    name = var?var:"";
                }
                if(stricmp(sqlite3_column_name(stmt,i) ,"pk")==0) is_pk = (0 != sqlite3_column_int(stmt, i));
            }
            if(is_pk)
                info.pks.insert(name);
            info.columns.insert(name);
        }
        sqlite3_finalize(stmt);
        _snapshoot[(*it)["tbl_name"]] = info;
    }
}
std::string SqliteWrap::get_user_version(){
    SqliteWrap::Rows rows;
    exec("PRAGMA user_version;",rows);
    if(rows.empty()) return "";
    return rows.front()["user_version"];
}