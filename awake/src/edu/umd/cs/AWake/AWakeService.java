package edu.umd.cs.AWake;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.os.IBinder;
import android.os.PowerManager;
import android.util.Log;

public class AWakeService extends Service
{
  private static final String TAG = "AWakeService";
  
  public static final String ACTION_START_SERVICE = "edu.umd.cs.AWake.ACTION_START_SERVICE";
  public static final String ACTION_STOP_SERVICE = "edu.umd.cs.AWake.ACTION_STOP_SERVICE";
  public static final String ACTION_WAKE = "edu.umd.cs.AWake.ACTION_WAKE";
  
  private Server server = null;
  private Thread serverThread = null;

  @Override
  public void onCreate()
  {
    super.onCreate();
  }

  @Override
  public void onDestroy() {
    super.onDestroy();

    server.close();
    server = null;
    serverThread = null;
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startID) {
    if ((intent != null) && (intent.getAction() != null))
    {
      if (intent.getAction().equals(ACTION_START_SERVICE))
      {
        if (server == null)
        {
          server = new Server(this);
          serverThread = new Thread(server);
          serverThread.start();
        }
      }
      else if (intent.getAction().equals(ACTION_STOP_SERVICE))
      {
        stopSelf();
      }
    }

    return START_STICKY;
  }

  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }

  private class Server implements Runnable 
  {
    private Context context = null;
    private LocalServerSocket serverSock = null;
    private LocalSocket clientSock = null;
    private BufferedInputStream istream = null;
    private BufferedOutputStream ostream = null;
    private PowerManager.WakeLock wakelock = null;
    private AlarmManager alarmMgr = null;
    private PowerManager powerMgr = null;

    Server(Context context)
    {
      this.context = context;
      alarmMgr = (AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
      powerMgr = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
      wakelock = powerMgr.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Sleeper");

      IntentFilter filter = new IntentFilter();
      filter.addAction(ACTION_WAKE);
      registerReceiver(brecv, filter);
    }

    public void alert()
    {
      if (clientSock != null)
      {
        try {
          ostream.write(1);
          ostream.flush();
        }
        catch(IOException e) { }
      }
    }

    public void close() {
      try
      {
        serverSock.close();
        clientSock.close();
        unregisterReceiver(brecv);
      }
      catch (Exception e) { }
    }

    public void run() {
      try {
        serverSock = new LocalServerSocket("AWake");
      } catch (IOException e) {
        Log.v(TAG, "Could not open local server socket 'AWake'");
        return;
      }

      while (true) {
        try {
          Log.v(TAG, "Waiting to accept a client connection...");
          
          clientSock = serverSock.accept();
          istream = new BufferedInputStream(clientSock.getInputStream());
          ostream = new BufferedOutputStream(clientSock.getOutputStream());
          
          wakelock.acquire();

          try
          {
            byte[] buffer = new byte[4];
            while (istream.read(buffer) != -1)
            {
              ByteBuffer wbuffer = ByteBuffer.wrap(buffer);
              wbuffer.order(ByteOrder.LITTLE_ENDIAN);
              
              int sleepTime = wbuffer.getInt(); 
              Log.v(TAG, "Sleeping for " + sleepTime + " ms");
              
              Intent intent = new Intent(ACTION_WAKE);
              PendingIntent pIntent = PendingIntent.getBroadcast(context, 0, intent, 0);
              alarmMgr.set(AlarmManager.RTC_WAKEUP, System.currentTimeMillis() + sleepTime, pIntent);
            }
          }
          catch (IOException e) { }

          Log.v(TAG, "Communication ended with the client");

          if(wakelock.isHeld())
          {
            wakelock.release();
          }

          Intent intent = new Intent(ACTION_WAKE);
          PendingIntent pIntent = PendingIntent.getBroadcast(context, 0, intent, 0);
          alarmMgr.cancel(pIntent);
        }
        catch (Exception e)
        {
          Log.v(TAG, "Shutting down the server");
          e.printStackTrace();
          close();
          break;
        }
      }
    }

    private final BroadcastReceiver brecv = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent)
      {
        if(intent.getAction().equals(ACTION_WAKE))
        {
          alert();
        }
      }
    };
  }
}
