package com.caoym.ezsync;
/**
 * 定义冲突时的合并策略
 * @author caoym
 *
 */
public interface MergeStrategy {

	public final static int UseLocal =0;
	public final static int UseRemote=1;
	public final static int Unknown=-1;
	/**
	 * 
	 * @param key 冲突项目
	 * @param local 冲突项目的本地信息
	 * @param remote 冲突项目的远端信息
	 * @return UseLocal 使用本地项目,UseRemote 使用远端项目
	 */
	public int merge(String key,Entry local, Entry remote);
}
