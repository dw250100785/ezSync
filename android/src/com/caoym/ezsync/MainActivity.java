package com.caoym.ezsync;

import java.io.File;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.HashMap;
import java.util.Map;

import android.os.Bundle;
import android.os.Environment;
import android.app.Activity;
import android.content.ContentValues;
import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class MainActivity extends Activity {

	public class DBHelper extends SQLiteOpenHelper {  
		 
	    private static final String TAG = "testdb";  
	    public static final int VERSION = 1;  

	    public DBHelper(Context context, String name, CursorFactory factory,  
	            int version) {  
	        super(context, name, factory, version);  
	    }  
	 
	    public void onCreate(SQLiteDatabase db) {  
	        String sql = "CREATE TABLE note_label ( id     VARCHAR( 128 )  NOT NULL,noteId VARCHAR( 128 )  NOT NULL,test   TEXT, PRIMARY KEY ( id, noteId ));";  
	        db.execSQL(sql);  
	    }  
	 
	    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {  

	    }  
	}  
	   
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		Button update = (Button)findViewById(R.id.update);
		update.setOnClickListener( new OnClickListener(){
			@Override
			public void onClick(View arg0) {
				testUpdate();
			}
		});
		Button delete = (Button)findViewById(R.id.delete);
		delete.setOnClickListener( new OnClickListener(){
			@Override
			public void onClick(View arg0) {
				testDelete();
			}
		});
		Button commit = (Button)findViewById(R.id.commit);
		commit.setOnClickListener( new OnClickListener(){
			@Override
			public void onClick(View arg0) {
				testCommit();
			}
		});
		_db = new DBHelper(this,"testdb",null,1);  
		AppSetting.enableLogger(Environment.getExternalStorageDirectory().toString(), "lifenote", AppSetting.INFO);
		try {

			
			//用户文件同步
			String user_file_sync = "{\"local_type\":\"file\",\"host\":\"127.0.0.1:80\",\"path\":\"/files\",\"encrypt\":\"1\"}";
			File path = new File(getFilesDir() + "/testezsync/local");
			if (!path.exists()) { path.mkdirs();}
			
			//用户数据库同步
			String user_sqlite_sync = "{\"local_type\":\"sqlite\",\"host\":\"10.94.16.61:8111\",\"path\":\"/db\",\"encrypt\":\"1\"}";
			SQLiteDatabase db =  _db.getReadableDatabase();
			String db_path = db.getPath();
			db.close();
			
			StorageDesc storage = StorageDesc.createFromJson(user_sqlite_sync);
			storage.setLocalStorage(db_path); //本地存储路径
			
			storage.setRemoteStorage("testdb"); // 云端存储路径
			//String [] ex = {"note_label/*","123/*","123/*"};
			//String [] include = {"note_label/*"};
			//storage.setExcept(ex); //设置排除项目,注意要带/
			Map<String, String> headers = new HashMap<String,String>();  
			headers.put("x-http-xxxxxxx", "123454555566");
			storage.setHttpHeaders(headers);
			
			_client = new Client(storage.getConfig());
		} catch (Throwable e) {
			e.printStackTrace();
		}
		
		
	}

	public void testCommit() {
		try {
			/*File file = new File(getFilesDir() + "/testezsync/local/1.txt");
			if (!file.exists()) { file.createNewFile();}
			FileOutputStream fout = new FileOutputStream(file);
			String s = "this is a test string writing to file.";
			byte[] buf = s.getBytes();
			fout.write(buf);
			fout.close();*/
			
			SQLiteDatabase db =  _db.getWritableDatabase();
			ContentValues values = new ContentValues();  
			SimpleDateFormat fmt=new SimpleDateFormat("yyyy-MM-ddhh:mm:ss");        
			values.put("id", fmt.format(new java.util.Date()));  
			values.put("noteId", "2"); 
			db.insert("note_label", "测试数据",values);
			db.close();
			
			_client.commit();
			
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}
	public void testUpdate(){
		try {
			 _client.update(new MergeStrategy() {
				@Override
				public int merge(String key, Entry local, Entry remote) {
					System.out.println(key + " use remote");
					return MergeStrategy.UseRemote;
				}
				
			});
			
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}
	public void testDelete(){
		try {
			_client.delete("note_label/?id=1&noteId=1");
			//commint
			_client.commit();
			
			
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	private Client _client;
	
	DBHelper _db;

}
