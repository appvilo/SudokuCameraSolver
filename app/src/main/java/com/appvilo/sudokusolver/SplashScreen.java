package com.appvilo.sudokusolver;

import android.content.Intent;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import com.appvilo.sudokusolver.utils.Constants;

public class SplashScreen extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_splash_screen);

        Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {

                Intent intent = new Intent(SplashScreen.this, MainActivity.class);
//                intent.putExtra(Constants.EXTRA_MESSAGE, Constants.TEST_LOCATION1 + File.separator + "sudoku1.jpg");
                intent.putExtra(Constants.EXTRA_MESSAGE, "");
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(intent);
                finish();

            }
        }, 200);
    }
}
