package com.example.capstone;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

public class Records extends AppCompatActivity {
    double startLatitude = 37.1234; // 시작 위치 위도
    double startLongitude = -122.1234; // 시작 위치 경도
    double destinationLatitude = 37.567; // 도착 위치 위도
    double destinationLongitude = -122.987; // 도착 위치 경도

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_records);

        startNavigation();
    }

    private void startNavigation() {
        // 네비게이션 앱 실행을 위한 인텐트 생성
        Uri gmmIntentUri = Uri.parse("google.navigation:q=" + destinationLatitude + "," + destinationLongitude +
                "&mode=d"); // mode=d로 설정하면 운전 경로가 표시됩니다.

        // 인텐트 실행
        Intent mapIntent = new Intent(Intent.ACTION_VIEW, gmmIntentUri);
        mapIntent.setPackage("com.google.android.apps.maps");
        if (mapIntent.resolveActivity(getPackageManager()) != null) {
            startActivity(mapIntent);
        }
    }
}
