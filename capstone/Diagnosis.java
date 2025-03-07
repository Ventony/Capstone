package com.example.capstone;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.opencsv.CSVReader;

import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

public class Diagnosis extends AppCompatActivity {
    private static final int REQUEST_ENABLE_BT = 1;
    private static final int REQUEST_PERMISSION_LOCATION = 2;
    private static final String TAG = "Diagnosis";

    private Map<String, String> errorCodeMap;
    private Map<String, String> errorCodeMapEng;
    private List<String> errorCodesList;
    private ErrorsAdapter errorsAdapter;

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothGatt bluetoothGatt;
    private BluetoothGattCharacteristic characteristic;
    private static final UUID SERVICE_UUID = UUID.fromString("0000FFF0-0000-1000-8000-00805F9B34FB");
    private static final UUID CHARACTERISTIC_UUID = UUID.fromString("0000FFF1-0000-1000-8000-00805F9B34FB");
    private static final String ARDUINO_MAC_ADDRESS = "50:51:A9:FE:FE:2C"; // 실제 블루투스 MAC 주소
    private boolean isBluetoothConnected = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        WindowInsetsControllerCompat windowInsetsController = ViewCompat.getWindowInsetsController(getWindow().getDecorView());
        if (windowInsetsController != null) {
            windowInsetsController.hide(WindowInsetsCompat.Type.systemBars());
            windowInsetsController.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }

        setContentView(R.layout.activity_diagnosis);

