#include "atapicdr.h"
#include "scsicd.h"

// device type definition
const device_type ATAPI_CDROM = &device_creator<atapi_cdrom_device>;

atapi_cdrom_device::atapi_cdrom_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: atapi_hle_device(mconfig, ATAPI_CDROM, "ATAPI CDROM", tag, owner, clock, "cdrom", __FILE__)
{
}

static MACHINE_CONFIG_FRAGMENT( atapicdr )
	MCFG_DEVICE_ADD("device", SCSICD, 0)
MACHINE_CONFIG_END

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor atapi_cdrom_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( atapicdr );
}

void atapi_cdrom_device::device_start()
{
	memset(m_identify_buffer, 0, sizeof(m_identify_buffer));

	m_identify_buffer[ 0 ] = 0x8500; // ATAPI device, cmd set 5 compliant, DRQ within 3 ms of PACKET command

	m_identify_buffer[ 23 ] = ('1' << 8) | '.';
	m_identify_buffer[ 24 ] = ('0' << 8) | ' ';
	m_identify_buffer[ 25 ] = (' ' << 8) | ' ';
	m_identify_buffer[ 26 ] = (' ' << 8) | ' ';

	m_identify_buffer[ 27 ] = ('M' << 8) | 'A';
	m_identify_buffer[ 28 ] = ('M' << 8) | 'E';
	m_identify_buffer[ 29 ] = (' ' << 8) | 'C';
	m_identify_buffer[ 30 ] = ('o' << 8) | 'm';
	m_identify_buffer[ 31 ] = ('p' << 8) | 'r';
	m_identify_buffer[ 32 ] = ('e' << 8) | 's';
	m_identify_buffer[ 33 ] = ('s' << 8) | 'e';
	m_identify_buffer[ 34 ] = ('d' << 8) | ' ';
	m_identify_buffer[ 35 ] = ('C' << 8) | 'D';
	m_identify_buffer[ 36 ] = ('R' << 8) | 'O';
	m_identify_buffer[ 37 ] = ('M' << 8) | ' ';
	m_identify_buffer[ 38 ] = (' ' << 8) | ' ';
	m_identify_buffer[ 39 ] = (' ' << 8) | ' ';
	m_identify_buffer[ 40 ] = (' ' << 8) | ' ';
	m_identify_buffer[ 41 ] = (' ' << 8) | ' ';
	m_identify_buffer[ 42 ] = (' ' << 8) | ' ';
	m_identify_buffer[ 43 ] = (' ' << 8) | ' ';
	m_identify_buffer[ 44 ] = (' ' << 8) | ' ';
	m_identify_buffer[ 45 ] = (' ' << 8) | ' ';
	m_identify_buffer[ 46 ] = (' ' << 8) | ' ';

	m_identify_buffer[ 49 ] = 0x0600; // Word 49=Capabilities, IORDY may be disabled (bit_10), LBA Supported mandatory (bit_9)

	atapi_hle_device::device_start();
}

void atapi_cdrom_device::perform_diagnostic()
{
	m_error = IDE_ERROR_DIAGNOSTIC_PASSED;
}

void atapi_cdrom_device::identify_packet_device()
{
}
