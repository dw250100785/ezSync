package com.caoym.ezsync;
/**
 * 应用相关的全局设置
 * @author caoym
 *
 */
public class AppSetting {
	public final static int DEBUG = 1;
	public final static int INFO = 2;
	public final static int WARN = 3;
	public final static int ERROR = 4;
	public final static int ALL = 0;
	public final static int RELEASE = INFO;
	static {  
        System.loadLibrary("ezsync");
    }
	static public void enableLogger(String logPath,String logName, int flag){
		setting.enableLog(logPath,logName,flag);
	}
	static public void disableLogger(){
		setting.disableLog();
	}
	/**
	 * 开启日志输出,重复调用无效
	 * @param logPath 日志路径
	 * @param logName 日志文件名
	 * @param flag 需要输出的日志,用|组合,如WARN|ERROR
	 */
	private native void enableLog(String logPath,String logName, int flag);
	/**
	 * 关闭日志
	 */
	private native void disableLog();
	private static AppSetting setting = new AppSetting();
}
