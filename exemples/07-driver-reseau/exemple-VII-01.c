/************************************************************************\
  Exemples de la formation
    "Ecriture de drivers et programmation noyau Linux"
  Chapitre "Driver reseau"

  (c) 2005-2019 Christophe Blaess
  http://www.blaess.fr/christophe/

\************************************************************************/


	#include <linux/module.h>
	#include <linux/version.h>

	#include <linux/netdevice.h>
	#include <linux/etherdevice.h>


	struct net_device * net_dev_ex = NULL;


static int example_open (struct net_device * net_dev)
{
	printk(KERN_INFO "%s - %s(%pK):\n",
	       THIS_MODULE->name, __FUNCTION__, net_dev);

	net_dev->dev_addr[0] = 0x00;
	net_dev->dev_addr[1] = 0x12;
	net_dev->dev_addr[2] = 0x34;
	net_dev->dev_addr[3] = 0x56;
	net_dev->dev_addr[4] = 0x78;
	net_dev->dev_addr[5] = 0x00;

	netif_start_queue(net_dev);

	return 0;
}


static int example_stop (struct net_device * net_dev)
{
	printk(KERN_INFO "%s - %s(%pK)\n",
	       THIS_MODULE->name, __FUNCTION__, net_dev);

	netif_stop_queue(net_dev);

	return 0;
}


static int example_start_xmit(struct sk_buff * sk_b, struct net_device * src)
{
	printk(KERN_INFO "%s - %s(%pK, %pK)\n",
	       THIS_MODULE->name, __FUNCTION__, sk_b, src);

	dev_kfree_skb(sk_b);
	return NETDEV_TX_OK;
}


struct net_device_ops example_netdev_ops = {
	.ndo_open       = example_open,
	.ndo_stop       = example_stop,
	.ndo_start_xmit = example_start_xmit,
};


static void example_setup (struct net_device * net_dev)
{
	printk(KERN_INFO "%s - %s(%pK)\n",
	       THIS_MODULE->name, __FUNCTION__, net_dev);

	ether_setup(net_dev);

	net_dev->netdev_ops = & example_netdev_ops;
}


static int __init example_init(void)
{

	printk(KERN_INFO "%s: %s()\n", THIS_MODULE->name, __FUNCTION__);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
	net_dev_ex = alloc_netdev(0, "ex%d", example_setup);
#else
	net_dev_ex = alloc_netdev(0, "ex%d", NET_NAME_UNKNOWN, example_setup);
#endif
	if (net_dev_ex == NULL)
		return -ENOMEM;

	if (register_netdev(net_dev_ex) != 0) {
		unregister_netdev(net_dev_ex);
		free_netdev(net_dev_ex);
		return -ENODEV;
	}
	return 0;
}


static void example_exit(void)
{
	printk(KERN_INFO "%s: %s()\n", THIS_MODULE->name, __FUNCTION__);

	if (net_dev_ex != NULL) {
		unregister_netdev(net_dev_ex);
		free_netdev(net_dev_ex);
	}
}


	module_init(example_init)
	module_exit(example_exit)

	MODULE_DESCRIPTION("False device implementation.");
	MODULE_AUTHOR("Christophe Blaess <Christophe.Blaess@Logilin.fr>");
	MODULE_LICENSE("GPL");

