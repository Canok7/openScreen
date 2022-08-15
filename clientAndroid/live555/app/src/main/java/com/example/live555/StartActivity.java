package com.example.live555;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;


import com.example.live555.databinding.ActivityStartBinding;

public class StartActivity extends AppCompatActivity {
    private static final String TAG = "StartActivity";
    private ActivityStartBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityStartBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        binding.server.setOnClickListener(
                (v) -> {
                    Log.d(TAG, "onCreate: start Server");
                    startActivity(new Intent(StartActivity.this, CameraServerActivity.class));
                }
        );

        binding.client.setOnClickListener(
                (v) -> {
                    Log.d(TAG, "onCreate: start client");
                    startActivity(new Intent(StartActivity.this, ClientActivity.class));
                }
        );
    }

}