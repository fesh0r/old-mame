/*
	Corvus Concept driver

	Raphael Nabet, 2003
*/

enum
{
	rom0_base = 0x10000
};

enum
{
	input_port_keyboard_concept = 0
};

DEVICE_LOAD( corvus_floppy );
MACHINE_INIT(concept);
VIDEO_START(concept);
VIDEO_UPDATE(concept);
INTERRUPT_GEN( concept_interrupt );
READ16_HANDLER(concept_io_r);
WRITE16_HANDLER(concept_io_w);
