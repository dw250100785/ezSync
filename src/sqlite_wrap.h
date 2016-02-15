/**
* $Id: sqlite_wrap.h 984 2014-07-08 07:03:48Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include <string>
#include <set>
#include <map>
#include <list>

typedef struct sqlite3 sqlite3;
/**
* sqlite 简单包装
*/
class SqliteWrap{
public:
    SqliteWrap(const std::string&db);
    ~SqliteWrap();
    std::set<std::string> get_tables();
    typedef std::map<std::string,std::string> Row;
    typedef std::list<Row> Rows;
    void exec(const std::string&sql,Rows& rows);
    void exec(const std::string&sql);
    std::set<std::string> get_pks(const std::string&table);
    std::set<std::string> get_columns(const std::string&table);
    void create_snapshoot();
    void create_snapshoot(const std::string&table);
    bool is_table_exist(const std::string& table);
    std::string get_user_version();
private:
    struct TableInfo{
        std::set<std::string> pks;
        std::set<std::string> columns;
    };
    typedef std::map<std::string,TableInfo> Snapshoot;
    static int callback(void*p,int count,char**value ,char**name);
    sqlite3* _db;
    Snapshoot _snapshoot;
};
