package com.aidoo.audio;

import android.app.Activity;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.Nullable;

import com.aidoo.AudioFocusMgr;
import com.aidoo.libdroidvirualpad.VPEventCallback;
import com.aidoo.libdroidvirualpad.VPView;
import com.aidoo.test.R;

import java.io.InputStream;

public class TestActivity extends Activity {
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.test_activity);
        findViewById(R.id.test_b1).setOnClickListener(this::onTest1);
        findViewById(R.id.test_b2).setOnClickListener(this::onTest2);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }


    AudioFocusMgr audioFocusMgr = null;
    byte[] audioBytes = null;
    AudioTrack audioTrack = null;
    Thread audioByteWriteThread = null;
    int bufferSize = 0;
    AudioFocusMgr.OnAudioFocusChangeListener audioFocusChangeListener = new AudioFocusMgr.OnAudioFocusChangeListener() {
        @Override
        public void decreaseVolume() {
            tryStop();
        }

        @Override
        public void increaseVolume() {
            tryPlay();
        }

        @Override
        public void pause() {
            tryStop();
        }

        @Override
        public void resume() {
            tryPlay();
        }
    };

    private void tryRequestAudioFocus() {
        if (audioFocusMgr != null) return;
        audioFocusMgr = new AudioFocusMgr.Builder(this)
                .setAudioFocusChangeListener(audioFocusChangeListener)
                .setAcceptsDelayedFocus(false)
                .setPauseWhenDucked(true)
                .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                .setDurationHint(AudioManager.AUDIOFOCUS_GAIN)
                .setUsage(AudioAttributes.USAGE_MEDIA)
                .build();
        audioFocusMgr.requestFocus();
    }

    private void tryStop() {
        if (audioTrack == null) return;
        audioTrack.pause();
    }

    private void tryPlay() {
        if (audioBytes == null) {
            Log.e("TestActivity", "tryPlay: audioBytes is null");
            return;
        }
        if (audioTrack == null) {
            int sampleRate = 48000; // 典型采样率
            int channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
            int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
            bufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat);

            if (bufferSize == AudioTrack.ERROR || bufferSize == AudioTrack.ERROR_BAD_VALUE) {
                Log.e("AudioTrack", "Invalid buffer size");
                return;
            } else {
                Log.d("AudioTrack", "bufferSize: " + bufferSize);
            }

            audioTrack = new AudioTrack(
                    AudioManager.STREAM_MUSIC, sampleRate, channelConfig, audioFormat,
                    bufferSize, AudioTrack.MODE_STREAM
            );


        }
        if (audioTrack != null) {
            if (audioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING) {
                audioTrack.play();
            }
            if (audioByteWriteThread == null || !audioByteWriteThread.isAlive()) {
                audioByteWriteThread = new Thread(new Runnable() {
                    @Override
                    public void run() {
                        // Write audioBytes to audioTrack in every 1s, till end of audioBytes
                        int offset = 0;
                        while (offset < audioBytes.length) {
                            int writeSize = Math.min(bufferSize, audioBytes.length - offset);
                            int writeResult = audioTrack.write(audioBytes, offset, writeSize);
                            if (writeResult < 0) {
                                Log.e("TestActivity", "tryPlay: writeResult < 0");
                                break;
                            }
                            else{
                                Log.w("TestActivity", "tryPlay: writeResult: " + writeResult);
                            }
                        }

                    }
                });
                audioByteWriteThread.start();
            }


        } else {
            Log.e("TestActivity", "tryPlay: audioTrack is null");
        }
    }

    private void onTest2(View v) {
        tryStop();
    }

    private void onTest1(View v) {
        //Load 1.pcm into memory bytes

        try {
            if (audioBytes == null) {
                InputStream is = getAssets().open("1.pcm");
                audioBytes = new byte[is.available()];
                is.read(audioBytes);
                is.close();
                Log.d("TestActivity", "onTest1 1.pcm: " + audioBytes.length);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        tryRequestAudioFocus();
    }

}
