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
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;

public class Apple2SettingsMenu extends Apple2AbstractMenu {

    private final static String TAG = "Apple2SettingsMenu";

    public Apple2SettingsMenu(Apple2Activity activity) {
        super(activity);
    }

    @Override
    public final String[] allTitles() {
        return SETTINGS.titles(mActivity);
    }

    @Override
    public final IMenuEnum[] allValues() {
        return SETTINGS.values();
    }

    @Override
    public final boolean areAllItemsEnabled() {
        return true;
    }

    @Override
    public final boolean isEnabled(int position) {
        if (position < 0 || position >= SETTINGS.size) {
            throw new ArrayIndexOutOfBoundsException();
        }
        return true;
    }

    enum SETTINGS implements Apple2AbstractMenu.IMenuEnum {
        TOUCH_MENU_ENABLED {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.touch_menu_enable);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.touch_menu_enable_summary);
            }

            @Override
            public View getView(final Apple2Activity activity, View convertView) {
                convertView = _basicView(activity, this, convertView);
                CheckBox cb = _addCheckbox(activity, this, convertView, Apple2Preferences.TOUCH_MENU_ENABLED.booleanValue(activity));
                cb.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        Apple2Preferences.TOUCH_MENU_ENABLED.saveBoolean(activity, isChecked);
                    }
                });
                return convertView;
            }
        },
        TOUCH_MENU_VISIBILITY {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.touch_menu_visibility);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.touch_menu_visibility_summary);
            }

            @Override
            public View getView(final Apple2Activity activity, View convertView) {
                return _sliderView(activity, this, Apple2Preferences.ALPHA_SLIDER_NUM_CHOICES, /*showFloatValue:*/true, new IPreferenceLoadSave() {
                    @Override
                    public void saveInt(int progress) {
                        Apple2Preferences.TOUCH_MENU_VISIBILITY.saveInt(activity, progress);
                    }
                    @Override
                    public int intValue() {
                        return Apple2Preferences.TOUCH_MENU_VISIBILITY.intValue(activity);
                    }
                });
            }
        },
        CURRENT_INPUT {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.input_current);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.input_current_summary);
            }

            @Override
            public View getView(final Apple2Activity activity, View convertView) {
                convertView = _basicView(activity, this, convertView);
                _addPopupIcon(activity, this, convertView);
                return convertView;
            }

            @Override
            public void handleSelection(final Apple2Activity activity, final Apple2AbstractMenu settingsMenu, boolean isChecked) {
                _alertDialogHandleSelection(activity, new String[]{
                        activity.getResources().getString(R.string.joystick),
                        activity.getResources().getString(R.string.keyboard),
                }, new IPreferenceLoadSave() {
                    @Override
                    public int intValue() {
                        return Apple2Preferences.CURRENT_TOUCH_DEVICE.intValue(activity) - 1;
                    }

                    @Override
                    public void saveInt(int value) {
                        Apple2Preferences.CURRENT_TOUCH_DEVICE.saveTouchDevice(activity, Apple2Preferences.TouchDevice.values()[value + 1]);
                    }
                });
            }
        },
        KEYBOARD_CONFIGURE {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.keyboard_configure);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.keyboard_configure_summary);
            }

            @Override
            public void handleSelection(final Apple2Activity activity, final Apple2AbstractMenu settingsMenu, boolean isChecked) {
                //new Apple2KeyboardSettingsMenu().show();
            }
        },
        JOYSTICK_CONFIGURE {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.joystick_configure);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.joystick_configure_summary);
            }

            @Override
            public void handleSelection(final Apple2Activity activity, final Apple2AbstractMenu settingsMenu, boolean isChecked) {
                //new Apple2JoystickSettingsMenu(activity).show();
            }
        },
        AUDIO_CONFIGURE {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.audio_configure);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.audio_configure_summary);
            }

            @Override
            public void handleSelection(Apple2Activity activity, Apple2AbstractMenu settingsMenu, boolean isChecked) {
                new Apple2AudioSettingsMenu(activity).show();
            }
        },
        VIDEO_CONFIGURE {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.video_configure);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.video_configure_summary);
            }

            @Override
            public View getView(Apple2Activity activity, View convertView) {
                convertView = _basicView(activity, this, convertView);
                _addPopupIcon(activity, this, convertView);
                return convertView;
            }

            @Override
            public void handleSelection(final Apple2Activity activity, final Apple2AbstractMenu settingsMenu, boolean isChecked) {
                _alertDialogHandleSelection(activity, new String[]{
                        settingsMenu.mActivity.getResources().getString(R.string.color_bw),
                        settingsMenu.mActivity.getResources().getString(R.string.color_color),
                        settingsMenu.mActivity.getResources().getString(R.string.color_interpolated),
                }, new IPreferenceLoadSave() {
                    @Override
                    public int intValue() {
                        return Apple2Preferences.HIRES_COLOR.intValue(activity);
                    }

                    @Override
                    public void saveInt(int value) {
                        Apple2Preferences.HIRES_COLOR.saveHiresColor(settingsMenu.mActivity, Apple2Preferences.HiresColor.values()[value]);
                    }
                });
            }
        },
        ABOUT {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.about_apple2ix);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.about_apple2ix_summary);
            }

            @Override
            public void handleSelection(Apple2Activity activity, final Apple2AbstractMenu settingsMenu, boolean isChecked) {
                String url = "http://github.com/mauiaaron/apple2";
                Intent i = new Intent(Intent.ACTION_VIEW);
                i.setData(Uri.parse(url));
                activity.startActivity(i);
            }
        },
        ABOUT_APPLE2 {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.about_apple2);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.about_apple2_summary);
            }

            @Override
            public void handleSelection(Apple2Activity activity, final Apple2AbstractMenu settingsMenu, boolean isChecked) {
                String url = "http://wikipedia.org/wiki/Apple_II";
                Intent i = new Intent(Intent.ACTION_VIEW);
                i.setData(Uri.parse(url));
                activity.startActivity(i);
            }
        },
        RESET_PREFERENCES {
            @Override
            public String getTitle(Apple2Activity activity) {
                return activity.getResources().getString(R.string.preferences_reset_title);
            }

            @Override
            public String getSummary(Apple2Activity activity) {
                return activity.getResources().getString(R.string.preferences_reset_summary);
            }

            @Override
            public void handleSelection(final Apple2Activity activity, final Apple2AbstractMenu settingsMenu, boolean isChecked) {
                AlertDialog.Builder builder = new AlertDialog.Builder(activity).setIcon(R.drawable.ic_launcher).setCancelable(true).setTitle(R.string.preferences_reset_really).setMessage(R.string.preferences_reset_warning).setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        Apple2Preferences.resetPreferences(activity);
                    }
                }).setNegativeButton(R.string.no, null);
                builder.show();
            }
        };

        public static final int size = SETTINGS.values().length;

        @Override
        public void handleSelection(Apple2Activity activity, Apple2AbstractMenu settingsMenu, boolean isChecked) {
            /* ... */
        }

        @Override
        public View getView(Apple2Activity activity, View convertView) {
            return _basicView(activity, this, convertView);
        }

        public static String[] titles(Apple2Activity activity) {
            String[] titles = new String[size];
            int i = 0;
            for (SETTINGS setting : values()) {
                titles[i++] = setting.getTitle(activity);
            }
            return titles;
        }
    }
}
