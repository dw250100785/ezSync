package com.caoym.ezsync;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

import org.json.JSONObject;
/**
 * 存储方式描述,用于初始化Client对象
 * @author caoym
 *
 */
public class StorageDesc {
	/**
	 * 从json创建
	 * @param str json内容
	 * @return StorageDesc
	 * @throws Throwable
	 */
	public static StorageDesc createFromJson(String str) throws Throwable{
		JSONObject json = new JSONObject(str);
		Iterator<String> it = json.keys();  
        String key;  
        Map<String, String> conf = new HashMap<String,String>();  
        while (it.hasNext()) {  
            key = it.next();  
            conf.put(key, json.getString(key));  
        }  
        return new StorageDesc(conf);  
	}
	/*public void put(String key,String val){
		_config.put(key,val);
	}*/
	/**
	 * 设置本地存储路径
	 * @param path 本地存储路径
	 */
	public void setLocalStorage(String path){
		_config.put("local_storage",path);
	}
	/**
	 * 设置远端存储路径
	 * @param path 远端存储路径
	 */
	public void setRemoteStorage(String path){
		_config.put("remote_storage",path);
	}
	/**
	 * 设置访问令牌
	 * @param bduss 访问令牌
	 */
	public void setBDUSS(String bduss){
		_config.put("bduss",(bduss == null) ?"":bduss);
	}
	/**
	 * 设置包含哪些路径
	 * @param include 排除路径
	 * 1. * 所有路径
	 * 2. abc/*.txt abc/路径下所有以.txt结尾的
	 * 3. abc/*.txt* abc/路径下所有包含.txt字符串的
	 */
	public void setInclude(String []include){
		
		String str = "";
		if(include != null){
			for(String i : include){
				str += i;
				str += "\n";
			}
		}
		_config.put("include",str);
	}
	public void setHttpHeaders(Map<String, String> headers){
		String str = "";
		if(headers != null){
			for(Entry<String, String> i : headers.entrySet()){
				str += i.getKey();
				str += ":";
				str += i.getValue();
				str += "\n";
			}
		}
		_config.put("http_headers",str);
	}
	/**
	 * 设置排除哪些路径
	 * @param except @see setInclude
	 */
	public void setExcept(String []except){
		String str = "";
		if(except !=null){
			for(String i : except){
				str += i;
				str += "\n";
			}
		}
		_config.put("except",str);
	}
	
	/**
	 * 设置uid
	 * @param uid 
	 */
	public void setUID(String uid){
		_config.put("uid",(uid == null) ?"":uid);
	}
	/**
	 * 设置uid
	 * @param uid 
	 */
	public void setDeviceID(String id){
		_config.put("device_id",(id == null) ?"":id);
	}
	/**
	 * 获取当前的配置信息
	 * @return 配置信息,用于初始化Client对象
	 */
	public Map<String, String> getConfig(){
		if(!_config.containsKey("local_history")){
				_config.put("local_history", _config.get("local_storage"));  
		}
		if(!_config.containsKey("remote_history")){
			_config.put("remote_history", _config.get("remote_storage"));  
		}
		return _config;
	}
	private StorageDesc(Map<String, String> config){
		_config = config;
	}
	private Map<String, String> _config;
}
