/***************************************************************************
* 
* 
* 
**************************************************************************/
/**
 * $Id: Client.java 986 2014-07-08 08:38:49Z caoyangmin $ 
 * @author caoyangmin(caoyangmin@gmail.com)
 * @brief ezsync client(文件同步客户端)
 */
package com.caoym.ezsync;
import java.util.Map;
/**
 * 文件和sqlite数据库同步工具,可以将本地的数据同步到远端,且在多个设备间维持数据的一致性
 * 内部使用版本方式管理数据项
 */
public class Client {
	static {  
        System.loadLibrary("ezsync");
    }
	

	/**
	 * @param conf 配置信息 使用 StorageDesc 初始化配置信息
	 */
	public Client(Map<String, String> conf)throws AuthFailException, Throwable {
		createJniObj(conf);
	}
	/**
	 * 递交指定路径下的修改项
	 * 如果远端版本与本地不一致,需先执行update
	 * @param path 相对路径,如果为空则更新全部
	 * @return 递交的条目
	 * @throws ...
	 */
	public native Changed[] commit(String path)throws AuthFailException, Throwable;
	/**
	 * 递交所有修改
	 * @return 递交的条目
	 * @throws Throwable
	 */
	public  Changed[] commit()throws AuthFailException, Throwable{
		return commit("");
	}
	/**
	 * 删除指定项目,执行commit后将删除远端项目
	 * 如果需要删除本地,但是不希望同步后删除远端项目,则不要使用该方法,而应该通过常规方法(如系统api)删除项目
	 * @param key 指定需要删除的项目
	 */
	public native void delete(String key) throws Throwable;
	
	
	/**
	 * 更新(下载)指定目录的项目
	 * @param mergeStrategy 合并策略,项目更新冲突时被调用,@see MergeStrategy
	 * @param include 包含的项目,支持通配符*,注意：如果需要下载的项目已知道，应直接指定项目名，而不是使用通配符*，应为那将导致下载多余的版本信息
	 * @param except 排除的项目,支持通配符*
	 * @param listener 事件监听
	 * @return 本次更新的项目
	 * @throws AuthFailException 验证失败,可能是cookie过期,bduss无效等,总之重新登录就对了
	 * @throws Throwable
	 */
	public Changed[] update(MergeStrategy mergeStrategy, String[] include, String[] except,UpdateListener listener)throws AuthFailException,Throwable{
		String include_str = "";
		if(include !=null){
			for(String i : include){
				include_str += i; include_str += "\n";
			}
		}
		String except_str = "";
		if(except !=null){
			for(String i : except){
				except_str += i; except_str += "\n";
			}
		}
		return update(mergeStrategy,include_str,except_str,listener);
	}
	/**
	 * @see update
	 */
	public Changed[] update(MergeStrategy mergeStrategy, String[] include, String[] except)throws AuthFailException,Throwable{
		return update(mergeStrategy,include,except,null);
	}
	/**
	 * 更新(下载)指定目录的项目
	 * @param mergeStrategy 合并策略,项目更新冲突时被调用,@see MergeStrategy
	 * @param include 包含的项目,多个项目以\n分隔,支持通配符*
	 * @param except 排除的项目,多个项目以\n分隔，支持通配符*
	 * @param update_level ONLY_UPDATE_MODIFIED UPDATE_MODIFIED_NEW UPDATE_MODIFIED_NEW_LOST
	 * @param listener
	 * @return 本次更新的项目
	 * @throws AuthFailException 验证失败,可能是cookie过期,bduss无效等,总之重新登录就对了
	 * @throws Throwable
	 */
	private native Changed[] update(MergeStrategy mergeStrategy, String include, String except,UpdateListener listener)throws AuthFailException,Throwable;
	
	/**
	 * @param mergeStrategy
	 * @return
	 * @throws AuthFailException
	 * @throws Throwable
	 */
	public Changed[] update(MergeStrategy mergeStrategy)throws AuthFailException,Throwable{
		return update( mergeStrategy,  "",  "",null);
	}
	
	/**
	 * 取消当前正在执行的操作
	 * 被终止的操作将抛出CaceledException异常
	 */
	public native void cancel();
	
	/**
	 * 更新本地存储数据结构，如更新数据库表结构 
	 */
	public native void upgradeLocalStorage()throws Throwable;
	
	@Override
	protected void finalize() {
		try {
			destroyJniObj();
			super.finalize();
		} catch (Throwable e) {
			System.out.println(e.toString());
		}

	}
	private native void destroyJniObj();
	private native void createJniObj(Map<String, String> config);
    /**
     * jni 内部使用,不要访问!
     */
    private long __jni_object=0;
}
