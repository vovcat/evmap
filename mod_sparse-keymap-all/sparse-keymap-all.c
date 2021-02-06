// SPDX-License-Identifier: GPL-2.0-only

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

#include <linux/input.h>
#include <linux/input/sparse-keymap.h>

/*
 * This kprobe overrides the functions that get/set sparse
 * keymap entry.
 * Example:
 *
 *  # ../evmap -d /dev/input/event8 -p
 *  index scancode    keycode name
 *      0 00100057       0xe0 BRIGHTNESSDOWN
 *      1 00100058       0xe1 BRIGHTNESSUP
 *      2 00100150       0xf8 MICMUTE
 *      3 00100850       0x94 PROG1
 *      4 00100851       0x95 PROG2
 *      5 00100852       0xca PROG3
 *      6 0000e005       0xe0 BRIGHTNESSDOWN
 *      7 0000e006       0xe1 BRIGHTNESSUP
 *      8 0000e007       0xec BATTERY
 *      9 0000e009       0xa1 EJECTCD
 *     10 0000e00b       0xe3 SWITCHVIDEOMODE
 *     11 0000e011       0xee WLAN
 *     12 0000e025       0xcb PROG4
 *     13 0000e026       0x8e SLEEP
 *     14 0000e027      0x1af BRIGHTNESS_TOGGLE
 *     15 0000e029       0xcb PROG4
 *  # s insmod sparse-keymap-all.ko
 *  # ../evmap -d /dev/input/event8 -p
 *  index scancode    keycode name
 *      0 00100000          0 RESERVED
 *      1 00100001          0 RESERVED
 *      2 0010003f          0 RESERVED
 *      3 00100057       0xe0 BRIGHTNESSDOWN
 *      4 00100058       0xe1 BRIGHTNESSUP
 *      5 00100150       0xf8 MICMUTE
 *      6 00100151          0 RESERVED
 *      7 00100152       0xe4 KBDILLUMTOGGLE
 *      8 00100153       0xf7 RFKILL
 *      9 00100154          0 RESERVED
 *     10 00100155          0 RESERVED
 *     11 00100156          0 RESERVED
 *     12 00100157          0 RESERVED
 *     13 00100850       0x94 PROG1
 *     14 00100851       0x95 PROG2
 *     15 00100852       0xca PROG3
 *     16 0010e008       0xf7 RFKILL
 *     17 0010e035          0 RESERVED
 *     18 0011fff0          0 RESERVED
 *     19 0011fff1          0 RESERVED
 *     20 0011fff2          0 RESERVED
 *     21 0011fff3          0 RESERVED
 *     22 001101e1          0 RESERVED
 *     23 001101e2          0 RESERVED
 *     24 001101e3          0 RESERVED
 *     25 001102ea          0 RESERVED
 *     26 001102eb          0 RESERVED
 *     27 001102ec          0 RESERVED
 *     28 001102f6          0 RESERVED
 *     29 0012e035          0 RESERVED
 *     30 0000003a       0x3a CAPSLOCK
 *     31 0000e005       0xe0 BRIGHTNESSDOWN
 *     32 0000e006       0xe1 BRIGHTNESSUP
 *     33 0000e007       0xec BATTERY
 *     34 0000e008       0xf7 RFKILL
 *     35 0000e009       0xa1 EJECTCD
 *     36 0000e00b       0xe3 SWITCHVIDEOMODE
 *     37 0000e00c       0xe4 KBDILLUMTOGGLE
 *     38 0000e00d          0 RESERVED
 *     39 0000e00e          0 RESERVED
 *     40 0000e011       0xee WLAN
 *     41 0000e013          0 RESERVED
 *     42 0000e020       0x71 MIN_INTERESTING
 *     43 0000e025       0xcb PROG4
 *     44 0000e026       0x8e SLEEP
 *     45 0000e027      0x1af BRIGHTNESS_TOGGLE
 *     46 0000e029       0xcb PROG4
 *     47 0000e02e       0x72 VOLUMEDOWN
 *     48 0000e030       0x73 VOLUMEUP
 *     49 0000e033       0xe6 KBDILLUMUP
 *     50 0000e034       0xe5 KBDILLUMDOWN
 *     51 0000e03a       0x3a CAPSLOCK
 *     52 0000e043          0 RESERVED
 *     53 0000e044          0 RESERVED
 *     54 0000e045       0x45 NUMLOCK
 *     55 0000e046       0x46 SCROLLLOCK
 *     56 0000e06e          0 RESERVED
 *     57 0000e0f7       0x71 MIN_INTERESTING
 *     58 0000e0f8       0x72 VOLUMEDOWN
 *     59 0000e0f9       0x73 VOLUMEUP
 *  #
 */

