package org.librose;


import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.util.Log;


import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


/**
    SDLBle library
*/
public class SDLWifi {
    private static final String TAG = "SDLWifi";
    // protected static SDLWifi mSingleton;
    private Activity mActivity;

    public SDLWifi(Activity activity) {
        // mSingleton = this;
        mActivity = activity;
    }

    private WifiManager wifiManager;
    private ConnectivityManager connectivityManager;
    private static List<ScanResult> resultList = new ArrayList<ScanResult>();
    private List<WifiConfiguration> wificonfigList = new ArrayList<WifiConfiguration>();
    private boolean mWifiScanResultChanged = true;

    //将搜索到的wifi根据信号强度从强到弱进行排序
    private void sortByLevel(List<android.net.wifi.ScanResult> resultList) {
        for(int i=0;i<resultList.size();i++)
            for(int j=1;j<resultList.size();j++)
            {
                if(resultList.get(i).level<resultList.get(j).level)    //level属性即为强度
                {
                    android.net.wifi.ScanResult temp = null;
                    temp = resultList.get(i);
                    resultList.set(i, resultList.get(j));
                    resultList.set(j, temp);
                }
            }
    }
    /**
     * 当搜索到可用wifi时，将结果封装到resultList中，并按rssi排序
     */
    private final BroadcastReceiver wifiReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            // TODO Auto-generated method stub
            if (WifiManager.WIFI_STATE_CHANGED_ACTION.equals(intent.getAction())) {
                // 这个监听wifi的打开与关闭，与wifi的连接无关
                // WiFi状态变化广播：如果WiFi已经完成开启，即可进行WiFi扫描
                int wifiState = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE, 0);
                Log.e(TAG, "[wifiReceiver]WifiManager.WIFI_STATE_CHANGED_ACTION, wifiState" + wifiState);
                switch (wifiState) {
                    case WifiManager.WIFI_STATE_ENABLED:
                        Log.d(TAG, "**********wifi开启成功，开始扫描周边网络***********");
                        // 这里可以自定义一个扫描等待对话框，待完成扫描结果后关闭
                        // Objects.requireNonNull(wifiManager).startScan();
                        wifiManager.startScan();
                        break;
                    case WifiManager.WIFI_STATE_DISABLED:
                        Log.d(TAG, "**********wifi开关未打开***********");
                        resultList.clear();
                        wificonfigList.clear();
                        mWifiScanResultChanged = true;
                        break;
                    //
                }

            } else if (intent.getAction().equals(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)) {
                resultList = wifiManager.getScanResults();
                Log.e(TAG, "[wifiReceiver]WifiManager.SCAN_RESULTS_AVAILABLE_ACTION, results:" + resultList.size());
                sortByLevel(resultList);
                mWifiScanResultChanged = true;

            } else if (ConnectivityManager.CONNECTIVITY_ACTION.equals(intent.getAction())) {
                // 这个监听网络连接的设置，包括wifi和移动数据的打开和关闭。.
                // 最好用的还是这个监听。wifi如果打开，关闭，以及连接上可用的连接都会接到监听。见log
                // 这个广播的最大弊端是比上边两个广播的反应要慢，如果只是要监听wifi，我觉得还是用上边两个配合比较合适
                Log.e(TAG, "[wifiReceiver]ConnectivityManager.CONNECTIVITY_ACTION");
                mWifiScanResultChanged = true;
/*               ConnectivityManager manager = (ConnectivityManager) context
                        .getSystemService(Context.CONNECTIVITY_SERVICE);
                NetworkInfo gprs = manager
                        .getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
                NetworkInfo wifi = manager
                        .getNetworkInfo(ConnectivityManager.TYPE_WIFI);
                Log.i(TAG, "网络状态改变:" + wifi.isConnected() + " 3g:" + gprs.isConnected());


                NetworkInfo info = intent
                        .getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
                if (info != null) {
                    Log.e("H3c", "info.getTypeName()" + info.getTypeName());
                    Log.e("H3c", "getSubtypeName()" + info.getSubtypeName());
                    Log.e("H3c", "getState()" + info.getState());
                    Log.e("H3c", "getDetailedState()"
                            + info.getDetailedState().name());
                    Log.e("H3c", "getDetailedState()" + info.getExtraInfo());
                    Log.e("H3c", "getType()" + info.getType());

                    if (NetworkInfo.State.CONNECTED == info.getState()) {
                    } else if (info.getType() == 1) {
                        if (NetworkInfo.State.DISCONNECTING == info.getState()) {

                        }
                    }
                }
 */
            }

        }
    };

    public void registerWifi() {
        //获取wifi管理服务
        wifiManager = (WifiManager) mActivity.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        connectivityManager = (ConnectivityManager)mActivity.getSystemService(Context.CONNECTIVITY_SERVICE);

        //注册WiFi扫描结果、WiFi状态变化的广播接收
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION); //监听wifi扫描结果
        intentFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION); //监听wifi是开关变化的状态
        intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION); // 监听wifi是否连接成功的广播
        mActivity.registerReceiver(wifiReceiver, intentFilter);
    }

    public void enableWifi(boolean enable) {
        if (wifiManager == null) {
            return;
        }
        if (enable && !wifiManager.isWifiEnabled()) {
            //WiFi未打开，开启wifi
            wifiManager.setWifiEnabled(true);
        } else if (!enable && wifiManager.isWifiEnabled()) {
            //WiFi已打开，关闭wifi
            wifiManager.setWifiEnabled(false);
        }
    }

    /**
     * 得到配置信息
     */
    public void getConfigurations() {
        wificonfigList = wifiManager.getConfiguredNetworks();
    }

    /**
     * 该链接是否已经配置过
     * @param SSID
     * @return
     */
    public int isConfigured(String SSID) {
        for (int i = 0; i < wificonfigList.size(); i++) {
            String ssid = ajdustSSID(wificonfigList.get(i).SSID);
            if (ssid != null && ssid.equals(SSID)) {
                return wificonfigList.get(i).networkId;
            }
        }
        return -1;
    }
    /**
     * 链接到制定wifi
     * @param wifiId
     * @return
     */
    public boolean ConnectWifi(int wifiId){
        boolean isConnect = false;
        int id= 0;
        for(int i = 0; i < wificonfigList.size(); i++){
            WifiConfiguration wifi = wificonfigList.get(i);
            id = wifi.networkId;
            if(id == wifiId){
                while(!(wifiManager.enableNetwork(wifiId, true))){
                    Log.i("ConnectWifi",String.valueOf(wificonfigList.get(wifiId).status));
                }

                if ( wifiManager.enableNetwork(wifiId, true)) {
                    isConnect = true;
                }
            }
        }
        return isConnect;
    }

    public enum WifiCipherType {
        WIFICIPHER_WEP, WIFICIPHER_WPA, WIFICIPHER_NOPASS, WIFICIPHER_INVALID
    }

    /**
     * 判断wifi热点支持的加密方式
     */
    public WifiCipherType getWifiCipher(String capabilities) {
        if (capabilities.isEmpty()) {
            return WifiCipherType.WIFICIPHER_INVALID;
        } else if (capabilities.contains("WEP")) {
            return WifiCipherType.WIFICIPHER_WEP;
        } else if (capabilities.contains("WPA") || capabilities.contains("WPA2") || capabilities.contains("WPS")) {
            return WifiCipherType.WIFICIPHER_WPA;
        } else {
            return WifiCipherType.WIFICIPHER_NOPASS;
        }
    }

    /**
     * 添加wifi配置
     * @param wifiList
     * @param ssid
     * @param pwd
     * @return
     */
    public int AddWifiConfig(List<android.net.wifi.ScanResult> wifiList, String ssid, String pwd){
        int wifiId = -1;
        for(int i = 0;i < wifiList.size(); i++){
            android.net.wifi.ScanResult wifi = wifiList.get(i);
            if(wifi.SSID.equals(ssid)){
                Log.i("AddWifiConfig","equals");
                WifiConfiguration wifiCong = new WifiConfiguration();
                wifiCong.SSID = "\""+wifi.SSID+"\"";
                wifiCong.preSharedKey = "\""+pwd+"\"";
                wifiCong.hiddenSSID = false;
                wifiCong.status = WifiConfiguration.Status.ENABLED;
                wifiId = wifiManager.addNetwork(wifiCong);
                if(wifiId != -1){
                    return wifiId;
                }
            }
        }
        return wifiId;
    }

    private enum SDL_WifiFlag {
        SDL_WifiFlagEnable(0x1),
        SDL_WifiFlagScanResultChanged(0x2);

        private int mValue = 0;
        private SDL_WifiFlag(int value) {
            mValue = value;
        }
        public int value() {
            return mValue;
        }
    }

    public int wifiGetFlags() {
        int ret = 0;
        if (wifiManager.isWifiEnabled()) {
            ret += SDL_WifiFlag.SDL_WifiFlagEnable.value();
        }
        if (mWifiScanResultChanged) {
            ret += SDL_WifiFlag.SDL_WifiFlagScanResultChanged.value();
        }
        // Log.d(TAG, "wifiGetFlags return " + ret);
        return ret;
    }

    private enum SDL_WifiApFlag {
        SDL_WifiApFlagConnected(0x1);

        private int mValue = 0;
        private SDL_WifiApFlag(int value) {
            mValue = value;
        }
        public int value() {
            return mValue;
        }
    }

    public class SDLWifiApInfo {
        public String ssid;
        public int rssi;
        public int flags;
    }

    private String ajdustSSID(String ssid) {
        String ret = ssid;
        if (ssid != null && ssid.startsWith("\"")) {
            // getSSID得到的ssid名称极可能是前后加了双引号，像"leagor"。此处去掉这双引号
            ret = ssid.replace("\"", "");
        }
        return ret;
    }

    public List<SDLWifi.SDLWifiApInfo> wifiGetScanResults() {
        Log.w(TAG, "Trying to wifiGetScanResults");

        if (wifiManager.isWifiEnabled() && resultList.isEmpty()) {
            // wifi已开启，但没有搜到ap，此处不待通知，强制获取
            // 安装后第一次运行，打开ACCESS_FINE_LOCATION权限，之后可能会进入这里
            resultList = wifiManager.getScanResults();
            sortByLevel(resultList);
        }

        List<SDLWifi.SDLWifiApInfo> result = new ArrayList<SDLWifi.SDLWifiApInfo>();
        // 连接ssid，但输入密码，调用wifiMananger.getConnectionInfo()得到WifiInfo，当中getSSID得到的依旧有可能是有效的ssid。
        // 为此先用connectivityManager进行判断，看wifi是否处于已连接状态
        String ssid = null;
        NetworkInfo networkInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (networkInfo != null && networkInfo.isConnected()) {
            android.net.wifi.WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            if (wifiInfo != null) {
                ssid = ajdustSSID(wifiInfo.getSSID());
            }
            if (ssid == null || ssid.isEmpty()) {
                // 网上有说，8.0/9.0时，WifiInfo.getSSID获取Wifi名字可能为空
                ssid = ajdustSSID(networkInfo.getExtraInfo());
            }
        }

        for (int i = 0; i < resultList.size(); i++){
            android.net.wifi.ScanResult wifi = resultList.get(i);
            if (wifi.SSID == null || wifi.SSID.isEmpty()) {
                continue;
            }
            SDLWifi.SDLWifiApInfo info = new SDLWifiApInfo();

            info.ssid = wifi.SSID;
            info.rssi = wifi.level;
            info.flags = 0;
            if (ssid != null && ssid.equals(wifi.SSID)) {
                info.flags += SDL_WifiApFlag.SDL_WifiApFlagConnected.value();
            }
            result.add(info);
        }
        mWifiScanResultChanged = false;
        return result;
    }

    public boolean wifiConnect(String ssid, String password) {
        Log.w(TAG, "Trying to wifiConnect, ssid: " + ssid + ", password: " + password);
        int result = 0;
        int netWorkId = AddWifiConfig(resultList, ssid, password);//添加该网络的配置
        if (netWorkId !=-1) {
            getConfigurations();
            boolean isConnect = ConnectWifi(netWorkId);
            if (isConnect) {
                result = 1;
            }

        }
        return true;
    }

    public boolean wifiRemove(String ssid) {
        if (ssid == null) {
            return false;
        }
        getConfigurations();
        int networkId = isConfigured(ssid);
        if (networkId == -1) {
            Log.w(TAG, "Desire to remove networkId of ssid: " + ssid + " isn't existed");
            return false;
        }

        Log.w(TAG, "Trying to wifiRemove, ssid: " + ssid);
        return wifiManager.removeNetwork(networkId);
    }


}
