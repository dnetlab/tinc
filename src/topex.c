/*
    top.c -- Show real-time statistics from a running tincd
    Copyright (C) 2011-2013 Guus Sliepen <guus@tinc-vpn.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "system.h"

#ifdef HAVE_CURSES

#undef KEY_EVENT  /* There are conflicting declarations for KEY_EVENT in Windows wincon.h and curses.h. */
#include <curses.h>
#include <dirent.h>

#include "control_common.h"
#include "list.h"
#include "names.h"
#include "tincctl.h"
#include "top.h"
#include "xalloc.h"
#include "cJSON.h"

typedef struct nodestats_t {
	char *name;
	int i;
	uint64_t in_packets;
	uint64_t in_bytes;
	uint64_t out_packets;
	uint64_t out_bytes;
	uint64_t udp_confirmed;
	uint64_t in_packets_rate;
	uint64_t in_bytes_rate;
	uint64_t out_packets_rate;
	uint64_t out_bytes_rate;
	bool known;
} nodestats_t;

static const char *const sortname[] = {
	"name",
	"in pkts",
	"in bytes",
	"out pkts",
	"out bytes",
	"tot pkts",
	"tot bytes"
};

static int sortmode = 0;
static bool cumulative = false;

static list_t node_list;
static struct timeval cur, prev, diff;
static int delay = 5000;
static bool changed = true;
static const char *bunit = "bytes";
static float bscale = 1;
static const char *punit = "pkts";
static float pscale = 1;

