/* $(CROSS_COMPILE)gcc -Wall -O2 -g -lusb-1.0 -o usbtest usbtest.c */
/**
 * usbtest - Enter USB usbtest.c
 *
 * Copyright (C) 2010-2013 Felipe Balbi <balbi@ti.com>
 *
 * This file is part of the USB Verification Tools Project
 *
 * USB Tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public Liicense as published by
 * the Free Software Foundation, either version 3 of the license, or
 * (at your option) any later version.
 *
 * USB Tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with USB Tools. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <libusb-1.0/libusb.h>

static unsigned debug;

#define DBG(fmt, args...)				\
	if (debug)					\
		fprintf(stderr, "%s: " fmt, __func__, ## args)

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#define DEFAULT_TIMEOUT	2000	/* ms */

#define OPTION(n, h, v)			\
{					\
	.name		= #n,		\
	.has_arg	= h,		\
	.val		= v,		\
}

#define TEST_MODE			2
#define PORT_SUSPEND		2

#define TEST_J				0x01
#define TEST_K				0x02
#define TEST_SE0_NAK		0x03
#define TEST_PACKET			0x04
#define TEST_Force_ENABLE	0x05

#define TEST_BAD_DESCRIPTOR		0xff

static struct option testmode_opts[] = {
	OPTION("debug",		0, 'd'),
	OPTION("help",		0, 'h'),
	OPTION("test",		1, 't'),
	OPTION("device",	1, 'D'),
	{  }	/* Terminating entry */
};

static int bad_descriptor_test(libusb_device_handle *udevh)
{
	struct libusb_device_descriptor desc;
	int			ret;
	uint8_t			buf[1024];

	memset(buf, 0x00, sizeof(buf));

	ret = libusb_get_descriptor(udevh, 0xcc, 0, buf, sizeof(buf));
	if (ret >= 0) {
		DBG("this should have failed\n");
		return -EINVAL;
	}

	ret = libusb_get_device_descriptor(libusb_get_device(udevh),
			&desc);
	if (ret < 0) {
		DBG("failed to get descriptor\n");
		return ret;
	}

	ret = libusb_reset_device(udevh);
	if (ret < 0)
		DBG("failed to reset device\n");

	return ret;
}

static int do_test(libusb_device_handle *udevh, int test)
{
	if (test == TEST_BAD_DESCRIPTOR)
		return bad_descriptor_test(udevh);

	return libusb_control_transfer(udevh,
			LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_SET_FEATURE,
			TEST_MODE, (test << 8), NULL, 0, DEFAULT_TIMEOUT);
}

static int port_suspend(libusb_device_handle *udevh, char port)
{
	int ret = libusb_control_transfer(udevh,
			LIBUSB_RECIPIENT_OTHER | LIBUSB_REQUEST_TYPE_CLASS,
			LIBUSB_REQUEST_SET_FEATURE,
			PORT_SUSPEND, port, NULL, 0, DEFAULT_TIMEOUT);
	
	if(ret < 0){
		DBG("port %d suspend failed\n",port);
	}
	else{
		DBG("port %d suspend\n",port);
	}
	return 0;
}

static int start_testmode(libusb_device_handle *udevh,
		char *testmode)
{
	int			test = 0;
	int			ret = -EINVAL;

	printf("Test \"%s\":        ", testmode);

	if (!strncmp(testmode, "test_j", 6))
		test = TEST_J;

	if (!strncmp(testmode, "test_k", 6))
		test = TEST_K;

	if (!strncmp(testmode, "test_se0_nak", 12))
		test = TEST_SE0_NAK;

	if (!strncmp(testmode, "test_packet", 11))
		test = TEST_PACKET;

	if (!strncmp(testmode, "test_force_enable", 17))
		test = TEST_Force_ENABLE;


	ret = do_test(udevh, test);
	if (ret < 0)
		printf("failed\n");
	else
		printf("success\n");

	return ret;
}

static void usage(char *cmd)
{
	fprintf(stdout, "%s <-D VID:PID> <-t testmode> [-d]\n\
		test_packet, test_se0_nak, test_k, test_j,\n\
		test_force_enable\n\
		Example:\n\
			usbtest -D 951:1666 -t test_j -d\n", cmd);
}

int main(int argc, char *argv[])
{
	libusb_context		*context;
	libusb_device_handle	*udevh;

	unsigned		vid = 0;
	unsigned		pid = 0;

	int			ret = 0;

	char			*testmode = NULL;

	while (1) {
		int		optidx;
		int		opt;

		char		*token;

		opt = getopt_long(argc, argv, "t:D:dh", testmode_opts, &optidx);
		if (opt == -1)
			break;

		switch (opt) {
		case 't':
			testmode = optarg;
			break;
		case 'D':
			token = strtok(optarg, ":");
			vid = strtoul(token, NULL, 16);
			token = strtok(NULL, ":");
			pid = strtoul(token, NULL, 16);
			break;
		case 'd':
			debug = 1;
			break;
		case 'h': /* FALLTHROUGH */
		default:
			usage(argv[0]);
			exit(-1);
		}
	}

	libusb_init(&context);

	udevh = libusb_open_device_with_vid_pid(context, vid, pid);
	if (!udevh) {
		DBG("couldn't open %04x:%04x\n", vid, pid);
		ret = -ENODEV;
		goto out0;
	}
	
		ret = start_testmode(udevh, testmode);
		if (ret < 0) {
			DBG("couldn't start test '%s' err %d\n", testmode, ret);
			goto out1;
		}

out1:
	libusb_close(udevh);

out0:
	libusb_exit(context);

	return ret;
}


