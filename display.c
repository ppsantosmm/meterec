/*

  meterec
  Console based multi track digital peak meter and recorder for JACK
  Copyright (C) 2009-2013 Fabrice Lebas
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdlib.h>
#include <string.h>

#include <sndfile.h>
#include <jack/jack.h>
#include <curses.h>
#include "position.h"
#include "meterec.h"
#include "disk.h"
#include "display.h"

char *scale ;
char *line ;

void display_init_windows(struct meterec_s *meterec) {
	
	unsigned int w, h, p;
	
	if (meterec->display.width == getmaxx(stdscr))
		if (meterec->display.height == getmaxy(stdscr))
			return;
	
	w = meterec->display.width = getmaxx(stdscr);
	h = meterec->display.height = getmaxy(stdscr);
	p = meterec->n_ports;
	
	meterec->display.wrds = newwin(1, 20,        0,  0);
	meterec->display.wwrs = newwin(1, 20,        1,  0);
	meterec->display.wttl = newwin(2, w-20-3*13, 0, 20);
	meterec->display.wloo = newwin(1, 3*13,      0,  w-3*13);
	meterec->display.wcpu = newwin(1, 3*13,      1,  w-3*13);
	meterec->display.wsc1 = newwin(2, w-8,       2,  8);
	meterec->display.wpor = newwin(p,   8,       4,  0);
	meterec->display.wvum = newwin(p, w-8,       4,  8);
	meterec->display.wsc2 = newwin(2, w-8,     p+4,  8);
	meterec->display.wcon = newwin(h-4-1, w,      4,  0);
	meterec->display.wbot = newwin(1, w-20,    h-1,  0);
	meterec->display.wbdb = newwin(1, 20,      h-1,  w-20);
	
	display_init_scale(0, meterec->display.wsc1);
	display_init_scale(1, meterec->display.wsc2);
	
	/*
	box(meterec->display.wbot,0,0);
	wnoutrefresh(meterec->display.wbot);
	box(meterec->display.wbdb,0,0);
	wnoutrefresh(meterec->display.wbdb);
	*/
}

static int iec_scale(float db, int size) {
	
	float def = 0.0f; /* Meter deflection %age */
	
	if (db < -70.0f) 
		def = 0.0f;
	else if (db < -60.0f) 
		def = (db + 70.0f) * 0.25f;
	else if (db < -50.0f) 
		def = (db + 60.0f) * 0.5f + 2.5f;
	else if (db < -40.0f) 
		def = (db + 50.0f) * 0.75f + 7.5f;
	else if (db < -30.0f) 
		def = (db + 40.0f) * 1.5f + 15.0f;
	else if (db < -20.0f) 
		def = (db + 30.0f) * 2.0f + 30.0f;
	else if (db < 0.0f) 
		def = (db + 20.0f) * 2.5f + 50.0f;
	else 
		def = 100.0f;
	
	return (int)((def / 100.0f) * ((float) size));
}


void display_session_name(struct meterec_s *meterec, WINDOW *win) {
	unsigned int len, w, pos;
	
	wclear(win);
	
	len = strlen(meterec->session) + 4;
	w = getmaxx(win);
	pos = (w - len) / 2;
	
	mvwprintw(win, 0, pos, "~ %s ~", meterec->session);
	
	wnoutrefresh(win);
}

void display_fill_remaining(unsigned int remain) {
	unsigned int i;
	unsigned int width, y, x;
	int spaces;
	
	width = getmaxx(stdscr);
	getyx(stdscr, y, x); (void)y;
	spaces = width - x - remain;
	
	if (spaces < 0)
		return;
	
	for (i=0; i<spaces; i++) 
		printw(" ");
}

void display_right_aligned(char *message, unsigned int remain) {
	unsigned int i, len;
	unsigned int width, y, x;
	int spaces;
	
	len =strlen(message);
	
	width = getmaxx(stdscr);
	getyx(stdscr, y, x); (void)y;
	spaces = width - x - remain - len;
	
	if (spaces < 0)
		spaces += len;
		
	if (spaces < 0)
		return;
	
	for (i=0; i<spaces; i++) 
		printw(" ");
	
	if ( spaces == width - x - remain - len)
		printw("%s",message);
}

