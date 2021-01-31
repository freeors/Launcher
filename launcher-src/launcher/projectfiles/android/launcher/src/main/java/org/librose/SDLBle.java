package org.librose;

import android.app.Activity;
import android.app.ActivityManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanRecord;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.ParcelUuid;
import android.util.Log;
import android.util.SparseArray;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;


/**
    SDLBle library
*/
public class SDLBle {
    private static final String TAG = "SDLBle";
    public static final int REQUEST_ENABLE_BT = 1;
    protected static SDLBle mSingleton;
    private Activity mActivity;
    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;

    private boolean mBleCenterInitialized;
    private BluetoothLeScanner mLEScanner;
    private List<ScanFilter> mScanFilters = new ArrayList<>();

    private BluetoothGatt mBluetoothGatt;

    public SDLBle(Activity activity) {
        mSingleton = this;
        mActivity = activity;
    }

    public static boolean isBackground(Context context) {
        ActivityManager activityManager = (ActivityManager) context
                .getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningAppProcessInfo> appProcesses = activityManager
                .getRunningAppProcesses();
        for (ActivityManager.RunningAppProcessInfo appProcess : appProcesses) {
            if (appProcess.processName.equals(context.getPackageName())) {
                if (appProcess.importance != ActivityManager.RunningAppProcessInfo.IMPORTANCE_FOREGROUND) {
                    return true;
                } else {
                    return false;
                }
            }
        }
        return false;
    }

    private void bleInitManager() {
        if (!mActivity.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            // toast("设备不支持BLE");
            mActivity.finish();
        }
        mBluetoothManager = (BluetoothManager)mActivity.getSystemService(Context.BLUETOOTH_SERVICE);
        if (mBluetoothManager == null) {
            Log.e(TAG, "Unable to initialize BluetoothManager.");
            return;
        } else {
            mBluetoothAdapter = mBluetoothManager.getAdapter();
        }

        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Unable to obtain a BluetoothAdapter.");
            return;
        }

