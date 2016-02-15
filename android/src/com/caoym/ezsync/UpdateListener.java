package com.caoym.ezsync;
/**
 * 接收Update完成事件
 * @author caoym
 *
 */
public interface UpdateListener {
	/**
	 * update执行前,通知将有哪些项目需要更新，返回false将取消更新
	 */
	//public boolean preUpdate(Changed[] changed);
	/**
	 * update完成后调用，如果返回false,本次update将被当作失败
	 */
	public boolean postUpdate(Changed[] changed);
}
