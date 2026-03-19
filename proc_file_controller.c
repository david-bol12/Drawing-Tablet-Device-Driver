#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "cdev_controller.h"
#include "tablet.h"

static struct proc_dir_entry *proc_entry;


static int proc_show(struct seq_file *m, void *v);
static int proc_open(struct inode *inode, struct file *file);


static const struct proc_ops proc_ops = {
    .proc_open    = proc_open,
    .proc_read    = seq_read,
    .proc_release = single_release,
};

static int proc_show(struct seq_file *m, void *v) {
    struct tablet_event current_event;

    mutex_lock(&tablet_mutex);
    current_event = event_buffer;
    mutex_unlock(&tablet_mutex);

    seq_printf(m, "Total reads: %d\n", total_reads);
    seq_printf(m, "Total writes: %d\n", total_writes);
    seq_printf(m, "Current open count: %d\n", open_count);
    seq_printf(m, "Current X:        %d\n", current_event.x);
    seq_printf(m, "Current Y:        %d\n", current_event.y);
    seq_printf(m, "Current pressure: %d\n", current_event.pressure);


    return 0;
}

static int proc_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_show, NULL);

}

int proc_init(void) {
    proc_entry = proc_create("tablet_stats", 0444, NULL, &proc_ops);
    if (!proc_entry) {
        return -ENOMEM;
    }
    printk(KERN_INFO "/proc/tablet_stats created\n");
    return 0;
}

void proc_exit(void) {
    proc_remove(proc_entry);
    printk(KERN_INFO "tablet: /proc/tablet_stats removed\n");

}
