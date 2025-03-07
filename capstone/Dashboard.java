package com.example.capstone;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.IOException;
import java.io.InputStream;
import java.util.UUID;

public class Dashboard extends AppCompatActivity {

    private static final String TAG = "Dashboard";
    private final UUID portUuid = UUID.fromString("0000ffe0-0000-1000-8000-00805f9b34fb");

    private final int REQUEST_BLUETOOTH_PERMISSIONS = 1;

    private TextView engineLoadTextView;
    private TextView coolantTempTextView;
    private TextView intakeMapTextView;
    private TextView rpmTextView;
    private TextView speedTextView;
    private TextView intakeTempTextView;
    private TextView throttleTextView;
    private TextView airFuelRatioTextView;
    ImageButton back_button;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dashboard); // XML 레이아웃을 사용

        engineLoadTextView = findViewById(R.id.engineLoad);
        coolantTempTextView = findViewById(R.id.coolantTemp);
        intakeMapTextView = findViewById(R.id.intakeMap);
        rpmTextView = findViewById(R.id.rpm);
        speedTextView = findViewById(R.id.speed);
        intakeTempTextView = findViewById(R.id.intakeTemp);
        throttleTextView = findViewById(R.id.throttle);
        airFuelRatioTextView = findViewById(R.id.airFuelRatio);
        back_button = findViewById(R.id.back_Button);

        back_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(getApplicationContext(), MainActivity.class);
                startActivityForResult(intent, 101);
            }
        });

        checkPermissionsAndSetupBluetooth();
    }

    private void checkPermissionsAndSetupBluetooth() {
        String[] permissions = {
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_ADMIN
        };

        boolean permissionsToRequest = false;
        for (String permission : permissions) {
            if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                permissionsToRequest = true;
                break;
            }
        }

        if (permissionsToRequest) {
            ActivityCompat.requestPermissions(this, permissions, REQUEST_BLUETOOTH_PERMISSIONS);
        } else {
            setupBluetooth();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_BLUETOOTH_PERMISSIONS) {
            boolean allPermissionsGranted = true;
            for (int result : grantResults) {
                if (result != PackageManager.PERMISSION_GRANTED) {
                    allPermissionsGranted = false;
                    break;
                }
            }

            if (allPermissionsGranted) {
                setupBluetooth();
            } else {
                // 권한이 거부된 경우 처리
                Log.e(TAG, "Bluetooth 권한이 거부되었습니다.");
                Toast.makeText(this, "Bluetooth 권한이 거부되었습니다.", Toast.LENGTH_SHORT).show();
            }
        }
    }

    private void setupBluetooth() {
        BluetoothManager bluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        BluetoothAdapter bluetoothAdapter = bluetoothManager.getAdapter();

        if (bluetoothAdapter == null) {
            Log.e(TAG, "블루투스 어댑터를 사용할 수 없습니다.");
            Toast.makeText(this, "블루투스 어댑터를 사용할 수 없습니다.", Toast.LENGTH_SHORT).show();
            return;
        }

        if (!bluetoothAdapter.isEnabled()) {
            Log.e(TAG, "블루투스가 활성화되지 않았습니다.");
            Toast.makeText(this, "블루투스가 활성화되지 않았습니다.", Toast.LENGTH_SHORT).show();
            return;
        }

        // 권한 확인
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_ADMIN) != PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG, "Bluetooth 권한이 없습니다.");
            Toast.makeText(this, "Bluetooth 권한이 부족합니다.", Toast.LENGTH_SHORT).show();
            return;
        }

        String deviceAddress = "50:51:A9:FE:FE:2C"; // Bluetooth 모듈 주소
        BluetoothDevice device = bluetoothAdapter.getRemoteDevice(deviceAddress);
        bluetoothAdapter.cancelDiscovery();
        try (BluetoothSocket bluetoothSocket = device.createRfcommSocketToServiceRecord(portUuid)) {
            bluetoothSocket.connect();
            try (InputStream inputStream = bluetoothSocket.getInputStream()) {
                listenForData(inputStream);
            }
        } catch (IOException e) {
            Log.e(TAG, "블루투스 연결에 실패했습니다.", e);
            Toast.makeText(this, "블루투스 연결에 실패했습니다.", Toast.LENGTH_SHORT).show();
        } catch (SecurityException e) {
            Log.e(TAG, "블루투스 권한이 부족합니다.", e);
            Toast.makeText(this, "블루투스 권한이 부족합니다.", Toast.LENGTH_SHORT).show();
        }
    }

    private void listenForData(InputStream inputStream) {
        final byte[] buffer = new byte[1024];

        new Thread(() -> {
            try {
                int bytes;
                while ((bytes = inputStream.read(buffer)) != -1) {
                    String data = new String(buffer, 0, bytes);
                    runOnUiThread(() -> updateUI(data));
                }
            } catch (IOException e) {
                runOnUiThread(() -> {
                    Log.e(TAG, "데이터 수신 중 오류가 발생했습니다.", e);
                    Toast.makeText(this, "데이터 수신 중 오류가 발생했습니다.", Toast.LENGTH_SHORT).show();
                });
            }
        }).start();
    }

    private void updateUI(String data) {
        String[] values = data.split(",");

        if (values.length >= 8) {
            updateTextView(rpmTextView, values[0], "");
            updateTextView(speedTextView, values[1], " km/h");
            updateTextView(coolantTempTextView, values[2], " °C");
            updateTextView(intakeTempTextView, values[3], " °C");
            updateTextView(engineLoadTextView, values[4], " %");
            updateTextView(throttleTextView, values[5], " %");
            updateTextView(intakeMapTextView, values[6], " %");
            updateTextView(airFuelRatioTextView, values[7], " %");
        } else {
            Log.e(TAG, "데이터의 길이가 충분하지 않습니다: " + data);
        }
    }

    private void updateTextView(TextView textView, String value, String unit) {
        if (value.isEmpty() || value.equals("--")) {
            textView.setText("해당 차량에서는 지원하지 않는 센서 값");
        } else {
            textView.setText(value + unit);
        }
    }
}
