/***************************************************************************

    NEC uPD4990A

    Serial I/O Calendar & Clock IC

***************************************************************************/

#ifndef __PD4990A_H__
#define __PD4990A_H__

#include "devlegcy.h"


/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

class upd4990a_device : public device_t
{
public:
	upd4990a_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~upd4990a_device() { global_free(m_token); }

	// access to legacy token
	void *token() const { assert(m_token != NULL); return m_token; }
protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();
private:
	// internal state
	void *m_token;
};

extern const device_type UPD4990A;


#define MCFG_UPD4990A_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, UPD4990A, 0)


/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* this should be refactored, once RTCs get unified */
extern void upd4990a_addretrace( device_t *device );

extern READ8_DEVICE_HANDLER( upd4990a_testbit_r );
extern READ8_DEVICE_HANDLER( upd4990a_databit_r );
extern WRITE16_DEVICE_HANDLER( upd4990a_control_16_w );


#endif	/*__PD4990A_H__*/
