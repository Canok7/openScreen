package com.example.live555.Source;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.MediaCodec;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Range;
import android.util.Size;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

public class CameraSource implements SourceInterface{
    private final String TAG="CameraSource";
    private Context mContext;
    private CameraDevice mCameraDevice;
    private CaptureRequest.Builder mPreviewBuilder;
    private CameraCaptureSession mPreviewSession;
    private HandlerThread bgThread;
    private Handler bgHandler;
    private Surface mtoEncoderSurface;
    private Surface mLocalPreviewSurface;
    private CameraManager cameraManager;
    private Semaphore mCameraOpenCloseLock = new Semaphore(1);
    private int mCamraId;
    private SourceConfig mConfig;

    @Override
    public void openSource(Surface surface,SourceConfig config) {
        mConfig = config;
        try {
            prepare(surface,config.w,config.h);
        }catch (Exception e){
            e.printStackTrace();
        }
    }

    @Override
    public void closeSource() {
        Log.d(TAG, "closeSource: ");
        //stop preview,release camera
        closeCamera();
        bgThread.quitSafely();
        try {
            bgThread.join();
            bgThread = null;
            bgHandler = null;
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void shiftSource(int id) {
        //if you want to change size, please to call
        //MultiScreenShareInterface::setSurfaceProp
        if(id !=  mCamraId){
            mCamraId = id;
            closeCamera();
            openSource(mtoEncoderSurface,mConfig);
        }
    }

    @Override
    public SourceInfo[] getSourceInfo(){
        SourceInfo[] infos =null;
        try {
            String []cameList;
            cameList = cameraManager.getCameraIdList();
            infos = new SourceInfo[cameList.length];

            for(int i=0;i<infos.length;i++) {
                CameraCharacteristics characteristics = cameraManager.getCameraCharacteristics(cameList[i]);
                infos[i].fpsRange = characteristics.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);

                StreamConfigurationMap map = characteristics
                        .get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                infos[i].sizes =  map.getOutputSizes(MediaCodec.class);
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        return infos;
    }

    /*------------------------------------*/
    public CameraSource(Context context,int camraId) {
        this(context,camraId,null);
    }
    public CameraSource(Context context,int camraId,Surface localSurface) {
        this.mContext = context;
        this.mCamraId=camraId;
        this.mLocalPreviewSurface = localSurface;
        cameraManager = (CameraManager) mContext.getSystemService(Context.CAMERA_SERVICE);
    }


    private void startPreview() {
        try {
            mPreviewBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
            List<Surface> surfaces = new ArrayList<>();
            surfaces.add(mtoEncoderSurface);
            mPreviewBuilder.addTarget(mtoEncoderSurface);
            if(mLocalPreviewSurface != null) {
                surfaces.add(mLocalPreviewSurface);
                mPreviewBuilder.addTarget(mLocalPreviewSurface);
            }

            mCameraDevice.createCaptureSession(surfaces, new CameraCaptureSession.StateCallback() {
                @Override
                public void onConfigured(@NonNull CameraCaptureSession session) {
                    mPreviewSession = session;
                    mPreviewBuilder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
                    //mPreviewBuilder.set(CaptureRequest.CONTROL_AE_MODE,CameraMetadata.CONTROL_AE_MODE_ON);
                    //   CONTROL_AF_MODE_AUTO
                    //mPreviewBuilder.set(CaptureRequest.CONTROL_AF_MODE,CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_VIDEO);
                    //mPreviewBuilder.set(CaptureRequest.CONTROL_AWB_MODE,CameraMetadata.CONTROL_AWB_MODE_AUTO);
                    //mPreviewBuilder.set(CaptureRequest.JPEG_ORIENTATION, 0);//just for take photo

                    String cameraId ;
                    try {
                        Log.d(TAG, "onConfigured: ");
                        String []cameList = cameraManager.getCameraIdList();
                        if(mCamraId < cameList.length){
                            cameraId = cameList[mCamraId];
                        }else{
                            Log.e(TAG, "onConfigured: not sopport camerid:"+mCamraId+"will change to 0");
                            cameraId = cameList[0];
                        }
                        CameraCharacteristics characteristics = cameraManager.getCameraCharacteristics(cameraId);
                        Range<Integer>[] fpsRange = characteristics.get(
                                CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
                        Log.d(TAG, "fpsRange: "+ Arrays.toString(fpsRange));
                        if (fpsRange != null && fpsRange.length > 0) {
                            Range<Integer> maxFps = fpsRange[0];
                            Range<Integer> minFps = fpsRange[0];
                            Range<Integer> chooseFps = null;
                            for (Range<Integer> aFpsRange : fpsRange) {
                                if (maxFps.getLower() * maxFps.getUpper() < aFpsRange.getLower() * aFpsRange.getUpper()) {
                                    maxFps = aFpsRange;
                                }

                                if(maxFps.getLower() * maxFps.getUpper() > aFpsRange.getLower() * aFpsRange.getUpper()){
                                    minFps = aFpsRange;
                                }
                                //first ,we need minfps >= 15
                                if(aFpsRange.getLower() >=mConfig.fps){
                                    if(chooseFps==null){
                                        chooseFps = aFpsRange;
                                    }
                                    //then , we choose max range
                                    if(chooseFps.getUpper()-chooseFps.getLower() < aFpsRange.getUpper()-aFpsRange.getLower()){
                                        chooseFps = aFpsRange;
                                    }
                                }
                            }
                            Log.e(TAG, "onConfigured  maxFps=" + maxFps.toString()+", minfps="+minFps.toString()+", choose:fps"+chooseFps);
                            mPreviewBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, chooseFps);
                        }
                        mPreviewSession.setRepeatingRequest(mPreviewBuilder.build(), null, bgHandler);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }

                @Override
                public void onConfigureFailed(@NonNull CameraCaptureSession session) {
                    Log.e(TAG,"createCaptureSession onConfigureFailed");
                }
            }, null);

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }


    private void prepare(Surface surface, int height, int width) throws IllegalStateException {
        mtoEncoderSurface = surface;
        bgThread = new HandlerThread("background thread");
        bgThread.start();
        bgHandler = new Handler(bgThread.getLooper());
        //open camera,set parameters
        openCamera();
    }

    private void openCamera() {
        Log.d(TAG, "openCamera: ------");
        try {
            if (!mCameraOpenCloseLock.tryAcquire(2500, TimeUnit.MILLISECONDS)) {
                throw new RuntimeException("Time out waiting to lock camera opening.");
            }

            String cameraId ;
            String []cameList = cameraManager.getCameraIdList();
            if(mCamraId < cameList.length){
                cameraId = cameList[mCamraId];
            }else{
                Log.e("get CamersaSize", "onConfigured: not sopport camerid:"+mCamraId+"will change to camera 0");
                cameraId = cameList[0];
            }
            if (ActivityCompat.checkSelfPermission(mContext, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                Log.d(TAG, "openCamera: no permission!!!!!!!!!!!!");
                return;
            }
            cameraManager.openCamera(cameraId, mStateCallback, bgHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera opening.");
        }
    }


    private void closeCamera() {
        try {
            mCameraOpenCloseLock.acquire();
            if (mPreviewSession != null) {
                mPreviewSession.close();
                mPreviewSession = null;
            }
            if (null != mCameraDevice) {
                mCameraDevice.close();
                mCameraDevice = null;
            }
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted while trying to lock camera closing.");
        } finally {
            mCameraOpenCloseLock.release();
        }
    }



    /**
     * {@link CameraDevice.StateCallback} is called when {@link CameraDevice} changes its status.
     */
    private CameraDevice.StateCallback mStateCallback = new CameraDevice.StateCallback() {

        @Override
        public void onOpened(@NonNull CameraDevice cameraDevice) {
            mCameraDevice = cameraDevice;
            startPreview();
            mCameraOpenCloseLock.release();
        }

        @Override
        public void onDisconnected(@NonNull CameraDevice cameraDevice) {
            mCameraOpenCloseLock.release();
            cameraDevice.close();
            mCameraDevice = null;
        }

        @Override
        public void onError(@NonNull CameraDevice cameraDevice, int error) {
            mCameraOpenCloseLock.release();
            cameraDevice.close();
            mCameraDevice = null;
        }

    };
}