        if (!mBluetoothAdapter.isEnabled()) {
            Log.e(TAG, "Current don't enable bluetooth, try to enable...");
            // "Settings" --> "Bluetooth" --> Off(wan to On)
            //  蓝牙正处于“关闭”状态，开启有两种方法，第一种是代码自动打开，第二种弹出一种提示框，叫用户选择是否要打开。
            boolean sliceTurnon = true;
            if (sliceTurnon) {
                boolean success = mBluetoothAdapter.enable();
                Log.e(TAG, "Enable bluetooth adapter, result: " + (success ? "true" : "false"));
                if (!success) {
                    return;
                }
            } else {
                //  以下代码请求“开启”，这会弹出一个提示框
                //   某个应用想要开启此设备的蓝牙功能。
                //                   拒绝    允许
                // 选择允许，程序会自动去开启，等开启成功后，app会收到requestCode == REQUEST_ENABLE_BT的onActivityResult
                // 目前peripherl只用于Launcher,launcher要求无人值守，没人会去单击按钮，为此注释掉以下代码。
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                mActivity.startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            }
        }
/*
        if (Build.VERSION.SDK_INT >= 21) {
            if (Build.VERSION.SDK_INT >= 23) {
                if (mActivity.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                    int yourPermissionRequestCode = 1;
                    mActivity.requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, yourPermissionRequestCode);
                }
            }
        }
 */
        Log.d(TAG, "Initialize bleInitManager success.");
    }

    public void bleInitCenter()
    {
        if (mBleCenterInitialized) {
            return;
        }
        mBleCenterInitialized = true;

        if (mBluetoothManager == null) {
            bleInitManager();
        }
        if (mBluetoothAdapter == null) {
            return;
        }

        if (Build.VERSION.SDK_INT >= 21) {
            mLEScanner = mBluetoothAdapter.getBluetoothLeScanner();
        }

        Log.e(TAG, "Initialize BluetoothAdapter success.");
    }

    //
    // use as center section
    //
    public static native void nativeDiscoverPeripheral(String address, String name, int rssi, byte[] manufacturerData);
    public static native void nativeConnectionStateChange(String address, boolean connected);
    public static native void nativeServicesDiscovered(BluetoothGatt gatt, boolean success);
    public static native void nativeCharacteristicRead(BluetoothGattCharacteristic chara, byte[] buffer, boolean notify);
    public static native void nativeCharacteristicWrite(BluetoothGattCharacteristic chara, boolean success);
    public static native void nativeDescriptorWrite(BluetoothGattDescriptor descriptor);

    private static ScanCallback mScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            // Log.i(TAG, "callbackType: " + String.valueOf(callbackType));
            Log.i(TAG, "result: " + result.toString());
            BluetoothDevice device = result.getDevice();
            // id: 0x12 0x13, data: 0x01 0x02 0x03 ==> manufacturerData: 0x12 0x13 0x01 0x02 0x03
            // 如果存在多个manufacturerData, 只返回最后一个.
            byte[] manufacturerData = null;
            ScanRecord record = result.getScanRecord();
            SparseArray<byte[]> dataArray = record.getManufacturerSpecificData();
            for (int i = 0; dataArray != null && i < dataArray.size(); i ++) {
                int key = dataArray.keyAt(i);
                byte[] user = dataArray.get(key);
                manufacturerData = new byte[2 + (user != null? user.length: 0)];
                manufacturerData[0] = ((byte)(key & 0xFF));
                manufacturerData[1] = ((byte)(key >> 8 & 0xFF));
                for (int at = 0; user != null && at < user.length; at ++) {
                    manufacturerData[2 + at] = user[at];
                }
            }
            nativeDiscoverPeripheral(device.getAddress(), device.getName(), result.getRssi(), manufacturerData);
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            for (ScanResult sr : results) {
                Log.i(TAG, "ScanResult - Results: " + sr.toString());
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.e(TAG, "Scan Failed, Error Code: " + errorCode);
        }
    };

    // Implements callback methods for GATT events that the app cares about.  For example,
    // connection change and services discovered.
    public static BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            String address = gatt.getDevice().getAddress();
            if (address == null || address.isEmpty()) {
                return;
            }
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.i(TAG, "Connected to GATT server.");
                nativeConnectionStateChange(address, true);

            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                Log.i(TAG, "Disconnected from GATT server.");
                nativeConnectionStateChange(address, false);
                // 确保"一次性"调用BluetoothGatt.disconnect(), BluetoothGatt.close()
                mSingleton.close(address);
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            // Log.i(TAG, "onServiceDiscovered: status: " + status + ", service count: " + gatt.getServices().size());
            nativeServicesDiscovered(gatt, status == BluetoothGatt.GATT_SUCCESS);
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            // Log.i(TAG, "onCharacteristicRead: uuid " + characteristic.getUuid().toString() + ", status: " + status);
            nativeCharacteristicRead(characteristic, status == BluetoothGatt.GATT_SUCCESS? characteristic.getValue(): null, false);
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,
                                          BluetoothGattCharacteristic characteristic, int status) {
            // Log.i(TAG, "onCharacteristicWrite: uuid " + characteristic.getUuid().toString() + ", status: " + status);
            nativeCharacteristicWrite(characteristic, status == BluetoothGatt.GATT_SUCCESS);
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            // Log.i(TAG, "onCharacteristicChanged: uuid " + characteristic.getUuid().toString());
            // peripheral有数据可以读时，包括app read和peripheral notification触发
            nativeCharacteristicRead(characteristic, characteristic.getValue(), true);
        }

        @Override
        public void onDescriptorRead(BluetoothGatt gatt, BluetoothGattDescriptor descriptor,
                                     int status) {
            Log.i(TAG, "onDescriptorRead: uuid " + descriptor.getUuid().toString() + ", status: " + status);
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor,
                                      int status) {
            // Log.i(TAG, "onDescriptorWrite: uuid " + descriptor.getUuid().toString() + ", status: " + status);
            nativeDescriptorWrite(descriptor);
        }
    };

    private ScanSettings setScanSettings() {
        boolean background = isBackground(mActivity);
        Log.d(TAG, "currently in the background:>>>>>"+background);

        mScanFilters.clear();
        ScanSettings scanSettings;
        // ScanFilter filter = Ble.options().getScanFilter();
        ScanFilter filter = null;
        if (filter != null){
            mScanFilters.add(filter);
        }
        if (background){
            // UUID uuidService = Ble.options().getUuidService();
            UUID uuidService = null;
            if (filter == null && uuidService != null){
                mScanFilters.add(new ScanFilter.Builder()
                        .setServiceUuid(ParcelUuid.fromString(uuidService.toString())) // 8.0以上手机后台扫描，必须开启
                        .build());
            }
            scanSettings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_POWER)
                    .build();
        }else {
            scanSettings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                    .build();
        }
        return scanSettings;
    }

    private static BluetoothAdapter.LeScanCallback mLeScanCallback =
            new BluetoothAdapter.LeScanCallback() {

                @Override
                public void onLeScan(final BluetoothDevice device, final int rssi, byte[] scanRecord) {
                    // it isn't in main thread. to safe, should switch to main
                    // Log.i(TAG, "address: " + device.getAddress() + " name: " + device.getName());
                    byte[] manufacturerData = new byte[] { (byte)0xf0, (byte)0xff, (byte)0x6b,
                            0x6f, (byte)0x73, 0x2d, 0x64, 0x65, (byte)0x76, (byte)0x69, 0x63, 0x65 };
                    nativeDiscoverPeripheral(device.getAddress(), device.getName(), rssi, manufacturerData);
                }
            };
    private boolean useOldScan = false;
    private void startScanOld() {
        mBluetoothAdapter.startLeScan(mLeScanCallback);
    }

    public void startScan() {
        if (useOldScan) {
            // 只是为了测试。如果真使能，还须要写useOldScan=true时的stopScan代码
            startScanOld();
            return;
        }
        ScanSettings settings = new ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build();
        List<ScanFilter> filters = new ArrayList<ScanFilter>();
        settings = setScanSettings();
        //  An app must hold ACCESS_COARSE_LOCATION or ACCESS_FINE_LOCATION permission in order to get results.
        mLEScanner.startScan(mScanFilters, settings, mScanCallback);
    }

    public void stopScan() {
        mLEScanner.stopScan(mScanCallback);
    }

    public boolean connect(String address) {
        if (mBluetoothGatt != null && address.equals(mBluetoothGatt.getDevice().getAddress())) {
            // 禁止在mBluetoothGatt != null的情况下，再次调用device.connectGatt.
            Log.d(TAG, "Must not call BluetoothGatt.connectGatt, when mBluetoothGatt != null");
            return false;
        }
        final BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address);
        BluetoothGattCallback gattCallback = mGattCallback;
        // We want to directly connect to the device, so we are setting the autoConnect parameter to false
        BluetoothGatt bluetoothGatt;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && device.getType() == BluetoothDevice.DEVICE_TYPE_DUAL) {
            bluetoothGatt = device.connectGatt(mActivity, false, gattCallback, BluetoothDevice.TRANSPORT_LE);
        } else {
            bluetoothGatt = device.connectGatt(mActivity, false, gattCallback);
        }
        if (bluetoothGatt != null) {
            mBluetoothGatt = bluetoothGatt;
            Log.d(TAG, "Trying to create a new connection.");
            return true;
        }
        Log.d(TAG, "Trying to create a new connection fail.");
        return false;
    }

    public void disconnect(String address) {
        if (mBluetoothGatt == null) {
            return;
        }
        if (address.equals(mBluetoothGatt.getDevice().getAddress())) {
            mBluetoothGatt.disconnect();
        }
/*
        //notifyIndex = 0;
        notifyCharacteristics.clear();
        writeCharacteristicMap.remove(address);
        readCharacteristicMap.remove(address);
        otaWriteCharacteristic = null;
 */
    }

    public void close(String address) {
        if (mBluetoothGatt == null) {
            return;
        }
        if (address.equals(mBluetoothGatt.getDevice().getAddress())) {
            mBluetoothGatt.close();
            mBluetoothGatt = null;
        }
    }

    public void discoverServices(String address) {
        if (mBluetoothGatt != null && address.equals(mBluetoothGatt.getDevice().getAddress())) {
            mBluetoothGatt.discoverServices();
        }
    }

    /**
     * 写入数据
     *
     * @param address 蓝牙地址
     * @param value   发送的字节数组
     * @return 写入是否成功(这个是客户端的主观认为)
     */
