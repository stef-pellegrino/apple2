/*
 * Apple // emulator for *nix
 *
 * This software package is subject to the GNU General Public License
 * version 2 or later (your choice) as published by the Free Software
 * Foundation.
 *
 * THERE ARE NO WARRANTIES WHATSOEVER.
 *
 */

package org.deadc0de.apple2ix;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;

import org.deadc0de.apple2ix.basic.BuildConfig;
import org.deadc0de.apple2ix.basic.R;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.util.concurrent.atomic.AtomicBoolean;

public class Apple2CrashHandler {

    public final static String javaCrashFileName = "jcrash.txt";

    public static Apple2CrashHandler getInstance() {
        return sCrashHandler;
    }

    public enum CrashType {
        JAVA_CRASH {
            @Override
            public final String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.crash_java_npe);
            }
        },
        NULL_DEREF {
            @Override
            public final String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.crash_null);
            }
        },
        STACKCALL_OVERFLOW {
            @Override
            public final String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.crash_stackcall_overflow);
            }
        },
        STACKBUF_OVERFLOW {
            @Override
            public final String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.crash_stackbuf_overflow);
            }
        };


        public static final int size = CrashType.values().length;

        public abstract String getTitle(Apple2Activity activity);

        public static String[] titles(Apple2Activity activity) {
            String[] titles = new String[size];
            int i = 0;
            for (CrashType setting : values()) {
                titles[i++] = setting.getTitle(activity);
            }
            return titles;
        }
    }

    public synchronized void setCustomExceptionHandler(Apple2Activity activity) {
        if (mDefaultExceptionHandler != null) {
            return;
        }
        mDefaultExceptionHandler = Thread.getDefaultUncaughtExceptionHandler();

        final String homeDir = "/data/data/" + activity.getPackageName();
        final Thread.UncaughtExceptionHandler defaultExceptionHandler = mDefaultExceptionHandler;

        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread thread, Throwable t) {
                try {
                    StackTraceElement[] stackTraceElements = t.getStackTrace();
                    StringBuffer traceBuffer = new StringBuffer();

                    // append the Java stack trace
                    traceBuffer.append(t.getClass().getName());
                    traceBuffer.append("\n");
                    final int maxTraceSize = 2048 + 1024 + 512; // probably should keep this less than a standard Linux PAGE_SIZE
                    for (StackTraceElement elt : stackTraceElements) {
                        traceBuffer.append(elt.toString());
                        traceBuffer.append("\n");
                        if (traceBuffer.length() >= maxTraceSize) {
                            break;
                        }
                    }
                    traceBuffer.append("\n");

                    nativeOnUncaughtException(homeDir, traceBuffer.toString());
                } catch (Throwable terminator2) {
                    // Yo dawg, I hear you like exceptions in your exception handler! ...
                }

                defaultExceptionHandler.uncaughtException(thread, t);
            }
        });
    }

    public boolean areJavaCrashesPresent(Apple2Activity activity) {
        File javaCrash = _javaCrashFile(activity);
        return javaCrash.exists();
    }

    public boolean areNativeCrashesPresent(Apple2Activity activity) {
        File[] nativeCrashes = _nativeCrashFiles(activity);
        return nativeCrashes != null && nativeCrashes.length > 0;
    }

    public boolean areCrashesPresent(Apple2Activity activity) {
        return areJavaCrashesPresent(activity) || areNativeCrashesPresent(activity);
    }

    public void performCrash(int crashType) {
        if (BuildConfig.DEBUG) {
            nativePerformCrash(crashType);
        }
    }

    // ------------------------------------------------------------------------
    // privates

    private Apple2CrashHandler() {
        /* ... */
    }

    private File _javaCrashFile(Apple2Activity activity) {
        return new File(Apple2DisksMenu.getDataDir(activity), javaCrashFileName);
    }

    private File[] _nativeCrashFiles(Apple2Activity activity) {
        FilenameFilter dmpFilter = new FilenameFilter() {
            public boolean accept(File dir, String name) {
                File file = new File(dir, name);
                if (file.isDirectory()) {
                    return false;
                }

                // check file extensions ... sigh ... no String.endsWithIgnoreCase() ?

                final String extension = ".dmp";
                final int nameLen = name.length();
                final int extLen = extension.length();
                if (nameLen <= extLen) {
                    return false;
                }

                String suffix = name.substring(nameLen - extLen, nameLen);
                return (suffix.equalsIgnoreCase(extension));
            }
        };

        return new File(Apple2DisksMenu.getDataDir(activity)).listFiles(dmpFilter);
    }

    private String _dumpPath2ProcessedPath(String crashPath) {
        return crashPath.substring(0, crashPath.length() - 4) + ".txt";
    }

    private boolean _readFile(File file, StringBuilder fileData) {
        final int maxAttempts = 5;
        int attempts = 0;
        do {
            try {
                BufferedReader reader = new BufferedReader(new FileReader(file));
                char[] buf = new char[1024];
                int numRead = 0;
                while ((numRead = reader.read(buf)) != -1) {
                    String readData = String.valueOf(buf, 0, numRead);
                    fileData.append(readData);
                }
                reader.close();
                break;
            } catch (InterruptedIOException ie) {
                /* EINTR, EAGAIN ... */
            } catch (IOException e) {
                Log.d(TAG, "Error reading file at path : " + file.toString());
            }

            try {
                Thread.sleep(100, 0);
            } catch (InterruptedException e) {
                /* ... */
            }
            ++ attempts;
        } while (attempts < maxAttempts);

        return attempts < maxAttempts;
    }

    private File _writeTempLogFile(StringBuilder allCrashData) {

        File allCrashFile = null;

        String storageState = Environment.getExternalStorageState();
        if (storageState.equals(Environment.MEDIA_MOUNTED)) {
            allCrashFile = new File(Environment.getExternalStorageDirectory(), "apple2ix_crash.txt");
        } else {
            allCrashFile = new File("/data/local/tmp", "apple2ix_crash.txt");
        }

        Log.d(TAG, "Writing all crashes to temp file : " + allCrashFile);
        final int maxAttempts = 5;
        int attempts = 0;
        do {
            try {
                BufferedWriter writer = new BufferedWriter(new FileWriter(allCrashFile));
                writer.append(allCrashData);
                writer.flush();
                writer.close();
                break;
            } catch (InterruptedIOException ie) {
                /* EINTR, EAGAIN ... */
            } catch (IOException e) {
                Log.e(TAG, "Exception attempting to write data : " + e);
            }

            try {
                Thread.sleep(100, 0);
            } catch (InterruptedException e) {
                /* ... */
            }
            ++attempts;
        } while (attempts < maxAttempts);

        return allCrashFile;
    }

    private void _sendEmailToDeveloperWithCrashData(Apple2Activity activity, StringBuilder allCrashData) {
        mAlreadySentReport.set(true);

        Intent emailIntent = new Intent(Intent.ACTION_SENDTO, Uri.fromParts("mailto", "apple2ix_crash@deadcode.org"/*non-zero variant is correct endpoint at the moment*/, null));
        emailIntent.putExtra(Intent.EXTRA_SUBJECT, "Crasher");

        File allCrashFile = _writeTempLogFile(allCrashData);
        // Putting all the text data into the EXTRA_TEXT appears to trigger android.os.TransactionTooLargeException ...
        //emailIntent.putExtra(Intent.EXTRA_TEXT, allCrashData.toString());
        emailIntent.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(allCrashFile));

        // But we can put some text data
        emailIntent.putExtra(Intent.EXTRA_TEXT, "Greeting Apple2ix developers!  The app crashed, please help!");

        Log.d(TAG, "STARTING CHOOSER FOR EMAIL ...");
        activity.startActivity(Intent.createChooser(emailIntent, "Send email"));
        Log.d(TAG, "AFTER START ACTIVITY ...");
    }


    private final static String TAG = "Apple2CrashHandler";
    private final static Apple2CrashHandler sCrashHandler = new Apple2CrashHandler();

    private Thread.UncaughtExceptionHandler mDefaultExceptionHandler;
    private AtomicBoolean mAlreadyRanCrashCheck = new AtomicBoolean(false);
    private AtomicBoolean mAlreadySentReport = new AtomicBoolean(false);

    private static native void nativePerformCrash(int crashType); // testing

    private static native void nativeOnUncaughtException(String home, String trace);

}
