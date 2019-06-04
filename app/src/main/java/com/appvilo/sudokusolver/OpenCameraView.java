package com.appvilo.sudokusolver;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.RectF;
import android.hardware.Camera;
import android.net.Uri;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;

import com.appvilo.sudokusolver.utils.Constants;

import org.opencv.android.JavaCameraView;
import org.opencv.core.Mat;

import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.List;

public class OpenCameraView extends JavaCameraView implements Camera.PictureCallback, Camera.AutoFocusCallback {

    private static final String TAG = OpenCameraView.class.getSimpleName();

    private String mPictureFileName;

    public static int minWidthQuality = 400;

    private Context context;


    public OpenCameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.context = context;
    }

    public void SetFocusMode() {
        Camera.Parameters params = mCamera.getParameters();
        List<String> FocusModes = params.getSupportedFocusModes();
        mCamera.cancelAutoFocus();
        mCamera.autoFocus(this);
        if (FocusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO))
            params.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);

        mCamera.setParameters(params);
    }


    public List<String> getEffectList() {
        return mCamera.getParameters().getSupportedColorEffects();
    }


    public boolean isEffectSupported() {
        return (mCamera.getParameters().getColorEffect() != null);
    }


    public String getEffect() {
        return mCamera.getParameters().getColorEffect();
    }

    public void setEffect(String effect) {
        Camera.Parameters params = mCamera.getParameters();
        params.setColorEffect(effect);
        mCamera.setParameters(params);
    }


    public List<Camera.Size> getResolutionList() {
        return mCamera.getParameters().getSupportedPreviewSizes();
    }


    public void setResolution(Camera.Size resolution) {
        disconnectCamera();
        mMaxHeight = resolution.height;
        mMaxWidth = resolution.width;
        connectCamera(getWidth(), getHeight());
    }


    public Camera.Size getResolution() {
        return mCamera.getParameters().getPreviewSize();
    }


    public void takePicture(final String fileName, Mat matResult) {
        Log.i(TAG, "Taking picture");
        this.mPictureFileName = fileName;
        mCamera.setPreviewCallback(null);

//        mCamera.takePicture(null, null, this);
        long addr = matResult.getNativeObjAddr();
        Intent intent = new Intent(context, MainActivity.class);
        intent.putExtra( "Image", addr );
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity( intent );

    }


    @SuppressLint("WrongThread")
    @Override
    public void onPictureTaken(byte[] data, Camera camera) {
        Log.i(TAG, "Saving a bitmap to file");
        // The camera preview was automatically stopped. Start it again.
        mCamera.startPreview();
        mCamera.setPreviewCallback(this);


        Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);
        Uri uri = Uri.parse(mPictureFileName);

        Log.d(TAG, "selectedImage: " + uri);
        Bitmap bm = null;
        bm = rotate(bitmap, 90);

        // Write the image in a file (in jpeg format)
        try {
            FileOutputStream fos = new FileOutputStream(mPictureFileName);
            bm.compress(Bitmap.CompressFormat.JPEG, 80, fos);
            fos.close();

        } catch (java.io.IOException e) {
            Log.e("PictureDemo", "Exception in photoCallback", e);
        }

        Intent newIntent = new Intent(context, MainActivity.class);
        newIntent.putExtra(Constants.EXTRA_MESSAGE, mPictureFileName);
        newIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(newIntent);
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

    @Override
    protected void AllocateCache() {
        super.AllocateCache();
        setPictureSize();
    }

    protected boolean setPictureSize() {
        try {
            Camera.Parameters params = mCamera.getParameters();
            Log.d(TAG, "getSupportedPictureSizes()");
            List<Camera.Size> sizes = params.getSupportedPictureSizes();
            if (sizes == null) {
                Log.w(TAG, "getSupportedPictureSizes() = null, cannot set a custom size");
                return false;
            }

            int maxSize = 0;
            // choose the largest size that matches the preview AR
            for (android.hardware.Camera.Size size : sizes) {
                if (size.height * mFrameWidth != size.width * mFrameHeight) {
                    continue; // the picture size doesn't match
                }
                if (maxSize > size.width * size.height) {
                    continue; // don't need this size
                }
                params.setPictureSize(size.width, size.height);
                maxSize = size.width * size.height;
            }
            if (maxSize == 0) {
                Log.w(TAG, "getSupportedPictureSizes() has no matches for " + mFrameWidth + 'x' + mFrameHeight);
                return false;
            }
            Log.d(TAG, "try Picture size " + params.getPictureSize().width + 'x' + params.getPictureSize().height);
            mCamera.setParameters(params);
        } catch (Exception e) {
            Log.e(TAG, "setPictureSize for " + mFrameWidth + 'x' + mFrameHeight, e);
            return false;
        }
        return true;
    }

    @Override
    public void onAutoFocus(boolean success, Camera camera) {

    }

    public void focusOnTouch(MotionEvent event) {
        Rect focusRect = calculateTapArea(event.getRawX(), event.getRawY(), 1f);
        Rect meteringRect = calculateTapArea(event.getRawX(), event.getRawY(), 1.5f);

        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);

        if (parameters.getMaxNumFocusAreas() > 0) {
            List<Camera.Area> focusAreas = new ArrayList<Camera.Area>();
            focusAreas.add(new Camera.Area(focusRect, 1000));

            parameters.setFocusAreas(focusAreas);
        }

        if (parameters.getMaxNumMeteringAreas() > 0) {
            List<Camera.Area> meteringAreas = new ArrayList<Camera.Area>();
            meteringAreas.add(new Camera.Area(meteringRect, 1000));

            parameters.setMeteringAreas(meteringAreas);
        }
        mCamera.setParameters(parameters);
        mCamera.autoFocus(this);
    }

    private Rect calculateTapArea(float x, float y, float coefficient) {
        float focusAreaSize = 300;
        int areaSize = Float.valueOf(focusAreaSize * coefficient).intValue();

        int centerX = (int) (x / getResolution().width - 1000);
        int centerY = (int) (y / getResolution().height - 1000);

        int left = clamp(centerX - areaSize / 2, -1000, 1000);
        int top = clamp(centerY - areaSize / 2, -1000, 1000);

        RectF rectF = new RectF(left, top, left + areaSize, top + areaSize);

        return new Rect(Math.round(rectF.left), Math.round(rectF.top), Math.round(rectF.right), Math.round(rectF.bottom));
    }

    private int clamp(int x, int min, int max) {
        if (x > max) {
            return max;
        }
        if (x <min) {
            return min;
        }
        return x;
    }

    public void turnOffTheFlash() {
        Camera.Parameters params = mCamera.getParameters();
        params.setFlashMode(params.FLASH_MODE_OFF);
        mCamera.setParameters(params);
    }

    public void turnOnTheFlash() {
        Camera.Parameters params = mCamera.getParameters();
        params.setFlashMode(params.FLASH_MODE_TORCH);
        mCamera.setParameters(params);
    }
}
