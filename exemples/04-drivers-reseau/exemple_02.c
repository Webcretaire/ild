/************************************************************************\
  Chapitre "Peripheriques et protocoles reseau"
  exemple_02

  Peripheriques virtuels lies

  Exemples de la formation "Programmation Noyau sous Linux"

  (c) 2005-2014 Christophe Blaess
  http://www.blaess.fr/christophe/

  Librement inspire d'un exemple du livre "Linux Device Driver"
  d'Alessandro Rubini et Jonathan Corbet.

\************************************************************************/

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/version.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/skbuff.h>

	struct net_device * net_dev_ex_0 = NULL;
	struct net_device * net_dev_ex_1 = NULL;

	struct exemple_net_dev_priv {

		/* Paquet a emettre */
		struct sk_buff * sk_b;

		/* Donnees recues */
		unsigned char data[ETH_DATA_LEN];
		int data_len;
	};


static irqreturn_t exemple_irq_tx_handler(int irq, void * irq_id, struct pt_regs * regs);
static irqreturn_t exemple_irq_rx_handler(int irq, void * irq_id, struct pt_regs * regs);


static int exemple_open (struct net_device * net_dev)
{
	printk(KERN_INFO "%s: exemple_open(%p)\n",
	       THIS_MODULE->name, net_dev);

	/*
	    Installer le(s) gestionnaire(s) d'interruption avec :

	      request_irq(IRQ_RX_NUM, exemple_irq_rx_handler,
		              IRQF_SHARED, THIS_MODULE->name, net_dev);

	      request_irq(IRQ_TX_NUM, exemple_irq_tx_handler,
		              IRQF_SHARED, THIS_MODULES->name, net_dev);

	    Ici ils seront simplement simules par un appel direct.
	*/

	net_dev->dev_addr[0] = 0x00;
	net_dev->dev_addr[1] = 0x12;
	net_dev->dev_addr[2] = 0x34;
	net_dev->dev_addr[3] = 0x56;
	net_dev->dev_addr[4] = 0x78;
	if (net_dev == net_dev_ex_0)
		net_dev->dev_addr[5] = 0x00;
	else
		net_dev->dev_addr[5] = 0x01;
		
	netif_start_queue(net_dev);
	return 0;
}


static int exemple_stop (struct net_device * net_dev)
{
	printk(KERN_INFO "%s: stop_exemple(%p)\n",
	       THIS_MODULE->name, net_dev);

	netif_stop_queue(net_dev);
	
	/*
	    Liberer les gestionnaires d'interruption avec :
	     free_irq(IRQ_RX_NUM, net_dev);
	     free_irq(IRQ_TX_NUM, net_dev);
	*/
	return 0;
}


static int exemple_start_xmit(struct sk_buff * sk_b, struct net_device * src)
{
	struct exemple_net_dev_priv * dst_priv;
	struct exemple_net_dev_priv * src_priv;
	
	struct net_device * dst;
	struct iphdr * ip_header;
	
	unsigned char * ptr_src;
	unsigned char * ptr_dst;

	char * data;
	int    len;
	char   short_packet[ETH_ZLEN];

	printk(KERN_INFO "%s: exemple_start_xmit(%p, %p)\n",
	       THIS_MODULE->name, sk_b, src);

	if (src == net_dev_ex_0)
		dst = net_dev_ex_1;
	else
		dst = net_dev_ex_0;

	src_priv = netdev_priv(src);
	dst_priv = netdev_priv(dst);

	data = sk_b->data;
	len  = sk_b->len;
	
	/* Longueur minimale d'un paquet ethernet: ETH_ZLEN (60 octets) */
	if (len < ETH_ZLEN) {
		memset(short_packet, 0, ETH_ZLEN);
		memcpy(short_packet, data, len);
		len = ETH_ZLEN;
		data = short_packet;
	}

	if (len > ETH_DATA_LEN)
		return -ENOMEM;

	src->trans_start = jiffies; /* Pour le timeout */
	
	/* Le paquet source est memorise pour le liberer apres emission */
	src_priv->sk_b = sk_b;

	/* Modification du 3eme octets des adresses IP source et destination */
	ip_header = (struct iphdr *) (data + sizeof(struct ethhdr));

	ptr_src = (unsigned char *) &(ip_header -> saddr);
	ptr_dst = (unsigned char *) &(ip_header -> daddr);
		
	ptr_src += 2;
	* ptr_src = 255 - *ptr_src;
	ptr_dst += 2;
	* ptr_dst = 255 - *ptr_dst;
	
	ip_header->check = 0;
	ip_header->check = ip_fast_csum((unsigned char *)ip_header, ip_header->ihl);


	/* Copie des donnees dans le structure privee de destination */
	memcpy(dst_priv->data, data, len);
	dst_priv->data_len = len;

	/* Puis declenchement de la pseudo-interruption RX */
	exemple_irq_rx_handler (0, (void *) dst, NULL);

	/* Déclenchement de la pseudo-interruption "fin de TX" */
	exemple_irq_tx_handler (0, (void *) src, NULL);
	
	return NETDEV_TX_OK;
}