void display_port_info(struct meterec_s *meterec) {
	
	char *take_name = NULL;
	unsigned int port = meterec->pos.port;
	struct port_s *port_p = &meterec->ports[port];
	WINDOW *win = meterec->display.wbot;
	
	wclear(win);
	
	wprintw(win, "  Port %2d ", port);
	
	if (port_p->playback_take)
		take_name = meterec->takes[port_p->playback_take].name;
	
	if (take_name == NULL)
		take_name = "";
	
	if (port_p->record==REC)
		wprintw(win, "[REC]");
	else if (port_p->record==OVR)
		wprintw(win, "[OVR]");
	else if (port_p->record==DUB)
		wprintw(win, "[DUB]");
	else 
		wprintw(win, "[   ]");
	
	if (port_p->mute)
		wprintw(win, "[MUTED]");
	else 
		wprintw(win, "[     ]");
	
	if (port_p->thru)
		wprintw(win, "[THRU]");
	else 
		wprintw(win, "[    ]");
	
	if ( port_p->playback_take ) 
		wprintw(win, " PLAYING take %d (%s)", port_p->playback_take, take_name);
	else 
		wprintw(win, " PLAYING no take");
	
	if (port_p->name)
		display_right_aligned(port_p->name,21);
	else 
		display_fill_remaining(21);
	
	wnoutrefresh(win);
	
}

void display_port_db_digital(struct meterec_s *meterec) {
	
	unsigned int port = meterec->pos.port;
	struct port_s *port_p = &meterec->ports[port];
	WINDOW *win = meterec->display.wbdb;
	
	wclear(win);
	
	wprintw(win, "%5.1fdB (%5.1fdB)", port_p->db_in, port_p->db_max_in);
	
	wnoutrefresh(win);
	
}

void display_tiny_meter(struct meterec_s *meterec, unsigned int port, WINDOW *win) {
	
	char *blink = " \0.\0o\0O\0#";
	int pos;
	
	pos = iec_scale( meterec->ports[port].db_out, 5);
	
	wprintw(win, blink + 2*pos);
	
}

void display_ports_modes(struct meterec_s *meterec) {
	
	unsigned int port;
	WINDOW *win; 
	
	win = meterec->display.wpor;
	
	wclear(win);
	
	for (port=0; port < meterec->n_ports; port++) {
		
		mvwprintw(win, port, 0, "%02d|",port+1);
		
		if ( meterec->ports[port].record == REC )
			wprintw(win, "R");
		else if ( meterec->ports[port].record == DUB )
			wprintw(win, "D");
		else if ( meterec->ports[port].record == OVR )
			wprintw(win, "O");
		else 
			wprintw(win, " ");
		
		if ( meterec->ports[port].thru )
			wprintw(win, "T");
		else 
			wprintw(win, " ");
		
		if ( meterec->ports[port].mute )
			wprintw(win, "M");
		else 
			wprintw(win, " ");
		
		display_tiny_meter(meterec, port, win);
		
	}
	
	wnoutrefresh(win);
}

static void color_port(struct meterec_s *meterec, unsigned int port, WINDOW *win) {
	
	if ( meterec->ports[port].record ) 
		if (meterec->record_sts == ONGOING)
			wcolor_set(win, RED, NULL);
		else 
			wcolor_set(win, YELLOW, NULL);
	else 
		if (meterec->ports[port].mute)
			wcolor_set(win, DEFAULT, NULL);
		else 
			wcolor_set(win, GREEN, NULL);
}

void display_meter(struct meterec_s *meterec, int display_names, int decay_len)
{
	int size_out, size_in, i;
	unsigned int port, width;
	WINDOW *win = meterec->display.wvum;;
	
	wclear(win);
	
	width = getmaxx(win);
	
	for ( port=0 ; port < meterec->n_ports ; port++) {
		
		color_port(meterec, port, win);
		
		if (meterec->pos.port == port) 
			wattron(win, A_REVERSE);
		else 
			wattroff(win, A_REVERSE);
		
		wmove(win, port, 0);
		
		size_in = iec_scale( meterec->ports[port].db_in, width );
		size_out = iec_scale( meterec->ports[port].db_out, width );
		
		if (size_in > meterec->ports[port].dkmax_in)
			meterec->ports[port].dkmax_in = size_in;
		
		if (size_in > meterec->ports[port].dkpeak_in) {
			meterec->ports[port].dkpeak_in = size_in;
			meterec->ports[port].dktime_in = 0;
		} 
		else if (meterec->ports[port].dktime_in++ > decay_len) {
			meterec->ports[port].dkpeak_in = size_in;
		}
		
		for ( i=0; i<width; i++) {
			
			if (display_names)
				if (i == width/5) 
					if (meterec->ports[port].name) {
						wprintw(win, "%s",meterec->ports[port].name);
						i += strlen(meterec->ports[port].name);
					}
			
			if (i < size_in-1) {
				wprintw(win, "#");
			}
			else if ( i==meterec->ports[port].dkpeak_in-1 ) {
				wprintw(win, "I");
			}
			else if ( i==meterec->ports[port].dkmax_in-1 ) {
				if (i>width-3)
					wprintw(win, "X");
				else
					wprintw(win, ":");
			}
			else if ( i < size_out-1 ) {
				wprintw(win, "-");
			}
			else {
				wprintw(win, " ");
			}
		}
	}
	
	wattroff(win, A_REVERSE);
	wcolor_set(win, DEFAULT, NULL);
	
	wnoutrefresh(win);
}

