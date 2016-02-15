# ezSync
cross-platform sync tool for File and Sqlite
## WHAT
ezync 提供简单的方式用于将数据保存到云端服务器，以及在不同设备间同步数据。其特性包括：

- 支持对文件和目录的同步，更新的最小单位是文件，对文件格式无要求
- 支持对sqlite数据库的增量更新同步，更新的最小单位是行，要求同步的数据库表必须有PK。能自动处理数据库表升级导致的字段增减问题。
- 基于版本控制的多设备间同步
- 严格的原子性控制，防止更新过程中数据损坏
- 能够检查到版本冲突，并提供途径由上层应用解决冲突
- 简单的服务端实现，只需实现基本的RESTful接口：GET、POST、PUT、HEAD、MOVE、DELETE
- 支持IOS、Android、Linux、Windows

典型使用场景：
- 笔记类应用，如有道笔记等。
- 配置项下发
- 客户端日志收集

## HOW

### C++
```CPP
//版本冲突处理策略
//冲突时总是使用本地版本
class UseLocal:public IEzsyncMergeStrategy{
public:
    ezsync::UseWitch merge(const std::string& key,const ezsync::Entry&,const ezsync::Entry&){
        std::cout << key << " merged:use_local"<<std::endl;
        return USE_LOCAL;
    }
}_use_local;


std::shared_ptr<IEzsyncClientWrap> client;
std::map<std::string, std::string> map ;
map["local_type"]="file";
map["host"]="xxx.com:443";  //服务器地址
map["local_storage"]="/home/caoym/testfles/"; //需同步的数据的本地地址
map["remote_storage"]="/testfles/"; //需同步的数据的远端地址

map["local_history"]=path.UTF8String; //本地版本信息存放地址
map["remote_history"]=remote.UTF8String; //远端版本信息存放地址
//创建客户端
if(map["local_type"] == "file"){
    client.reset(new EzsyncClientWrap<ezsync::FileSyncClient2>(ezsync::FileSyncClientBuilder2::build(map)));
}else if(map["local_type"] == "sqlite"){
    client.reset(new EzsyncClientWrap<ezsync::SqliteSyncClient2>(ezsync::SqliteSyncClientBuilder2::build(map)));
}
//下载
_client->update(_use_local, "", "", NULL);
//上传
client->commint("");
```

### Android
```JAVA
//用户数据库同步
String user_sqlite_sync = "{\"local_type\":\"sqlite\",\"host\":\"10.94.16.61:8111\",\"encrypt\":\"1\"}";
SQLiteDatabase db =  _db.getReadableDatabase();
String db_path = db.getPath();
db.close();

StorageDesc storage = StorageDesc.createFromJson(user_sqlite_sync);
storage.setLocalStorage(db_path); //本地存储路径
storage.setRemoteStorage("testdb"); // 云端存储路径

Client client = new Client(storage.getConfig());
//下载
client.update(new MergeStrategy() {
		@Override
		public int merge(String key, Entry local, Entry remote) {
			System.out.println(key + " use remote");
			return MergeStrategy.UseRemote;
		}
		
	});
//上传
client.commit();
```
### iOS
```OBJC
SyncClient*client = [[SyncClient alloc] initWithLocal:"xxxx" 
                                              forType:@"file"
                                             withHost:@"127.0.0.1:443" 
                                          withRemote:"/tests"];
//下载
[client update];
//上传
[client commit];
```
### 依赖
- curl
- boost
- openssl
- rapidjson
        
