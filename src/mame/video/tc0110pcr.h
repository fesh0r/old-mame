#ifndef _TC0110PCR_H_
#define _TC0110PCR_H_

struct tc0110pcr_interface
{
	int               m_pal_offs;
};

class tc0110pcr_device : public device_t,
											public tc0110pcr_interface
{
public:
	tc0110pcr_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~tc0110pcr_device() {}

	DECLARE_READ16_MEMBER( word_r );
	DECLARE_WRITE16_MEMBER( word_w ); /* color index goes up in step of 2 */
	DECLARE_WRITE16_MEMBER( step1_word_w );   /* color index goes up in step of 1 */
	DECLARE_WRITE16_MEMBER( step1_rbswap_word_w );    /* swaps red and blue components */
	DECLARE_WRITE16_MEMBER( step1_4bpg_word_w );  /* only 4 bits per color gun */

	void restore_colors();

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();

private:
	UINT16 *     m_ram;
	int          m_type;
	int          m_addr;
};

extern const device_type TC0110PCR;

#define MCFG_TC0110PCR_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, TC0110PCR, 0) \
	MCFG_DEVICE_CONFIG(_interface)

#endif