void display_init_scale(int side, WINDOW *win) {
	
	unsigned int i=0, width;
	const int marks[12] = { 0, -3, -5, -10, -15, -20, -25, -30, -35, -40, -50, -60 };
	
	if (side)
		side = 1;
	
	wclear(win);
	
	width = getmaxx(win);
	
	mvwhline(win, !side, 0, 0, width);
	
	/* 'draw' on each of the db marks */
	for(i=0; i < 12; i++) {
		char mark[5];
		int pos = iec_scale( marks[i], width ) - 1;
		int spos, slen;
		
		/* Create string of the db value */
		snprintf(mark, 4, "%d", marks[i]);
		
		/* Position the label string */
		slen = strlen(mark);
		spos = pos - (slen/2);
		if (spos<0) 
			spos=0;
		if (spos+slen > width) 
			spos = width - slen;
		
		mvwprintw(win, side, spos, "%s", mark);
		
		/* Position ticks along the scale */
		mvwaddch(win, !side, pos, ACS_PLUS);
		
	}
	
	wnoutrefresh(win);
	
}

void display_rd_buffer(struct meterec_s *meterec, WINDOW *win) {
	int size, i;
	static int peak=0;
	static char *pedale = "|";
	const int width = 11;
	
	size = width * read_disk_buffer_level(meterec);
	
	if (size > peak && meterec->playback_sts == ONGOING) 
		peak = size;
		
	
	for (i=0; i<width; i++) {
		if (i == peak-1)
			wprintw(win, ":");
		else if (i > size-1)
			wprintw(win, "I");
		else 
			wprintw(win, " ");
	}
	wprintw(win, "%s", pedale);
	
	if (meterec->playback_sts==ONGOING) {
		if      (*pedale=='/')
			pedale = "-";
		else if (*pedale=='-')
			pedale = "\\";
		else if (*pedale=='\\')
			pedale = "|";
		else if (*pedale=='|')
			pedale = "/";
	}
	
}

void display_wr_buffer(struct meterec_s *meterec, WINDOW *win) {
	int size, i;
	static int peak=0;
	static char *pedale = "|";
	const int width = 11;
	
	size = width * write_disk_buffer_level(meterec);
	
	if (size > peak) 
		peak = size;
		
	wprintw(win, "%s", pedale);
	for (i=0; i<width; i++) {
		if (i < size-1)
			wprintw(win, "I");
		else if (i == peak-1)
			wprintw(win, ":");
		else 
			wprintw(win, " ");
	}
	
	if (meterec->record_sts==ONGOING) {
		if      (*pedale=='/')
			pedale = "-";
		else if (*pedale=='-')
			pedale = "\\";
		else if (*pedale=='\\')
			pedale = "|";
		else if (*pedale=='|')
			pedale = "/";
	}
	
}

void display_cpu_load_digital(struct meterec_s *meterec, WINDOW *win) {
	wclear(win);
	
	mvwprintw(win, 0, 39-7, "%6.2f%%", jack_cpu_load(meterec->client));
	
	wnoutrefresh(win);
}

void display_loop(struct meterec_s *meterec, WINDOW *win) {
	
	struct time_s low, high, now;
	wclear(win);
	
	if (meterec->loop.low == MAX_UINT) 
		wprintw(win, "[-:--:--.---]");
	else {
		low.frm = meterec->loop.low;
		low.rate = meterec->jack.sample_rate ;
		time_hms(&low);
		wprintw(win, "[%d:%02d:%02d.%03d]", low.h, low.m, low.s, low.ms);
	}
	
	now.frm = meterec->jack.playhead;
	now.rate = meterec->jack.sample_rate ;
	time_hms(&now);
	wprintw(win, " %d:%02d:%02d.%03d ", now.h, now.m, now.s, now.ms);
		
	if (meterec->loop.high == MAX_UINT) 
		wprintw(win, "[-:--:--.---]");
	else {
		high.frm = meterec->loop.high;
		high.rate = meterec->jack.sample_rate ;
		time_hms(&high);
		wprintw(win, "[%d:%02d:%02d.%03d]", high.h, high.m, high.s, high.ms);
	}
	
	wnoutrefresh(win);
}

