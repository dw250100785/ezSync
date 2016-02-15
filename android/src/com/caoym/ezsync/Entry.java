package com.caoym.ezsync;
/**
 * 表示一个更新的条目
 * @author caoym
 *
 */
public class Entry {
	/**
	 * 修改时间
	 */
    public long modifiedTime;  
    /**
     * 条目大小
     */
    public long size;
    /**
     * 由Client决定,如果是文件,content中包含的是文件路径,如果是数据库,content是行的json格式的文本
     */
    public String content = new String();  
}
