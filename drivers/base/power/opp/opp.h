/*
 * Generic OPP Interface
 *
 * Copyright (C) 2009-2010 Texas Instruments Incorporated.
 *	Nishanth Menon
 *	Romit Dasgupta
 *	Kevin Hilman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DRIVER_OPP_H__
#define __DRIVER_OPP_H__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/limits.h>
#include <linux/pm_opp.h>
#include <linux/rculist.h>
#include <linux/rcupdate.h>

/* Lock to allow exclusive modification to the device and opp lists */
extern struct mutex dev_opp_list_lock;

/*
 * Internal data structure organization with the OPP layer library is as
 * follows:
 * dev_opp_list (root)
 *	|- device 1 (represents voltage domain 1)
 *	|	|- opp 1 (availability, freq, voltage)
 *	|	|- opp 2 ..
 *	...	...
 *	|	`- opp n ..
 *	|- device 2 (represents the next voltage domain)
 *	...
 *	`- device m (represents mth voltage domain)
 * device 1, 2.. are represented by dev_opp structure while each opp
 * is represented by the opp structure.
 */

/**
 * struct dev_pm_opp - Generic OPP description structure
 * @node:	opp list node. The nodes are maintained throughout the lifetime
 *		of boot. It is expected only an optimal set of OPPs are
 *		added to the library by the SoC framework.
 *		RCU usage: opp list is traversed with RCU locks. node
 *		modification is possible realtime, hence the modifications
 *		are protected by the dev_opp_list_lock for integrity.
 *		IMPORTANT: the opp nodes should be maintained in increasing
 *		order.
 * @available:	true/false - marks if this OPP as available or not
 * @dynamic:	not-created from static DT entries.
 * @turbo:	true if turbo (boost) OPP
 * @suspend:	true if suspend OPP
 * @rate:	Frequency in hertz
 * @u_volt:	Target voltage in microvolts corresponding to this OPP
 * @u_volt_min:	Minimum voltage in microvolts corresponding to this OPP
 * @u_volt_max:	Maximum voltage in microvolts corresponding to this OPP
 * @u_amp:	Maximum current drawn by the device in microamperes
 * @clock_latency_ns: Latency (in nanoseconds) of switching to this OPP's
 *		frequency from any other OPP's frequency.
 * @dev_opp:	points back to the device_opp struct this opp belongs to
 * @rcu_head:	RCU callback head used for deferred freeing
 * @np:		OPP's device node.
 * @dentry:	debugfs dentry pointer (per opp)
 *
 * This structure stores the OPP information for a given device.
 */
struct dev_pm_opp {
	struct list_head node;

	bool available;
	bool dynamic;
	bool turbo;
	bool suspend;
	unsigned long rate;

	unsigned long u_volt;
	unsigned long u_volt_min;
	unsigned long u_volt_max;
	unsigned long u_amp;
	unsigned long clock_latency_ns;

	struct device_opp *dev_opp;
	struct rcu_head rcu_head;

	struct device_node *np;

#ifdef CONFIG_DEBUG_FS
	struct dentry *dentry;
#endif
};

/**
 * struct device_list_opp - devices managed by 'struct device_opp'
 * @node:	list node
 * @dev:	device to which the struct object belongs
 * @rcu_head:	RCU callback head used for deferred freeing
 * @dentry:	debugfs dentry pointer (per device)
 *
 * This is an internal data structure maintaining the list of devices that are
 * managed by 'struct device_opp'.
 */
struct device_list_opp {
	struct list_head node;
	const struct device *dev;
	struct rcu_head rcu_head;

#ifdef CONFIG_DEBUG_FS
	struct dentry *dentry;
#endif
};

/**
 * struct device_opp - Device opp structure
 * @node:	list node - contains the devices with OPPs that
 *		have been registered. Nodes once added are not modified in this
 *		list.
 *		RCU usage: nodes are not modified in the list of device_opp,
 *		however addition is possible and is secured by dev_opp_list_lock
 * @srcu_head:	notifier head to notify the OPP availability changes.
 * @rcu_head:	RCU callback head used for deferred freeing
 * @dev_list:	list of devices that share these OPPs
 * @opp_list:	list of opps
 * @np:		struct device_node pointer for opp's DT node.
 * @clock_latency_ns_max: Max clock latency in nanoseconds.
 * @shared_opp: OPP is shared between multiple devices.
 * @suspend_opp: Pointer to OPP to be used during device suspend.
 * @supported_hw: Array of version number to support.
 * @supported_hw_count: Number of elements in supported_hw array.
 * @prop_name: A name to postfix to many DT properties, while parsing them.
 * @dentry:	debugfs dentry pointer of the real device directory (not links).
 * @dentry_name: Name of the real dentry.
 *
 * @voltage_tolerance_v1: In percentage, for v1 bindings only.
 *
 * This is an internal data structure maintaining the link to opps attached to
 * a device. This structure is not meant to be shared to users as it is
 * meant for book keeping and private to OPP library.
 *
 * Because the opp structures can be used from both rcu and srcu readers, we
 * need to wait for the grace period of both of them before freeing any
 * resources. And so we have used kfree_rcu() from within call_srcu() handlers.
 */
struct device_opp {
	struct list_head node;

	struct srcu_notifier_head srcu_head;
	struct rcu_head rcu_head;
	struct list_head dev_list;
	struct list_head opp_list;

	struct device_node *np;
	unsigned long clock_latency_ns_max;

	/* For backward compatibility with v1 bindings */
	unsigned int voltage_tolerance_v1;

	bool shared_opp;
	struct dev_pm_opp *suspend_opp;

	unsigned int *supported_hw;
	unsigned int supported_hw_count;
	const char *prop_name;

#ifdef CONFIG_DEBUG_FS
	struct dentry *dentry;
	char dentry_name[NAME_MAX];
#endif
};

/* Routines internal to opp core */
struct device_opp *_find_device_opp(struct device *dev);
struct device_list_opp *_add_list_dev(const struct device *dev,
				      struct device_opp *dev_opp);
struct device_node *_of_get_opp_desc_node(struct device *dev);

#ifdef CONFIG_DEBUG_FS
void opp_debug_remove_one(struct dev_pm_opp *opp);
int opp_debug_create_one(struct dev_pm_opp *opp, struct device_opp *dev_opp);
int opp_debug_register(struct device_list_opp *list_dev,
		       struct device_opp *dev_opp);
void opp_debug_unregister(struct device_list_opp *list_dev,
			  struct device_opp *dev_opp);
#else
static inline void opp_debug_remove_one(struct dev_pm_opp *opp) {}

static inline int opp_debug_create_one(struct dev_pm_opp *opp,
				       struct device_opp *dev_opp)
{ return 0; }
static inline int opp_debug_register(struct device_list_opp *list_dev,
				     struct device_opp *dev_opp)
{ return 0; }

static inline void opp_debug_unregister(struct device_list_opp *list_dev,
					struct device_opp *dev_opp)
{ }
#endif		/* DEBUG_FS */

#endif		/* __DRIVER_OPP_H__ */
