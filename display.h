void display_session(struct meterec_s *meterec, int y_pos, int x_pos);
void display_port_info(struct port_s *port_p);
void display_port_recmode(struct port_s *port_p);
void display_status(struct meterec_s *meterec, unsigned int playhead);
void display_buffer(struct meterec_s *meterec, int width);
void display_meter(struct meterec_s *meterec, unsigned int y_pos, int display_names, int width, int decay_len);
void init_display_scale(int width);

