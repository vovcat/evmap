# evmap

Manipulate evdev keycode tables using EVIOCGKEYCODE_V2/EVIOCSKEYCODE_V2

Usage: evmap -d device [-p] [-s scancode=keycode]

    -d device                  select the input device
    -p                         print the current map
                               columns: index scancode keycode key_name
    -s [idx:]scancode=keycode  change the mapping for a scancode
                               (key names work too; use 0x0 for RESERVED)
    -h                         print this message

    Options are processed in order and can be repeated.

# mod_sparse-keymap-all

Linux kernel module that allows setting sparse keymap entries hidden by
KE_IGNORE key_entry type is in mod_sparse-keymap-all folder. It is needed to
set mapping for events ignored by many WMI Linux kernel drivers including
dell_wmi module.

The module uses kprobes system to install replacement for
sparse_keymap_getkeycode() and sparse_keymap_setkeycode() kernel functions.

# xi2watch

X11 and hot-plugged keyboards and multiple layouts handler without root
privileges.

Handling the layout of hot-plugged keyboards involves two X11
extensions, XKEYBOARD aka XKB and XInputExtension aka XI2.

xi2watch tool listens to XI2 events and whenever a change in the XI2
hierarchy is signaled, it calls the command given as arguments with
environment variables describing the event:

    ./xi2watch env
    DEVICE=15
    DEVICE_NAME=  mini keyboard Consumer Control
    ENABLED=1
    FLAG_MASTER_ADDED=0
    FLAG_MASTER_REMOVED=0
    FLAG_SLAVE_ADDED=0
    FLAG_SLAVE_REMOVED=0
    FLAG_SLAVE_ATTACHED=0
    FLAG_SLAVE_DETACHED=0
    FLAG_DEVICE_ENABLED=1
    FLAG_DEVICE_DISABLED=0
    USE=slave_keyboard

The command can then be a shell script that will choose the layout and
apply it.

    case ${USE}:${FLAG_DEVICE_ENABLED}:${DEVICE_NAME} in
      slave_keyboard:1:*mini keyboard*)
        setxkbmap -device $DEVICE fr
        ;;
    esac

With that, we can configure the layout of keyboards automatically, as
they are plugged, without the need for root privileges. It can also be
used to set the speed of mice. I believe it should be widely available.

# Authors

* Nicolas George, 2020-08-03
* Volodymyr Prodan, 2020-11-27, https://github.com/vovcat/evmap

# License

Public domain
