

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
int your_open(dev) {  /* start the network queue */  }

int your_stop(dev) {  /* stop the network queue */  }

static void your_tx_i_handler(struct net_device* dev) {
	/* normally stats are kept - you do not need to do much
		here if you do not want to except resume the queue 
		if it is not accepting packets.
	*/
	
	struct os_priv* priv = netdev_priv(dev);
	if(netif_queue_stopped(priv->pkt->dev)) netif_wake_queue(priv->pkt->dev); //If stopped, resume.
}

static void your_rx_i_handler(dev) {
	/* allocate space for a socket buffer
		add two bytes of space to align on 16 byte boundary
		copy the packet from the private part of dev to the socket buffer
		set the protocol field of the socket buffer using 'eth_type_trans'
		set the dev field of the socket buffer to dev (argument)
		invoke 'netif_rx()' on the socket buffer
		resume the network queue if it is not accepting packets
	*/
}

netdev_tx_t your_start_xmit(skb, dev) {
	/* pull the packet and its length from skb
		locate the IP header in the packet (after the eth header below)
		switch the third octet of the source and destination addresses
		save the modified packet (even add some data if you like)
		in the private space reserved for it
		simulate a receive interrupt by calling 'your_rx_i_handler'
		simulate a transmit interrupt by calling 'your_tx_i_handler'
		free skb
	*/
}

int your_header(skb, dev, type, daddr, saddr, len) {
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