/*
    public boolean writeCharacteristic(String address, byte[] value) {
        BluetoothGattCharacteristic gattCharacteristic = writeCharacteristicMap.get(address);
        if (gattCharacteristic != null) {
            if (options.uuid_write_cha.equals(gattCharacteristic.getUuid())) {
                gattCharacteristic.setValue(value);
                boolean result = getBluetoothGatt(address).writeCharacteristic(gattCharacteristic);
                BleLog.d(TAG, address + " -- write result:" + result);
                return result;
            }
        }else {
            if (null != writeWrapperCallback){
                writeWrapperCallback.onWriteFailed(getBleDeviceInternal(address), BleStates.NotInitUuid);
            }
        }
        return false;
    }
*/
    public boolean writeCharacteristicByUuid(String address, byte[] value, UUID serviceUUID, UUID characteristicUUID) {
        if (mBluetoothGatt == null || !address.equals(mBluetoothGatt.getDevice().getAddress())) {
            return false;
        }
        BluetoothGatt bluetoothGatt = mBluetoothGatt;
        BluetoothGattCharacteristic characteristic = gattCharacteristic(bluetoothGatt, serviceUUID, characteristicUUID);
        if (characteristic != null) {
            characteristic.setValue(value);
            boolean result = bluetoothGatt.writeCharacteristic(characteristic);
            Log.d(TAG, address + " -- write result:" + result);
            return result;
        }
        return false;
    }

    public BluetoothGattCharacteristic gattCharacteristic(BluetoothGatt gatt, UUID serviceUUID, UUID characteristicUUID){
        if (gatt == null){
            Log.e(TAG, "BluetoothGatt is null");
            return null;
        }
        BluetoothGattService gattService = gatt.getService(serviceUUID);
        if (gattService == null){
            Log.e(TAG, "serviceUUID is null");
            return null;
        }
        BluetoothGattCharacteristic characteristic = gattService.getCharacteristic(characteristicUUID);
        if (characteristic == null){
            Log.e(TAG, "characteristicUUID is null");
            return null;
        }
        return characteristic;
    }

    /**
     * 读取数据
     *
     * @param address 蓝牙地址
     * @return 读取是否成功(这个是客户端的主观认为)
     */
