package com.aidoo;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.os.Build;
import android.util.Log;

import androidx.annotation.NonNull;

public final class AudioFocusMgr {

    private final Context context;
    private final boolean pauseWhenDucked;
    private final int streamType;
    private final int durationHint;
    private final OnAudioFocusChangeListener myListener;
    private final AudioManager.OnAudioFocusChangeListener audioFocusChangeListener;
    private final AudioManager audioManager;
    private final AudioFocusRequest audioFocusRequest;

    private boolean volumeDucked;

    public interface OnAudioFocusChangeListener {

        void decreaseVolume();

        void increaseVolume();

        /**
         * Pause the playback.
         */
        void pause();

        /**
         * Resume/start the playback.
         */
        void resume();

    }

    /**
     * Builder class for {@link AudioFocusMgr} class objects.
     */
    public static final class Builder {

        private final Context context;
        private int usage;
        private int contentType;
        private boolean acceptsDelayedFocus;
        private boolean pauseWhenDucked;
        private OnAudioFocusChangeListener myListener;
        private int stream;
        private int durationHint;

        /**
         * @param context The {@link Context} that is asking for audio focus.
         */
        public Builder(@NonNull Context context) {

            this.context = context;

            acceptsDelayedFocus = true;
            pauseWhenDucked = false;

            myListener = null;

            usage = AudioAttributes.USAGE_UNKNOWN;
            durationHint = AudioManager.AUDIOFOCUS_GAIN;
            contentType = AudioAttributes.CONTENT_TYPE_UNKNOWN;
            stream = AudioManager.USE_DEFAULT_STREAM_TYPE;
        }

        /**
         * Sets the attribute describing what is the intended use of the audio signal.
         *
         * @param usage one of {@link AudioAttributes#USAGE_UNKNOWN},
         *              {@link AudioAttributes#USAGE_MEDIA},
         *              {@link AudioAttributes#USAGE_VOICE_COMMUNICATION},
         *              {@link AudioAttributes#USAGE_VOICE_COMMUNICATION_SIGNALLING},
         *              {@link AudioAttributes#USAGE_ALARM},
         *              {@link AudioAttributes#USAGE_NOTIFICATION},
         *              {@link AudioAttributes#USAGE_NOTIFICATION_RINGTONE},
         *              {@link AudioAttributes#USAGE_NOTIFICATION_COMMUNICATION_REQUEST},
         *              {@link AudioAttributes#USAGE_NOTIFICATION_COMMUNICATION_INSTANT},
         *              {@link AudioAttributes#USAGE_NOTIFICATION_COMMUNICATION_DELAYED},
         *              {@link AudioAttributes#USAGE_NOTIFICATION_EVENT},
         *              {@link AudioAttributes#USAGE_ASSISTANT},
         *              {@link AudioAttributes#USAGE_ASSISTANCE_ACCESSIBILITY},
         *              {@link AudioAttributes#USAGE_ASSISTANCE_NAVIGATION_GUIDANCE},
         *              {@link AudioAttributes#USAGE_ASSISTANCE_SONIFICATION},
         *              {@link AudioAttributes#USAGE_GAME}.
         * @return The same Builder instance.
         */
        public Builder setUsage(int usage) {
            this.usage = usage;
            return this;
        }

        /**
         * Sets the attribute describing the content type of the audio signal, such as
         * speech, or music.
         *
         * @param contentType the content type values, one of
         *                    {@link AudioAttributes#CONTENT_TYPE_MOVIE},
         *                    {@link AudioAttributes#CONTENT_TYPE_MUSIC},
         *                    {@link AudioAttributes#CONTENT_TYPE_SONIFICATION},
         *                    {@link AudioAttributes#CONTENT_TYPE_SPEECH},
         *                    {@link AudioAttributes#CONTENT_TYPE_UNKNOWN}.
         * @return the same Builder instance.
         */
        public Builder setContentType(int contentType) {
            this.contentType = contentType;
            return this;
        }

        /**
         * Sets whether the app will accept delayed focus gain. Default is {@code true}.
         *
         * @param acceptsDelayedFocus Whether the app accepts delayed focus gain.
         * @return The same Builder instance.
         */
        public Builder setAcceptsDelayedFocus(boolean acceptsDelayedFocus) {
            this.acceptsDelayedFocus = acceptsDelayedFocus;
            return this;
        }

        /**
         * Sets whether the audio will be paused instead of ducking when
         * {@link AudioManager#AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK} is received.
         * Default is
         * {@code false}.
         *
         * @param pauseWhenDucked Whether the audio will be paused instead of ducking.
         * @return The same Builder instance.
         */
        public Builder setPauseWhenDucked(boolean pauseWhenDucked) {
            this.pauseWhenDucked = pauseWhenDucked;
            return this;
        }

        /**
         * Sets the {@link OnAudioFocusChangeListener} that will receive callbacks.
         *
         * @param listener The {@link OnAudioFocusChangeListener} implementation that
         *                 will receive callbacks.
         * @return The same Builder instance.
         */
        public Builder setAudioFocusChangeListener(
                @NonNull OnAudioFocusChangeListener listener) {
            this.myListener = listener;
            return this;
        }