static irqreturn_t exemple_irq_rx_handler(int irq, void * irq_id, struct pt_regs * regs)
{
	unsigned char * data;
	struct sk_buff * sk_b;
	struct net_device * net_dev;
	struct exemple_net_dev_priv * priv;

	printk(KERN_INFO "%s: exemple_irq_rx_handler(%d, %p, %p)\n",
	       THIS_MODULE->name, irq, irq_id, regs);

	
	net_dev = (struct net_device *) irq_id;
	priv = netdev_priv(net_dev);
	if (priv == NULL)
		return IRQ_NONE;
		
	sk_b = dev_alloc_skb(priv->data_len);
	if (! sk_b)
		return IRQ_HANDLED;

	data = skb_put(sk_b, priv->data_len);
	memcpy(data, priv->data, priv->data_len);
	sk_b->dev = net_dev;
	sk_b->protocol = eth_type_trans(sk_b, net_dev);
	sk_b->ip_summed = CHECKSUM_UNNECESSARY;
	netif_rx(sk_b);
	
	return IRQ_HANDLED;
}


static irqreturn_t exemple_irq_tx_handler(int irq, void * irq_id, struct pt_regs * regs)
{
	struct net_device * net_dev;
	struct exemple_net_dev_priv * priv;

	printk(KERN_INFO "%s: exemple_irq_tx_handler(%d, %p, %p)\n",
	       THIS_MODULE->name, irq, irq_id, regs);

	net_dev = (struct net_device *) irq_id;
	priv = netdev_priv(net_dev);
	if (priv == NULL)
		return IRQ_NONE;
		
	dev_kfree_skb(priv->sk_b);
	return IRQ_HANDLED;
}

static int exemple_hard_header(struct sk_buff * sk_b, struct net_device * net_dev,
                        unsigned short type, const void * dst_addr, const void * src_addr,
                        unsigned int len)
{
	struct ethhdr * eth_hdr = NULL;

	printk(KERN_INFO "%s: hard_header(%p, %p, %d, %p, %p, %d)\n",
	       THIS_MODULE->name, sk_b, net_dev, type, dst_addr, src_addr, len);
	
	eth_hdr = (struct ethhdr *) skb_push(sk_b, ETH_HLEN);
	eth_hdr->h_proto = htons(type);
	
	if (src_addr == NULL)
		src_addr = net_dev->dev_addr;
	if (dst_addr == NULL)
		dst_addr = net_dev->dev_addr;
		
	memcpy(eth_hdr->h_source, src_addr, net_dev->addr_len);
	memcpy(eth_hdr->h_dest,   dst_addr, net_dev->addr_len);

	if (eth_hdr->h_dest[ETH_ALEN-1] == 0)
		eth_hdr->h_dest[ETH_ALEN-1] = 1;
	else
		eth_hdr->h_dest[ETH_ALEN-1] = 0;
		
	return net_dev->hard_header_len;
}

static const struct header_ops header_ops_exemple = {
        .create = exemple_hard_header,
};

struct net_device_ops exemple_netdev_ops = {
	.ndo_open       = exemple_open,
	.ndo_stop       = exemple_stop,
	.ndo_start_xmit = exemple_start_xmit,
};


static void exemple_setup (struct net_device * net_dev)
{
	struct exemple_net_dev_priv * private = NULL;

	printk(KERN_INFO "%s: exemple_setup(%p)\n",
	       THIS_MODULE->name, net_dev);

	/* Objet de base : ethernet */
	ether_setup(net_dev);

	/* Surcharge de certaines methodes */
	net_dev->netdev_ops = & exemple_netdev_ops;
	net_dev->header_ops = & header_ops_exemple;

	/* Complements des champs de base */
	net_dev->flags    |= IFF_NOARP;
	
	/* Initialisation des champs prives */
	private = netdev_priv(net_dev);
	memset(private, 0, sizeof(struct exemple_net_dev_priv));
}


static void exemple_exit(void);

static int __init exemple_init(void)
{
	
	printk(KERN_INFO "%s: exemple_init\n", THIS_MODULE->name);

	net_dev_ex_0 = alloc_netdev(sizeof(struct exemple_net_dev_priv),
	                           "ex%d", exemple_setup);
	if (net_dev_ex_0 == NULL)
		return -ENOMEM;

	if (register_netdev(net_dev_ex_0) != 0) {
		exemple_exit();
		return -ENODEV;
	}

	net_dev_ex_1 = alloc_netdev(sizeof(struct exemple_net_dev_priv),
	              "ex%d", exemple_setup);
	if (net_dev_ex_1 == NULL) {
		exemple_exit();
		return -ENOMEM;
	}

	if (register_netdev(net_dev_ex_1) != 0) {
		exemple_exit();
		return -ENODEV;
	}
	return 0;
}


static void exemple_exit(void)
{
	printk(KERN_INFO "%s: exemple_exit\n", THIS_MODULE->name);

	if (net_dev_ex_1 != NULL) {
		unregister_netdev(net_dev_ex_1);		
		free_netdev(net_dev_ex_1);
		net_dev_ex_1 = NULL;
	}

	if (net_dev_ex_0 != NULL) {
		unregister_netdev(net_dev_ex_0);
		free_netdev(net_dev_ex_0);
		net_dev_ex_0 = NULL;
	}
}

module_init(exemple_init)
module_exit(exemple_exit)
MODULE_LICENSE("GPL");

