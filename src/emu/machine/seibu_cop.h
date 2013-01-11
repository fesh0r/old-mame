/***************************************************************************

Template for skeleton device

***************************************************************************/

#pragma once

#ifndef __SEIBU_COPDEV_H__
#define __SEIBU_COPDEV_H__



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_SEIBU_COP_ADD(_tag,_freq) \
	MCFG_DEVICE_ADD(_tag, SEIBU_COP, _freq) \

#define SEIBU_COP_INTERFACE(_name) \
	const seibu_cop_interface (_name) =


struct seibu_cop_interface
{
	// memory accessors
	devcb_read16        m_in_byte_cb;
	devcb_read16        m_in_word_cb;
	devcb_read16        m_in_dword_cb;
	devcb_write16       m_out_byte_cb;
	devcb_write16       m_out_word_cb;
	devcb_write16       m_out_dword_cb;
};


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> seibu_cop_device

class seibu_cop_device :    public device_t,
							public device_memory_interface,
							public seibu_cop_interface
{
public:
	// construction/destruction
	seibu_cop_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// I/O operations
	DECLARE_WRITE16_MEMBER( write );
	DECLARE_READ16_MEMBER( read );
	DECLARE_WRITE16_MEMBER( dma_write_trigger_w );
	DECLARE_WRITE16_MEMBER(fill_val_lo_w);
	DECLARE_WRITE16_MEMBER(fill_val_hi_w);
	DECLARE_WRITE16_MEMBER(pal_brightness_val_w);
	DECLARE_WRITE16_MEMBER(pal_brightness_mode_w);
	DECLARE_WRITE16_MEMBER(dma_unk_param_w);
	DECLARE_WRITE16_MEMBER(dma_pal_fade_table_w);
	DECLARE_WRITE16_MEMBER(dma_src_w);
	DECLARE_WRITE16_MEMBER(dma_size_w);
	DECLARE_WRITE16_MEMBER(dma_dst_w);
	DECLARE_READ16_MEMBER(dma_trigger_r);
	DECLARE_WRITE16_MEMBER(dma_trigger_w);

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_validity_check(validity_checker &valid) const;
	virtual void device_start();
	virtual void device_reset();
	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const;

private:
	devcb_resolved_read16       m_in_byte_func;
	devcb_resolved_read16       m_in_word_func;
	devcb_resolved_read16       m_in_dword_func;
	devcb_resolved_write16      m_out_byte_func;
	devcb_resolved_write16      m_out_word_func;
	devcb_resolved_write16      m_out_dword_func;
	inline UINT16 read_word(offs_t address);
	inline void write_word(offs_t address, UINT16 data);

	UINT16 m_dma_unk_param, m_dma_pal_fade_table, m_dma_src[8], m_dma_dst[8], m_dma_size[8], m_dma_exec_param;
	UINT8 m_dma_trigger;
	UINT16 m_fill_val_lo,m_fill_val_hi;
	UINT32 m_fill_val;
	UINT16 m_pal_brightness_val, m_pal_brightness_mode;

	const address_space_config      m_space_config;

	const UINT8 fade_table(int v);
	void normal_dma_transfer(void);
	void palette_dma_transfer(void);
	void fill_word_transfer(void);
	void fill_dword_transfer(void);
};


// device type definition
extern const device_type SEIBU_COP;



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************



#endif