/*
    public boolean readCharacteristic(String address) {
        BluetoothGattCharacteristic gattCharacteristic = readCharacteristicMap.get(address);
        if (gattCharacteristic != null) {
            if (options.uuid_read_cha.equals(gattCharacteristic.getUuid())) {
                boolean result = getBluetoothGatt(address).readCharacteristic(gattCharacteristic);
                BleLog.d(TAG, "read result:" + result);
                return result;
            }
        }else {
            if (null != readWrapperCallback){
                readWrapperCallback.onReadFailed(getBleDeviceInternal(address), BleStates.NotInitUuid);
            }
        }
        return false;
    }
*/
    public boolean readCharacteristicByUuid(String address, UUID serviceUUID, UUID characteristicUUID) {
        if (mBluetoothGatt == null || !address.equals(mBluetoothGatt.getDevice().getAddress())) {
            return false;
        }
        BluetoothGatt bluetoothGatt = mBluetoothGatt;
        BluetoothGattCharacteristic gattCharacteristic = gattCharacteristic(bluetoothGatt, serviceUUID, characteristicUUID);
        if (gattCharacteristic != null) {
            boolean result = bluetoothGatt.readCharacteristic(gattCharacteristic);
            Log.d(TAG, address + " -- read result:" + result);
            return result;
        }
        return false;
    }

    public boolean readDescriptor(String address, UUID serviceUUID, UUID characteristicUUID, UUID descriptorUUID){
        if (mBluetoothGatt == null || !address.equals(mBluetoothGatt.getDevice().getAddress())) {
            return false;
        }
        BluetoothGatt bluetoothGatt = mBluetoothGatt;
        BluetoothGattCharacteristic gattCharacteristic = gattCharacteristic(bluetoothGatt, serviceUUID, characteristicUUID);
        if (gattCharacteristic != null){
            BluetoothGattDescriptor descriptor = gattCharacteristic.getDescriptor(descriptorUUID);
            if (descriptor != null){
                return bluetoothGatt.readDescriptor(descriptor);
            }
        }
        return false;
    }

    public boolean writeDescriptor(String address, byte[] data, UUID serviceUUID, UUID characteristicUUID, UUID descriptorUUID){
        if (mBluetoothGatt == null || !address.equals(mBluetoothGatt.getDevice().getAddress())) {
            return false;
        }
        BluetoothGatt bluetoothGatt = mBluetoothGatt;
        BluetoothGattCharacteristic gattCharacteristic = gattCharacteristic(bluetoothGatt, serviceUUID, characteristicUUID);
        if (gattCharacteristic != null){
            BluetoothGattDescriptor descriptor = gattCharacteristic.getDescriptor(descriptorUUID);
            if (descriptor != null){
                descriptor.setValue(data);
                return bluetoothGatt.writeDescriptor(descriptor);
            }
        }
        return false;
    }

    /**
     * 读取远程rssi
     * @param address 蓝牙地址
     * @return 是否读取rssi成功
     */
    public boolean readRssi(String address) {
        if (mBluetoothGatt == null || !address.equals(mBluetoothGatt.getDevice().getAddress())) {
            return false;
        }
        boolean result = mBluetoothGatt.readRemoteRssi();
        Log.d(TAG, address + "read result:" + result);
        return result;
    }

    /**
     * 启用或禁用给定特征的通知
     *
     * @param address        蓝牙地址
     * @param enabled   是否设置通知使能
     */
