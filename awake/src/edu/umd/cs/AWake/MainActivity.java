package edu.umd.cs.AWake;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends Activity
{
  private static String TAG = "MainActivity";
  
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
   
    Log.v(TAG, "Starting up the AWakeService..."); 
    Intent startWakeServiceIntent = new Intent(AWakeService.ACTION_START_SERVICE);
    startService(startWakeServiceIntent);
  }
}
