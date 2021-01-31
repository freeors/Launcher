package org.libsdl.app;


import android.app.IntentService;
import android.content.Intent;
import android.util.Log;

public class ForOnDestroyService extends IntentService {
    private static final String TAG = "SDL";

    public static boolean mExitThread = false;

    /**
     * A constructor is required, and must call the super IntentService(String)
     * constructor with a name for the worker thread.
     */
    public ForOnDestroyService() {
        super("ForOnDestroyService");
        Log.i("SDL", "---ForOnDestroyService--- ForOnDestroyService(), " + System.currentTimeMillis());
    }

    /**
     * The IntentService calls this method from the default worker thread with
     * the intent that started the service. When this method returns, IntentService
     * stops the service, as appropriate.
     */
    @Override
    protected void onHandleIntent(Intent intent) {
        Log.i("SDL", "ForOnDestroyService--- onHandleIntent(), " + System.currentTimeMillis());

        while (!mExitThread) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                // Nom nom
            }
        }

        Log.i("SDL", "---ForOnDestroyService X, onHandleIntent(), " + System.currentTimeMillis());
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i("SDL", "ForOnDestroyService, onDestroy(), " + System.currentTimeMillis());
    }
}