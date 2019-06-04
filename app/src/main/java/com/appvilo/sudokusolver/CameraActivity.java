package com.appvilo.sudokusolver;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.os.AsyncTask;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;

import com.appvilo.sudokusolver.R;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvException;
import org.opencv.core.Mat;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;

import java.util.Arrays;
import java.util.Timer;

public class CameraActivity extends AppCompatActivity
        implements CameraBridgeViewBase.CvCameraViewListener2{

    private static final String TAG = "test1";//CameraActivity.class.getSimpleName();
    private OpenCameraView mOpenCvCameraView;
    private Mat matInput = new Mat(), matResize = new Mat(), matResult = new Mat(), matTarget = new Mat();

    private boolean bIsFlashOn;
    ImageView flashBtn;
    ImageView takePictureBtn, preview_img;
    int m_nCnt = 5;
    Boolean[] mQue = new Boolean[m_nCnt];
    boolean m_bOk = false;
    Timer timer;

    public native int FindCorner(long matAddrInput, long matAddrResult, long matAddrTarget);
    public native void DrawPreRect(long matAddrInput);

    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                    mOpenCvCameraView.enableView();
                    break;
                default:
                    super.onManagerConnected(status);
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        InitialQue();

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_camera);

        bIsFlashOn = false;

        mOpenCvCameraView = (OpenCameraView) findViewById(R.id.camera_view);
        mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
        mOpenCvCameraView.setCvCameraViewListener(this);
        mOpenCvCameraView.setCameraIndex(0); // front-camera(1),  back-camera(0)
        mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
//        mOpenCvCameraView.SetFocusMode();
//        mOpenCvCameraView.setOnTouchListener(this);
//        mOpenCvCameraView.setFocusable(true);

        preview_img = (ImageView)findViewById(R.id.preview_img);
        takePictureBtn = (ImageView)findViewById(R.id.take_picture);
        takePictureBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(matTarget.empty()) return;
                long addr = matTarget.getNativeObjAddr();
                Intent intent = new Intent(CameraActivity.this, MainActivity.class);
                intent.putExtra( "Image", addr );
                startActivity( intent );
            }
        });
//
//        flashBtn = (ImageView)findViewById(R.id.flash);
//        flashBtn.setOnClickListener(new View.OnClickListener() {
//            @Override
//            public void onClick(View v) {
//                bIsFlashOn = !bIsFlashOn;
//                if(bIsFlashOn) {
//                    flashBtn.setImageResource(R.drawable.flash_on);
//                    mOpenCvCameraView.turnOnTheFlash();
//                }
//                else {
//                    flashBtn.setImageResource(R.drawable.flash_off);
//                    mOpenCvCameraView.turnOffTheFlash();
//                }
//            }
//        });

