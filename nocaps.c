#define pr_fmt(fmt) "%s:%s():%d: " fmt, KBUILD_MODNAME, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>

#define KBD_STATUS_MASK 0x80
#define KBD_SCANCODE_MASK 0x7f
#define KBD_SCANCODE_CAPS 58
#define KBD_SCANCODE_CTRL 29

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

static int __kprobes caps_to_ctrl(struct kprobe *p, struct pt_regs *regs) {
    unsigned char data = regs->si; /* rsi is second argument in x86. */
    if ((data & KBD_SCANCODE_MASK) != KBD_SCANCODE_CAPS)
        return 0;

    regs->si = KBD_SCANCODE_CTRL + (data & KBD_STATUS_MASK);
    return 0;
}

static struct kprobe kp = {
    .symbol_name = SYMBOL_NAME,
    .pre_handler = caps_to_ctrl,
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
