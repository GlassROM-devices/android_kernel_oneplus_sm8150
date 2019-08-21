/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2019 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include "atlfwd_args.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

static void print_usage(const char *binname)
{
	fprintf(stderr,
		"Usage: %s [-d devname] [-v] command <command_args>\n\n",
		binname);
	fprintf(stderr, "%s\n", "Options:");
	fprintf(stderr, "\t%s\n",
		"-d devname\t- specify device name to work with");
	fprintf(stderr, "\t%s\n", "-v\t\t- verbose mode");
	fprintf(stderr, "%s\n", "");
	fprintf(stderr, "%s\n", "Supported commands:");
	fprintf(stderr, "\t%s\n",
		"request_ring <flags> <ring_size> <buf_size> <page_order>");
	fprintf(stderr, "\t%s\n", "release_ring <ring_index>");
	fprintf(stderr, "\t%s\n", "enable_ring <ring_index>");
	fprintf(stderr, "\t%s\n", "disable_ring <ring_index>");
	fprintf(stderr, "%s\n", "");
	fprintf(stderr, "\t%s\n", "force_icmp_tx_via <ring_index>");
	fprintf(stderr, "\t%s\n", "force_tx_via <ring_index>");
	fprintf(stderr, "\t%s\n", "disable_redirections");
}

static enum atlfwd_nl_command get_command(const char *str)
{
	static const char cmd_req_ring[] = "request_ring";
	static const char cmd_rel_ring[] = "release_ring";
	static const char cmd_enable_ring[] = "enable_ring";
	static const char cmd_disable_ring[] = "disable_ring";
	static const char cmd_disable_redirections[] = "disable_redirections";
	static const char cmd_force_icmp_tx_via[] = "force_icmp_tx_via";
	static const char cmd_force_tx_via[] = "force_tx_via";

	if (strncmp(str, cmd_req_ring, ARRAY_SIZE(cmd_req_ring)) == 0)
		return ATL_FWD_CMD_REQUEST_RING;

	if (strncmp(str, cmd_rel_ring, ARRAY_SIZE(cmd_rel_ring)) == 0)
		return ATL_FWD_CMD_RELEASE_RING;

	if (strncmp(str, cmd_enable_ring, ARRAY_SIZE(cmd_enable_ring)) == 0)
		return ATL_FWD_CMD_ENABLE_RING;

	if (strncmp(str, cmd_disable_ring, ARRAY_SIZE(cmd_disable_ring)) == 0)
		return ATL_FWD_CMD_DISABLE_RING;

	if (strncmp(str, cmd_disable_redirections,
		    ARRAY_SIZE(cmd_disable_redirections)) == 0)
		return ATL_FWD_CMD_DISABLE_REDIRECTIONS;

	if (strncmp(str, cmd_force_icmp_tx_via,
		    ARRAY_SIZE(cmd_force_icmp_tx_via)) == 0)
		return ATL_FWD_CMD_FORCE_ICMP_TX_VIA;

	if (strncmp(str, cmd_force_tx_via, ARRAY_SIZE(cmd_force_tx_via)) == 0)
		return ATL_FWD_CMD_FORCE_TX_VIA;

	return ATL_FWD_CMD_UNSPEC;
}

static const char *get_arg(const int argc, char **argv)
{
	if (argc == optind) {
		/* not enough arguments => exit immediately */
		print_usage(argv[0]);
		exit(EINVAL);
	}

	return argv[optind++];
}

static const char *get_last_arg(const int argc, char **argv)
{
	const char *result = get_arg(argc, argv);

	if (optind != argc) {
		/* too many arguments => exit immediately */
		print_usage(argv[0]);
		exit(EINVAL);
	}

	return result;
}

struct atlfwd_args *parse_args(const int argc, char **argv)
{
	static struct atlfwd_args parsed_args;
	int opt;

	while ((opt = getopt(argc, argv, "d:v")) != -1) {
		switch (opt) {
		case 'd':
			parsed_args.devname = optarg;
			break;
		case 'v':
			parsed_args.verbose = true;
			break;
		default:
			print_usage(argv[0]);
			return NULL;
		}
	}

	parsed_args.cmd = get_command(get_arg(argc, argv));

	switch (parsed_args.cmd) {
	case ATL_FWD_CMD_REQUEST_RING:
		parsed_args.flags = (uint32_t)atoi(get_arg(argc, argv));
		parsed_args.ring_size = (uint32_t)atoi(get_arg(argc, argv));
		parsed_args.buf_size = (uint32_t)atoi(get_arg(argc, argv));
		parsed_args.page_order =
			(uint32_t)atoi(get_last_arg(argc, argv));
		break;
	case ATL_FWD_CMD_RELEASE_RING:
		/* fall through */
	case ATL_FWD_CMD_ENABLE_RING:
		/* fall through */
	case ATL_FWD_CMD_DISABLE_RING:
		/* fall through */
	case ATL_FWD_CMD_FORCE_ICMP_TX_VIA:
		/* fall through */
	case ATL_FWD_CMD_FORCE_TX_VIA:
		parsed_args.ring_index =
			(uint32_t)atoi(get_last_arg(argc, argv));
		break;
	case ATL_FWD_CMD_DISABLE_REDIRECTIONS:
		break;
	default:
		print_usage(argv[0]);
		return NULL;
	}

	return &parsed_args;
}