/*
    public void setCharacteristicNotification(String address, boolean enabled) {
        if (notifyCharacteristics.size() > 0){
            for (BluetoothGattCharacteristic characteristic: notifyCharacteristics) {
                setCharacteristicNotificationInternal(getBluetoothGatt(address), characteristic, enabled);
            }
        }
    }
*/
    public void setCharacteristicNotificationByUuid(String address, boolean enabled, UUID serviceUUID, UUID characteristicUUID) {
        if (mBluetoothGatt == null || !address.equals(mBluetoothGatt.getDevice().getAddress())) {
            return;
        }
        BluetoothGatt bluetoothGatt = mBluetoothGatt;
        BluetoothGattCharacteristic characteristic = gattCharacteristic(bluetoothGatt, serviceUUID, characteristicUUID);
        setCharacteristicNotificationInternal(bluetoothGatt, characteristic, enabled);
    }

    private void setCharacteristicNotificationInternal(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, boolean enabled){
        if (characteristic == null){
            Log.e(TAG, "characteristic is null");
            return;
        }
        gatt.setCharacteristicNotification(characteristic, enabled);
        //If the number of descriptors in the eigenvalue of the notification is greater than zero
        List<BluetoothGattDescriptor> descriptors = characteristic.getDescriptors();
        if (!descriptors.isEmpty()) {
            //Filter descriptors based on the uuid of the descriptor
            for(BluetoothGattDescriptor descriptor : descriptors){
                if (descriptor != null) {
                    //Write the description value
                    if((characteristic.getProperties() & BluetoothGattCharacteristic.PROPERTY_NOTIFY) != 0){
                        descriptor.setValue(enabled?BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE:BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE);
                    }else if((characteristic.getProperties() & BluetoothGattCharacteristic.PROPERTY_INDICATE) != 0){
                        //两个都是通知的意思，notify和indication的区别在于，notify只是将你要发的数据发送给手机，没有确认机制，
                        //不会保证数据发送是否到达。而indication的方式在手机收到数据时会主动回一个ack回来。即有确认机制，只有收
                        //到这个ack你才能继续发送下一个数据。这保证了数据的正确到达，也起到了流控的作用。所以在打开通知的时候，需要设置一下。
                        descriptor.setValue(enabled?BluetoothGattDescriptor.ENABLE_INDICATION_VALUE:BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE);
                    }
                    gatt.writeDescriptor(descriptor);
                    Log.d(TAG, "setCharacteristicNotificationInternal is "+enabled);
                }
            }
        }
    }

    /**
     * 清理蓝牙缓存
     */
    public boolean refreshDeviceCache(String address) {
        if (mBluetoothGatt == null || !address.equals(mBluetoothGatt.getDevice().getAddress())) {
            return false;
        }
        BluetoothGatt gatt = mBluetoothGatt;
        if (gatt != null) {
            try {
                Method localMethod = gatt.getClass().getMethod(
                        "refresh", new Class[0]);
                if (localMethod != null) {
                    boolean bool = ((Boolean) localMethod.invoke(
                            gatt, new Object[0])).booleanValue();
                    return bool;
                }
            } catch (Exception localException) {
                Log.e(TAG, "An exception occured while refreshing device");
            }
        }
        return false;
    }

    //
    // use as peripheral section
    //
    public static native void nativePConnectionStateChange(String address, boolean connected);
    public static native void nativePCharacteristicRead(String chara, byte[] buffer, boolean notify);
    public static native void nativePNotificationSent(String chara, int status);

    /**
     * 以字符串表示形式返回字节数组的内容
     *
     * @param bytes 字节数组
     * @return 字符串形式的 <tt>bytes</tt>
     * [01, fe, 08, 35, f1, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00]
     */
    public static String toHexString(byte[] bytes) {
        if (bytes == null)
            return "null";
        int iMax = bytes.length - 1;
        if (iMax == -1)
            return "[]";
        StringBuilder b = new StringBuilder();
        b.append('[');
        for (int i = 0; ; i++) {
            b.append(String.format("%02x", bytes[i] & 0xFF));
            if (i == iMax)
                return b.append(']').toString();
            b.append(", ");
        }
    }

    public static final int REQUEST_CODE = 2;
    public static final UUID UUID_SERVER = UUID.fromString(UuidUtils.uuid16To128("fd00"));
    public static final UUID UUID_CHARWRITE = UUID.fromString(UuidUtils.uuid16To128("fd01"));
    public static final UUID UUID_CHARREAD = UUID.fromString(UuidUtils.uuid16To128("fd02"));
    public static final UUID UUID_NOTIFY = UUID.fromString(UuidUtils.uuid16To128("fd03"));
    public static final UUID UUID_DESCRIPTOR = UUID.fromString(UuidUtils.uuid16To128("2902"));

    private boolean blePeripheralInitialized = false;
    private boolean mAdvertisering = false;

    private BluetoothDevice mPBleDevice;

    private void bleInitPeripheral() {
        blePeripheralInitialized = true;
        if (mBluetoothManager == null) {
            bleInitManager();
        }
        if (mBluetoothAdapter == null) {
            return;
        }

        BluetoothLeAdvertiser mAdvertiser = mBluetoothAdapter.getBluetoothLeAdvertiser();
        if (mAdvertiser == null) {
            Log.d(TAG,"设备不支持广播蓝牙");
            mActivity.finish();
        }
        // mBluetoothAdapter.setName("RDP Discovery");
    }

    public void startAdvertiser(String name, int manufacturerId, byte[] manufacturerData) {
        if (!blePeripheralInitialized) {
            bleInitPeripheral();
        }
        // 测试下来, setName参数传空字符串时(不是null, null会导致app非法退出)，虽不报错但其实无影响, 这里为简单还是不判断吧
        // 但这样一来, 希望上面代码不传null/""下来
        String name2 = name != null? name: "";
        mBluetoothAdapter.setName(name2);

        // ADVERTISE_MODE_LOW_LATENCY ==> 100ms
        // ADVERTISE_MODE_BALANCED ==> 250ms
        // ADVERTISE_MODE_LOW_POWER ==> 1s
        // 网上写的是这间隔，但我在rk3399-aio测下来都大概是100ms, 不知道是哪里没设对. 但不管怎样设一个最大间隔ADVERTISE_MODE_LOW_POWER
        AdvertiseSettings settings = new AdvertiseSettings.Builder()
                .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_POWER)//设置广播间隔100ms
                .setConnectable(true)
                .setTimeout(0)
                .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH)
                .build();
