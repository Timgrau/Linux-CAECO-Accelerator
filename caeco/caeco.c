/* Driver for the CAECO-IP.
   Copyright (C) 2023  Timo G.
    
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/delay.h>

#define DRIVER_NAME "caeco"
#define CAECO_SIGNAL 23

/* Register Offsets */
#define CAECO_IP_CMD    0x0
#define CAECO_IP_RESULT 0x4
#define CAECO_IP_DEBUG1 0x8
#define CAECO_IP_DEBUG2 0xC


struct caeco_local {
  int irq;
  unsigned long mem_start;
  unsigned long mem_end;
  void __iomem *base_addr;
  struct clk *pl_clk;
};

static struct class *caeco_cls;
static int caeco_major;
static struct task_struct *us_task = NULL;

static inline u32 caeco_read(struct caeco_local *caeco, u32 reg)
{
  return ioread32(caeco->base_addr + reg);
}

static inline void caeco_write(struct caeco_local *caeco, u32 reg, u32 val)
{
  iowrite32(val, caeco->base_addr + reg);
}

static inline void caeco_start(struct caeco_local *caeco)
{
  caeco_write(caeco, CAECO_IP_CMD, 0x1);
  printk(KERN_INFO DRIVER_NAME " Ready to receive data.\n");
}

static inline void get_caeco_result(struct caeco_local *caeco)
{
  u32 result = caeco_read(caeco, CAECO_IP_RESULT);
  printk(KERN_INFO DRIVER_NAME " Classified result = 0x%08x\n", result);
}

static int caeco_open(struct inode *inode, struct file *file)
{
  /* Get the PID of the opening task */
  us_task = get_current();
  if(us_task == NULL)
    printk(KERN_INFO DRIVER_NAME " Could not register userspace task\n");
  else 
    printk(KERN_INFO DRIVER_NAME " Registered task with pid %d\n", us_task->pid);

  return 0;
}

static int caeco_release(struct inode *inode, struct file *file)
{
  /* Unregister the task */
  struct task_struct *ref_task = get_current();
  if(ref_task == us_task) {
    printk(KERN_INFO DRIVER_NAME " Unregister task with pid %d\n", us_task->pid);
    us_task = NULL;
  }
  
  return 0;
}

static struct file_operations fops =
  {
   .owner = THIS_MODULE,
   .open  = caeco_open,
   .release = caeco_release,
  };

static irqreturn_t caeco_irq(int irq, void *lp)
{
  struct siginfo info;
  
  printk("caeco interrupt\n");
  get_caeco_result(lp);

  /* Send signal to userspace process */
  memset(&info, 0, sizeof(info));
  info.si_signo = CAECO_SIGNAL;
  info.si_code  = SI_QUEUE;
  
  if(us_task != NULL) {
    printk(KERN_INFO DRIVER_NAME " Tyring to send a signal to PID: %d\n", us_task->pid);
    if(send_sig_info(CAECO_SIGNAL, (struct kernel_siginfo *) &info, us_task) < 0)
      printk(KERN_INFO DRIVER_NAME " Unable to send signal\n");
  }
	
  return IRQ_HANDLED;
}

static int caeco_clk_init(struct device *dev, struct clk *pl_clk)
{
  int rc;

  /* Clock consumer ID should match the DT */
  pl_clk = devm_clk_get(dev, "config_aclk");
  
  /* Check if pl_clk is a valid kernel-pointer */
  if(IS_ERR(pl_clk))
    return dev_err_probe(dev, PTR_ERR(pl_clk),  " failed to get pl_clk.\n");

  /* Try to enable the clk */
  rc = clk_prepare_enable(pl_clk);
  if(rc) {
    dev_err(dev, "failed to enable pl_clk (%d)\n", rc);
    goto disable_plclk;
  }

  printk(KERN_INFO DRIVER_NAME " Caeco clk frequency: %ld Hz\n",
	 clk_get_rate(pl_clk));
  
  return 0;

 disable_plclk:
  clk_disable_unprepare(pl_clk);

  return rc;
}

