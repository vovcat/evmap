/*
 * evdev_map -- manipulate evdev keycode tables
 *
 * Nicolas George, 2020-08-03
 * Public domain
 */

/*
$ ./evdev_map -d /dev/input/event8 -p
index scancode    keycode name
    0 00100057       0xe0 BRIGHTNESSDOWN
    1 00100058       0xe1 BRIGHTNESSUP
    2 00100150       0xf8 MICMUTE
    3 00100850       0x94 PROG1
    4 00100851       0x95 PROG2
    5 00100852       0xca PROG3
    6 0000e005       0xe0 BRIGHTNESSDOWN
    7 0000e006       0xe1 BRIGHTNESSUP
    8 0000e007       0xec BATTERY
    9 0000e009       0xa1 EJECTCD
   10 0000e00b       0xe3 SWITCHVIDEOMODE
   11 0000e011       0xee WLAN
   12 0000e025       0xcb PROG4
   13 0000e027      0x1af BRIGHTNESS_TOGGLE
   14 0000e029       0xcb PROG4
*/

/*

Multiple keyboard layouts on the Linux console
https://lists.debian.org/debian-user/2020/08/msg00179.html

    To: debian-user@lists.debian.org
    Subject: Multiple keyboard layouts on the Linux console
    From: Nicolas George <george@nsup.org>
    Date: Tue, 4 Aug 2020 17:32:57 +0200
    Message-id: <[?] 20200804153257.oxj5kc4gwdl46jec@phare.normalesup.org>
    Reply-to: debian-user@lists.debian.org

Hi.

Some time ago, I explained here how to use different keyboard layouts
with X11.

https://lists.debian.org/debian-user/2020/02/msg00755.html

Now I will explain how to do it with the Linux console. Unfortunately,
it also relies on a non-standard tool.

The use-case I will take as an example is this: imagine there is on your
desktop a keyboard with fancy extra keys, including a "power" key in the
corner; and that you have a cat.

(We could set "HandlePowerKey=ignore" in /etc/systemd/logind.conf, but
that would also prevent from using the button on the box itself. The cat
does not walk on this one.) (This is an example, what I explain can work
for other kinds of keyboards.)

First, a disappointment: the standard way of setting the keyboard layout
for the Linux console, loadkeys, is global. It cannot handle several
keyboards with different layouts.

Fortunately, there is another layer of conversion: before the console
converts key codes to characters or functions, the keyboard driver
converts device-dependant scan codes into device-independent key codes.
We can tweak that.

Unfortunately, it seems no tool exists to call the kernel interfaces
that allow to change the scan code to key code conversion. I have
written my own, see the bottom of this mail.

First, we need to find out which device corresponds to the keyboard;
worse: to the key, because some keyboards create several devices, with
some keys going to one and some other keys going to another.

So, try, with root privileges:

xxd /dev/input/eventX

for various values of X (you can use other dump tools than xxd of
course) and press the key. The correct device is the one that causes
data to appear each time the key is pressed. Let us say it is
/dev/input/event6.

Next, let us find the default layout:

sudo /usr/local/sbin/evdev_map -d /dev/input/event6 -p
...
  569 000c023b       0xf0 (UNKNOWN)
  570 000c023c       0xf0 (UNKNOWN)
  571 00010081       0x74 (POWER)
  572 00010082       0x8e (SLEEP)
  573 00010083       0x8f (WAKEUP)
...

The offending key seems to have scan code 00010081. Let us try to remap
it:

sudo /usr/local/sbin/evdev_map -d /dev/input/event6 -s 00010081=A

if pressing the key now produces a 'a', we have won. Otherwise, maybe
there is another scan code mapped to the same key code, keyboards often
declare way more scan codes than they have.

No, we only need to automate it, using for example 0x0 to disable the
key ("0" would mean the char '0'). It is a job for udev:

UBSYSTEM=="input", ACTION=="add|change", \
  ATTRS{name}=="USB-compliant keyboard System Control", \
  RUN+="/usr/local/sbin/evdev_map -d $devnode -s 00010081=0x0"

(I will not develop here how to use udevadm info -a -p
/sys/class/input/event6 to get the conditions that allow to identify
this keyboard over others.)

If the purpose is to change the layout in a significant way, it may be
more complex. For example, azerty's key [1] yields '&' unshifted and '1'
shifted. To handle that, you would have to find an unused key code, and
then use loadkeys table to map it.

Hope this helps somebody.

Regards,

--
  Nicolas George

----8<----8<----8<----8<---- evdev_map.c ---->8---->8---->8---->8----

*/

