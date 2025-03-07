package com.example.capstone;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageButton;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    public static final int REQUEST_CODE_MENU = 101;
    private BluetoothService bluetoothService;
    private boolean isBound = false;

    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            BluetoothService.LocalBinder binder = (BluetoothService.LocalBinder) service;
            bluetoothService = binder.getService();
            isBound = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            isBound = false;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Intent intent = new Intent(this, BluetoothService.class);
        bindService(intent, connection, Context.BIND_AUTO_CREATE);

        ImageButton bluetooth_button = findViewById(R.id.bt_button);
        bluetooth_button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(getApplicationContext(), bluetoothDeviceLists.class);
                startActivityForResult(intent, REQUEST_CODE_MENU);
            }
        });

        ImageButton dash_btn = findViewById(R.id.dashboard);
        dash_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (isBound) {
                    bluetoothService.writeData("2");
                }
                Intent intent = new Intent(getApplicationContext(), Dashboard.class);
                startActivityForResult(intent, REQUEST_CODE_MENU);
            }
        });

        ImageButton monitor_btn = findViewById(R.id.monitoring);
        monitor_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (isBound) {
                    bluetoothService.writeData("1");
                }
                Intent intent = new Intent(getApplicationContext(), Monitoring.class);
                startActivityForResult(intent, REQUEST_CODE_MENU);
            }
        });

        ImageButton Diag_btn = findViewById(R.id.test);
        Diag_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (isBound) {
                    bluetoothService.writeData("4");
                }
                Intent intent = new Intent(getApplicationContext(), Diagnosis.class);
                startActivityForResult(intent, REQUEST_CODE_MENU);
            }
        });

        ImageButton record_btn = findViewById(R.id.record);
        record_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (isBound) {
                    bluetoothService.writeData("3");
                }
                Intent intent = new Intent(getApplicationContext(), Records.class);
                startActivityForResult(intent, REQUEST_CODE_MENU);
            }
        });

        ImageButton location_btn = findViewById(R.id.locations);
        location_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(getApplicationContext(), Locations.class);
                startActivityForResult(intent, REQUEST_CODE_MENU);
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater menuInflater = getMenuInflater();
        menuInflater.inflate(R.menu.menu_toolbar, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.menu) {
            Intent NewActivity = new Intent(getApplicationContext(), bluetoothDeviceLists.class);
            startActivity(NewActivity);
        }
        return true;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE_MENU && resultCode == RESULT_OK) {
            String deviceAddress = data.getStringExtra("device_address");
            if (isBound && deviceAddress != null) {
                bluetoothService.connectToDevice(deviceAddress);
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isBound) {
            unbindService(connection);
            isBound = false;
        }
    }
}
