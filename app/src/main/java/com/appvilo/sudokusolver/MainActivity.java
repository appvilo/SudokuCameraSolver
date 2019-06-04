package com.appvilo.sudokusolver;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.res.AssetManager;
import android.graphics.Point;
import android.os.AsyncTask;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.Toast;

import com.appvilo.sudokusolver.utils.FolderUtil;
import com.karumi.dexter.Dexter;
import com.karumi.dexter.MultiplePermissionsReport;
import com.karumi.dexter.PermissionToken;
import com.karumi.dexter.listener.PermissionRequest;
import com.karumi.dexter.listener.multi.MultiplePermissionsListener;
import com.appvilo.sudokusolver.utils.Constants;

import org.opencv.core.Mat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("opencv_java3");
        System.loadLibrary("native-lib");
    }

    private static final String TAG = MainActivity.class.getSimpleName();
    private static final int PICK_IMAGE_ID = 303;

    public GridLayout gridLayout;
    private String mFileName = "";

    private static int nRows = 9, nCols = 9;
    static final String[] numbers = new String[nRows*nCols];

    Mat mMat;
    ImageView btnSolve, btnAgain, btnCamera, btnUnsolve;
    int dimensions;

    private int[][] unsole=new int[9][9];

    int check = 0;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        setupUI(findViewById(R.id.close_keyboard));

        Dexter.withActivity(this)
                .withPermissions(Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.CAMERA)
                .withListener(new MultiplePermissionsListener() {
                    @Override
                    public void onPermissionsChecked(MultiplePermissionsReport report) {
                        if(!report.areAllPermissionsGranted()){
                            Toast.makeText(MainActivity.this, "You need to grant all permission to use this app features", Toast.LENGTH_SHORT).show();
                        }
                    }

                    @Override
                    public void onPermissionRationaleShouldBeShown(List<PermissionRequest> permissions, PermissionToken token) {

                    }
                })
                .check();

        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);

        btnSolve = (ImageView)findViewById(R.id.solve);
        btnAgain = (ImageView)findViewById(R.id.clearButton);
        btnCamera = (ImageView)findViewById(R.id.solveButton);
        btnUnsolve = (ImageView)findViewById(R.id.unsolve) ;

        gridLayout =(GridLayout)findViewById(R.id.sudokuGrid);

        //get screen size in pixels to adjust size of sudoku cells
        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        dimensions = size.x/11;
        SudokuGrid.initGrid(this, gridLayout, dimensions);

        if(mFileName.isEmpty()) {
            btnSolve.setVisibility(View.GONE);
            btnAgain.setVisibility(View.GONE);
            btnUnsolve.setVisibility(View.GONE);

        }


        btnAgain.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                int [][] array = new int [9][9];
                for(int i=0;i<nRows;i++)
                    for(int j=0;j<nCols;j++)
                        array[i][j] = 0;

                SudokuGrid.cellValues = array;
//
//                btnSolve.setVisibility(View.GONE);
//                btnAgain.setVisibility(View.GONE);
//                btnCamera.setVisibility(View.VISIBLE);
//

                check = 0;
                gridLayout.removeAllViews();
                SudokuGrid.initGrid(MainActivity.this, gridLayout, dimensions);
                Intent intent = new Intent(MainActivity.this, CameraActivity.class);
                startActivity(intent);
            }
        });

        btnSolve.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SudokuGrid.getCellValues();
                if(!SudokuGrid.getSolution()){
                    Toast.makeText(getApplicationContext(),"Solution does not exist",Toast.LENGTH_SHORT).show();
                    return;
                }
                Toast.makeText(getApplicationContext(),"Solution Found",Toast.LENGTH_SHORT).show();
                SudokuGrid.updateSolution();

                btnSolve.setVisibility(View.GONE);
                btnUnsolve.setVisibility(View.VISIBLE);

                check = 1;
            }
        });

        btnUnsolve.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {


            /*    int [][] array = new int [9][9];
                for(int i=0;i<nRows;i++)
                    for(int j=0;j<nCols;j++)
                        array[i][j] = 0;
*/
  //              SudokuGrid.cellValues = unsole;



                int [][] array = new int [9][9];

                if(unsole.length>0) {



                    for (int i = 0; i < nRows; i++)
                        for (int j = 0; j < nCols; j++)
                    array[i][j] = unsole[i][j];

                }else{
                    Log.e("GNIDA1", "PYSTO");
                }



                SudokuGrid.cellValues = array;



                btnSolve.setVisibility(View.VISIBLE);
                btnUnsolve.setVisibility(View.GONE);
//
                gridLayout.removeAllViews();
                SudokuGrid.initGrid(MainActivity.this, gridLayout, dimensions);





            }
        });

        btnCamera.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                check = 0;

                Intent intent = new Intent(MainActivity.this, CameraActivity.class);
                startActivity(intent);
