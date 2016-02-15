package com.caoym.ezsync;
/**
 * 数据同步的结果
 * @author caoym
 *
 */
public class Changed {
	//不使用枚举类型是为了简化jni参数传递
	/**同步操作 删除被删除的项目*/
	public final static int RECOVER = 0;
	/**同步操作 新增*/
	public final static int NEW = 1;
	/**同步操作 修改*/
	public final static int MODIFY= 2;
	/**同步操作 删除*/
	public final static int DELETE= 3;
	/**同步操作 未知*/
	public final static int UNKNOWN = -1;
	/**同步项目名*/
	public String key = "";
	/**同步操作*/
	public int operator = UNKNOWN;
}