static bool update(int fd) {
	if(!sendline(fd, "%d %d", CONTROL, REQ_DUMP_TRAFFIC))
		return false;

	gettimeofday(&cur, NULL);

	timersub(&cur, &prev, &diff);
	prev = cur;
	float interval = diff.tv_sec + diff.tv_usec * 1e-6;

	char line[4096];
	char name[4096];
	int code;
	int req;
	uint64_t in_packets;
	uint64_t in_bytes;
	uint64_t out_packets;
	uint64_t out_bytes;
	uint64_t udp_confirmed;
	//FILE *pf = fopen("/tmp/tinctop.txt", "rw");

	for list_each(nodestats_t, ns, &node_list)
		ns->known = false;

	while(recvline(fd, line, sizeof line)) {
		int n = sscanf(line, "%d %d %s %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64, &code, &req, name, &in_packets, &in_bytes, &out_packets, &out_bytes, &udp_confirmed);

		if(n == 2)
			return true;

		if(n != 8)
			return false;

		nodestats_t *found = NULL;

		for list_each(nodestats_t, ns, &node_list) {
			int result = strcmp(name, ns->name);
			if(result > 0) {
				continue;
			} if(result == 0) {
				found = ns;
				break;
			} else {
				found = xzalloc(sizeof *found);
				found->name = xstrdup(name);
				list_insert_before(&node_list, node, found);
				changed = true;
				break;
			}
		}

		if(!found) {
			found = xzalloc(sizeof *found);
			found->name = xstrdup(name);
			list_insert_tail(&node_list, found);
			changed = true;
		}

		found->known = true;
		found->in_packets_rate = (in_packets - found->in_packets) / interval;
		found->in_bytes_rate = (in_bytes - found->in_bytes) / interval;
		found->out_packets_rate = (out_packets - found->out_packets) / interval;
		found->out_bytes_rate = (out_bytes - found->out_bytes) / interval;
		found->in_packets = in_packets;
		found->in_bytes = in_bytes;
		found->out_packets = out_packets;
		found->out_bytes = out_bytes;
		found->udp_confirmed = udp_confirmed;

	}

	return false;
}

static int cmpfloat(float a, float b) {
	if(a < b)
		return -1;
	else if(a > b)
		return 1;
	else
		return 0;
}

static int cmpu64(uint64_t a, uint64_t b) {
	if(a < b)
		return -1;
	else if(a > b)
		return 1;
	else
		return 0;
}

static int sortfunc(const void *a, const void *b) {
	const nodestats_t *na = *(const nodestats_t **)a;
	const nodestats_t *nb = *(const nodestats_t **)b;
	switch(sortmode) {
		case 1:
			if(cumulative)
				return -cmpu64(na->in_packets, nb->in_packets) ?: na->i - nb->i;
			else
				return -cmpfloat(na->in_packets_rate, nb->in_packets_rate) ?: na->i - nb->i;
		case 2:
			if(cumulative)
				return -cmpu64(na->in_bytes, nb->in_bytes) ?: na->i - nb->i;
			else
				return -cmpfloat(na->in_bytes_rate, nb->in_bytes_rate) ?: na->i - nb->i;
		case 3:
			if(cumulative)
				return -cmpu64(na->out_packets, nb->out_packets) ?: na->i - nb->i;
			else
				return -cmpfloat(na->out_packets_rate, nb->out_packets_rate) ?: na->i - nb->i;
		case 4:
			if(cumulative)
				return -cmpu64(na->out_bytes, nb->out_bytes) ?: na->i - nb->i;
			else
				return -cmpfloat(na->out_bytes_rate, nb->out_bytes_rate) ?: na->i - nb->i;
		case 5:
			if(cumulative)
				return -cmpu64(na->in_packets + na->out_packets, nb->in_packets + nb->out_packets) ?: na->i - nb->i;
			else
				return -cmpfloat(na->in_packets_rate + na->out_packets_rate, nb->in_packets_rate + nb->out_packets_rate) ?: na->i - nb->i;
		case 6:
			if(cumulative)
				return -cmpu64(na->in_bytes + na->out_bytes, nb->in_bytes + nb->out_bytes) ?: na->i - nb->i;
			else
				return -cmpfloat(na->in_bytes_rate + na->out_bytes_rate, nb->in_bytes_rate + nb->out_bytes_rate) ?: na->i - nb->i;
		default:
			return strcmp(na->name, nb->name) ?: na->i - nb->i;
	}
}

static void redraw(char *vpname) {

	static nodestats_t **sorted = 0;
	static int n = 0;
	if(changed) {
		n = 0;
		sorted = xrealloc(sorted, node_list.count * sizeof *sorted);
		for list_each(nodestats_t, ns, &node_list)
			sorted[n++] = ns;
		changed = false;
	}

	for(int i = 0; i < n; i++)
		sorted[i]->i = i;

	if(sorted)
		qsort(sorted, n, sizeof *sorted, sortfunc);

	cJSON *root = cJSON_CreateObject();
	cJSON *arr = cJSON_CreateArray();
	cJSON *item = NULL;

	for(int i = 0; i < n; i++) {
		nodestats_t *node = sorted[i];

		cJSON *obj = cJSON_CreateObject();
		item = cJSON_CreateString(node->name);
		cJSON_AddItemToObject(obj, "name", item);
		
		//item = cJSON_CreateBool(node->known);
		//cJSON_AddItemToObject(obj, "known", item);
		
		//item = cJSON_CreateNumber(node->in_packets);
		//cJSON_AddItemToObject(obj, "in_packets", item);
		
		item = cJSON_CreateNumber(node->in_bytes);
		cJSON_AddItemToObject(obj, "in_bytes", item);
		
		//item = cJSON_CreateNumber(node->out_packets);
		//cJSON_AddItemToObject(obj, "out_packets", item);
		
		item = cJSON_CreateNumber(node->out_bytes);
		cJSON_AddItemToObject(obj, "out_bytes", item);
		

		//item = cJSON_CreateNumber(node->in_packets_rate);
		//cJSON_AddItemToObject(obj, "in_packets_rate", item);

		item = cJSON_CreateNumber(node->in_bytes_rate);
		cJSON_AddItemToObject(obj, "in_bytes_rate", item);

		//item = cJSON_CreateNumber(node->out_packets_rate);
		//cJSON_AddItemToObject(obj, "out_packets_rate", item);

		item = cJSON_CreateNumber(node->out_bytes_rate);
		cJSON_AddItemToObject(obj, "out_bytes_rate", item);

		item = cJSON_CreateNumber(node->udp_confirmed);
		cJSON_AddItemToObject(obj, "direct", item);
		cJSON_AddItemToArray(arr, obj);
	}
	cJSON_AddItemToObject(root, "lists", arr);
	char *strJson = cJSON_Print(root);
	//printf("%s\r\n", strJson);

	char fdname[256];
	char cmdbuf[256];
	int dirok = 0;
	//sprintf(fdname, "/tmp/tinc/%s", vpname);
	DIR *dir = NULL;
	int ret;
#if 0
	if((dir=opendir(fdname))==NULL)
	{
		sprintf(cmdbuf, "mkdir -p %s", fdname);
		//printf("debug1.1, fdname=%s, cmdbuf='%s'\r\n", fdname, cmdbuf);
		
		if((ret=system(cmdbuf))==0)
		{
			//printf("debug2\r\n");
			dirok = 1;
		}
		//printf("debug1.2, ret=%d\r\n", ret);
	}
	else
	{
		//printf("debug3\r\n");
		closedir(dir);
		dirok = 1;
	}
	if(dirok)
	{
		//printf("debug4\r\n");
		sprintf(fdname, "/tmp/tinc/%s/tinctop.info", vpname);
		FILE *pf = fopen(fdname, "w");
		if(pf)
		{
			//printf("debug5\r\n");
			fwrite(strJson, 1, strlen(strJson), pf);
			fflush(pf);
			fclose(pf);
		}
		
		sprintf(fdname, "/tmp/tinc/%s", vpname);
		if (chdir(fdname) != -1)
		{
			system("tar czf tinctop.info.tar.gz tinctop.info");
		}
	}
#else
	sprintf(fdname, "/tmp/traffic_%s.info", vpname);
	FILE *pf = fopen(fdname, "w");
	if(pf)
	{
		//printf("debug5\r\n");
		fwrite(strJson, 1, strlen(strJson), pf);
		fflush(pf);
		fclose(pf);
	}
#endif

	//printf("debug6\r\n");
	free(strJson);
	
	cJSON_Delete(root);
}

void topex(int fd, char *vpname) {
	//initscr();
	//timeout(delay);
	bool running = true;

	while(running) {
		if(!update(fd))
			break;

		redraw(vpname);

		usleep(delay*1000);
		//getch();
		//timeout(delay);

/*		switch(getch()) {
			case 's': {
				timeout(-1);
				float input = delay * 1e-3;
				//mvprintw(1, 0, "Change delay from %.1fs to: ", input);
				//scanw("%f", &input);
				if(input < 0.1)
					input = 0.1;
				delay = input * 1e3;
				timeout(delay);
				break;
			}
			case 'c':
				cumulative = !cumulative;
				break;
			case 'n':
				sortmode = 0;
				break;
			case 'i':
				sortmode = 2;
				break;
			case 'I':
				sortmode = 1;
				break;
			case 'o':
				sortmode = 4;
				break;
			case 'O':
				sortmode = 3;
				break;
			case 't':
				sortmode = 6;
				break;
			case 'T':
				sortmode = 5;
				break;
			case 'b':
				bunit = "bytes";
				bscale = 1;
				punit = "pkts";
				pscale = 1;
				break;
			case 'k':
				bunit = "kbyte";
				bscale = 1e-3;
				punit = "pkts";
				pscale = 1;
				break;
			case 'M':
				bunit = "Mbyte";
				bscale = 1e-6;
				punit = "kpkt";
				pscale = 1e-3;
				break;
			case 'G':
				bunit = "Gbyte";
				bscale = 1e-9;
				punit = "Mpkt";
				pscale = 1e-6;
				break;
			case 'q':
			case KEY_BREAK:
				running = false;
				break;
			default:
				break;
		}*/
	}

	//endwin();
}

void tinctop_setparams(int val)
{
	delay = val;
}

#endif