//                finish();
            }
        });

        Log.d("test1", "onCreate - main!");

        Intent intent = getIntent();
//        mFileName = intent.getStringExtra(Constants.EXTRA_MESSAGE);
        long addr = intent.getLongExtra("Image", 0);
        if(addr!=0) {
            Mat temp = new Mat(addr);
            mMat = temp.clone();
            Sudoku_Scan();
        }
    }



    public void setupUI(View view) {

        try {
            // Set up touch listener for non-text box views to hide keyboard.
            if (!(view instanceof EditText)) {
                view.setOnTouchListener(new View.OnTouchListener() {
                    public boolean onTouch(View v, MotionEvent event) {
                        hideSoftKeyboard(MainActivity.this);
                        return false;
                    }
                });
            }

            //If a layout container, iterate over children and seed recursion.
            if (view instanceof ViewGroup) {
                for (int i = 0; i < ((ViewGroup) view).getChildCount(); i++) {
                    View innerView = ((ViewGroup) view).getChildAt(i);
                    setupUI(innerView);
                }
            }
        }catch (Exception e){
            Log.e("Catch",e+"");
        }
    }


    public static void hideSoftKeyboard(Activity activity) {

        try {
            InputMethodManager inputMethodManager =
                    (InputMethodManager) activity.getSystemService(
                            Activity.INPUT_METHOD_SERVICE);
            inputMethodManager.hideSoftInputFromWindow(
                    activity.getCurrentFocus().getWindowToken(), 0);

        }catch (Exception e){
            Log.e("Chatch",e+"");
        }
    }

    String copyModelFile(String filename) {
        FolderUtil.createDefaultFolder(Constants.SUDOKU_LOCATION);
        String modelPath = Constants.SUDOKU_LOCATION + File.separator + filename;
        if(FolderUtil.checkIfFileExist(modelPath))
            return modelPath;

        AssetManager assetMgr = this.getAssets();
        InputStream inputStream = null;
        OutputStream outputStream = null;
        try {
            inputStream = assetMgr.open(filename);
            String destFile = modelPath;
            outputStream = new FileOutputStream(destFile);
            byte[] buffer = new byte[1024];
            int read;
            while ((read = inputStream.read(buffer)) != -1) {
                outputStream.write(buffer, 0, read);
            }
            inputStream.close();
            outputStream.flush();
            outputStream.close();
        }catch (IOException e) {
            e.printStackTrace();
        }
        return modelPath;
    }

    void InitYolo() {
        String modelPath = copyModelFile("sudoku.weights");
        String cfgPath = copyModelFile("sudoku.cfg");
        String namePath = copyModelFile("sudoku.names");
        init(modelPath, cfgPath, namePath);


        Log.d("test1", "initYolo - main!");
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    private void Sudoku_Scan() {
        Mat img_output = new Mat();
        img_output = mMat.clone();
        Log.d("test1", "scan - main!");
        new MainActivity.AsyncTess().execute(img_output);
    }

    public native String SudokuResult(long inputImage);

    public native void init(String modelPath, String cfgPath, String namePath);

    private boolean isRTL() {
        Set<String> lang = new HashSet<String>();
        lang.add("ar"); // Arabic
        lang.add("dv"); // Divehi
        lang.add("fa"); // Persian (Farsi)
        lang.add("ha"); // Hausa
        lang.add("he"); // Hebrew
        lang.add("iw_IL"); // Hebrew (old code)
        lang.add("iw"); // Hebrew (old code)
        lang.add("ji"); // Yiddish (old code)
        lang.add("ps"); // Pashto, Pushto
        lang.add("ur"); // Urdu
        lang.add("yi"); // Yiddish
        Set<String> RTL = Collections.unmodifiableSet(lang);

        Locale defLocale = Locale.getDefault();
        if(defLocale == null)
            return false;

        return RTL.contains(defLocale.getLanguage());
    }

    @SuppressLint("StaticFieldLeak")
    private class AsyncTess extends AsyncTask<Mat, Integer, String> {
        ProgressDialog asyncDialog = new ProgressDialog(MainActivity.this);
        @Override

        protected void onPreExecute() {
            asyncDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
            asyncDialog.setMessage("Processing...");
            asyncDialog.show();
            super.onPreExecute();
        }

        @Override
        protected String doInBackground(Mat... mRelativeParams) {
            InitYolo();
            String result = "";
            try {
                 result = SudokuResult(mRelativeParams[0].getNativeObjAddr());
            } catch (Exception e) {

            }
            Log.i("test1", "async - main");

            return result;
        }

        protected void onPostExecute(String result) {
            super.onPostExecute(result);
            asyncDialog.dismiss();
            int [][] array = new int [9][9];
            for(int i=0;i<nRows;i++) {
                for (int j = 0; j < nCols; j++) {
                    array[i][j] = 0;
                }
            }

            String[] box = {};
            if(!result.equalsIgnoreCase(""))
                box = result.split("#");
            int gap = 540 / nRows;
            for(int i=0;i<box.length;i++) {
                String[] item = box[i].split("\\|");
                int val = (Integer.parseInt(item[0]) + 1);

                int cx = Integer.parseInt(item[1]) + (int)(Integer.parseInt(item[3]) / 2.0);
                int cy = Integer.parseInt(item[2]) + (int)(Integer.parseInt(item[4]) / 2.0);

                int row = 0, col = 0;
                for(int r = 0; r < nRows; r++) {
                    if(cy>r*gap && cy<(r+1)*gap) {
                        row = r;
                        break;
                    }
                }

                for(int c = 0; c < nCols; c++) {
                    if(cx>c*gap && cx<(c+1)*gap) {
                        col = c;
                        break;
                    }
                }
                array[row][col] = val;

                if(check==0){
                    unsole[row][col] = val;
                    Log.e("CYKA","перезаписал");
                }

            }

            if(isRTL()) {
                //in Right To Left layout
                for(int r=0; r<nRows;r++) {
                    for(int c=0;c<nCols/2;c++){
                        int temp;
                        temp = array[r][c];
                        array[r][c] = array[r][nCols-c-1];
                        if(check==0) {
                            unsole[r][c] = array[r][nCols - c - 1];
                            Log.e("CYKA","перезаписал");
                        }
                        array[r][nCols-c-1] = temp;
                        if(check==0) {
                            unsole[r][nCols - c - 1] = temp;
                            Log.e("CYKA","перезаписал");
                        }
                    }
                }
            }


            SudokuGrid.cellValues = array;
            btnCamera.setVisibility(View.GONE);
            btnUnsolve.setVisibility(View.GONE);
            btnSolve.setVisibility(View.VISIBLE);
            btnAgain.setVisibility(View.VISIBLE);

            gridLayout.removeAllViews();
            SudokuGrid.initGrid(MainActivity.this, gridLayout, dimensions);

            Log.d("test1", "async_p - main!");
        }
    }

    private class AsyncInit extends AsyncTask<String, Integer, String> {
        ProgressDialog asyncDialog = new ProgressDialog(MainActivity.this);
        @Override

        protected void onPreExecute() {
            btnCamera.setVisibility(View.GONE);
            asyncDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
            asyncDialog.setMessage("Processing...");
            asyncDialog.show();
            super.onPreExecute();
        }

        @Override
        protected String doInBackground(String... mRelativeParams) {
            InitYolo();
            return "";
        }

        protected void onPostExecute(String result) {
            super.onPostExecute(result);
            asyncDialog.dismiss();
            btnCamera.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
        finishAffinity();
        System.runFinalizersOnExit(true);
        System.exit(0);
    }
}
