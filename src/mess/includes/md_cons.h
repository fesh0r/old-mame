

class md_cons_state : public md_base_state
{
public:
	md_cons_state(const machine_config &mconfig, device_type type, const char *tag)
	: md_base_state(mconfig, type, tag),
	m_slotcart(*this, "mdslot")
	{ }

	ioport_port *m_io_ctrlr;
	ioport_port *m_io_pad3b[4];
	ioport_port *m_io_pad6b[2][4];

	optional_device<md_cart_slot_device> m_slotcart;

	DECLARE_DRIVER_INIT(mess_md_common);
	DECLARE_DRIVER_INIT(genesis);
	DECLARE_DRIVER_INIT(md_eur);
	DECLARE_DRIVER_INIT(md_jpn);

	READ8_MEMBER(mess_md_io_read_data_port);
	WRITE16_MEMBER(mess_md_io_write_data_port);

	DECLARE_MACHINE_START( md_common );     // setup ioport_port
	DECLARE_MACHINE_START( ms_megadriv );   // setup ioport_port + install cartslot handlers
	DECLARE_MACHINE_RESET( ms_megadriv );
};
