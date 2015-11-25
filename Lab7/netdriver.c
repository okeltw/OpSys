#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/icmp.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>

// ARP stuff, in case we go chasin' brownies.
#include <linux/if_arp.h>
#include <net/arp.h>

//**********************
//From Lab7 Assistance
struct net_device *os0, *os1;

struct os_packet {
	struct net_device *dev;
	int datalen;
	u8 data[ETH_DATA_LEN];
};

struct os_priv {
	struct net_device_stats stats;
	struct sk_buff *skb;
	struct os_packet *pkt;
	struct net_device *dev;
};

//Instantiate net_device_ops (STUBS)
int os_open(struct net_device *dev) { return 0; }
int os_stop(struct net_device *dev) { return 0; }
int os_start_xmit(struct sk_buff *skb, struct net_device *dev) { return 0; }
struct net_device_stats *os_stats(struct net_device *dev) {
	return &(((struct os_priv*)netdev_priv(dev))->stats);
}

//Map stubs
static const struct net_device_ops os_device_ops = {
	.ndo_open = os_open,
	.ndo_stop = os_stop,
	.ndo_start_xmit = os_start_xmit,
	.ndo_get_stats = os_stats,
};
//End Lab7 Assistance
//********************

//From 'Organization'
int your_open(struct net_device* dev) {
	/* start the network queue */
	netif_start_queue(dev);
	return 0;
}

int your_stop(struct net_device* dev) {
	/* stop the network queue */
	netif_stop_queue(dev);
	return 0;
}

static void your_tx_i_handler(struct net_device* dev) {
	/* normally stats are kept - you do not need to do much
		here if you do not want to except resume the queue
		if it is not accepting packets.
	*/

	struct os_priv* priv = netdev_priv(dev);
	if(netif_queue_stopped(priv->pkt->dev)) netif_wake_queue(priv->pkt->dev); //If stopped, resume.
}

static void your_rx_i_handler(struct net_device* dev) {
	/* allocate space for a socket buffer
		add two bytes of space to align on 16 byte boundary  (???)
		copy the packet from the private part of dev to the socket buffer
		set the protocol field of the socket buffer using 'eth_type_trans'
		set the dev field of the socket buffer to dev (argument)
		invoke 'netif_rx()' on the socket buffer
		resume the network queue if it is not accepting packets
	*/

	struct os_priv* priv = netdev_priv(dev);
	int len = priv->pkt->length;

	struct sk_buff *buf = dev_allocate_skb(len);
	memcpy(skb_put(buf, len), priv->pkt->data, len);

	buf->dev = dev;
	buf->protocol = eth_type_trans(buf, dev);

	netif_rx(buf);

	if (netif_queue_stopped(priv->pkt->dev)) netif_wake_queue(priv->pkt->dev); //If stopped, resume.
}

netdev_tx_t your_start_xmit(struct sk_buff* skb, struct net_device* dev) {
	/* pull the packet and its length from skb */
	char* data = skb->data;
	int len = skb->len;

	/* locate the IP header in the packet (after the eth header below)*/
	struct iphdr* ih = (struct iphdr*)(data+sizeof(struct ethhdr));
	struct icmphdr *icmp_header;
	struct arphdr *arp_header; // For ARP portion
	u32 *saddr = &ih->saddr;
	u32 *daddr = &ih->daddr;

	// Tap into private data
	struct os_priv* priv_os0 = netdev_priv(os0);
	struct os_priv* priv_os1 = netdev_priv(os1);

	/*	switch the third octet of the source and destination addresses */
	((u8*)saddr)[2] ^= 1;
	((u8*)daddr)[2] ^= 1;

	/*
	// Modify Checksum
	ih->check = 0;
	ih->check = ip_fast_csum((unsigned char*)ih, ih->ih1);
	*/

	/*	save the modified packet (even add some data if you like)
		in the private space reserved for it */
	memcpy(data, (char*)"Hello from xmit!\0");
	if((int)(((u8*)daddr)[2]) == 0) {// if destination os0
		priv_os1->pkt->length = len;
		memcpy(priv_os0->pkt->data, data, len);

		/*	simulate a receive interrupt by calling 'your_rx_i_handler'
			simulate a transmit interrupt by calling 'your_tx_i_handler' */
		your_rx_i_handler(os0);
		your_tx_i_handler(os1);
	}
	else { // Else destination os1
		priv_os0->pkt->length = len;
		memcpy(priv_os1->pkt->data, data, len);

		/*	simulate a receive interrupt by calling 'your_rx_i_handler'
			simulate a transmit interrupt by calling 'your_tx_i_handler' */
		your_rx_i_handler(os1);
		your_tx_i_handler(os0);
	}

	/*	free skb */
	dev_kfree_skb(skb);

	if (netif_queue_stopped(priv_os0->pkt->dev)) netif_wake_queue(priv_os0->pkt->dev);  // If stopped, resume
	if (netif_queue_stopped(priv_os1->pkt->dev)) netif_wake_queue(priv_os1->pkt->dev);

	return 0;
}

