/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2018 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#ifndef _ATL_FWD_H_
#define _ATL_FWD_H_

struct atl_fwd_event;

/**
 *	atl_fwd_rxbufs - offload engine's ring's Rx buffers
 *
 *	Buffers are allocated by the driver when a ring is created
 *
 *	The entire buffer space for the ring may optionally be
 *	allocated as a single physically-contiguous block.
 *
 *	As Rx descriptors are overwritten with the write-back
 *	descriptors, a vector of DMA-addresses of buffers is provided
 *	in @daddr_vec to simplify Rx descriptor refill by the offload
 *	engine.
 *
 *	@daddr_vec_base:	DMA address of the base of the @daddr_vec
 *    	@daddr_vec:		Pointer to a vector of buffers' DMA
 *    				addresses
 */
struct atl_fwd_rxbufs {
	dma_addr_t daddr_vec_base;
	dma_addr_t *daddr_vec;
	/* ... */
};

union atl_desc;

/**
 * 	atl_hw_ring - low leverl descriptor ring structure
 *
 * 	@descs:		Pointer to the descriptor ring
 * 	@size:		Number of descriptors in the ring
 * 	@reg_base:	Offset of ring's register block from start of
 * 			BAR 0
 * 	@daddr:		DMA address of the ring
 */
struct atl_hw_ring {
	union atl_desc *descs;
	uint32_t size;
	uint32_t reg_base;
	dma_addr_t daddr;
};

/**
 *	atl_fwd_ring - Offload engine-controlled ring
 *
 *	Buffer space is allocated by the driver on ring creation.
 *
 *	@hw:    	Low-level ring information
 *	@evt:		Ring's event, either an MSI-X vector (either
 *			Tx or Rx) or head pointer writeback address
 *			(Tx ring only). NULL on ring allocation, set
 *			by atl_fwd_request_event()
 *	@rxbufs:	Ring's Rx buffers
 *	@nic:		struct atl_nic backreference
 */
struct atl_fwd_ring {
	struct atl_hw_ring hw;
	struct atl_fwd_event *evt;
	struct atl_fwd_rxbufs *rxbufs;
	struct atl_nic *nic;
	/* ... */
};

enum atl_fwd_event_flags {
	ATL_FWD_EVT_TYPE = BIT(0), /* Event type: 0 for MSI, 1 for Tx
				    * head WB */
	ATL_FWD_EVT_AUTOMASK = BIT(1), /* Disable event after
					* raising, MSI only. */
};

/**
 * 	atl_fwd_event - Ring's notification event
 *
 * 	@flags		Event type and flags
 * 	@ring		Ring backreference
 * 	@msi_addr	MSI message address
 * 	@msi_data	MSI message data
 * 	@idx		MSI index (0 .. 31)
 * 	@tx_head_wrb	Tx head writeback location
 */
struct atl_fwd_event {
	enum atl_fwd_event_flags flags;
	struct atl_fwd_ring *ring;
	union {
		struct {
			dma_addr_t msi_addr;
			uint32_t msi_data;
			int idx;
		};
		dma_addr_t tx_head_wrb;
	};
	/* ... */
};

enum atl_fwd_ring_flags {
	ATL_FWR_TX = BIT(0),	/* Direction: 0 for Rx, 1 for Tx */
	ATL_FWR_VLAN = BIT(1),	/* Enable VLAN tag stripping / insertion */
	ATL_FWR_LXO = BIT(2),	/* Enable LRO / LSO */
	ATL_FWR_CONTIG_BUFS = BIT(3), /* Alloc Rx buffers as physically contiguous */
};

/**
 * atl_fwd_request_channel() - Create a ring for an offload engine
 *
 * 	@ndev:		network device
 * 	@size:		number of descriptors
 * 	@flags:		ring flags
 *
 * atl_fwd_request_channel() creates a ring for an offload engine,
 * allocates Rx buffer memory in case of Rx ring and initializes
 * ring's registers. Ring is inactive until explicitly enabled via
 * atl_fwd_enable_channel().
 *
 * Returns the ring pointer on success, ERR_PTR(error code) on failure
 */
struct atl_fwd_ring *atl_fwd_request_chan(struct net_device *ndev, int size,
	enum atl_fwd_ring_flags flags);

/**
 * atl_fwd_release_channel() - Free offload engine's ring
 *
 * 	@ring:	ring to be freed
 *
 * Stops the ring, frees Rx buffers in case of Rx ring, disables and
 * releases ring's event if non-NULL, and frees the ring.
 */
void atl_fwd_release_channel(struct atl_fwd_ring *ring);

/**
 * atl_fwd_enable_channel() - Enable offload engine's ring
 *
 * 	@ring: ring to be enabled
 *
 * Starts the ring. Returns 0 on success or negative error code.
 */
int atl_fwd_enable_channel(struct atl_fwd_ring *ring);
/**
 * atl_fwd_disable_channel() - Disable offload engine's ring
 *
 * 	@ring: ring to be disabled
 *
 * Stops and resets the ring. On next ring enable head and tail
 * pointers will be zero.
 */
void atl_fwd_disable_channel(struct atl_fwd_ring *ring);

/**
 * atl_fwd_request_event() - Creates and attaches a ring notification
 * event
 *
 * 	@evt:		event structure
 *
 * Caller must allocate a struct atl_fwd_event and fill the @flags,
 * @ring and either @tx_head_wrb or @msi_addr and @msi_data depending
 * on the type bit in @flags. Event is created in disabled state.
 *
 * For an MSI event type, an MSI vector table slot is
 * allocated and programmed, and it's index is saved in @evt->idx.
 *
 * @evt is then attached to the ring.
 *
 * Returns 0 on success or negative error code.
 */
int atl_fwd_request_event(struct atl_fwd_event *evt);

/**
 * atl_fwd_release_event() - Release a ring notification event
 *
 * 	@evt:		event structure
 *
 * Disables the event if enabled, frees the MSI vector for an MSI-type
 * event and detaches @evt from the ring. The @evt itself is not
 * freed.
 */
void atl_fwd_release_event(struct atl_fwd_event *evt);

/**
 * atl_fwd_enable_event() - Enable a ring event
 *
 * 	@evt:		event structure
 *
 * Enables the event.
 */
int atl_fwd_enable_event(struct atl_fwd_event *evt);

/**
 * atl_fwd_disable_event() - Disable a ring event
 *
 * 	@evt:		event structure
 *
 * Disables the event.
 */
void atl_fwd_disable_event(struct atl_fwd_event *evt);

int atl_fwd_receive_skb(struct net_device *ndev, struct sk_buff *skb);
int atl_fwd_transmit_skb(struct net_device *ndev, struct sk_buff *skb);

#endif
