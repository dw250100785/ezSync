/**
* $Id$ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#include <sys/stat.h>
#include <iostream>
#include <map>
#include <list>
#include <set>
#include "boost/algorithm/string.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "boost/algorithm/string.hpp"
#include "ezsync/sqlite_storage.h"
#include "sqlite3.h"
#include "ezsync/verify_true.h"
#include "sqlite_wrap.h"
//TODO: 行数据->json转换时,使用排序过的列名,防止列表顺序不一致导致json内容不一致
using namespace ezsync;
//TODO: 使用事务进行更新

//TODO: 增加__ezsync_modified_log_0数据清除机制
namespace ezsync{
class SqliteHelper : public SqliteWrap{
public:
    SqliteHelper(const std::string&db)
        :SqliteWrap(db)
        ,_bug602(false){
            if(is_table_exist("__ezsync_extend_data") && !is_table_exist("__ezsync_metadata")){
                _bug602 = true;
            }
            create_extend_table();
            create_metadata_table();
    }
    bool _bug602;//bug 602,__ezsync_extend_data表的_extend需要decode两次
    ~SqliteHelper(){
    }
    
   
    bool is_trigger_exist(const std::string& table,const std::string& trigger){
        std::ostringstream oss;
        oss << "SELECT name FROM sqlite_master "
            << "WHERE type = \'trigger\' AND name=\'"<<trigger<<"\' AND tbl_name=\'"<<table<<"\'";
        Rows rows;
        exec(oss.str(),rows);
        return !rows.empty();
    }
    void append_extend(const std::string&table,const std::string&row,Row&dest){
        if(is_table_exist("__ezsync_extend_data")){
            SqliteHelper::Rows extend;
            exec("select _extend from __ezsync_extend_data where _table =\'"+table+"\' and _row = "+row,extend);
            if(!extend.empty()){
                boost::property_tree::ptree doc;
                std::stringstream ss;
                ss << extend.front()["_extend"];
                boost::property_tree::json_parser::read_json(ss,doc);

                for(boost::property_tree::ptree::iterator it = doc.begin();
                    it != doc.end();
                    it++){
                        dest.insert(std::make_pair(it->first,decode_key(it->second.get_value<std::string>())));
                }
            }
        }
    }
    /**
     * 数据库表结构变化后,尝试将扩展数据重新导入原表中
     * 
     */
    void reload_extend(){
        if( is_table_exist("__ezsync_extend_data") ){
            SqliteHelper::Rows extend;
            exec("select _row,_table,_extend from __ezsync_extend_data",extend);
            for(SqliteHelper::Rows::iterator it = extend.begin();
                it != extend.end(); it++){
                    boost::property_tree::ptree doc;
                    std::stringstream ss;
                    ss << (*it)["_extend"];
                    boost::property_tree::json_parser::read_json(ss,doc);

                    std::set<std::string> columns= get_columns((*it)["_table"]);
                    std::ostringstream sql;
                    sql << "update "<<(*it)["_table"] <<" SET ";
                    size_t reload_cols = 0;
                    for(boost::property_tree::ptree::iterator item = doc.begin();
                        item != doc.end();
                        item++){
                            if(columns.find(item->first) == columns.end()){ //只更新存在的列
                                log::Debug() << "table: "<< (*it)["_table"] << " col: "<< item->first << " not exist while reload extend";
                                continue;
                            }
                            if(reload_cols !=0) {
                                sql << ",";
                            }
                            sql << item->first << " = \'";
                            if(_bug602){// 此bug导致需要decode两次
                                   sql << boost::replace_all_copy(decode_key(decode_key(item->second.get_value<std::string>())),"\'","\'\'" )  ;
                            }else{
                                   sql << boost::replace_all_copy(decode_key(item->second.get_value<std::string>()),"\'","\'\'" )  ;
                            }
                            sql <<"\' ";
                            reload_cols++;
                    }
                    if(reload_cols){
                        sql << " where rowid=" << (*it)["_row"];
                        exec(sql.str());
                    }
                    if(reload_cols == doc.size() 
                        ||( reload_cols !=0 && _bug602)//bug602, 数据丢弃,否则有些需要decode两次,有些只需要1次,难处理
                        ){
                        exec("delete from __ezsync_extend_data where _row="+ (*it)["_row"]+" and _table=\'"+(*it)["_table"]+"\'");
                    }
                    log::Debug() << "table: "<< (*it)["_table"] << " row: "<< (*it)["_row"]  << " extend reload";
            }
        }
        
    }
    
    void save_extend(const std::string& path,const boost::property_tree::ptree& extend){
        if(!extend.empty()){
            std::ostringstream oss;
            boost::property_tree::json_parser::write_json(oss,extend);
            Rows rows;
            exec("select rowid "+path_to_query(path), rows);
            VERIFY_TRUE(!rows.empty())<<path <<" not exist";
            create_extend_table();
            exec("insert or replace into __ezsync_extend_data (_table,_row,_extend) VALUES (\'" +get_table_name(path) + "\'," +rows.front()["rowid"]+",\'"+oss.str()+"\');");
        }
    }
    std::string get_ezsync_metadata(const std::string&key){
         Rows rows;
         exec("select _value from __ezsync_metadata where _key=\'"+key+"\';",rows);
         if(rows.empty()) return "";
         return rows.front()["_value"];
    }
    void set_ezsync_metadata(const std::string&key, const std::string& value){
         Rows rows;
         exec("insert or replace into __ezsync_metadata (_key,_value) VALUES (\'"+key+"\',\'"+value+"\');");
    }
    bool create_extend_table(){
        if(!is_table_exist("__ezsync_extend_data")){
            exec("BEGIN;\n"
            "CREATE TABLE __ezsync_extend_data ( \n"
            "_mtime TIMESTAMP DEFAULT (strftime('%s','now')),\n"
            "_row INTEGER,\n"
            "_table VARCHAR(128),\n"
            "_extend TEXT,\n"
            "primary key (_row,_table)\n"
            ");\n"
            "COMMIT;"
            );
            create_snapshoot("__ezsync_extend_data");
            log::Debug()<<"create __ezsync_extend_data OK";
            return true;
        }
        return false;
    }
     bool create_metadata_table(){
        if(!is_table_exist("__ezsync_metadata")){
            exec("BEGIN;\n"
            "CREATE TABLE __ezsync_metadata ( \n"
            "_key VARCHAR(128) PRIMARY KEY,\n"
            "_value TEXT\n"
            ");\n"
            "COMMIT;"
            );
            create_snapshoot("__ezsync_metadata");
            log::Debug()<<"create __ezsync_metadata OK";
            return true;
        }
        return false;
    }
    /**创建触发器*/
    bool create_trigger(const std::string& table){
        bool created = false;
        //先判断日志表是否存在,如果不存在,先创建

        if(!is_table_exist("__ezsync_modified_log_0")){
            exec("BEGIN;\n"
            "CREATE TABLE __ezsync_modified_log_0 ( \n"
            "_mtime TIMESTAMP DEFAULT (strftime('%s','now')),\n"
            "_row INTEGER,\n"
            "_table VARCHAR(128),\n"
            "_op VARCHAR(32),\n"
            "primary key (_row,_table)\n"
            ");\n"
            "COMMIT;"
            );
            log::Debug()<<"create __ezsync_modified_log_0 OK";
            created = true;
        }
        //这个版本的trigger有问题
        if(is_trigger_exist(table,std::string("__ezsync_trigger_")+table+"_insert")){
            exec("DROP TRIGGER " + std::string("__ezsync_trigger_")+table+"_insert");
        }
        if(is_trigger_exist(table,std::string("__ezsync_trigger_")+table+"_update")){
            exec("DROP TRIGGER " + std::string("__ezsync_trigger_")+table+"_update");
        }
        if(is_trigger_exist(table,std::string("__ezsync_trigger_")+table+"_del")){
            exec("DROP TRIGGER " + std::string("__ezsync_trigger_")+table+"_del");
        }
        const char*triggers[]={"insert","update","delete"};
        for(size_t i = 0; i<sizeof(triggers)/sizeof(triggers[0]); i++){
            if(!is_trigger_exist(table,std::string("__ezsync_trigger_0_") +table+"_"+triggers[i])){
                std::ostringstream oss;
                oss << "CREATE TRIGGER __ezsync_trigger_0_"<<table<<"_"<<triggers[i]<<" AFTER "<<triggers[i] <<"\n"
                    << "ON "<<table<<"\n"
                    << "FOR EACH ROW"<<"\n"
                    << "BEGIN"<<"\n"
                    << "INSERT OR REPLACE INTO __ezsync_modified_log_0 (_row,_table,_op) VALUES (new.rowid,\'"<<table<<"\',\'"<<triggers[i]<<"\');"<<"\n"
                    << "END;";
                exec(oss.str());
                log::Debug()<<"create trigger on " << table <<" "<<triggers[i] << " OK";
                created = true;
            }

        } 
        return created ;
        
    }
    std::string path_to_select(const std::string& path){
        return std::string("select * ")+path_to_query(path);
    }
    std::string get_table_name(const std::string& path){
        VERIFY_TRUE(!path.empty()) << "request table name!" ;
        std::vector<std::string> data;
        boost::algorithm::split(data,path,boost::algorithm::is_any_of("/"));
        if(data.empty())  return std::string(" from ")+ path;
        VERIFY_TRUE(data.size() == 2) <<"invalid path " <<  path;
        return data[0];
    }
    std::string path_to_query_key(const std::string&path){
        VERIFY_TRUE(!path.empty()) << "request table name!" ;

        std::vector<std::string> data;
        boost::algorithm::split(data,path,boost::algorithm::is_any_of("/"));
        if(data.empty())  return "";
        VERIFY_TRUE(data.size() == 2) <<"invalid path " <<  path;

        if(!data[1].empty() && data[1][0] == '?'){
            // 路径类型
            // path/?a=123&b=456
            std::set<std::string> table_pks = get_pks(data[0]);
            VERIFY_TRUE(!table_pks.empty()) <<"none PK table can not be sync" <<  path; // 目前只能同步有主键的表
            std::set<std::string> query_pks;
            std::vector<std::string> vars;
            std::string query = data[1].substr(1);
            boost::algorithm::split(vars,query,boost::algorithm::is_any_of("&"));
            VERIFY_TRUE(data.size() == 2) <<"invalid path " <<  path;
            std::ostringstream oss;
            oss << " ";
            for(std::vector<std::string>::iterator it = vars.begin(); it != vars.end(); it++){
                std::vector<std::string> pair;
                boost::algorithm::split(pair,*it,boost::algorithm::is_any_of("="));
                VERIFY_TRUE(pair.size() == 2) <<"invalid path " <<  path;
                query_pks.insert(pair[0]);
                if(it != vars.begin() ) oss << " and ";
                oss << pair[0] << " = \'" << pair[1] << "\'";
            }
            //TODO: 那么,以后升级数据库必须保证主键一致
            VERIFY_TRUE(table_pks == query_pks) <<"invalid PK query " <<  path;
            return oss.str();

        }else{
            // 路径类型
            // path/123
            //此路径描述方式只能支持一个主键的情况
            std::set<std::string> pks = get_pks(data[0]);
            VERIFY_TRUE(pks.size() == 1) <<"composite multiple PK with " <<  path;
            std::ostringstream oss;
            oss << " " << *pks.begin() << " = \'" << data[1] << "\'";
            return oss.str();
        }
    }
    std::string path_to_query(const std::string&path){
        VERIFY_TRUE(!path.empty()) << "request table name!" ;

        std::vector<std::string> data;
        boost::algorithm::split(data,path,boost::algorithm::is_any_of("/"));
        if(data.size() == 1 || (data.size() == 2 && data[1].empty()))  return std::string(" from ")+ path;
        VERIFY_TRUE(data.size() == 2) <<"invalid path " <<  path;

        std::ostringstream oss;
        oss << " from " << data[0] << " where " + path_to_query_key(path);
        return oss.str();

    }
    std::string query_to_path(const std::string&path,const Row& row){
        std::vector<std::string> query;
        boost::algorithm::split(query,path,boost::algorithm::is_any_of("/"));
        if(query.size()>1) return path;

        std::ostringstream oss;
        oss<< path <<"/?";
        std::set<std::string> table_pks = get_pks(path);
        VERIFY_TRUE(!table_pks.empty()) <<"none PK table can not be sync" <<  path; // 目前只能同步有主键的表
        for(std::set<std::string>::iterator it = table_pks.begin() ;it!= table_pks.end() ;it++){
            if(it != table_pks.begin())  oss << "&";
            Row::const_iterator var = row.find(*it);
            VERIFY_TRUE(var != row.end()) << "find PK failed";
            oss << *it <<"=" << var->second <<"";
        }
        return oss.str();
    }
};