//        timer = new Timer();
//        timer.schedule(new TimerTask()
//        {
//            @Override
//            public void run()
//            {
//                takePictureBtn.performClick();
//            }
//        }, 1000, 1000);
        Log.d(TAG, "onCreate");
    }


    @Override
    public void onPause()
    {
        super.onPause();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
        Log.d(TAG, "onPause!");
    }

    @Override
    public void onResume()
    {
        super.onResume();

        if (!OpenCVLoader.initDebug()) {
            Log.d(TAG, "onResume :: Internal OpenCV library not found.");
            OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION, this, mLoaderCallback);
        } else {
            Log.d(TAG, "onResum :: OpenCV library found inside package. Using it!");
            mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
        }

        m_bOk = false;
    }

    public void onDestroy() {
        super.onDestroy();

        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
        Log.d(TAG, "onDestory!");
    }

    @Override
    public void onCameraViewStarted(int width, int height) {

    }

    @Override
    public void onCameraViewStopped() {

    }

    void push_stack(Boolean cur) {
        for(int i=1;i<m_nCnt;i++) {
            mQue[i-1] = mQue[i];
        }
        mQue[m_nCnt-1] = cur;
    }

    boolean IsUsefulFrame() {
        boolean ret = true;
        for(int i=0;i<m_nCnt;i++) {
            if(!mQue[i]) {
                ret = false;
                break;
            }
        }
        return ret;
    }

    void InitialQue() {
        Arrays.fill(mQue, Boolean.FALSE);
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {

        {
            matInput = inputFrame.rgba();
            Size inputSize = matInput.size();

            Imgproc.resize(matInput, matResize, new Size(800, 600));
            int cnt = FindCorner(matResize.getNativeObjAddr(), matResult.getNativeObjAddr(), matTarget.getNativeObjAddr());
            if (cnt == 0)
                push_stack(false);
            else {
                push_stack(true);
                new AsyncImage().execute(matTarget);
            }

            Imgproc.resize(matResult, matInput, inputSize);
            DrawPreRect(matInput.getNativeObjAddr());
        }

        return matInput;
    }

    private class AsyncImage extends AsyncTask<Mat, Integer, Mat> {

        @Override
        protected Mat doInBackground(Mat... mats) {

//            Bitmap bitmap = convertMatToBitMap(mats[0]);
//
//            String mPictureFileName = Constants.SCAN_IMAGE_LOCATION + File.separator + Utilities.generateFilename();
//            FolderUtil.createDefaultFolder(Constants.SCAN_IMAGE_LOCATION);
//            Uri uri = Uri.parse(mPictureFileName);
//
//            Log.d(TAG, "selectedImage: " + uri);
//            Bitmap bm = null;
//            bm = rotate(bitmap, 90);
//            // Write the image in a file (in jpeg format)
//            try {
//                FileOutputStream fos = new FileOutputStream(mPictureFileName);
//                bm.compress(Bitmap.CompressFormat.JPEG, 100, fos);
//                fos.close();
//
//            } catch (java.io.IOException e) {
//                Log.e("PictureDemo", "Exception in photoCallback", e);
//            }

            return mats[0];
        }

        protected void onPostExecute(Mat result) {
            super.onPostExecute(result);
            Bitmap bitmap = convertMatToBitMap(result);
            preview_img.setImageBitmap(bitmap);


//                Handler handler = new Handler();
//                handler.postDelayed(new Runnable() {
//                    @Override
//                    public void run() {
//                        takePictureBtn.performClick();
//                    }
//                }, 1000);

        }
    }

    @SuppressLint("StaticFieldLeak")
    private class AsyncCamera extends AsyncTask<Mat, Integer, Mat> {
        @Override
        protected Mat doInBackground(Mat... mRelativeParams) {
            return mRelativeParams[0];
        }

        protected void onPostExecute(Mat result) {
            super.onPostExecute(result);
            Log.d(TAG, "AsyncCamera");

            mOpenCvCameraView.setCvCameraViewListener((CameraBridgeViewBase.CvCameraViewListener2) null);

            long addr = result.getNativeObjAddr();
            Intent intent = new Intent(CameraActivity.this, MainActivity.class);
            intent.putExtra( "Image", addr );
            startActivity( intent );
            finish();
        }
    }

    private static Bitmap convertMatToBitMap(Mat input){
        Bitmap bmp = null;
        Mat rgb = new Mat();
        Imgproc.cvtColor(input, rgb, Imgproc.COLOR_RGBA2RGB);

        try {
            bmp = Bitmap.createBitmap(rgb.cols(), rgb.rows(), Bitmap.Config.ARGB_8888);
            Utils.matToBitmap(rgb, bmp);
        }
        catch (CvException e){
            Log.d("Exception",e.getMessage());
        }
        return bmp;
    }

    private static Bitmap rotate(Bitmap bm, int rotation) {
        if (rotation != 0) {
            Matrix matrix = new Matrix();
            matrix.postRotate(rotation);
            Bitmap bmOut = Bitmap.createBitmap(bm, 0, 0, bm.getWidth(), bm.getHeight(), matrix, true);
            return bmOut;
        }
        return bm;
    }
}