        /**
         * Sets the audio stream for devices lower than Android Oreo.
         *
         * @param stream The stream that will be used for playing the audio. Should be
         *               one of {@link AudioManager#STREAM_ACCESSIBILITY},
         *               {@link AudioManager#STREAM_ALARM}, {@link AudioManager#STREAM_DTMF},
         *               {@link AudioManager#STREAM_MUSIC},
         *               {@link AudioManager#STREAM_NOTIFICATION},
         *               {@link AudioManager#STREAM_RING}, {@link AudioManager#STREAM_SYSTEM} or
         *               {@link AudioManager#STREAM_VOICE_CALL}.
         * @return The same Builder instance.
         */
        public Builder setStream(int stream) {
            this.stream = stream;
            return this;
        }

        /**
         * Sets the duration for which the audio will be played.
         *
         * @param durationHint The duration hint, one of
         *                     {@link AudioManager#AUDIOFOCUS_GAIN},
         *                     {@link AudioManager#AUDIOFOCUS_GAIN_TRANSIENT},
         *                     {@link AudioManager#AUDIOFOCUS_GAIN_TRANSIENT_EXCLUSIVE} or
         *                     {@link AudioManager#AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK}.
         * @return The same Builder instance.
         */
        public Builder setDurationHint(int durationHint) {
            this.durationHint = durationHint;
            return this;
        }


        /**
         * Builds a new {@link AudioFocusMgr} instance combining all the
         * information gathered by this {@code Builder}'s configuration methods.
         * <p>
         * Throws {@link IllegalStateException} when the listener has not been set.
         * </p>
         *
         * @return the {@link AudioFocusMgr} instance qualified by all the
         * properties set on this {@code Builder}.
         */
        @NonNull
        public AudioFocusMgr build() {
            if (myListener == null) {
                throw new IllegalStateException("Listener cannot be null.");
            }
            return new AudioFocusMgr(this);
        }
    }

    private AudioFocusMgr(@NonNull Builder builder) {

        context = builder.context;
        boolean acceptsDelayedFocus = builder.acceptsDelayedFocus;
        pauseWhenDucked = builder.pauseWhenDucked;
        myListener = builder.myListener;
        int usage = builder.usage;
        int contentType = builder.contentType;
        streamType = builder.stream;
        durationHint = builder.durationHint;

        volumeDucked = false;

        audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);

        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(usage)
                .setContentType(contentType)
                .build();

        audioFocusChangeListener = (focusChange) -> {
            Log.w("RetroRunner", "focusChange: " + focusChange);
            switch (focusChange) {

                case AudioManager.AUDIOFOCUS_LOSS:
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT: {
                    myListener.pause();
                    break;
                }

                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK: {
                    if (pauseWhenDucked) {
                        myListener.pause();
                    } else {
                        myListener.decreaseVolume();
                        volumeDucked = true;
                    }
                    break;
                }

                case AudioManager.AUDIOFOCUS_GAIN: {
                    if (volumeDucked) {
                        volumeDucked = false;
                        myListener.increaseVolume();
                    } else {
                        myListener.resume();
                    }
                    break;
                }

                default:
                    break;
            }

        };

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            audioFocusRequest = new AudioFocusRequest.Builder(durationHint)
                    .setAudioAttributes(attributes)
                    .setWillPauseWhenDucked(pauseWhenDucked)
                    .setAcceptsDelayedFocusGain(acceptsDelayedFocus)
                    .setOnAudioFocusChangeListener(audioFocusChangeListener)
                    .build();
        } else {
            audioFocusRequest = null;
        }
    }

    /**
     * Requests audio focus from the system.
     * <p>
     * This function should be called every time you want to start/resume playback. If
     * the
     * system grants focus, you will get a call in
     * {@link OnAudioFocusChangeListener#resume()}.
     * </p>
     * <p>
     * If the system issues delayed focus, or rejects the request, then no callback will
     * be issued. However, once the system grants full focus after delayed focus has been
     * issued, the {@link OnAudioFocusChangeListener#resume()} method will be called.
     * </p>
     */
    public void requestFocus() {
        Log.w("RetroRunner", "before requestFocus, music is on: " + audioManager.isMusicActive());
        int status = 0;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            status = audioManager.requestAudioFocus(audioFocusRequest);
        } else {
            status = audioManager.requestAudioFocus(audioFocusChangeListener, streamType
                    , durationHint);
        }
        Log.w("RetroRunner", "requestFocus result: " + status );
        if (status == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            myListener.resume();
            if (volumeDucked) {
                myListener.increaseVolume();
                volumeDucked = false;
            }
        }

        registerNoisyReceiver();
    }

    /**
     * Abandons audio focus.
     * <p>
     * Call this method every time you stop/pause playback. This will free audio focus
     * for
     * other apps.
     * </p>
     */
    public void abandonFocus() {
        Log.w("RetroRunner", "abandonFocus");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            audioManager.abandonAudioFocusRequest(audioFocusRequest);
        } else {
            audioManager.abandonAudioFocus(audioFocusChangeListener);
        }

        unregisterNoisyReceiver();
    }

    private BroadcastReceiver noisyReceiver = null;

    private void registerNoisyReceiver() {
        if (noisyReceiver == null) {
            noisyReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, android.content.Intent intent) {
                    if (android.media.AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(intent.getAction())) {
                        requestFocus();
                    }
                }
            };
            context.registerReceiver(noisyReceiver, new IntentFilter(android.media.AudioManager.ACTION_AUDIO_BECOMING_NOISY));
        }
    }

    private void unregisterNoisyReceiver() {
        if (noisyReceiver != null) {
            context.unregisterReceiver(noisyReceiver);
            noisyReceiver = null;
        }
    }

}