static unsigned int sparse_keymap_get_key_index(struct input_dev *dev,
                        const struct key_entry *k)
{
    struct key_entry *key;
    unsigned int idx = 0;
    for (key = dev->keycode; key->type != KE_END; key++) {
        if (key == k)
            break;
        idx++;
    }
    return idx;
}

static struct key_entry *sparse_keymap_entry_by_index_all(struct input_dev *dev,
                              unsigned int index)
{
    struct key_entry *key;
    unsigned int key_cnt = 0;
    for (key = dev->keycode; key->type != KE_END; key++)
        if (key_cnt++ == index)
            return key;
    return NULL;
}

/**
 * sparse_keymap_entry_from_scancode - perform sparse keymap lookup
 * @dev: Input device using sparse keymap
 * @code: Scan code
 *
 * This function is used to perform &struct key_entry lookup in an
 * input device using sparse keymap.
 */
static struct key_entry *sparse_keymap_entry_from_scancode_all(struct input_dev *dev,
                            unsigned int code)
{
    struct key_entry *key;
    for (key = dev->keycode; key->type != KE_END; key++)
        if (code == key->code)
            return key;
    return NULL;
}

/**
 * sparse_keymap_entry_from_keycode - perform sparse keymap lookup
 * @dev: Input device using sparse keymap
 * @keycode: Key code
 *
 * This function is used to perform &struct key_entry lookup in an
 * input device using sparse keymap.

static struct key_entry *sparse_keymap_entry_from_keycode_UNUSED(struct input_dev *dev,
                           unsigned int keycode)
{
    struct key_entry *key;
    for (key = dev->keycode; key->type != KE_END; key++)
        if (key->type == KE_KEY && keycode == key->keycode)
            return key;
    return NULL;
}
 */

static struct key_entry *sparse_keymap_locate_all(struct input_dev *dev,
                    const struct input_keymap_entry *ke)
{
    struct key_entry *key;
    unsigned int scancode;
    if (ke->flags & INPUT_KEYMAP_BY_INDEX)
        key = sparse_keymap_entry_by_index_all(dev, ke->index);
    else if (input_scancode_to_scalar(ke, &scancode) == 0)
        key = sparse_keymap_entry_from_scancode_all(dev, scancode);
    else
        key = NULL;
    return key;
}


static int sparse_keymap_getkeycode_all(struct input_dev *dev,
                    struct input_keymap_entry *ke)
{
    const struct key_entry *key;
    if (dev->keycode) {
        key = sparse_keymap_locate_all(dev, ke);
        if (key) {
            ke->keycode = key->keycode;
            if (!(ke->flags & INPUT_KEYMAP_BY_INDEX))
                ke->index = sparse_keymap_get_key_index(dev, key);
            ke->len = sizeof(key->code);
            memcpy(ke->scancode, &key->code, sizeof(key->code));
            return 0;
        }
    }
    return -EINVAL;
}

static int sparse_keymap_setkeycode_all(struct input_dev *dev,
                    const struct input_keymap_entry *ke,
                    unsigned int *old_keycode)
{
    struct key_entry *key;
    int old_type;

    if (dev->keycode) {
        key = sparse_keymap_locate_all(dev, ke);
        if (key) {
            if (ke->len > sizeof(key->code))
                return -EINVAL;

            old_type = key->type;
            *old_keycode = key->keycode;

            /*
             * Override key->type when new keycode is not null
             * and old type is in (KE_KEY, KE_IGNORE)
             */
            if (ke->keycode == KEY_RESERVED) {
                 if (key->type == KE_KEY) key->type = KE_IGNORE;
            } else {
                 if (key->type == KE_IGNORE) key->type = KE_KEY;
            }

            key->keycode = ke->keycode;
            key->code = 0;
            memcpy(&key->code, ke->scancode, ke->len);

            /*
             * Update dev->keybit:
             *     KE_KEY -> KE_IGNORE: clear old
             *     KE_IGNORE -> KE_KEY: set new
             *     KE_KEY -> KE_KEY: clear old, set new
             *     KE_IGNORE -> KE_IGNORE: do nothing
             */
            if (old_type == KE_KEY) {
                clear_bit(*old_keycode, dev->keybit);
                if (sparse_keymap_entry_from_keycode(dev, *old_keycode))
                    set_bit(*old_keycode, dev->keybit);
            }

            if (key->type == KE_KEY) {
                set_bit(ke->keycode, dev->keybit);
                if (!sparse_keymap_entry_from_keycode(dev, *old_keycode))
                    clear_bit(*old_keycode, dev->keybit);
            }

            return 0;
        }
    }
    return -EINVAL;
}

