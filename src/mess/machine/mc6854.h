/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Motorola 6854 emulation (network interface).

**********************************************************************/

#ifndef MC6854
#define MC6854

/* we provide two interfaces:
   - a bit-based interface:   out_tx, set_rx
   - a frame-based interface: out_frame, send_frame

   The bit-based interface is low-level and slow.
   Use it to simulate the actual bits sent into the wires, e.g., to connect
   the emulator to another bit-based emulated network device, or an actual
   device.

   The frame-based interface is higher-level and faster.
   It passes bytes directly from one end to the other without bothering with
   the actual bit-encoding, synchronization, and CRC.
   Once completed, a frame is sent through out_frame. Aborted frames are not
   transmitted at all. No start flag, stop flag, or crc bits are trasmitted.
   send_frame makes a frame available to the CPU through the 6854 (it may
   fail and return -1 if the 6854 is not ready to accept the frame; even
   if the frame is accepted and 0 is returned, the CPU may abort it). Ony
   full frames are accepted.
*/


/* ---------- configuration ------------ */

typedef struct _mc6854_interface mc6854_interface;
struct _mc6854_interface
{
  /* low-level, bit-based interface */
  void ( * out_tx  ) ( int state ); /* transmit bit */

  /* high-level, frame-based interface */
  void ( * out_frame ) ( UINT8* data, int length );

  /* control lines */
  void ( * out_rts ) ( int state ); /* 1 = transmitting, 0 = idle */
  void ( * out_dtr ) ( int state ); /* 1 = data transmit ready, 0 = busy */
};


/* ---------- functions ------------ */

extern void mc6854_config ( const mc6854_interface* func );

/* reset by external signal */
extern void mc6854_reset ( void );

/* interface to CPU via address/data bus*/
extern READ8_HANDLER  ( mc6854_r );
extern WRITE8_HANDLER ( mc6854_w );

/* low-level, bit-based interface */
extern void mc6854_set_rx ( int state );

/* high-level, frame-based interface */
extern int mc6854_send_frame( UINT8* data, int length ); /* ret -1 if busy */

/* control lines */
extern void mc6854_set_cts ( int state ); /* 1 = clear-to-send, 0 = busy */
extern void mc6854_set_dcd ( int state ); /* 1 = carrier, 0 = no carrier */

#endif