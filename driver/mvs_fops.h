#ifndef DRIVER_MVS_FOPS_H_
#define DRIVER_MVS_FOPS_H_

#include <linux/fs.h>

int mvs_open(struct inode *inode, struct file *filp);
long mvs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int mvs_lock(struct file *filp, int cmd, struct file_lock *fl);

#endif /* DRIVER_MVS_FOPS_H_ */