// kprobe pre_handler: called just before the probed instruction is executed

static int sparse_keymap_getkeycode_pre(struct kprobe *p, struct pt_regs *regs)
{
#ifdef CONFIG_X86
    pr_info("<%s> getkeycode_pre: p->addr = 0x%p, ip = %lx, flags = 0x%lx\n",
        p->symbol_name, p->addr, regs->ip, regs->flags);
    regs->ip = (ulong) &sparse_keymap_getkeycode_all;
    return 1; // stop single stepping and just return to the given address
#endif
#ifdef CONFIG_PPC
    pr_info("<%s> getkeycode_pre: p->addr = 0x%p, nip = 0x%lx, msr = 0x%lx\n",
        p->symbol_name, p->addr, regs->nip, regs->msr);
#endif
#ifdef CONFIG_MIPS
    pr_info("<%s> getkeycode_pre: p->addr = 0x%p, epc = 0x%lx, status = 0x%lx\n",
        p->symbol_name, p->addr, regs->cp0_epc, regs->cp0_status);
#endif
#ifdef CONFIG_ARM64
    pr_info("<%s> getkeycode_pre: p->addr = 0x%p, pc = 0x%lx,"
            " pstate = 0x%lx\n",
        p->symbol_name, p->addr, (long)regs->pc, (long)regs->pstate);
#endif
#ifdef CONFIG_S390
    pr_info("<%s> getkeycode_pre: p->addr, 0x%p, ip = 0x%lx, flags = 0x%lx\n",
        p->symbol_name, p->addr, regs->psw.addr, regs->flags);
#endif

    /* A dump_stack() here will give a stack backtrace */
    return 0;
}

static int sparse_keymap_setkeycode_pre(struct kprobe *p, struct pt_regs *regs)
{
#ifdef CONFIG_X86
    pr_info("<%s> setkeycode_pre: p->addr = 0x%p, ip = %lx, flags = 0x%lx\n",
        p->symbol_name, p->addr, regs->ip, regs->flags);
    regs->ip = (ulong) &sparse_keymap_setkeycode_all;
    return 1; // stop single stepping and just return to the given address
#endif
#ifdef CONFIG_PPC
    pr_info("<%s> setkeycode_pre: p->addr = 0x%p, nip = 0x%lx, msr = 0x%lx\n",
        p->symbol_name, p->addr, regs->nip, regs->msr);
#endif
#ifdef CONFIG_MIPS
    pr_info("<%s> setkeycode_pre: p->addr = 0x%p, epc = 0x%lx, status = 0x%lx\n",
        p->symbol_name, p->addr, regs->cp0_epc, regs->cp0_status);
#endif
#ifdef CONFIG_ARM64
    pr_info("<%s> setkeycode_pre: p->addr = 0x%p, pc = 0x%lx,"
            " pstate = 0x%lx\n",
        p->symbol_name, p->addr, (long)regs->pc, (long)regs->pstate);
#endif
#ifdef CONFIG_S390
    pr_info("<%s> setkeycode_pre: p->addr, 0x%p, ip = 0x%lx, flags = 0x%lx\n",
        p->symbol_name, p->addr, regs->psw.addr, regs->flags);
#endif

    /* A dump_stack() here will give a stack backtrace */
    return 0;
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
    pr_info("fault_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
    /* Return 0 because we don't handle the fault. */
    return 0;
}

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp[] = {
    {
        .symbol_name = "sparse_keymap_getkeycode",
        .pre_handler = sparse_keymap_getkeycode_pre,
        .fault_handler = handler_fault
    },
    {
        .symbol_name = "sparse_keymap_setkeycode",
        .pre_handler = sparse_keymap_setkeycode_pre,
        .fault_handler = handler_fault
    },
};

static int __init sparse_keymap_all_init(void)
{
    int ret;

    for (int i = 0; i < ARRAY_SIZE(kp); i++) {
        ret = register_kprobe(&kp[i]);
        if (ret < 0) {
            pr_err("register_kprobe failed, returned %d\n", ret);
            return ret;
        }
        pr_info("Planted kprobe %s at %p\n", kp[i].symbol_name, kp[i].addr);
    }
    return 0;
}

static void __exit sparse_keymap_all_exit(void)
{
    for (int i = 0; i < ARRAY_SIZE(kp); i++) {
        unregister_kprobe(&kp[i]);
        pr_info("Removed kprobe %s at %p\n", kp[i].symbol_name, kp[i].addr);
    }
}

module_init(sparse_keymap_all_init)
module_exit(sparse_keymap_all_exit)
MODULE_LICENSE("GPL");
