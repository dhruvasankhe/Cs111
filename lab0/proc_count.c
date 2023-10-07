#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>


static struct proc_dir_entry* my_new_entry;

static int seq_show(struct seq_file *s, void *v) {
	int processCount = 0;
	struct task_struct* t;
	for_each_process(t) {
		processCount++;
	}
	seq_printf(s, "%d\n", processCount);
	return 0;
}

static int __init proc_count_init(void)
{
	pr_info("proc_count: init\n");
	my_new_entry = proc_create_single("count", 0644, NULL, seq_show);
	return 0;
}

static void __exit proc_count_exit(void)
{
	pr_info("proc_count: exit\n");
	proc_remove(my_new_entry);
}

module_init(proc_count_init);
module_exit(proc_count_exit);

MODULE_AUTHOR("Your name");
MODULE_DESCRIPTION("One sentence description");
MODULE_LICENSE("GPL");