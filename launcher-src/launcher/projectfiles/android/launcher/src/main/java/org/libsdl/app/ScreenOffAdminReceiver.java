package org.libsdl.app;

import android.app.admin.DeviceAdminReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * Receiver class which shows notifications when the Device Administrator status
 * of the application changes.
 */
public class ScreenOffAdminReceiver extends DeviceAdminReceiver {

	@Override
	public void onEnabled(Context context, Intent intent) {
		// DevicePolicyManager is active.
	}

	@Override
	public void onDisabled(Context context, Intent intent) {
		// DevicePolicyManager is inactive.
	}

}
