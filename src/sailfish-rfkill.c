/*
 *
 *  Connection Manager Sailfish rfkill plugin
 *
 *  Copyright (C) 2016 Jolla Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/*
 * Ensure that rfkill is set to desired state on startup for
 * Bluetooth, even before bluetooth legacy driver figures out BT
 * adapter presence via BlueZ D-Bus API.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#define CONNMAN_API_SUBJECT_TO_CHANGE
#include <connman/plugin.h>
#include <connman/device.h>

#define BLUETOOTH_RFKILL_IDENT "bluetooth_rfkill"

#define BT_DEVICE 0

static struct connman_device *bt_device = NULL;

struct connman_technology;

struct connman_technology_driver {
	const char *name;
	enum connman_service_type type;
	int priority;
	int (*probe) (struct connman_technology *technology);
	void (*remove) (struct connman_technology *technology);
	void (*add_interface) (struct connman_technology *technology,
						int index, const char *name,
							const char *ident);
	void (*remove_interface) (struct connman_technology *technology,
								int index);
	int (*set_tethering) (struct connman_technology *technology,
				const char *identifier, const char *passphrase,
				const char *bridge, bool enabled);
	int (*set_regdom) (struct connman_technology *technology,
						const char *alpha2);
	void (*set_offline) (bool offline);
};

struct rfkill_event {
	uint32_t idx;
	uint8_t  type;
	uint8_t  op;
	uint8_t  soft;
	uint8_t  hard;
};

int connman_technology_driver_register(struct connman_technology_driver *driver);

void connman_technology_driver_unregister(struct connman_technology_driver *driver);

static void DEBUG(const char *format, ...)
{
	openlog("sailfish-rfkill", LOG_PID, LOG_USER);
	va_list v;
	va_start(v, format);
	vsyslog(LOG_INFO, format, v);
	va_end(v);
	closelog();
}

void rfkill_block(bool block)
{
	uint8_t rfkill_type;
	struct rfkill_event event;
	ssize_t len;
	int fd;

	DEBUG("block %d", block);
	rfkill_type = 2; /* RFKILL_TYPE_BLUETOOTH */

	fd = open("/dev/rfkill", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		DEBUG("Failed to change RFKILL state:%s", strerror(errno));
		return;
	}

	memset(&event, 0, sizeof(event));
	event.op = 3;/* RFKILL_OP_CHANGE_ALL */
	event.type = rfkill_type;
	event.soft = block;

	len = write(fd, &event, sizeof(event));
	if (len < 0) {
		DEBUG("Failed to change RFKILL state:%s", strerror(errno));
	}

	close(fd);
}

static int bluetooth_rfkill_device_probe(struct connman_device *device)
{
	struct hci_dev_info dev_info;
	int fd = -1;
	int r = 0;

	DEBUG("device %p", device);

	fd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (fd < 0) {
		DEBUG("Cannot open BT socket: %s(%d)", strerror(errno), errno);
		r = -errno;
		goto out;
	}

	memset(&dev_info, 0, sizeof(dev_info));
	dev_info.dev_id = BT_DEVICE;
	if (ioctl(fd, HCIGETDEVINFO, &dev_info) < 0) {
		DEBUG("Cannot get BT info: %s(%d)", strerror(errno), errno);
		r = -errno;
		goto out;
	}

	if (ioctl(fd, HCIDEVUP, dev_info.dev_id) < 0) {
		DEBUG("Cannot raise BT dev %d: %s(%d)", dev_info.dev_id,
					strerror(errno), errno);
		if (errno != ERFKILL && errno != EALREADY) {
			r = -errno;
			goto out;
		}
	}

	DEBUG("Probe done.");

out:
	if (fd >= 0)
		close(fd);

	return r;
}

static void bluetooth_rfkill_device_remove(struct connman_device *device)
{
	DEBUG("device %p", device);
}

static int bluetooth_rfkill_device_enable(struct connman_device *device)
{
	DEBUG("device %p", device);
	return 0;
}

static int bluetooth_rfkill_device_disable(struct connman_device *device)
{
	DEBUG("device %p", device);
	return 0;
}

static struct connman_device_driver dev_driver = {
	.name = "bluetooth_rfkill",
	.type = CONNMAN_DEVICE_TYPE_BLUETOOTH,
	.probe = bluetooth_rfkill_device_probe,
	.remove = bluetooth_rfkill_device_remove,
	.enable = bluetooth_rfkill_device_enable,
	.disable = bluetooth_rfkill_device_disable
};

static int bluetooth_rfkill_tech_probe(struct connman_technology *technology)
{
	DEBUG("technology %p", technology);
	rfkill_block(true);
	return 0;
}

static void bluetooth_rfkill_tech_remove(struct connman_technology *technology)
{
	DEBUG("technology %p", technology);
}

static struct connman_technology_driver tech_driver = {
	.name = "bluetooth_rfkill",
	.type = CONNMAN_SERVICE_TYPE_BLUETOOTH,
	.probe = bluetooth_rfkill_tech_probe,
	.remove = bluetooth_rfkill_tech_remove,
};

static int sailfish_rfkill_init(void)
{
	int err;

	DEBUG("Initializing dummy device for BT rfkill.");

	err = connman_device_driver_register(&dev_driver);
	if (err < 0)
		return err;

	err = connman_technology_driver_register(&tech_driver);
	if (err < 0) {
		connman_device_driver_unregister(&dev_driver);
		return err;
	}

	/* Force loading of BT settings and applying BT rfkill */
	bt_device = connman_device_create("bluetooth_rfkill",
					CONNMAN_DEVICE_TYPE_BLUETOOTH);
	if (bt_device != NULL) {
		connman_device_set_ident(bt_device, BLUETOOTH_RFKILL_IDENT);
		if (connman_device_register(bt_device) < 0) {
			connman_device_unref(bt_device);
			connman_technology_driver_unregister(&tech_driver);
			connman_device_driver_unregister(&dev_driver);
			return err;
		}
	}

	return 0;
}

static void sailfish_rfkill_exit(void)
{
	DEBUG("");

	if (bt_device != NULL) {
		connman_device_unregister(bt_device);
		connman_device_unref(bt_device);
		bt_device = NULL;
	}

	connman_technology_driver_unregister(&tech_driver);
	connman_device_driver_unregister(&dev_driver);
}

CONNMAN_PLUGIN_DEFINE(sailfish_rfkill, "Sailfish rfkill", CONNMAN_VERSION, CONNMAN_PLUGIN_PRIORITY_DEFAULT,
                      sailfish_rfkill_init, sailfish_rfkill_exit)