static int caeco_probe(struct platform_device *pdev)
{
  struct resource *r_irq;   /* Interrupt resources */
  struct resource *r_mem;   /* IO mem resources */
  struct device *dev = &pdev->dev;
  struct caeco_local *lp = NULL;

  int rc = 0;
  dev_info(dev, "Device Tree Probing\n");
	
  /* Get iospace for the device */
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    dev_err(dev, "invalid address\n");
    return -ENODEV;
  }
  
  lp = (struct caeco_local *) kmalloc(sizeof(struct caeco_local), GFP_KERNEL);
  if (!lp) {
    dev_err(dev, "Cound not allocate caeco device\n");
    return -ENOMEM;
  }
  
  dev_set_drvdata(dev, lp);
  lp->mem_start = r_mem->start;
  lp->mem_end = r_mem->end;	
  dev_info(&(pdev->dev), "dev_info");
  if (!request_mem_region(lp->mem_start,
			  lp->mem_end - lp->mem_start + 1,
			  DRIVER_NAME)) {
    dev_err(dev, "Couldn't lock memory region at %p\n",
	    (void *)lp->mem_start);
    rc = -EBUSY;
    goto error1;
  }

  lp->base_addr = ioremap(lp->mem_start, lp->mem_end - lp->mem_start + 1);
  if (!lp->base_addr) {
    dev_err(dev, "caeco: Could not allocate iomem\n");
    rc = -EIO;
    goto error2;
  }

  /* Get IRQ for the device */
  r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
  if (!r_irq) {
    dev_info(dev, "no IRQ found\n");
    return 0;
  }
  
  lp->irq = r_irq->start;	
  printk(KERN_INFO DRIVER_NAME " Trying to requst irq: %d\n", lp->irq);	
  rc = request_irq(lp->irq, &caeco_irq, 0, DRIVER_NAME, lp);
  if (rc) {
    dev_err(dev, "testmodule: Could not allocate interrupt %d.\n",
	    lp->irq);
    goto error3;
  }
  
  dev_info(dev,"caeco at 0x%08lx mapped to 0x%08lx, irq=%d\n",
	   (uintptr_t)lp->mem_start,
	   (uintptr_t)lp->base_addr,
	   lp->irq);
  /* Try to initialize the pl clk */
  caeco_clk_init(dev, lp->pl_clk);
  /* Write into the CMD-Register, so that caeco is ready to receive data */
  caeco_start(lp);
     
  return 0;
  
 error3:
  free_irq(lp->irq, lp);
 error2:
  release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
 error1:
  kfree(lp);
  dev_set_drvdata(dev, NULL);
  return rc;
}

static int caeco_chdev_init(void)
{
  /* Register chrdev-file for caeco */
  caeco_major = register_chrdev(0, DRIVER_NAME, &fops);
  if(caeco_major < 0) {
    printk(KERN_INFO DRIVER_NAME " Could not register char device\n");
    return -1;
  }
  
  printk(KERN_INFO DRIVER_NAME " Major number %d\n", caeco_major);
  caeco_cls = class_create(THIS_MODULE, DRIVER_NAME);
  device_create(caeco_cls, NULL, MKDEV(caeco_major, 0), NULL, DRIVER_NAME);
  pr_info("Device created on /dev/%s\n", DRIVER_NAME); 
  
  return 0; 
}

static void caeco_chdev_exit(void)
{
  /* Unregister chrdev-file */
  device_destroy(caeco_cls, MKDEV(caeco_major, 0));
  class_destroy(caeco_cls);
  unregister_chrdev(caeco_major, DRIVER_NAME);
}

static int caeco_remove(struct platform_device *pdev)
{
  /* rmmod: Freeup allocated resources */
  struct device *dev = &pdev->dev;
  struct caeco_local *lp = dev_get_drvdata(dev);
  free_irq(lp->irq, lp);
  iounmap(lp->base_addr);
  release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
  kfree(lp);
  dev_set_drvdata(dev, NULL);
  
  return 0;
}

/* This should match the DT */
static struct of_device_id caeco_of_match[] = {
  { .compatible = "xlnx,caeco-ip-2.0", },
};

static struct platform_driver caeco_driver =
  {
   .driver = {
	      .name  = DRIVER_NAME,
	      .owner = THIS_MODULE,
	      .of_match_table = caeco_of_match,
	      },
   .probe  = caeco_probe,
   .remove = caeco_remove,
  };

static int __init caeco_init(void)
{
  /* Register File operation */
  int rc;
  printk(KERN_ALERT DRIVER_NAME " initializing.\n");
  
  rc = caeco_chdev_init();
  if(rc < 0)
    return rc;
  
  return platform_driver_register(&caeco_driver);
}


static void __exit caeco_exit(void)
{
  caeco_chdev_exit();
  platform_driver_unregister(&caeco_driver);
  printk(KERN_ALERT DRIVER_NAME " unregistered.\n");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Timo Grautstueck");
MODULE_DESCRIPTION("Driver for the caeco-ipc.");

module_init(caeco_init);
module_exit(caeco_exit);