/*
        AdvertiseData advertiseData = new AdvertiseData.Builder()
                .setIncludeDeviceName(true)
                .setIncludeTxPowerLevel(true)
                .addManufacturerData(65520, new byte[]{'J', 'Z', '6', '7', '8', '9'})
                .build();
*/
        AdvertiseData.Builder builder = new AdvertiseData.Builder()
                .setIncludeDeviceName(true)
                .setIncludeTxPowerLevel(true);
        if (manufacturerData != null) {
            builder.addManufacturerData(manufacturerId, manufacturerData);
        }
        AdvertiseData advertiseData = builder.build();

        AdvertiseData scanResponseData = new AdvertiseData.Builder()
                .addServiceUuid(new ParcelUuid(UUID_SERVER))
                .setIncludeTxPowerLevel(true)
                .build();

        BluetoothLeAdvertiser bluetoothLeAdvertiser = mBluetoothAdapter.getBluetoothLeAdvertiser();
        if (bluetoothLeAdvertiser == null){
            Log.d(TAG,"设备不支持广播蓝牙");
            return;
        }

        bluetoothLeAdvertiser.startAdvertising(settings, advertiseData, scanResponseData, callback);
        {
            int ii = 0;
            mAdvertisering = true;
        }
    }

    public void stopAdvertiser(){
        bluetoothGattServer.clearServices();
        BluetoothLeAdvertiser bluetoothLeAdvertiser = mBluetoothAdapter.getBluetoothLeAdvertiser();
        bluetoothLeAdvertiser.stopAdvertising(callback);
        mAdvertisering = false;
        Log.d(TAG,"开启广播");
        bluetoothGattServer.clearServices();
        bluetoothGattServer.close();
    }

    public boolean isAdvertisering() {
        return mAdvertisering;
    }

    public void pbleWriteCharacteristic(String chara, byte[] data) {
        Log.d(TAG, "[1/3]pbleWriteCharacteristic, data(" + data.length + "): " + toHexString(data) + " mPBleDevice: " + (mPBleDevice != null? "isn't null": "is null"));
        if (mPBleDevice != null) {
            BluetoothDevice device = mPBleDevice;

            Log.d(TAG, String.format("[2/3]pbleWriteCharacteristic, pre setValue, device name = %s, address = %s", device.getName(), device.getAddress()));
            characteristicNotify.setValue(data);

            // 通知数据改变
            bluetoothGattServer.notifyCharacteristicChanged(device, characteristicNotify, false);
            Log.d(TAG, "[3/3]pbleWriteCharacteristic, X, post notifyCharacteristicChanged");
        }
    }

    AdvertiseCallback callback = new AdvertiseCallback() {
        @Override
        public void onStartSuccess(AdvertiseSettings settingsInEffect) {
            Log.d(TAG, "BLE advertisement added successfully");
            initServices(mActivity.getBaseContext());
            mAdvertisering = true;
            // btn_start_ad.setText("停止广播");
        }

        @Override
        public void onStartFailure(int errorCode) {
            Log.d(TAG, "Failed to add BLE advertisement, reason: " + errorCode);
            mAdvertisering = false;
        }
    };

    private BluetoothGattServer bluetoothGattServer;
    private BluetoothGattCharacteristic characteristicNotify;
    private void initServices(Context context) {
        bluetoothGattServer = mBluetoothManager.openGattServer(context, bluetoothGattServerCallback);
        BluetoothGattService service = new BluetoothGattService(UUID_SERVER, BluetoothGattService.SERVICE_TYPE_PRIMARY);

        //add a read characteristic.
        BluetoothGattCharacteristic characteristicRead = new BluetoothGattCharacteristic(UUID_CHARREAD, BluetoothGattCharacteristic.PROPERTY_READ, BluetoothGattCharacteristic.PERMISSION_READ);
        service.addCharacteristic(characteristicRead);

        characteristicNotify = new BluetoothGattCharacteristic(UUID_NOTIFY, BluetoothGattCharacteristic.PROPERTY_NOTIFY, BluetoothGattCharacteristic.PROPERTY_WRITE);
        //add a descriptor
        BluetoothGattDescriptor descriptor = new BluetoothGattDescriptor(UUID_DESCRIPTOR, BluetoothGattCharacteristic.PERMISSION_WRITE);
        characteristicNotify.addDescriptor(descriptor);
        service.addCharacteristic(characteristicNotify);

        //add a write characteristic.
        BluetoothGattCharacteristic characteristicWrite = new BluetoothGattCharacteristic(UUID_CHARWRITE,
                BluetoothGattCharacteristic.PROPERTY_WRITE |
                        BluetoothGattCharacteristic.PROPERTY_READ |
                        BluetoothGattCharacteristic.PROPERTY_NOTIFY,
                BluetoothGattCharacteristic.PERMISSION_WRITE);
        service.addCharacteristic(characteristicWrite);

        bluetoothGattServer.addService(service);
        Log.d(TAG, "2. initServices ok");
        // toast("2. initServices ok");
    }

    private void handleDisconnected() {
        mPBleDevice = null;
        nativePConnectionStateChange(null, false);
    }

    /**
     * 服务事件的回调
     */
    private BluetoothGattServerCallback bluetoothGattServerCallback = new BluetoothGattServerCallback() {
        /**
         * 1.连接状态发生变化时
         * @param device
         * @param status
         * @param newState
         */
        @Override
        public void onConnectionStateChange(final BluetoothDevice device, int status, final int newState) {
            Log.d(TAG, String.format("1.onConnectionStateChange：device name = %s, address = %s", device.getName(), device.getAddress()));
            Log.d(TAG, String.format("1.onConnectionStateChange：status = %s, newState =%s ", status, newState));
            super.onConnectionStateChange(device, status, newState);

            if (newState == BluetoothProfile.STATE_CONNECTED){
                if (mPBleDevice != null) {
                    Log.d(TAG, "[Disconnected]Last connection has not been disconnected normally, and a new connection has arrived. fix for rose");
                    handleDisconnected();
                }
                mPBleDevice = device;
                Log.d(TAG, String.format("[Connected]: %s", device.getAddress()));
                nativePConnectionStateChange(device.getAddress(), true);
            } else {
                Log.d(TAG, "[Disconnected]: normal");
                handleDisconnected();
            }

        }

        @Override
        public void onServiceAdded(int status, BluetoothGattService service) {
            super.onServiceAdded(status, service);
            Log.d(TAG, String.format("onServiceAdded：status = %s", status));
        }

        @Override
        public void onCharacteristicReadRequest(BluetoothDevice device, int requestId, int offset, BluetoothGattCharacteristic characteristic) {
            Log.d(TAG, String.format("onCharacteristicReadRequest：device name = %s, address = %s", device.getName(), device.getAddress()));
            Log.d(TAG, String.format("onCharacteristicReadRequest：requestId = %s, offset = %s", requestId, offset));

            bluetoothGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset, characteristic.getValue());

//            super.onCharacteristicReadRequest(device, requestId, offset, characteristic);
        }

        /**
         * 3. onCharacteristicWriteRequest,接收具体的字节
         * @param device
         * @param requestId
         * @param characteristic
         * @param preparedWrite
         * @param responseNeeded
         * @param offset
         * @param requestBytes
         */
        @Override
        public void onCharacteristicWriteRequest(BluetoothDevice device, int requestId, BluetoothGattCharacteristic characteristic, boolean preparedWrite, boolean responseNeeded, int offset, byte[] requestBytes) {
            Log.d(TAG, String.format("3.onCharacteristicWriteRequest：device name = %s, address = %s, uuid = %s", device.getName(), device.getAddress(), characteristic.getUuid().toString()));
            Log.d(TAG, String.format("3.onCharacteristicWriteRequest：requestId = %s, preparedWrite=%s, responseNeeded=%s, offset=%s, value=%s", requestId, preparedWrite, responseNeeded, offset, toHexString(requestBytes)));
            bluetoothGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset, requestBytes);

            nativePCharacteristicRead(characteristic.getUuid().toString(), requestBytes, responseNeeded);
            final String msg = toHexString(requestBytes);
            Log.d(TAG, "4.收到:" + msg);
        }

        /**
         * 2.描述被写入时，在这里执行 bluetoothGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS...  收，触发 onCharacteristicWriteRequest
         * @param device
         * @param requestId
         * @param descriptor
         * @param preparedWrite
         * @param responseNeeded
         * @param offset
         * @param value
         */
        @Override
        public void onDescriptorWriteRequest(BluetoothDevice device, int requestId, BluetoothGattDescriptor descriptor, boolean preparedWrite, boolean responseNeeded, int offset, byte[] value) {
            Log.d(TAG, String.format("2.onDescriptorWriteRequest：device name = %s, address = %s", device.getName(), device.getAddress()));
            Log.d(TAG, String.format("2.onDescriptorWriteRequest：requestId = %s, preparedWrite = %s, responseNeeded = %s, offset = %s, value = %s,", requestId, preparedWrite, responseNeeded, offset, toHexString(value)));

            // now tell the connected device that this was all successfull
            bluetoothGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset, value);
        }

        /**
         * 5.特征被读取。当回复响应成功后，客户端会读取然后触发本方法
         * @param device
         * @param requestId
         * @param offset
         * @param descriptor
         */
        @Override
        public void onDescriptorReadRequest(BluetoothDevice device, int requestId, int offset, BluetoothGattDescriptor descriptor) {
            Log.d(TAG, String.format("onDescriptorReadRequest：device name = %s, address = %s", device.getName(), device.getAddress()));
            Log.d(TAG, String.format("onDescriptorReadRequest：requestId = %s", requestId));
//            super.onDescriptorReadRequest(device, requestId, offset, descriptor);
            bluetoothGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset, null);
        }

        @Override
        public void onNotificationSent(BluetoothDevice device, int status) {
            super.onNotificationSent(device, status);
            // 执行bluetoothGattServer.notifyCharacteristicChanged(device, characteristicNotify, false)后, 会回调这个
            Log.d(TAG, String.format("5.onNotificationSent：device name = %s, address = %s, status = %s", device.getName(), device.getAddress(), status));
            nativePNotificationSent(characteristicNotify.getUuid().toString(), status);
        }

        @Override
        public void onMtuChanged(BluetoothDevice device, int mtu) {
            super.onMtuChanged(device, mtu);
            Log.d(TAG, String.format("onMtuChanged：mtu = %s", mtu));
        }

        @Override
        public void onExecuteWrite(BluetoothDevice device, int requestId, boolean execute) {
            super.onExecuteWrite(device, requestId, execute);
            Log.d(TAG, String.format("onExecuteWrite：requestId = %s", requestId));
        }
    };
}
