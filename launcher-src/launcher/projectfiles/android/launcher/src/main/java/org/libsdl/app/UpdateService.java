package org.libsdl.app;

import android.app.IntentService;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import androidx.core.content.FileProvider;
import android.util.Log;
import java.io.File;
import com.kos.launcher.BuildConfig;

public class UpdateService extends IntentService {
    private static final String TAG = "SDL";
    public static boolean mDestroyed;

    private final IBinder mBinder = new LocalBinder();
    /**
     * A constructor is required, and must call the super IntentService(String)
     * constructor with a name for the worker thread.
     */
    public UpdateService() {
        super("UpdateService");
        Log.i("SDL", "UpdateService, UpdateService(), " + System.currentTimeMillis());
    }

    /**
     * The IntentService calls this method from the default worker thread with
     * the intent that started the service. When this method returns, IntentService
     * stops the service, as appropriate.
     */
    @Override
    protected void onHandleIntent(Intent paramIntent) {
        Log.i("SDL", "UpdateService, 1, onHandleIntent(), " + System.currentTimeMillis());

        String filePath2 = paramIntent.getExtras().getString("apkname");
        String filePath = SDL.getContext().getExternalFilesDir(null).getAbsolutePath() + "/" + filePath2;

        File apkFile = new File(filePath);
        Log.i(TAG, "start install: " + filePath);

        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_VIEW);
        intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            Uri contentUri = FileProvider.getUriForFile(UpdateService.this.getApplicationContext(),
                        BuildConfig.APPLICATION_ID + ".fileprovider", apkFile);
            Log.i(TAG, "contentUri: " + contentUri.toString());
            intent.setDataAndType(contentUri, "application/vnd.android.package-archive");

        } else {
            Uri uri = Uri.fromFile(apkFile);
            Log.i(TAG, "uri: " + uri.toString());
            intent.setDataAndType(uri, "application/vnd.android.package-archive");
        }

        // start now
        UpdateService.this.startActivity(intent);
        stopSelf();

        Log.i("SDL", "UpdateService, 3, onHandleIntent(), " + System.currentTimeMillis());
    }

    @Override
    public IBinder onBind(Intent intent) {
        // mBound = true;
        Log.i("SDL", "UpdateService, onBind(), " + System.currentTimeMillis());
        return mBinder;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        // mBound = false;
        // close();
        Log.i("SDL", "UpdateService, onUnbind(), " + System.currentTimeMillis());
        return super.onUnbind(intent);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mDestroyed = true;
        Log.i("SDL", "UpdateService, onDestroy(), " + System.currentTimeMillis());
    }

    /**
     * Local binder class
     */
    public class LocalBinder extends Binder {
        public UpdateService getService() {
            return UpdateService.this;
        }
    }
}