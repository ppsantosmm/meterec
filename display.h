/*

  meterec
  Console based multi track digital peak meter and recorder for JACK
  Copyright (C) 2009-2020 Fabrice Lebas

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

void display_loop(struct meterec_s *meterec);
void display_ports(struct meterec_s *meterec);
void display_connections(struct meterec_s *meterec);
void display_connections_init(struct meterec_s *meterec);
void display_connections_fill_ports(struct meterec_s *meterec);
void display_connections_fill_conns(struct meterec_s *meterec);
void display_session(struct meterec_s *meterec);
void display_port_info(struct meterec_s *meterec);
void display_port_recmode(struct port_s *port_p);
void display_ports_modes(struct meterec_s *meterec);
void display_ports_tiny_meters(struct meterec_s *meterec);
void display_rd_status(struct meterec_s *meterec);
void display_wr_status(struct meterec_s *meterec);
void display_current_view_name(struct meterec_s *meterec);
void display_meter(struct meterec_s *meterec);
void display_init_scale(int side, WINDOW *win);
void display_init_legend(WINDOW *win);
void display_init_clr(WINDOW *win);
void display_init_title(struct meterec_s *meterec);
void display_init_windows(struct meterec_s *meterec, unsigned int w, unsigned int h);
void display_port_db_digital(struct meterec_s *meterec);
void display_take_info(struct meterec_s *meterec);
void display_view_change(struct meterec_s *meterec);
void display_box(WINDOW *win);
void display_debug_windows(struct meterec_s *meterec);
void display_changed_size(struct meterec_s *meterec);
void display_changed_content(struct meterec_s *meterec);
void display_changed_view(struct meterec_s *meterec);
void display_changed_static_content(struct meterec_s *meterec);
void display_dynamic_content(struct meterec_s *meterec);
void display_refresh_view(struct meterec_s *meterec);
void display_init_curses(struct meterec_s *meterec);
void display_cleanup_curses(struct meterec_s *meterec);