/*
   Building:

   First, generate the table of key names:

   gcc -E -dM -x c - <<< '#include <linux/input-event-codes.h>' |perl -ne \
    'if (/^\#define (KEY_(\w+))\s+\S+/) { print "  { $1, \"$2\" },\n" }' \
    > key_names.h

   Then:

   c99 -Wall -Wextra -D_XOPEN_SOURCE=600 -g -O2 -o evdev_map evdev_map.c

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
# define REVERSE_SCANCODE 1
#endif

typedef struct Key_name {
    unsigned code;
    const char *name;
} Key_name;

static const Key_name key_names[] = {
#include "key_names.inc"
};

static void
scancode_to_string(char *out, __u8 *code, int len)
{
#ifdef REVERSE_SCANCODE
    int ic = len - 1, id = -1;
#else
    int ic = 0, id = +1;
#endif
    while (len-- > 0) {
        snprintf(out, 3, "%02x", code[ic]);
        ic += id;
        out += 2;
    }
}

static void
check_device(int dev)
{
    if (dev < 0) {
        fprintf(stderr, "No device opened\n");
        exit(1);
    }
}

static const char *
get_key_by_code(unsigned code)
{
    size_t i;

    for (i = 0; i < sizeof(key_names) / sizeof(*key_names); i++)
        if (key_names[i].code == code)
            return key_names[i].name;
    return NULL;
}

static unsigned
get_key_by_name(const char *name)
{
    size_t i;
    unsigned code, off = 0;

    for (i = 0; i < sizeof(key_names) / sizeof(*key_names); i++)
        if (strcmp(key_names[i].name, name) == 0)
            return key_names[i].code;
    sscanf(name, "%i%n", &code, &off);
    if (off == 0 || name[off] != 0) {
        fprintf(stderr, "Unknown key: %s\n", name);
        exit(1);
    }
    return code;
}

/**
 * struct input_keymap_entry - used by EVIOCGKEYCODE/EVIOCSKEYCODE ioctls
 * @scancode: scancode represented in machine-endian form.
 * @len: length of the scancode that resides in @scancode buffer.
 * @index: index in the keymap, may be used instead of scancode
 * @flags: allows to specify how kernel should handle the request. For
 *      example, setting INPUT_KEYMAP_BY_INDEX flag indicates that kernel
 *      should perform lookup in keymap by @index instead of @scancode
 * @keycode: key code assigned to this scancode
 *
 * The structure is used to retrieve and modify keymap data. Users have
 * option of performing lookup either by @scancode itself or by @index
 * in keymap entry. EVIOCGKEYCODE will also return scancode or index
 * (depending on which element was used to perform lookup).

struct input_keymap_entry {
#define INPUT_KEYMAP_BY_INDEX   (1 << 0)
        __u8  flags;
        __u8  len;
        __u16 index;
        __u32 keycode;
        __u8  scancode[32];
};
 */

