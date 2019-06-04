package com.appvilo.sudokusolver.utils;

import android.os.Environment;

import java.io.File;

public class Constants {


    private Constants(){
        //cannot be instantiated
    }

    public static final String SCAN_IMAGE_LOCATION = Environment.getExternalStorageDirectory() + File.separator + "OpenScanner";

    public static final String TEST_LOCATION = Environment.getExternalStorageDirectory() + "";
    public static final String SUDOKU_LOCATION = Environment.getExternalStorageDirectory()  + File.separator + "Sudoku";
    public static final String EXTRA_MESSAGE = "com.example.myfirstapp.MESSAGE";
}
