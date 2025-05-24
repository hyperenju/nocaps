#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/input.h>

static bool disable_caps = false;
module_param(disable_caps, bool, S_IRUGO | S_IWUSR);

#define KBD_STATUS_MASK 0x80
#define KBD_SCANCODE_MASK 0x7f

/* Which function to choose as the attach point:
 * Since builtin keyboard is probed by atkbd in my laptop like this:
 *     $ ls -l /sys/devices/platform/i8042/serio0/driver
 *     lrwxrwxrwx 1 root root 0 May 15 22:11 /sys/devices/platform/i8042/serio0/driver -> ../../../../bus/serio/drivers/atkbd
 * functions of atkbd driver are good candidates to attach the kprobe. Also we
 * can choose functions around i8042 and serio subsystem, as far as they have
 * scancode/keycode as their argument, such as serio_interrupt(). In this case,
 * of course, this kprobe has impacts to all devices that use those subsystem,
 * not only to atkbd.
 */
#define SYMBOL_NAME "atkbd_receive_byte"

#define EXTENDED_E0_PREFIX 0xe0
static atomic_t extended_e0_mode = ATOMIC_INIT(0);

static int __kprobes swap_scancode(struct kprobe *p, struct pt_regs *regs) {
    /* rsi is second argument in x86. */
    unsigned char raw = regs->si;
    unsigned char scancode = raw & KBD_SCANCODE_MASK;
    unsigned char status = raw & KBD_STATUS_MASK;

    /* Skip conversion when we are in the sequence of extended scancodes that
     * start with 0xE0 and consist of 2 bytes.
     */
    if (raw == EXTENDED_E0_PREFIX) {
        atomic_set(&extended_e0_mode, 1);
        return 0;
    }

    if (atomic_read(&extended_e0_mode)) {
        atomic_set(&extended_e0_mode, 0);
        return 0;
    }

    switch (scancode) {
    case KEY_CAPSLOCK:
        scancode = KEY_LEFTCTRL;
        break;

    case KEY_LEFTCTRL:
        if (disable_caps)
            return 0;
        scancode = KEY_CAPSLOCK;
        break;

    default:
        return 0;
    }

    regs->si = scancode + status;
    return 0;
}

static struct kprobe kp = {
    .symbol_name = SYMBOL_NAME,
    .pre_handler = swap_scancode,
};

static int nocaps_init(void) {
    int ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("register_kprobe failed, returned %d\n", ret);
        return ret;
    }
    pr_info("NOCAPS loaded at %s.\n", SYMBOL_NAME);
    return 0;
}

static void nocaps_exit(void) {
    unregister_kprobe(&kp);
    pr_info("NOCAPS unloaded.\n");
    return;
}

module_init(nocaps_init);
module_exit(nocaps_exit);

MODULE_LICENSE("GPL");