static void print_keymap(int dev)
{
    struct input_keymap_entry ke;
    char scancode[sizeof(ke.scancode) * 2 + 1];
    const char *name;
    unsigned i;
    int ret;

    check_device(dev);
    printf("%5s %8s %10s %s\n", "index", "scancode", "keycode", "name");

    for (i = 0; i < 0x10000; i++) {
        ke.index = i;
        ke.flags = INPUT_KEYMAP_BY_INDEX;
        ret = ioctl(dev, EVIOCGKEYCODE_V2, &ke);
        if (ret < 0) {
            if (errno == EINVAL)
                break;
            perror("ioctl(EVIOCGKEYCODE_V2)");
            exit(1);
        }
        if (ke.index != i) {
            fprintf(stderr, "Inconsistency detected: index: %d != %d\n",
                ke.index, i);
            exit(1);
        }
        if (ke.len > sizeof(ke.scancode)) {
            fprintf(stderr, "Inconsistency detected: len: %d > %zd\n",
                ke.len, sizeof(ke.scancode));
            exit(1);
        }
        scancode_to_string(scancode, ke.scancode, ke.len);
        name = get_key_by_code(ke.keycode);
        printf("%5d %8s %#10x %s\n", ke.index, scancode, ke.keycode,
            name == NULL ? "?" : name);
    }
    fflush(stdout);
}

static void
set_keycode(int dev, const char *def)
{
    struct input_keymap_entry ke;
    const char *sep;
    int ic, id, off, ret;
    unsigned c, i;

    check_device(dev);

    ke.flags = 0;
    ke.index = 0;

    off = 0;
    if (sscanf(def, "%hu:%n", &ke.index, &off) == 1 && off) {
        ke.flags |= INPUT_KEYMAP_BY_INDEX;
        def += off;
    }

    sep = strchr(def, '=');
    if (sep == NULL || (size_t)(sep - def) > 2 * sizeof(ke.scancode) ||
        (sep - def) % 2 != 0) {
        fprintf(stderr, "Invalid definition: %s\n", def);
        exit(1);
    }
    ke.len = (sep - def) / 2;
#ifdef REVERSE_SCANCODE
    ic = ke.len - 1;
    id = -1;
#else
    ic = 0;
    id = +1;
#endif
    for (i = 0; i < ke.len; i++) {
        off = 0;
        sscanf(def + i * 2, "%02x%n", &c, &off);
        if (off != 2) {
            fprintf(stderr, "Invalid scancode: %s\n", def);
            exit(1);
        }
        ke.scancode[ic] = c;
        ic += id;
    }
    ke.keycode = get_key_by_name(sep + 1);
    ret = ioctl(dev, EVIOCSKEYCODE_V2, &ke);
    fprintf(stderr, "Setting keymap[%d] with flags=%x: scancode=%08x len=%d ke.keycode=%#x returned %d\n",
        ke.index, ke.flags, *(int*)&ke.scancode, ke.len, ke.keycode, ret);
    if (ret < 0) {
        perror("ioctl(EVIOCSKEYCODE_V2)");
        exit(1);
    }
}

static void usage(int ret)
{
    FILE *out = ret ? stderr : stdout;

    fprintf(out,
        "evdev_map -- manipulate evdev keycode tables\n"
        "Usage: evdev_map -d device [-p] [-s scancode=keycode]\n"
        "\n"
        "    -d device                  select the input device\n"
        "    -p                         print the current map\n"
        "                               columns: index scancode keycode key_name\n"
        "    -s [idx:]scancode=keycode  change the mapping for a scancode\n"
        "                               (key names work too; use 0x0 for RESERVED)\n"
        "    -h                         print this message\n"
        "Options are processed in order and can be repeated.\n"
        );
    fflush(out);
    if (ret)
        exit(ret);
}

int main(int argc, char **argv)
{
    int dev = -1, opt, act = 0;

    while ((opt = getopt(argc, argv, "d:ps:h")) >= 0) {
        switch (opt) {
            case 'd':
                if (dev >= 0)
                    close(dev);
                dev = open(optarg, O_RDONLY);
                if (dev < 0) {
                    perror(optarg);
                    exit(1);
                }
                break;

            case 'p':
                print_keymap(dev);
                act = 1;
                break;

            case 's':
                set_keycode(dev, optarg);
                act = 1;
                break;

            case 'h':
                usage(0);
                break;

            default:
                usage(1);
                break;

        }
    }

    if (optind < argc || !act)
        usage(1);

    return 0;
}