void display_rd_status(struct meterec_s *meterec, WINDOW *win) {
	
	wclear(win);
	
	wprintw(win, "[> ");
		
	if (meterec->playback_sts==OFF) 
		wprintw(win, "%-8s","OFF");
	else if (meterec->playback_sts==STARTING) 
		wprintw(win, "%-8s","STARTING");
	else if (meterec->playback_sts==ONGOING) 
		wprintw(win, "%-8s","ONGOING");
	else if (meterec->playback_sts==PAUSED) 
		wprintw(win, "%-8s","PAUSED");
	else if (meterec->playback_sts==STOPING) 
		wprintw(win, "%-8s","STOPING");
	
	wprintw(win, "]");
	
	display_rd_buffer(meterec, win);
	
	wnoutrefresh(win);
}

void display_wr_status(struct meterec_s *meterec, WINDOW *win) {
	
	wclear(win);
	
	if (meterec->record_sts) 
	color_set(RED, NULL);
	
	wprintw(win, "[O ");
	
	if (meterec->record_sts==OFF) 
		wprintw(win, "%-8s","OFF");
	else if (meterec->record_sts==STARTING) 
		wprintw(win, "%-8s","STARTING");
	else if (meterec->record_sts==ONGOING) 
		wprintw(win, "%-8s","ONGOING");
	else if (meterec->record_sts==STOPING) 
		wprintw(win, "%-8s","STOPING");
	
	wprintw(win, "]");
	
	display_wr_buffer(meterec, win);
	
	if (meterec->record_sts) {
		attron(A_BOLD);
		wprintw(win, " Take %d",meterec->n_takes+1);
		attroff(A_BOLD); 
	}
	
	
	if (meterec->write_disk_buffer_overflow)
		wprintw(win, " OVERFLOWS(%d)",meterec->write_disk_buffer_overflow);
	
	color_set(DEFAULT, NULL);
	
	wnoutrefresh(win);
}

void display_header(struct meterec_s *meterec) {
	
	display_rd_status(meterec, meterec->display.wrds);
	display_wr_status(meterec, meterec->display.wwrs);
	display_loop(meterec, meterec->display.wloo);
	display_session_name(meterec, meterec->display.wttl);
	display_cpu_load_digital(meterec, meterec->display.wcpu);
	
}

void display_take_info(struct meterec_s *meterec) {
	
	WINDOW *win = meterec->display.wsc1;
	char *name ="";
	unsigned int y_pos, x_pos;
	
	wclear(win);
	
	y_pos = meterec->pos.port;
	x_pos = meterec->pos.take;
	
	if (meterec->takes[x_pos].name)
		name = meterec->takes[x_pos].name;
	
	wprintw(win, "Take %d (%s)\n",x_pos, name);
	wprintw(win, "%s",  meterec->takes[x_pos].port_has_track[y_pos]?"[CONTENT]":"[       ]" );
	wprintw(win, "%s",  meterec->takes[x_pos].port_has_lock[y_pos]?"[LOCKED]":"[      ]" );
	wprintw(win, "%s", (meterec->ports[y_pos].playback_take == x_pos)?"[PLAYING]":"[       ]" );
	
	wnoutrefresh(win);

}