        Toolbar diag_toolbar = findViewById(R.id.diag_toolbar);
        setSupportActionBar(diag_toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle("차량진단");

        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            v.setPadding(insets.getInsets(WindowInsetsCompat.Type.systemBars()).left,
                    insets.getInsets(WindowInsetsCompat.Type.systemBars()).top,
                    insets.getInsets(WindowInsetsCompat.Type.systemBars()).right,
                    insets.getInsets(WindowInsetsCompat.Type.systemBars()).bottom);
            return insets;
        });

        // UI 요소 초기화
        Button buttonDiagnose = findViewById(R.id.buttonDiagnose);
        Button buttonClearErrors = findViewById(R.id.buttonClearErrors);
        RecyclerView recyclerViewErrors = findViewById(R.id.recyclerViewErrors);

        // RecyclerView 설정
        recyclerViewErrors.setLayoutManager(new LinearLayoutManager(this));
        errorCodesList = new ArrayList<>();
        errorCodeMap = new HashMap<>();
        errorCodeMapEng = new HashMap<>();
        errorsAdapter = new ErrorsAdapter(errorCodesList, errorCodeMap, errorCodeMapEng);
        recyclerViewErrors.setAdapter(errorsAdapter);

        // CSV 파일 읽기 및 오류 코드 맵 초기화
        loadCsvData();

        // Bluetooth 초기화
        initializeBluetooth();

        // 버튼 클릭 리스너 설정
        buttonDiagnose.setOnClickListener(v -> {
            if (isBluetoothConnected) {
                sendSignalToArduino("4");
            } else {
                Toast.makeText(this, "Bluetooth 연결 실패. 장치에 연결되어 있지 않습니다.", Toast.LENGTH_SHORT).show();
            }
        });

        // 전체 삭제 버튼 클릭 리스너 설정
        buttonClearErrors.setOnClickListener(v -> {
            errorCodesList.clear(); // 오류 코드 리스트 초기화
            errorsAdapter.notifyDataSetChanged(); // 어댑터 갱신
            if (isBluetoothConnected) {
                sendSignalToArduino("5");
            } else {
                Toast.makeText(this, "Bluetooth 연결 실패. 장치에 연결되어 있지 않습니다.", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void loadCsvData() {
        try {
            InputStreamReader inputStreamReader = new InputStreamReader(getAssets().open("DTC.csv"));
            CSVReader reader = new CSVReader(inputStreamReader);
            String[] nextLine;
            while ((nextLine = reader.readNext()) != null) {
                if (nextLine.length >= 2) {
                    String errorCode = nextLine[0].trim();
                    String errorDescription = nextLine[1].trim();
                    errorCodeMap.put(errorCode, errorDescription);
                }
            }
            reader.close();

            InputStreamReader inputStreamReaderEng = new InputStreamReader(getAssets().open("DTC_ENG.csv"));
            CSVReader readerEng = new CSVReader(inputStreamReaderEng);
            String[] nextLineEng;
            while ((nextLineEng = readerEng.readNext()) != null) {
                if (nextLineEng.length >= 2) {
                    String errorCode = nextLineEng[0].trim();
                    String errorDescriptionEng = nextLineEng[1].trim();
                    errorCodeMapEng.put(errorCode, errorDescriptionEng);
                }
            }
            readerEng.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void displayErrorCodes(String errorCodes) {
        errorCodesList.clear(); // 기존 오류 코드 초기화
        String[] codes = errorCodes.split(",");

        if (codes.length == 1 && codes[0].trim().equals("666")) {
            // "차량에 이상이 없습니다." 메시지 표시
            errorCodesList.add("차량에 이상이 없습니다.");
        } else {
            for (String code : codes) {
                errorCodesList.add(code.trim());
            }
        }

        errorsAdapter.notifyDataSetChanged(); // 어댑터 갱신
    }

    private void sendSignalToArduino(String signal) {
        if (characteristic != null) {
            characteristic.setValue(signal);
            bluetoothGatt.writeCharacteristic(characteristic);
        }
    }

    private void initializeBluetooth() {
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null) {
            Toast.makeText(this, "Bluetooth not supported on this device", Toast.LENGTH_SHORT).show();
            return;
        }

        if (!bluetoothAdapter.isEnabled()) {
            Toast.makeText(this, "Please enable Bluetooth and restart the app", Toast.LENGTH_SHORT).show();
            return;
        }

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_ADMIN) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.BLUETOOTH, Manifest.permission.BLUETOOTH_ADMIN},
                    REQUEST_PERMISSION_LOCATION);
        } else {
            connectToArduino();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_PERMISSION_LOCATION) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                connectToArduino();
            } else {
                Toast.makeText(this, "Location permission is required for Bluetooth connection", Toast.LENGTH_SHORT).show();
            }
        }
    }

    private void connectToArduino() {
        try {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED ||
                    ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_ADMIN) != PackageManager.PERMISSION_GRANTED ||
                    ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {

                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.BLUETOOTH, Manifest.permission.BLUETOOTH_ADMIN, Manifest.permission.ACCESS_FINE_LOCATION},
                        REQUEST_PERMISSION_LOCATION);
            } else {
                BluetoothDevice arduinoDevice = bluetoothAdapter.getRemoteDevice(ARDUINO_MAC_ADDRESS);
                bluetoothGatt = arduinoDevice.connectGatt(this, false, gattCallback);
            }
        } catch (SecurityException e) {
            e.printStackTrace();
            Toast.makeText(this, "Bluetooth permission required", Toast.LENGTH_SHORT).show();
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.BLUETOOTH, Manifest.permission.BLUETOOTH_ADMIN, Manifest.permission.ACCESS_FINE_LOCATION},
                    REQUEST_PERMISSION_LOCATION);
        }
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == BluetoothGatt.STATE_CONNECTED) {
                Log.i(TAG, "Connected to GATT server.");
                bluetoothGatt.discoverServices();
            } else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
                Log.i(TAG, "Disconnected from GATT server.");
                isBluetoothConnected = false;
                runOnUiThread(() -> Toast.makeText(Diagnosis.this, "Bluetooth 연결이 끊어졌습니다.", Toast.LENGTH_SHORT).show());
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                BluetoothGattService service = bluetoothGatt.getService(SERVICE_UUID);
                if (service != null) {
                    characteristic = service.getCharacteristic(CHARACTERISTIC_UUID);
                    if (characteristic != null) {
                        isBluetoothConnected = true;
                        runOnUiThread(() -> Toast.makeText(Diagnosis.this, "Bluetooth 연결이 완료되었습니다.", Toast.LENGTH_SHORT).show());
                    } else {
                        Log.e(TAG, "Characteristic not found.");
                    }
                } else {
                    Log.e(TAG, "Service not found.");
                }
            } else {
                Log.e(TAG, "onServicesDiscovered received: " + status);
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                String data = new String(characteristic.getValue());
                Log.i(TAG, "Characteristic read: " + data);
                runOnUiThread(() -> displayErrorCodes(data));
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            String data = new String(characteristic.getValue());
            Log.i(TAG, "Characteristic changed: " + data);
            runOnUiThread(() -> displayErrorCodes(data));
        }
    };

    private class ErrorsAdapter extends RecyclerView.Adapter<ErrorsAdapter.ErrorViewHolder> {
        private final List<String> errorCodesList;
        private final Map<String, String> errorCodeMap;
        private final Map<String, String> errorCodeMapEng;

        public ErrorsAdapter(List<String> errorCodesList, Map<String, String> errorCodeMap, Map<String, String> errorCodeMapEng) {
            this.errorCodesList = errorCodesList;
            this.errorCodeMap = errorCodeMap;
            this.errorCodeMapEng = errorCodeMapEng;
        }

        @NonNull
        @Override
        public ErrorViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_error, parent, false);
            return new ErrorViewHolder(view);
        }

        @Override
        public void onBindViewHolder(@NonNull ErrorViewHolder holder, int position) {
            String errorCode = errorCodesList.get(position);
            if (errorCode.equals("차량에 이상이 없습니다.")) {
                holder.bindNoErrors();
            } else {
                holder.bind(errorCode, errorCodeMap.get(errorCode), errorCodeMapEng.get(errorCode));
            }
        }

        @Override
        public int getItemCount() {
            return errorCodesList.size();
        }

        class ErrorViewHolder extends RecyclerView.ViewHolder {
            private final TextView textViewErrorCode;
            private final TextView textViewErrorDescription;
            private final TextView textViewErrorDescriptionEng;

            public ErrorViewHolder(@NonNull View itemView) {
                super(itemView);
                textViewErrorCode = itemView.findViewById(R.id.textViewErrorCode);
                textViewErrorDescription = itemView.findViewById(R.id.textViewErrorDescription);
                textViewErrorDescriptionEng = itemView.findViewById(R.id.textViewErrorDescriptionEng);
            }

            public void bind(String errorCode, String errorDescription, String errorDescriptionEng) {
                textViewErrorCode.setText("오류 코드: " + errorCode);
                textViewErrorDescription.setText(errorDescription != null ? errorDescription : "오류 설명을 찾을 수 없습니다.");
                textViewErrorDescriptionEng.setText(errorDescriptionEng != null ? errorDescriptionEng : "Error description not available.");
            }

            public void bindNoErrors() {
                textViewErrorCode.setText("차량에 이상이 없습니다.");
                textViewErrorDescription.setText("");
                textViewErrorDescriptionEng.setText("");
            }
        }
    }
}
