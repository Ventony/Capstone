package com.example.capstone;

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;

import java.util.UUID;

public class BluetoothService extends Service {
    private static final String TAG = "BluetoothService";
    private final UUID serviceUuid = UUID.fromString("0000FFF0-0000-1000-8000-00805F9B34FB");
    private final UUID characteristicUuid = UUID.fromString("0000FFF1-0000-1000-8000-00805F9B34FB");
    private final IBinder binder = new LocalBinder();
    private BluetoothGatt bluetoothGatt;
    private BluetoothGattCharacteristic characteristic;
    private StringBuilder receivedData = new StringBuilder();

    public class LocalBinder extends Binder {
        BluetoothService getService() {
            return BluetoothService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_NOT_STICKY;
    }

    public boolean connectToDevice(String deviceAddress) {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        BluetoothDevice device = bluetoothAdapter.getRemoteDevice(deviceAddress);
        bluetoothAdapter.cancelDiscovery();
        bluetoothGatt = device.connectGatt(this, false, gattCallback);
        return true;
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            super.onConnectionStateChange(gatt, status, newState);
            if (newState == BluetoothGatt.STATE_CONNECTED) {
                Log.i(TAG, "Connected to GATT server.");
                bluetoothGatt.discoverServices();
            } else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
                Log.i(TAG, "Disconnected from GATT server.");
                Toast.makeText(BluetoothService.this, "Disconnected from GATT server.", Toast.LENGTH_SHORT).show();
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            super.onServicesDiscovered(gatt, status);
            if (status == BluetoothGatt.GATT_SUCCESS) {
                BluetoothGattService service = bluetoothGatt.getService(serviceUuid);
                if (service != null) {
                    characteristic = service.getCharacteristic(characteristicUuid);
                    if (characteristic != null) {
                        Log.i(TAG, "Characteristic found.");
                    } else {
                        Log.e(TAG, "Characteristic not found.");
                    }
                } else {
                    Log.e(TAG, "Service not found.");
                }
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            super.onCharacteristicRead(gatt, characteristic, status);
            if (status == BluetoothGatt.GATT_SUCCESS) {
                String data = new String(characteristic.getValue());
                Log.i(TAG, "Characteristic read: " + data);
                receivedData.append(data);
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            super.onCharacteristicChanged(gatt, characteristic);
            String data = new String(characteristic.getValue());
            Log.i(TAG, "Characteristic changed: " + data);
            receivedData.append(data);
        }
    };

    public void writeData(String data) {
        if (bluetoothGatt != null && characteristic != null) {
            characteristic.setValue(data);
            bluetoothGatt.writeCharacteristic(characteristic);
        } else {
            Log.e(TAG, "Cannot write data, GATT or Characteristic is null");
        }
    }

    public void disconnect() {
        if (bluetoothGatt != null) {
            bluetoothGatt.disconnect();
            bluetoothGatt.close();
            bluetoothGatt = null;
        }
    }

    public boolean isConnected() {
        return bluetoothGatt != null && bluetoothGatt.connect();
    }

    public String readData() {
        if (receivedData.length() > 0) {
            String data = receivedData.toString();
            receivedData.setLength(0); // Clear the buffer
            return data;
        }
        return null;
    }
}