void display_session(struct meterec_s *meterec) 
{
	
	WINDOW *win = meterec->display.wvum;
	unsigned int take, port;
	unsigned int y_pos, x_pos;
	
	wclear(win);
		
	y_pos = meterec->pos.port;
	x_pos = meterec->pos.take;
	
	for (port=0; port<meterec->n_ports; port++) {
		
		color_port(meterec, port, win);
		
		if (y_pos == port) 
			wattron(win, A_REVERSE);
		else 
			wattroff(win, A_REVERSE);
		
		for (take=1; take<meterec->n_takes+1; take++) {
			
			if ((y_pos == port) || (x_pos == take))
				wattron(win, A_REVERSE);
			else 
				wattroff(win, A_REVERSE);
			
			if ((y_pos == port) && (x_pos == take))
				wattroff(win, A_REVERSE);
			
			if ( meterec->ports[port].playback_take == take )
				wattron(win, A_BOLD);
			
			if ( meterec->takes[take].port_has_lock[port] )
				wprintw(win, meterec->takes[take].port_has_track[port]?"L":"l");
			else if ( meterec->ports[port].playback_take == take ) 
				wprintw(win, "P");
			else if ( meterec->takes[take].port_has_track[port] ) 
				wprintw(win, "X");
			else 
				wprintw(win, "-");
			
			wattroff(win, A_BOLD);
			
		}
		
		wprintw(win, "\n");
	}
	
	wattroff(win, A_REVERSE);
	wcolor_set(win, DEFAULT, NULL);
	
	wnoutrefresh(win);
}

void display_ports(struct meterec_s *meterec) 
{
	unsigned int port=0, i;
	int line=0;
	const char **in, **out;
	WINDOW *win = meterec->display.wcon;
	
	out=meterec->all_input_ports;
	in=meterec->all_output_ports;
	
	wclear(win);
	
	while ((in && *in) || (out && *out) || port < meterec->n_ports) {
		
		wprintw(win, "  ");
		
		if (in && *in) {
		
			if (meterec->pos.inout == CON_IN)
				if (meterec->pos.con_in == line)
					wattron(win, A_REVERSE);
					
			if (jack_port_connected_to(meterec->ports[meterec->pos.port].input, *in)) {
				wattron(win, A_BOLD);
				wprintw(win, "%20s",*in);
				wattroff(win, A_BOLD);
				wattroff(win, A_REVERSE);
				wprintw(win, "-+");
			}
			else if (port==meterec->pos.port) {
				wprintw(win, "%20s",*in);
				wattroff(win, A_REVERSE);
				wprintw(win, " +");
				color_port(meterec, meterec->pos.port, win);
			}
			else {
				wprintw(win, "%20s",*in);
				wattroff(win, A_REVERSE);
				wprintw(win, " |");
			}
			in++;
		} else {
			if (port<meterec->pos.port)
				wprintw(win, "%20s |","");
			else if (port==meterec->pos.port)
				wprintw(win, "%20s +","");
			else 
				wprintw(win, "%20s  ","");
		}
		
		
		if (port<meterec->n_ports) {
		
			if (meterec->pos.port == port) {
				wprintw(win, "-");
				if (!meterec->pos.inout)
					wattron(win, A_REVERSE);
				else
					wattron(win, A_BOLD);
			}
			else 
				wprintw(win, " ");
			
			color_port(meterec, port, win);
			
			wprintw(win, "%s:in_%-2d",  meterec->jack_name, port+1);
			
			wcolor_set(win, DEFAULT, NULL);
			
			if (meterec->ports[port].thru)
				wprintw(win, "-> ");
			else 
				wprintw(win, "   ");
			
			wprintw(win, "%s:out_%-2d",  meterec->jack_name, port+1);
			
			wattroff(win, A_BOLD);
			wattroff(win, A_REVERSE);
		
		}
		else 
			for (i=0; i<(2*strlen(meterec->jack_name)+17); i++)
				wprintw(win, " ");
		
		if (meterec->pos.port==port)
			wprintw(win, "-");
		else 
			wprintw(win, " ");
		
		if (out && *out) {
			if (jack_port_connected_to(meterec->ports[meterec->pos.port].output, *out)) {
				wprintw(win, "+-");
				wattron(win, A_BOLD);
			}
			else if (port==meterec->pos.port) {
				wprintw(win, "+ ");
			}
			else
				wprintw(win, "| ");
				
			if (meterec->pos.inout == CON_OUT)
				if (meterec->pos.con_out == line)
					wattron(win, A_REVERSE);
				
			wprintw(win, "%s",*out);
			wattroff(win, A_BOLD);
			wattroff(win, A_REVERSE);
			
			out++;
		} else {
			if (port<meterec->pos.port)
				wprintw(win, "|");
			else if (port==meterec->pos.port)
				wprintw(win, "+");
			else 
				wprintw(win, "");
		}
		wcolor_set(win, DEFAULT, NULL);
		wprintw(win, "\n");
		
		if (port < meterec->n_ports)
			port ++;
			
		line ++;
	}
	
	wcolor_set(win, DEFAULT, NULL);
	
	wnoutrefresh(win);
	
}
