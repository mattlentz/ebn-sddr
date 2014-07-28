package edu.umd.cs.AWake;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BootReceiver extends BroadcastReceiver
{
  private static final String TAG = "BootReceiver";

  @Override
  public void onReceive(Context context, Intent intent) {
    if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
      Intent startWakeServiceIntent = new Intent(AWakeService.ACTION_START_SERVICE);
      context.startService(startWakeServiceIntent);
    }
  }
}