int your_header(skb, struct net_device* dev, type, daddr, saddr, len) {
	/* make room for the header in the socket buffer (from argument 'skb')
		set the protocol field (from argument 'type')
		copy the address given by the device to both source and
		destination fields (from argument 'dev')
		reverse the LSB on the destination address
	*/
}


int init_module(void){
	struct os_priv* priv;
	int i;
	printk(KERN_INFO "[netdriver] Module Loading.\n");

	//Allocate os0, os1
	os0 = alloc_etherdev(sizeof(struct os_priv));
	os1 = alloc_etherdev(sizeof(struct os_priv));

	//Generate some MAC Addresses - simulated
	//Franco's code has these separate; I see no reason, so let's test it out.
	for(i=0;i<6;i++){
		os0->dev_addr[i] = (unsigned char)i;
		os0->broadcast[i] = (unsigned char)15; // FF:FF:FF . . .

		os1->dev_addr[i] = (unsigned char)i;
		os1->broadcast[i] = (unsigned char)15;
	}
	os1->dev_addr[5]++;
	os0->header_len = 14;
	os1->header_len = 14;

	//Name the kiddo's
	memcpy(os0->name, "os0\0", 4); //Be sure to terminate the name - "\0".
	memcpy(os1->name, "os1\0", 4);

	//File Ops:
	os0->netdev_ops = &os_device_ops;
	os0->header_ops = &os_header_ops;
	os1->netdev_ops = &os_device_ops;
	os1->header_ops = &os_header_ops;

	//Disable ARP
	//...if you're scared
	os0->flags |= IFF_NOARP
   	os1->flags |= IFF_NOARP
	//!! Brownie Opportunity: Use ARP !!

	//Initialize private area - "It already has space due to alloc_etherdev. It probably should be zeroed out like this: " src - Lab7 Assistance
	priv = netdev_priv(os0);
	memset(priv, 0, sizeof(struct os_priv));
	priv->pkt = kmalloc (sizeof (struct os_packet), GFP_KERNEL);
   	priv->pkt->dev = os0;

	priv = netdev_priv(os1);
	memset(priv, 0, sizeof(struct os_priv));
	priv->pkt = kmalloc (sizeof (struct os_packet), GFP_KERNEL);
   	priv->pkt->dev = os1;

	//Register os0, os1
	if(register_netdev(os0)) printk(KERN_INFO "[netdriver] os0 not registered!\n");
	else printk(KERN_INFO "[netdriver] os0 registered.\n")

	if(register_netdev(os0)) printk(KERN_INFO "[netdriver] os1 not registered!\n");
	else printk(KERN_INFO "[netdriver] os1 registered.\n")

	printk(KERN_INFO "[netdriver] Module Loaded.\n");
	return 0;
}

void cleanup_module(void){
	printk(KERN_INFO "[netdriver] Removing Module...\n");

	//Free & Unregister os0/os1, if applicable.
	struct os_priv *priv;
	if (os0)
	{
		priv = netdev_priv(os0);
		kfree(priv->pkt);
		unregister_netdev(os0);
	}
	if (os1)
	{
		priv = netdev_priv(os1);
		kfree(priv->pkt);
		unregister_netdev(os1);
	}

	printk(KERN_INFO "[netdriver] Module successfully removed.\n\n");
}