SqliteStorage::SqliteStorage(const std::string&root,const std::string &include/* = "*"*/,const std::string &except /*= ""*/)
    :_root(root)
    ,_path_clip(include,except.empty()?"__ezsync_*\nsqlite_*\nandroid_metadata*":(except+"\n__ezsync_*\nsqlite_*\nandroid_metadata*")){
        _db.reset(new SqliteHelper(_root));
        /*
        TODO: 使用触发器记录修改时间，用于优化递交时修改项目扫描速度，未证明真的有效，不过会导致更新冲突时，如果使用了本地项目，下次递交时无法扫描这些项目。
        //为所有需要同步的表添加触发器,用于记录数据修改
        SqliteHelper db(_root);
        std::set<std::string> table = db.get_tables();

        for(std::set<std::string>::iterator it = table.begin(); it!=table.end(); it++){
            if(_path_clip.include(*it)){
                if(db.create_trigger(*it)){
                    db.exec("INSERT OR REPLACE into __ezsync_modified_log_0 (_row,_table,_op) select rowid,\'"+*it+"\',\'insert\' from " + *it);
                }
            }
        }*/
}
const std::string& SqliteStorage::root()const{
    return _root;
}

void SqliteStorage::del(const std::string&key){
    if(!_path_clip.include(key)){
        log::Warn()<<"try to delete EXCEPT key " << key;
        return ;
    }
    get_db().exec( "delete "+ get_db().path_to_query(key));
}
std::string get_json(const SqliteHelper::Row &row){
    std::ostringstream oss;
    boost::property_tree::ptree doc;

    for(SqliteHelper::Row::const_iterator it = row.begin(); it != row.end(); it++){
        //这里对value进行自定义编码,因为默认json编码会把字符串当作utf8处理,
        //如果value使用其他编码,经过json编码再解码后无法还原成原始文本
        doc.put(it->first,encode_key(it->second)); 
    }
    boost::property_tree::write_json(oss,doc);
    return oss.str();
}
Entry SqliteStorage::get_entry(const std::string& key){ // TODO: 触发器
    if(!_path_clip.include(key)){
        log::Warn()<<"try to get EXCEPT key " << key;
        return Entry();
    }
    
    std::string table = get_db().get_table_name(key);
    if(!get_db().is_table_exist(table)) {
        log::Warn() << "table :" << table << " not exists with " << key;
        return Entry();
    }
    SqliteHelper::Rows rows;
    get_db().exec("select *,rowid "+get_db().path_to_query(key),rows);
    if(rows.empty()) return Entry(); // TODO: 超过一个呢?
    else {
        if(rows.size() >1 ){
            log::Warn() << "multiple rows found by "<< key;
        }
        get_db().append_extend(table,rows.front()["rowid"],rows.front());

        Entry e;
        rows.front().erase("rowid");
        std::string json = get_json(rows.front());
        e.size = json.size();
        e.md5 = get_text_md5(json);
        e.content = json;
        /*std::ostringstream sql;
        sql<< "SELECT __ezsync_modified_log_0._mtime FROM __ezsync_modified_log_0 "
           << "LEFT JOIN " << table <<" ON __ezsync_modified_log_0._row = "<<table<<".rowid "
           << " WHERE __ezsync_modified_log_0._table=\'"<<table<<"\' AND " << get_db().path_to_query_key(key);
        SqliteHelper::Rows row;
        get_db().exec(sql.str(),row);
        if(!row.empty())
            e.modified_time = strtol(row.front()["_mtime"].c_str(),0,10);*/
        return e;
    }
}
std::map<std::string,Entry> SqliteStorage::get_all_entries(
    const std::string& path,//TODO 路径还是文件名
    unsigned int modify_time){
        
        std::map<std::string,Entry> entrys;
        //SqliteHelper db(_root);
        {
            std::set<std::string> tables;
            if(path.empty()){
                tables = get_db().get_tables();
            }
            else {
                tables.insert(get_db().get_table_name(path));
            }
            for(std::set<std::string>::iterator it = tables.begin();it!=tables.end();it++){
                if(!_path_clip.include(*it + "/")) {
                    log::Debug() << " skip EXCEPT table : " << *it;
                    continue;
                }
                if(get_db().get_pks(*it).empty()) {
                    log::Debug() << " skip no pk table : " << *it;
                    continue; // 只能扫描存在主键的表
                }

                std::ostringstream oss;
                ///if(get_db().create_trigger(*it)){ //刚创建,之前还没有日志记录,所以取所有项作为修改项
                if(path.empty()) {
                    oss<< "SELECT rowid,* FROM "<<(*it);
                }else{
                    oss<<"SELECT rowid,* "<<get_db().path_to_query(path);
                }

               
                ///}else{
                ///    oss<< "SELECT DISTINCT "<<(*it)<<".*,__ezsync_modified_log_0._mtime FROM __ezsync_modified_log_0 "
                ///        << "LEFT JOIN " << *it <<" ON __ezsync_modified_log_0._row = "<<*it<<".rowid "
                ///        << " WHERE __ezsync_modified_log_0._table=\'"<<*it<<"\' AND __ezsync_modified_log_0._mtime >= " <<modify_time 
                ///        << " AND __ezsync_modified_log_0._row = " <<*it<<".rowid ";
                ///}
                SqliteHelper::Rows rows;
                get_db().exec(oss.str(),rows);
                

                for(SqliteHelper::Rows::iterator row = rows.begin(); row != rows.end(); row++){
                    std::string key = get_db().query_to_path(*it,*row);
                    if(!_path_clip.include(key)) {
                        log::Debug() << " skip EXCEPT key : " << key;
                        continue;
                    }
                    get_db().append_extend(*it,(*row)["rowid"],*row);
                    row->erase("rowid");
                    Entry e;
                    //e.modified_time = strtol((*row)["_mtime"].c_str(),0,10);
                    //row->erase("_mtime");//_mtime不属于这个表
                    std::string json = get_json(*row);
                    e.size = json.size();
                    e.md5 = get_text_md5(json);
                    e.content = json;
                    
                    entrys[key] = e;
                }
            }
        }
        //TODO: 如果path/?a=1&b=2 
        return entrys;
}
void SqliteStorage::set_entry(const std::string&path, std::istream& ins){
    if(!_path_clip.include(path)){
        log::Warn()<<"try to set EXCEPT key " << path;
        return ;
    }
  
    
    std::string table = get_db().get_table_name(path);
    // TODO: 不存在的表要不要创建?或者抛出异常?
    // 目前直接忽略不存在的表,同步时不会报错
    if(!get_db().is_table_exist(table)) {
        log::Warn() << "table :"<< table << " not exists,just skip";
        return ;
    }
    std::set<std::string> pks = get_db().get_pks(table);
    boost::property_tree::ptree doc;
    boost::property_tree::json_parser::read_json(ins,doc);
    //数据需要包含当前表的所有主键,否则无法插入
    for(std::set<std::string>::iterator it = pks.begin();
        it != pks.end();
        it++ ){
            VERIFY_TRUE(doc.get_optional<std::string>(*it));
    }
   
    std::set<std::string> columns =  get_db().get_columns(table);
    std::ostringstream sql;
    //if(rows.empty()){ //insert
        std::ostringstream keys;
        std::ostringstream values;
        boost::property_tree::ptree extend;//本地不存在的列,其数据保存到extend表,确保同步时这写数据不会丢失。
        sql << "insert or replace into "<<table <<" ( ";
        bool first_col = true;
        for(boost::property_tree::ptree::iterator it = doc.begin();
            it != doc.end();
            it++){
                if(columns.find(it->first) == columns.end()){ //只插入存在的列
                    log::Debug() << "table: "<< table << " col: "<< it->first << " not exist";
                    extend.add(it->first,it->second.get_value<std::string>());
                    continue;
                }
               
                if(!first_col) {
                    values << ", ";
                    keys << ", ";
                }
                keys << it->first;
                values << "\'"
                    << boost::replace_all_copy(decode_key(it->second.get_value<std::string>()),"\'","\'\'") 
                    <<"\' ";
                first_col = false;
        }
        sql << keys.str() << " ) " << " VALUES ( " << values.str() << " ) ";

        
       
    /*}else{ //updata
        sql << "update "<<table <<" SET ";
        bool first_col = true;
        for(boost::property_tree::ptree::iterator it = doc.begin();
            it != doc.end();
            it++){
                if(columns.find(it->first) == columns.end()){ //只更新存在的列
                    log::Debug() << "table: "<< table << " col: "<< it->first << " not exist";
                    continue;
                }
                if(!first_col) {
                    sql << ",";
                }
                sql << it->first << " = \'"
                    << boost::replace_all_copy(decode_key(it->second.get_value<std::string>()),"\'","\'\'" )  
                    <<"\' ";
                first_col = false;
        }
        sql << " where " << get_db().path_to_query_key(path);
    }*/
    get_db().exec(sql.str()); 
    //内部触发的修改(如update()),清除数据库日志,防止下次同步被当作修改
    /*if(get_db().is_table_exist("__ezsync_modified_log_0")){
        std::ostringstream oss ;
        oss << "select rowid from " << table << " where " << get_db().path_to_query_key(path);
        rows.clear();
        get_db().exec(oss.str(),rows); 
        for(SqliteHelper::Rows::iterator it = rows.begin(); it!=rows.end();it++){
            get_db().exec("delete from __ezsync_modified_log_0 where _table = \'"+table+"\' and _row="+(*it)["rowid"]); 
        }
    }*/
    get_db().save_extend(path,extend);
}
void SqliteStorage::set_entry(const std::string&path,const std::string& json){
    if(!_path_clip.include(path)){
        log::Warn()<<"try to set EXCEPT key " << path;
        return ;
    }
    std::stringstream ss;
    ss.write(json.c_str(),json.size());
    set_entry(path,ss);
}
void SqliteStorage::begin_transaction(){
    get_db().exec("BEGIN");
}
void SqliteStorage::commit_transaction(){
    get_db().exec("COMMIT");
}
void SqliteStorage::upgrade(){
    get_db().reload_extend();
}
}