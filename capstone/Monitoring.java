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

public class Monitoring extends AppCompatActivity {

    private static final String TAG = "Monitoring";
    private final UUID portUuid = UUID.fromString("0000ffe0-0000-1000-8000-00805f9b34fb");

    private final int REQUEST_BLUETOOTH_PERMISSIONS = 1;

    private TextView engineLoadTextView;
    private TextView coolantTempTextView;
    private TextView shortTermFuelTrim1TextView;
    private TextView longTermFuelTrim1TextView;
    private TextView shortTermFuelTrim2TextView;
    private TextView longTermFuelTrim2TextView;
    private TextView intakeMapTextView;
    private TextView rpmTextView;
    private TextView speedTextView;
    private TextView timingAdvanceTextView;
    private TextView intakeTempTextView;
    private TextView throttleTextView;
    private TextView runtimeTextView;
    private TextView distanceWithMILTextView;
    private TextView commandedEvaporativePurgeTextView;
    private TextView fuelLevelTextView;
    private TextView warmUpsTextView;
    private TextView distanceTextView;
    private TextView evapSysVaporPressureTextView;
    private TextView barometricTextView;
    private TextView catalystTempB1S1TextView;
    private TextView catalystTempB2S1TextView;
    private TextView controlModuleVoltageTextView;
    private TextView absoluteEngineLoadTextView;
    private TextView airFuelRatioTextView;
    private TextView relativeThrottlePosTextView;
    private TextView ambientTempTextView;
    private TextView absoluteThrottlePosBTextView;
    private TextView accPedalPosDTextView;
    private TextView accPedalPosETextView;
    private TextView commandedThrottleActuatorTextView;

    ImageButton back_button;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_monitoring); // XML 레이아웃을 사용

        engineLoadTextView = findViewById(R.id.engineLoad);
        coolantTempTextView = findViewById(R.id.coolantTemp);
        shortTermFuelTrim1TextView = findViewById(R.id.shortTermFuelTrim1);
        longTermFuelTrim1TextView = findViewById(R.id.longTermFuelTrim1);
        shortTermFuelTrim2TextView = findViewById(R.id.shortTermFuelTrim2);
        longTermFuelTrim2TextView = findViewById(R.id.longTermFuelTrim2);
        intakeMapTextView = findViewById(R.id.intakeMap);
        rpmTextView = findViewById(R.id.rpm);
        speedTextView = findViewById(R.id.speed);
        timingAdvanceTextView = findViewById(R.id.timingAdvance);
        intakeTempTextView = findViewById(R.id.intakeTemp);
        throttleTextView = findViewById(R.id.throttle);
        runtimeTextView = findViewById(R.id.runtime);
        distanceWithMILTextView = findViewById(R.id.distanceWithMIL);
        commandedEvaporativePurgeTextView = findViewById(R.id.commandedEvaporativePurge);
        fuelLevelTextView = findViewById(R.id.fuelLevel);
        warmUpsTextView = findViewById(R.id.warmUps);
        distanceTextView = findViewById(R.id.distance);
        evapSysVaporPressureTextView = findViewById(R.id.evapSysVaporPressure);
        barometricTextView = findViewById(R.id.barometric);
        catalystTempB1S1TextView = findViewById(R.id.catalystTempB1S1);
        catalystTempB2S1TextView = findViewById(R.id.catalystTempB2S1);
        controlModuleVoltageTextView = findViewById(R.id.controlModuleVoltage);
        absoluteEngineLoadTextView = findViewById(R.id.absoluteEngineLoad);
        airFuelRatioTextView = findViewById(R.id.airFuelRatio);
        relativeThrottlePosTextView = findViewById(R.id.relativeThrottlePos);
        ambientTempTextView = findViewById(R.id.ambientTemp);
        absoluteThrottlePosBTextView = findViewById(R.id.absoluteThrottlePosB);
        accPedalPosDTextView = findViewById(R.id.accPedalPosD);
        accPedalPosETextView = findViewById(R.id.accPedalPosE);
        commandedThrottleActuatorTextView = findViewById(R.id.commandedThrottleActuator);

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

        if (values.length >= 31) {
            updateTextView(engineLoadTextView, values[0], "%");
            updateTextView(coolantTempTextView, values[1], "°C");
            updateTextView(shortTermFuelTrim1TextView, values[2], "km");
            updateTextView(longTermFuelTrim1TextView, values[3], "km");
            updateTextView(shortTermFuelTrim2TextView, values[4], "km");
            updateTextView(longTermFuelTrim2TextView, values[5], "km");
            updateTextView(intakeMapTextView, values[6], "%");
            updateTextView(rpmTextView, values[7], "");
            updateTextView(speedTextView, values[8], "km/h");
            updateTextView(timingAdvanceTextView, values[9], "");
            updateTextView(intakeTempTextView, values[10], "°C");
            updateTextView(throttleTextView, values[11], "%");
            updateTextView(runtimeTextView, values[12], "초");
            updateTextView(distanceWithMILTextView, values[13], "mph");
            updateTextView(commandedEvaporativePurgeTextView, values[14], "");
            updateTextView(fuelLevelTextView, values[15], "km");
            updateTextView(warmUpsTextView, values[16], "초");
            updateTextView(distanceTextView, values[17], "km/h");
            updateTextView(evapSysVaporPressureTextView, values[18], "%");
            updateTextView(barometricTextView, values[19], "%");
            updateTextView(catalystTempB1S1TextView, values[20], "°C");
            updateTextView(catalystTempB2S1TextView, values[21], "°C");
            updateTextView(controlModuleVoltageTextView, values[22], "V");
            updateTextView(absoluteEngineLoadTextView, values[23], "%");
            updateTextView(airFuelRatioTextView, values[24], "%");
            updateTextView(relativeThrottlePosTextView, values[25], "%");
            updateTextView(ambientTempTextView, values[26], "°C");
            updateTextView(absoluteThrottlePosBTextView, values[27], "°");
            updateTextView(accPedalPosDTextView, values[28], "°");
            updateTextView(accPedalPosETextView, values[29], "°");
            updateTextView(commandedThrottleActuatorTextView, values[30], "");
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
