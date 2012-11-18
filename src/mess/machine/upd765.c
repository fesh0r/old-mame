#include "debugger.h"

#include "upd765.h"

const device_type UPD765A = &device_creator<upd765a_device>;
const device_type UPD765B = &device_creator<upd765b_device>;
const device_type I8272A = &device_creator<i8272a_device>;
const device_type UPD72065 = &device_creator<upd72065_device>;
const device_type SMC37C78 = &device_creator<smc37c78_device>;
const device_type N82077AA = &device_creator<n82077aa_device>;
const device_type PC_FDC_SUPERIO = &device_creator<pc_fdc_superio_device>;

DEVICE_ADDRESS_MAP_START(map, 8, upd765a_device)
	AM_RANGE(0x0, 0x0) AM_READ(msr_r)
	AM_RANGE(0x1, 0x1) AM_READWRITE(fifo_r, fifo_w)
ADDRESS_MAP_END

DEVICE_ADDRESS_MAP_START(map, 8, upd765b_device)
	AM_RANGE(0x0, 0x0) AM_READ(msr_r)
	AM_RANGE(0x1, 0x1) AM_READWRITE(fifo_r, fifo_w)
ADDRESS_MAP_END

DEVICE_ADDRESS_MAP_START(map, 8, i8272a_device)
	AM_RANGE(0x0, 0x0) AM_READ(msr_r)
	AM_RANGE(0x1, 0x1) AM_READWRITE(fifo_r, fifo_w)
ADDRESS_MAP_END

DEVICE_ADDRESS_MAP_START(map, 8, upd72065_device)
	AM_RANGE(0x0, 0x0) AM_READ(msr_r)
	AM_RANGE(0x1, 0x1) AM_READWRITE(fifo_r, fifo_w)
ADDRESS_MAP_END

DEVICE_ADDRESS_MAP_START(map, 8, smc37c78_device)
	AM_RANGE(0x2, 0x2) AM_READWRITE(dor_r, dor_w)
	AM_RANGE(0x3, 0x3) AM_READWRITE(tdr_r, tdr_w)
	AM_RANGE(0x4, 0x4) AM_READWRITE(msr_r, dsr_w)
	AM_RANGE(0x5, 0x5) AM_READWRITE(fifo_r, fifo_w)
	AM_RANGE(0x7, 0x7) AM_READWRITE(dir_r, ccr_w)
ADDRESS_MAP_END

DEVICE_ADDRESS_MAP_START(map, 8, n82077aa_device)
	AM_RANGE(0x0, 0x0) AM_READ(sra_r)
	AM_RANGE(0x1, 0x1) AM_READ(srb_r)
	AM_RANGE(0x2, 0x2) AM_READWRITE(dor_r, dor_w)
	AM_RANGE(0x3, 0x3) AM_READWRITE(tdr_r, tdr_w)
	AM_RANGE(0x4, 0x4) AM_READWRITE(msr_r, dsr_w)
	AM_RANGE(0x5, 0x5) AM_READWRITE(fifo_r, fifo_w)
	AM_RANGE(0x7, 0x7) AM_READWRITE(dir_r, ccr_w)
ADDRESS_MAP_END

DEVICE_ADDRESS_MAP_START(map, 8, pc_fdc_superio_device)
	AM_RANGE(0x0, 0x0) AM_READ(sra_r)
	AM_RANGE(0x1, 0x1) AM_READ(srb_r)
	AM_RANGE(0x2, 0x2) AM_READWRITE(dor_r, dor_w)
	AM_RANGE(0x3, 0x3) AM_READWRITE(tdr_r, tdr_w)
	AM_RANGE(0x4, 0x4) AM_READWRITE(msr_r, dsr_w)
	AM_RANGE(0x5, 0x5) AM_READWRITE(fifo_r, fifo_w)
	AM_RANGE(0x7, 0x7) AM_READWRITE(dir_r, ccr_w)
ADDRESS_MAP_END


int upd765_family_device::rates[4] = { 500000, 300000, 250000, 1000000 };

upd765_family_device::upd765_family_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) : pc_fdc_interface(mconfig, type, name, tag, owner, clock)
{
	ready_polled = true;
	ready_connected = true;
	select_connected = true;
	external_ready = true;
	dor_reset = 0x00;
}

void upd765_family_device::set_ready_line_connected(bool _ready)
{
	ready_connected = _ready;
}

void upd765_family_device::set_select_lines_connected(bool _select)
{
	select_connected = _select;
}

void upd765_family_device::set_mode(int _mode)
{
	// TODO
}

void upd765_family_device::setup_intrq_cb(line_cb cb)
{
	intrq_cb = cb;
}

void upd765_family_device::setup_drq_cb(line_cb cb)
{
	drq_cb = cb;
}

void upd765_family_device::device_start()
{
	for(int i=0; i != 4; i++) {
		char name[2];
		flopi[i].tm = timer_alloc(i);
		flopi[i].id = i;
		if(select_connected) {
			name[0] = '0'+i;
			name[1] = 0;
			floppy_connector *con = subdevice<floppy_connector>(name);
			if(con) {
				flopi[i].dev = con->get_device();
				flopi[i].dev->setup_index_pulse_cb(floppy_image_device::index_pulse_cb(FUNC(upd765_family_device::index_callback), this));
			} else
				flopi[i].dev = NULL;
		} else
			flopi[i].dev = NULL;
	}
	cur_rate = 250000;
	tc = false;

	// reset at upper levels may cause a write to tc ending up with
	// live_sync, which will crash if the live structure isn't
	// initialized enough

	cur_live.tm = attotime::never;
	cur_live.state = IDLE;
	cur_live.next_state = -1;
	cur_live.fi = NULL;

	if(ready_polled) {
		poll_timer = timer_alloc(TIMER_DRIVE_READY_POLLING);
		poll_timer->adjust(attotime::from_usec(1024), 0, attotime::from_usec(1024));
	} else
		poll_timer = NULL;

	cur_irq = false;
	locked = false;
}

void upd765_family_device::device_reset()
{
	dor = dor_reset;
	locked = false;
	soft_reset();
}

void upd765_family_device::soft_reset()
{
	main_phase = PHASE_CMD;
	for(int i=0; i<4; i++) {
		flopi[i].main_state = IDLE;
		flopi[i].sub_state = IDLE;
		flopi[i].live = false;
		flopi[i].ready = !ready_polled;
		flopi[i].irq = floppy_info::IRQ_NONE;
	}
	data_irq = false;
	polled_irq = false;
	internal_drq = false;
	fifo_pos = 0;
	command_pos = 0;
	result_pos = 0;
	if(!locked)
		fifocfg = FIF_DIS;
	cur_live.fi = 0;
	drq = false;
	cur_live.tm = attotime::never;
	cur_live.state = IDLE;
	cur_live.next_state = -1;
	cur_live.fi = NULL;
	tc_done = false;
	st0 = st1 = st2 = st3 = 0x00;

	check_irq();
	if(ready_polled)
		poll_timer->adjust(attotime::from_usec(1024), 0, attotime::from_usec(1024));
}

void upd765_family_device::tc_w(bool _tc)
{
	logerror("%s: tc=%d\n", tag(), _tc);
	if(tc != _tc && _tc) {
		live_sync();
		tc_done = true;
		tc = _tc;
		if(cur_live.fi)
			general_continue(*cur_live.fi);
	} else
		tc = _tc;
}

void upd765_family_device::ready_w(bool _ready)
{
	external_ready = _ready;
}

bool upd765_family_device::get_ready(int fid)
{
	if(ready_connected)
		return flopi[fid].dev ? !flopi[fid].dev->ready_r() : false;
	return external_ready;
}

void upd765_family_device::set_floppy(floppy_image_device *flop)
{
	for(int fid=0; fid<4; fid++) {
		if(flopi[fid].dev)
			flopi[fid].dev->setup_index_pulse_cb(floppy_image_device::index_pulse_cb());
		flopi[fid].dev = flop;
	}
	if(flop)
		flop->setup_index_pulse_cb(floppy_image_device::index_pulse_cb(FUNC(upd765_family_device::index_callback), this));
}

READ8_MEMBER(upd765_family_device::sra_r)
{
	UINT8 sra = 0;
	int fid = dor & 3;
	floppy_info &fi = flopi[fid];
	if(fi.dir)
		sra |= 0x01;
	if(fi.index)
		sra |= 0x04;
	if(cur_rate >= 500000)
		sra |= 0x08;
	if(fi.dev && fi.dev->trk00_r())
		sra |= 0x10;
	if(fi.main_state == SEEK_WAIT_STEP_SIGNAL_TIME)
		sra |= 0x20;
	sra |= 0x40;
	if(cur_irq)
		sra |= 0x80;
	if(mode == MODE_M30)
		sra ^= 0x1f;
	return sra;
}

READ8_MEMBER(upd765_family_device::srb_r)
{
	return 0;
}

READ8_MEMBER(upd765_family_device::dor_r)
{
	return dor;
}

WRITE8_MEMBER(upd765_family_device::dor_w)
{
	logerror("%s: dor = %02x\n", tag(), data);
	UINT8 diff = dor ^ data;
	dor = data;
	if(diff & 4)
		soft_reset();

	for(int i=0; i<4; i++) {
		floppy_info &fi = flopi[i];
		if(fi.dev)
			fi.dev->mon_w(!(dor & (0x10 << i)));
	}
	check_irq();
}

READ8_MEMBER(upd765_family_device::tdr_r)
{
	return 0;
}

WRITE8_MEMBER(upd765_family_device::tdr_w)
{
}

READ8_MEMBER(upd765_family_device::msr_r)
{
	UINT32 msr = 0;
	switch(main_phase) {
	case PHASE_CMD:
		msr |= MSR_RQM;
		if(command_pos)
			msr |= MSR_CB;
		break;
	case PHASE_EXEC:
		msr |= MSR_CB;
		if(spec & SPEC_ND)
			msr |= MSR_EXM;
		if(internal_drq) {
			msr |= MSR_RQM;
			if(!fifo_write)
				msr |= MSR_DIO;
		}
		break;

	case PHASE_RESULT:
		msr |= MSR_RQM|MSR_DIO|MSR_CB;
		break;
	}
	for(int i=0; i<4; i++)
		if(flopi[i].main_state == RECALIBRATE || flopi[i].main_state == SEEK) {
			msr |= 1<<i;
			msr |= MSR_CB;
		}

	if(data_irq) {
		data_irq = false;
		check_irq();
	}

	return msr;
}

WRITE8_MEMBER(upd765_family_device::dsr_w)
{
	logerror("%s: dsr_w %02x\n", tag(), data);
	if(data & 0x80)
		soft_reset();
	dsr = data & 0x7f;
	cur_rate = rates[dsr & 3];
}

void upd765_family_device::set_rate(int rate)
{
	cur_rate = rate;
}

READ8_MEMBER(upd765_family_device::fifo_r)
{
	UINT8 r = 0;
	switch(main_phase) {
	case PHASE_EXEC:
		if(internal_drq)
			return fifo_pop(false);
		logerror("%s: fifo_r in phase %d\n", tag(), main_phase);
		break;

	case PHASE_RESULT:
		r = result[0];
		result_pos--;
		memmove(result, result+1, result_pos);
		if(!result_pos)
			main_phase = PHASE_CMD;
		break;
	default:
		logerror("%s: fifo_r in phase %d\n", tag(), main_phase);
		break;
	}

	return r;
}

WRITE8_MEMBER(upd765_family_device::fifo_w)
{
	switch(main_phase) {
	case PHASE_CMD: {
		command[command_pos++] = data;
		int cmd = check_command();
		if(cmd == C_INCOMPLETE)
			break;
		if(cmd == C_INVALID) {
			logerror("%s: Invalid on %02x\n", tag(), command[0]);
			main_phase = PHASE_RESULT;
			result[0] = 0x80;
			result_pos = 1;
			return;
		}
		start_command(cmd);
		break;
	}
	case PHASE_EXEC:
		if(internal_drq) {
			fifo_push(data, false);
			return;
		}
		logerror("%s: fifo_w in phase %d\n", tag(), main_phase);
		break;

	default:
		logerror("%s: fifo_w in phase %d\n", tag(), main_phase);
		break;
	}
}

UINT8 upd765_family_device::do_dir_r()
{
	floppy_info &fi = flopi[dor & 3];
	if(fi.dev)
		return fi.dev->dskchg_r() ? 0x00 : 0x80;
	return 0x00;
}

READ8_MEMBER(upd765_family_device::dir_r)
{
	return do_dir_r();
}

WRITE8_MEMBER(upd765_family_device::ccr_w)
{
	dsr = (dsr & 0xfc) | (data & 3);
	cur_rate = rates[data & 3];
}

void upd765_family_device::set_drq(bool state)
{
	if(state != drq) {
		drq = state;
		if(!drq_cb.isnull())
			drq_cb(drq);
	}
}

bool upd765_family_device::get_drq() const
{
	return drq;
}

void upd765_family_device::enable_transfer()
{
	if(spec & SPEC_ND) {
		// PIO
		if(!internal_drq) {
			internal_drq = true;
			check_irq();
		}

	} else {
		// DMA
		if(!drq)
			set_drq(true);
	}
}

void upd765_family_device::disable_transfer()
{
	if(spec & SPEC_ND) {
		internal_drq = false;
		check_irq();
	} else
		set_drq(false);
}

void upd765_family_device::fifo_push(UINT8 data, bool internal)
{
	if(fifo_pos == 16) {
		if(internal) {
			if(!(st1 & ST1_OR))
				logerror("%s: Fifo overrun\n", tag());
			st1 |= ST1_OR;
		}
		return;
	}
	fifo[fifo_pos++] = data;
	fifo_expected--;

	int thr = (fifocfg & FIF_THR)+1;
	if(!fifo_write && (!fifo_expected || fifo_pos >= thr || (fifocfg & FIF_DIS)))
		enable_transfer();
	if(fifo_write && (fifo_pos == 16 || !fifo_expected))
		disable_transfer();
}


UINT8 upd765_family_device::fifo_pop(bool internal)
{
	if(!fifo_pos) {
		if(internal) {
			if(!(st1 & ST1_OR))
				logerror("%s: Fifo underrun\n", tag());
			st1 |= ST1_OR;
		}
		return 0;
	}
	UINT8 r = fifo[0];
	fifo_pos--;
	memmove(fifo, fifo+1, fifo_pos);
	if(!fifo_write && !fifo_pos)
		disable_transfer();
	int thr = fifocfg & 15;
	if(fifo_write && fifo_expected && (fifo_pos <= thr || (fifocfg & 0x20)))
		enable_transfer();
	return r;
}

void upd765_family_device::fifo_expect(int size, bool write)
{
	fifo_expected = size;
	fifo_write = write;
	if(fifo_write)
		enable_transfer();
}

READ8_MEMBER(upd765_family_device::mdma_r)
{
	return dma_r();
}

WRITE8_MEMBER(upd765_family_device::mdma_w)
{
	dma_w(data);
}

UINT8 upd765_family_device::dma_r()
{
	return fifo_pop(false);
}

void upd765_family_device::dma_w(UINT8 data)
{
	fifo_push(data, false);
}

void upd765_family_device::live_start(floppy_info &fi, int state)
{
	cur_live.tm = machine().time();
	cur_live.state = state;
	cur_live.next_state = -1;
	cur_live.fi = &fi;
	cur_live.shift_reg = 0;
	cur_live.crc = 0xffff;
	cur_live.bit_counter = 0;
	cur_live.data_separator_phase = false;
	cur_live.data_reg = 0;
	cur_live.previous_type = live_info::PT_NONE;
	cur_live.data_bit_context = false;
	cur_live.byte_counter = 0;
	cur_live.pll.reset(cur_live.tm);
	cur_live.pll.set_clock(attotime::from_hz(cur_rate*2));
	checkpoint_live = cur_live;
	fi.live = true;

	live_run();
}

void upd765_family_device::checkpoint()
{
	if(cur_live.fi)
		cur_live.pll.commit(cur_live.fi->dev, cur_live.tm);
	checkpoint_live = cur_live;
}

void upd765_family_device::rollback()
{
	cur_live = checkpoint_live;
}

void upd765_family_device::live_delay(int state)
{
	cur_live.next_state = state;
	if(cur_live.tm != machine().time())
		cur_live.fi->tm->adjust(cur_live.tm - machine().time());
	else
		live_sync();
}

void upd765_family_device::live_sync()
{
	if(!cur_live.tm.is_never()) {
		if(cur_live.tm > machine().time()) {
			rollback();
			live_run(machine().time());
			cur_live.pll.commit(cur_live.fi->dev, cur_live.tm);
		} else {
			cur_live.pll.commit(cur_live.fi->dev, cur_live.tm);
			if(cur_live.next_state != -1) {
				cur_live.state = cur_live.next_state;
				cur_live.next_state = -1;
			}
			if(cur_live.state == IDLE) {
				cur_live.pll.stop_writing(cur_live.fi->dev, cur_live.tm);
				cur_live.tm = attotime::never;
				cur_live.fi->live = false;
				cur_live.fi = 0;
			}
		}
		cur_live.next_state = -1;
		checkpoint();
	}
}

void upd765_family_device::live_abort()
{
	if(!cur_live.tm.is_never() && cur_live.tm > machine().time()) {
		rollback();
		live_run(machine().time());
	}

	if(cur_live.fi) {
		cur_live.pll.stop_writing(cur_live.fi->dev, cur_live.tm);
		cur_live.fi->live = false;
		cur_live.fi = 0;
	}

	cur_live.tm = attotime::never;
	cur_live.state = IDLE;
	cur_live.next_state = -1;
}

void upd765_family_device::live_run(attotime limit)
{
	if(cur_live.state == IDLE || cur_live.next_state != -1)
		return;

	if(limit == attotime::never) {
		if(cur_live.fi->dev)
			limit = cur_live.fi->dev->time_next_index();
		if(limit == attotime::never) {
			// Happens when there's no disk or if the fdc is not
			// connected to a drive, hence no index pulse. Force a
			// sync from time to time in that case, so that the main
			// cpu timeout isn't too painful.  Avoids looping into
			// infinity looking for data too.

			limit = machine().time() + attotime::from_msec(1);
			cur_live.fi->tm->adjust(attotime::from_msec(1));
		}
	}

	for(;;) {

		switch(cur_live.state) {
		case SEARCH_ADDRESS_MARK_HEADER:
			if(read_one_bit(limit))
				return;
#if 0
			fprintf(stderr, "%s: shift = %04x data=%02x c=%d\n", tts(cur_live.tm).cstr(), cur_live.shift_reg,
					(cur_live.shift_reg & 0x4000 ? 0x80 : 0x00) |
					(cur_live.shift_reg & 0x1000 ? 0x40 : 0x00) |
					(cur_live.shift_reg & 0x0400 ? 0x20 : 0x00) |
					(cur_live.shift_reg & 0x0100 ? 0x10 : 0x00) |
					(cur_live.shift_reg & 0x0040 ? 0x08 : 0x00) |
					(cur_live.shift_reg & 0x0010 ? 0x04 : 0x00) |
					(cur_live.shift_reg & 0x0004 ? 0x02 : 0x00) |
					(cur_live.shift_reg & 0x0001 ? 0x01 : 0x00),
					cur_live.bit_counter);
#endif

			if(cur_live.shift_reg == 0x4489) {
				cur_live.crc = 0x443b;
				cur_live.data_separator_phase = false;
				cur_live.bit_counter = 0;
				cur_live.state = READ_HEADER_BLOCK_HEADER;
			}
			break;

		case READ_HEADER_BLOCK_HEADER: {
			if(read_one_bit(limit))
				return;
#if 0
			fprintf(stderr, "%s: shift = %04x data=%02x counter=%d\n", tts(cur_live.tm).cstr(), cur_live.shift_reg,
					(cur_live.shift_reg & 0x4000 ? 0x80 : 0x00) |
					(cur_live.shift_reg & 0x1000 ? 0x40 : 0x00) |
					(cur_live.shift_reg & 0x0400 ? 0x20 : 0x00) |
					(cur_live.shift_reg & 0x0100 ? 0x10 : 0x00) |
					(cur_live.shift_reg & 0x0040 ? 0x08 : 0x00) |
					(cur_live.shift_reg & 0x0010 ? 0x04 : 0x00) |
					(cur_live.shift_reg & 0x0004 ? 0x02 : 0x00) |
					(cur_live.shift_reg & 0x0001 ? 0x01 : 0x00),
					cur_live.bit_counter);
#endif
			if(cur_live.bit_counter & 15)
				break;

			int slot = cur_live.bit_counter >> 4;

			if(slot < 3) {
				if(cur_live.shift_reg != 0x4489)
					cur_live.state = SEARCH_ADDRESS_MARK_HEADER;
				break;
			}
			if(cur_live.data_reg != 0xfe) {
				cur_live.state = SEARCH_ADDRESS_MARK_HEADER;
				break;
			}

			cur_live.bit_counter = 0;
			cur_live.state = READ_ID_BLOCK;

			break;
		}

		case SEARCH_ADDRESS_MARK_HEADER_FM:
			if(read_one_bit(limit))
				return;
#if 0
			fprintf(stderr, "%s: shift = %04x data=%02x c=%d\n", tts(cur_live.tm).cstr(), cur_live.shift_reg,
					(cur_live.shift_reg & 0x4000 ? 0x80 : 0x00) |
					(cur_live.shift_reg & 0x1000 ? 0x40 : 0x00) |
					(cur_live.shift_reg & 0x0400 ? 0x20 : 0x00) |
					(cur_live.shift_reg & 0x0100 ? 0x10 : 0x00) |
					(cur_live.shift_reg & 0x0040 ? 0x08 : 0x00) |
					(cur_live.shift_reg & 0x0010 ? 0x04 : 0x00) |
					(cur_live.shift_reg & 0x0004 ? 0x02 : 0x00) |
					(cur_live.shift_reg & 0x0001 ? 0x01 : 0x00),
					cur_live.bit_counter);
#endif

			if(cur_live.shift_reg == 0xf57e) {
				cur_live.crc = 0xef21;
				cur_live.data_separator_phase = false;
				cur_live.bit_counter = 0;
				cur_live.state = READ_ID_BLOCK;
			}
			break;

		case READ_ID_BLOCK: {
			if(read_one_bit(limit))
				return;
			if(cur_live.bit_counter & 15)
				break;
			int slot = (cur_live.bit_counter >> 4)-1;

			if(0)
				fprintf(stderr, "%s: slot=%d data=%02x crc=%04x\n", tts(cur_live.tm).cstr(), slot, cur_live.data_reg, cur_live.crc);
			cur_live.idbuf[slot] = cur_live.data_reg;
			if(slot == 5) {
				live_delay(IDLE);
				return;
			}
			break;
		}

		case SEARCH_ADDRESS_MARK_DATA:
			if(read_one_bit(limit))
				return;
#if 0
			fprintf(stderr, "%s: shift = %04x data=%02x c=%d.%x\n", tts(cur_live.tm).cstr(), cur_live.shift_reg,
					(cur_live.shift_reg & 0x4000 ? 0x80 : 0x00) |
					(cur_live.shift_reg & 0x1000 ? 0x40 : 0x00) |
					(cur_live.shift_reg & 0x0400 ? 0x20 : 0x00) |
					(cur_live.shift_reg & 0x0100 ? 0x10 : 0x00) |
					(cur_live.shift_reg & 0x0040 ? 0x08 : 0x00) |
					(cur_live.shift_reg & 0x0010 ? 0x04 : 0x00) |
					(cur_live.shift_reg & 0x0004 ? 0x02 : 0x00) |
					(cur_live.shift_reg & 0x0001 ? 0x01 : 0x00),
					cur_live.bit_counter >> 4, cur_live.bit_counter & 15);
#endif
			// Large tolerance due to perpendicular recording at extended density
			if(cur_live.bit_counter > 62*16) {
				live_delay(SEARCH_ADDRESS_MARK_DATA_FAILED);
				return;
			}

			if(cur_live.bit_counter >= 28*16 && cur_live.shift_reg == 0x4489) {
				cur_live.crc = 0x443b;
				cur_live.data_separator_phase = false;
				cur_live.bit_counter = 0;
				cur_live.state = READ_DATA_BLOCK_HEADER;
			}
			break;

		case READ_DATA_BLOCK_HEADER: {
			if(read_one_bit(limit))
				return;
#if 0
			fprintf(stderr, "%s: shift = %04x data=%02x counter=%d\n", tts(cur_live.tm).cstr(), cur_live.shift_reg,
					(cur_live.shift_reg & 0x4000 ? 0x80 : 0x00) |
					(cur_live.shift_reg & 0x1000 ? 0x40 : 0x00) |
					(cur_live.shift_reg & 0x0400 ? 0x20 : 0x00) |
					(cur_live.shift_reg & 0x0100 ? 0x10 : 0x00) |
					(cur_live.shift_reg & 0x0040 ? 0x08 : 0x00) |
					(cur_live.shift_reg & 0x0010 ? 0x04 : 0x00) |
					(cur_live.shift_reg & 0x0004 ? 0x02 : 0x00) |
					(cur_live.shift_reg & 0x0001 ? 0x01 : 0x00),
					cur_live.bit_counter);
#endif
			if(cur_live.bit_counter & 15)
				break;

			int slot = cur_live.bit_counter >> 4;

			if(slot < 3) {
				if(cur_live.shift_reg != 0x4489) {
					live_delay(SEARCH_ADDRESS_MARK_DATA_FAILED);
					return;
				}
				break;
			}
			if(cur_live.data_reg != 0xfb && cur_live.data_reg != 0xfd) {
				live_delay(SEARCH_ADDRESS_MARK_DATA_FAILED);
				return;
			}

			cur_live.bit_counter = 0;
			cur_live.state = READ_SECTOR_DATA;
			break;
		}

		case SEARCH_ADDRESS_MARK_DATA_FAILED:
			st1 |= ST1_MA;
			st2 |= ST2_MD;
			cur_live.state = IDLE;
			return;

		case SEARCH_ADDRESS_MARK_DATA_FM:
			if(read_one_bit(limit))
				return;
#if 0
			fprintf(stderr, "%s: shift = %04x data=%02x c=%d.%x\n", tts(cur_live.tm).cstr(), cur_live.shift_reg,
					(cur_live.shift_reg & 0x4000 ? 0x80 : 0x00) |
					(cur_live.shift_reg & 0x1000 ? 0x40 : 0x00) |
					(cur_live.shift_reg & 0x0400 ? 0x20 : 0x00) |
					(cur_live.shift_reg & 0x0100 ? 0x10 : 0x00) |
					(cur_live.shift_reg & 0x0040 ? 0x08 : 0x00) |
					(cur_live.shift_reg & 0x0010 ? 0x04 : 0x00) |
					(cur_live.shift_reg & 0x0004 ? 0x02 : 0x00) |
					(cur_live.shift_reg & 0x0001 ? 0x01 : 0x00),
					cur_live.bit_counter >> 4, cur_live.bit_counter & 15);
#endif
			if(cur_live.bit_counter > 23*16) {
				live_delay(SEARCH_ADDRESS_MARK_DATA_FAILED);
				return;
			}

			if(cur_live.bit_counter >= 11*16 && (cur_live.shift_reg == 0xf56a || cur_live.shift_reg == 0xf56f)) {
				cur_live.crc = cur_live.shift_reg == 0xf56a ? 0x8fe7 : 0xbf84;
				cur_live.data_separator_phase = false;
				cur_live.bit_counter = 0;
				cur_live.state = READ_SECTOR_DATA;
			}
			break;

		case READ_SECTOR_DATA: {
			if(read_one_bit(limit))
				return;
			if(cur_live.bit_counter & 15)
				break;
			int slot = (cur_live.bit_counter >> 4)-1;
			if(slot < sector_size) {
				// Sector data
				live_delay(READ_SECTOR_DATA_BYTE);
				return;

			} else if(slot < sector_size+2) {
				// CRC
				if(slot == sector_size+1) {
					live_delay(IDLE);
					return;
				}
			}
			break;
		}

		case READ_SECTOR_DATA_BYTE:
			if(!tc_done)
				fifo_push(cur_live.data_reg, true);
			cur_live.state = READ_SECTOR_DATA;
			checkpoint();
			break;

		case WRITE_SECTOR_SKIP_GAP2:
			cur_live.bit_counter = 0;
			cur_live.byte_counter = 0;
			cur_live.state = WRITE_SECTOR_SKIP_GAP2_BYTE;
			checkpoint();
			break;

		case WRITE_SECTOR_SKIP_GAP2_BYTE:
			if(read_one_bit(limit))
				return;
			if(cur_live.bit_counter != 22*16)
				break;
			cur_live.bit_counter = 0;
			cur_live.byte_counter = 0;
			live_delay(WRITE_SECTOR_DATA);
			return;

		case WRITE_SECTOR_DATA:
			if(cur_live.byte_counter < 12)
				live_write_mfm(0x00);
			else if(cur_live.byte_counter < 15)
				live_write_raw(0x4489);
			else if(cur_live.byte_counter < 16) {
				cur_live.crc = 0xcdb4;
				live_write_mfm(command[0] & 0x08 ? 0xf8 : 0xfb);
			} else if(cur_live.byte_counter < 16+sector_size)
				live_write_mfm(tc_done && !fifo_pos? 0x00 : fifo_pop(true));
			else if(cur_live.byte_counter < 16+sector_size+2)
				live_write_mfm(cur_live.crc >> 8);
			else if(cur_live.byte_counter < 16+sector_size+2+command[7])
				live_write_mfm(0x4e);
			else {
				cur_live.pll.stop_writing(cur_live.fi->dev, cur_live.tm);
				cur_live.state = IDLE;
				return;
			}
			cur_live.state = WRITE_SECTOR_DATA_BYTE;
			cur_live.bit_counter = 16;
			checkpoint();
			break;

		case WRITE_TRACK_PRE_SECTORS:
			if(!cur_live.byte_counter && command[3])
				fifo_expect(4, true);
			if(cur_live.byte_counter < 80)
				live_write_mfm(0x4e);
			else if(cur_live.byte_counter < 92)
				live_write_mfm(0x00);
			else if(cur_live.byte_counter < 95)
				live_write_raw(0x5224);
			else if(cur_live.byte_counter < 96)
				live_write_mfm(0xfc);
			else if(cur_live.byte_counter < 146)
				live_write_mfm(0x4e);
			else {
				cur_live.state = WRITE_TRACK_SECTOR;
				cur_live.byte_counter = 0;
				break;
			}
			cur_live.state = WRITE_TRACK_PRE_SECTORS_BYTE;
			cur_live.bit_counter = 16;
			checkpoint();
			break;

		case WRITE_TRACK_SECTOR:
			if(!cur_live.byte_counter) {
				command[3]--;
				if(command[3])
					fifo_expect(4, true);
			}
			if(cur_live.byte_counter < 12)
				live_write_mfm(0x00);
			else if(cur_live.byte_counter < 15)
				live_write_raw(0x4489);
			else if(cur_live.byte_counter < 16) {
				cur_live.crc = 0xcdb4;
				live_write_mfm(0xfe);
			} else if(cur_live.byte_counter < 20)
				live_write_mfm(fifo_pop(true));
			else if(cur_live.byte_counter < 22)
				live_write_mfm(cur_live.crc >> 8);
			else if(cur_live.byte_counter < 44)
				live_write_mfm(0x4e);
			else if(cur_live.byte_counter < 56)
				live_write_mfm(0x00);
			else if(cur_live.byte_counter < 59)
				live_write_raw(0x4489);
			else if(cur_live.byte_counter < 60) {
				cur_live.crc = 0xcdb4;
				live_write_mfm(0xfb);
			} else if(cur_live.byte_counter < 60+sector_size)
				live_write_mfm(command[5]);
			else if(cur_live.byte_counter < 62+sector_size)
				live_write_mfm(cur_live.crc >> 8);
			else if(cur_live.byte_counter < 62+sector_size+command[4])
				live_write_mfm(0x4e);
			else {
				cur_live.byte_counter = 0;
				cur_live.state = command[3] ? WRITE_TRACK_SECTOR : WRITE_TRACK_POST_SECTORS;
				break;
			}
			cur_live.state = WRITE_TRACK_SECTOR_BYTE;
			cur_live.bit_counter = 16;
			checkpoint();
			break;

		case WRITE_TRACK_POST_SECTORS:
			live_write_mfm(0x4e);
			cur_live.state = WRITE_TRACK_POST_SECTORS_BYTE;
			cur_live.bit_counter = 16;
			checkpoint();
			break;

		case WRITE_TRACK_PRE_SECTORS_BYTE:
		case WRITE_TRACK_SECTOR_BYTE:
		case WRITE_TRACK_POST_SECTORS_BYTE:
		case WRITE_SECTOR_DATA_BYTE:
			if(write_one_bit(limit))
				return;
			if(cur_live.bit_counter == 0) {
				cur_live.byte_counter++;
				live_delay(cur_live.state-1);
				return;
			}
			break;

		default:
			logerror("%s: Unknown live state %d\n", tts(cur_live.tm).cstr(), cur_live.state);
			return;
		}
	}
}

int upd765_family_device::check_command()
{
	// 0.000010 read track
	// 00000011 specify
	// 00000100 sense drive status
	// ..000101 write data
	// ...00110 read data
	// 00000111 recalibrate
	// 00001000 sense interrupt status
	// ..001001 write deleted data
	// 0.001010 read id
	// ...01100 read deleted data
	// 0.001101 format track
	// 00001110 dumpreg
	// 00101110 save
	// 01001110 restore
	// 10001110 drive specification command
	// 00001111 seek
	// 1.001111 relative seek
	// 00010000 version
	// ...10001 scan equal
	// 00010010 perpendicular mode
	// 00010011 configure
	// 00110011 option
	// .0010100 lock
	// ...10110 verify
	// 00010111 powerdown mode
	// 00011000 part id
	// ...11001 scan low or equal
	// ...11101 scan high or equal

	// MSDOS 6.22 format uses 0xcd to format a track, which makes one
	// think only the bottom 5 bits are decoded.

	switch(command[0] & 0x1f) {
	case 0x02:
		return command_pos == 9 ? C_READ_TRACK         : C_INCOMPLETE;

	case 0x03:
		return command_pos == 3 ? C_SPECIFY            : C_INCOMPLETE;

	case 0x04:
		return command_pos == 2 ? C_SENSE_DRIVE_STATUS : C_INCOMPLETE;

	case 0x05:
	case 0x09:
		return command_pos == 9 ? C_WRITE_DATA         : C_INCOMPLETE;

	case 0x06:
	case 0x0c:
		return command_pos == 9 ? C_READ_DATA          : C_INCOMPLETE;

	case 0x07:
		return command_pos == 2 ? C_RECALIBRATE        : C_INCOMPLETE;

	case 0x08:
		return C_SENSE_INTERRUPT_STATUS;

	case 0x0a:
		return command_pos == 2 ? C_READ_ID            : C_INCOMPLETE;

	case 0x0d:
		return command_pos == 6 ? C_FORMAT_TRACK       : C_INCOMPLETE;

	case 0x0e:
		return C_DUMP_REG;

	case 0x0f:
		return command_pos == 3 ? C_SEEK               : C_INCOMPLETE;

	case 0x12:
		return command_pos == 2 ? C_PERPENDICULAR      : C_INCOMPLETE;

	case 0x13:
		return command_pos == 4 ? C_CONFIGURE          : C_INCOMPLETE;

	case 0x14:
		return C_LOCK;

	default:
		return C_INVALID;
	}
}

void upd765_family_device::start_command(int cmd)
{
	command_pos = 0;
	result_pos = 0;
	main_phase = PHASE_EXEC;
	tc_done = false;
	switch(cmd) {
	case C_CONFIGURE:
		logerror("%s: command configure %02x %02x %02x\n",
				 tag(),
				 command[1], command[2], command[3]);
		// byte 1 is ignored, byte 3 is precompensation-related
		fifocfg = command[2];
		precomp = command[3];
		main_phase = PHASE_CMD;
		break;

	case C_DUMP_REG:
		logerror("%s: command dump regs\n", tag());
		main_phase = PHASE_RESULT;
		result[0] = flopi[0].pcn;
		result[1] = flopi[1].pcn;
		result[2] = flopi[2].pcn;
		result[3] = flopi[3].pcn;
		result[4] = (spec & 0xff00) >> 8;
		result[5] = (spec & 0x00ff);
		result[6] = sector_size;
		result[7] = locked ? 0x80 : 0x00;
		result[7] |= (perpmode & 0x30);
		result[8] = fifocfg;
		result[9] = precomp;
		result_pos = 10;
		break;

	case C_FORMAT_TRACK:
		format_track_start(flopi[command[1] & 3]);
		break;

	case C_LOCK:
		locked = command[0] & 0x80;
		main_phase = PHASE_RESULT;
		result[0] = locked ? 0x10 : 0x00;
		result_pos = 1;
		logerror("%s: command lock (%s)\n", tag(), locked ? "on" : "off");
		break;

	case C_PERPENDICULAR:
		logerror("%s: command perpendicular\n", tag());
		perpmode = command[1];
		main_phase = PHASE_CMD;
		break;

	case C_READ_DATA:
		read_data_start(flopi[command[1] & 3]);
		break;

	case C_READ_ID:
		read_id_start(flopi[command[1] & 3]);
		break;

	case C_READ_TRACK:
		read_track_start(flopi[command[1] & 3]);
		break;

	case C_RECALIBRATE:
		recalibrate_start(flopi[command[1] & 3]);
		main_phase = PHASE_CMD;
		break;

	case C_SEEK:
		seek_start(flopi[command[1] & 3]);
		main_phase = PHASE_CMD;
		break;

	case C_SENSE_DRIVE_STATUS: {
		floppy_info &fi = flopi[command[1] & 3];
		main_phase = PHASE_RESULT;
		result[0] = 0x08;
		if(get_ready(fi.id))
			result[0] |= 0x20;
		if(fi.dev)
			result[0] |=
				(fi.dev->wpt_r() ? 0x40 : 0x00) |
				(fi.dev->trk00_r() ? 0x00 : 0x10) |
				(fi.dev->ss_r() ? 0x04 : 0x00) |
				(command[1] & 3);
		logerror("%s: command sense drive status (%02x)\n", tag(), result[0]);
		result_pos = 1;
		break;
	}

	case C_SENSE_INTERRUPT_STATUS: {
		main_phase = PHASE_RESULT;

		int fid;
		for(fid=0; fid<4 && flopi[fid].irq == floppy_info::IRQ_NONE; fid++);
		if(fid == 4) {
			st0 = ST0_UNK;
			result[0] = st0;
			result[1] = 0x00;
			result_pos = 2;
			logerror("%s: command sense interrupt status (%02x %02x)\n", tag(), result[0], result[1]);
			break;
		}
		floppy_info &fi = flopi[fid];
		if(fi.irq == floppy_info::IRQ_POLLED) {
			// Documentation is somewhat contradictory w.r.t polling
			// and irq.  PC bios, especially 5150, requires that only
			// one irq happens.  That's also wait the ns82077a doc
			// says it does.  OTOH, a number of docs says you need to
			// call SIS 4 times, once per drive...
			//
			// There's also the interaction with the seek irq.  The
			// somewhat borderline tf20 code seems to think that
			// essentially ignoring the polling irq should work.
			//
			// So at that point the best bet seems to drop the
			// "polled" irq as soon as a SIS happens, and override any
			// polled-in-waiting information when a seek irq happens
			// for a given floppy.

			st0 = ST0_ABRT | fid;
		}
		fi.irq = floppy_info::IRQ_NONE;

		polled_irq = false;

		result[0] = st0;
		result[1] = fi.pcn;
		logerror("%s: command sense interrupt status (fid=%d %02x %02x)\n", tag(), fid, result[0], result[1]);
		result_pos = 2;

		check_irq();
		break;
	}

	case C_SPECIFY:
		logerror("%s: command specify %02x %02x\n",
				 tag(),
				 command[1], command[2]);
		spec = (command[1] << 8) | command[2];
		main_phase = PHASE_CMD;
		break;

	case C_WRITE_DATA:
		write_data_start(flopi[command[1] & 3]);
		break;

	default:
		fprintf(stderr, "start command %d\n", cmd);
		exit(1);
	}
}

void upd765_family_device::command_end(floppy_info &fi, bool data_completion)
{
	logerror("%s: command done (%s) -", tag(), data_completion ? "data" : "seek");
	for(int i=0; i != result_pos; i++)
		logerror(" %02x", result[i]);
	logerror("\n");
	fi.main_state = fi.sub_state = IDLE;
	if(data_completion)
		data_irq = true;
	else
		fi.irq = floppy_info::IRQ_SEEK;
	check_irq();
}

void upd765_family_device::recalibrate_start(floppy_info &fi)
{
	logerror("%s: command recalibrate\n", tag());
	fi.main_state = RECALIBRATE;
	fi.sub_state = SEEK_WAIT_STEP_TIME_DONE;
	fi.dir = 1;
	fi.counter = 77;
	seek_continue(fi);
}

void upd765_family_device::seek_start(floppy_info &fi)
{
	logerror("%s: command %sseek %d\n", tag(), command[0] & 0x80 ? "relative " : "", command[2]);
	fi.main_state = SEEK;
	fi.sub_state = SEEK_WAIT_STEP_TIME_DONE;
	fi.dir = fi.pcn > command[2] ? 1 : 0;
	seek_continue(fi);
}

void upd765_family_device::delay_cycles(emu_timer *tm, int cycles)
{
	tm->adjust(attotime::from_double(double(cycles)/cur_rate));
}

void upd765_family_device::seek_continue(floppy_info &fi)
{
	for(;;) {
		switch(fi.sub_state) {
		case SEEK_MOVE:
			if(fi.dev) {
				fi.dev->dir_w(fi.dir);
				fi.dev->stp_w(0);
			}
			fi.sub_state = SEEK_WAIT_STEP_SIGNAL_TIME;
			fi.tm->adjust(attotime::from_nsec(2500));
			return;

		case SEEK_WAIT_STEP_SIGNAL_TIME:
			return;

		case SEEK_WAIT_STEP_SIGNAL_TIME_DONE:
			if(fi.dev)
				fi.dev->stp_w(1);

			if(fi.main_state == SEEK) {
				if(fi.pcn > command[2])
					fi.pcn--;
				else
					fi.pcn++;
			}
			fi.sub_state = SEEK_WAIT_STEP_TIME;
			delay_cycles(fi.tm, 500*(16-(spec >> 12)));
			return;

		case SEEK_WAIT_STEP_TIME:
			return;

		case SEEK_WAIT_STEP_TIME_DONE: {
			bool done = false;
			switch(fi.main_state) {
			case RECALIBRATE:
				fi.counter--;
				done = !fi.dev || !fi.dev->trk00_r();
				if(done)
					fi.pcn = 0;
				else if(!fi.counter) {
					st0 = ST0_FAIL|ST0_SE|ST0_EC;
					command_end(fi, false);
					return;
				}
				break;
			case SEEK:
				done = fi.pcn == command[2];
				break;
			}
			if(done) {
				st0 = ST0_SE;
				command_end(fi, false);
				return;
			}
			fi.sub_state = SEEK_MOVE;
			break;
		}
		}
	}
}

void upd765_family_device::read_data_start(floppy_info &fi)
{
	fi.main_state = READ_DATA;
	fi.sub_state = HEAD_LOAD_DONE;
	logerror("%s: command read%s data%s%s%s%s cmd=%02x sel=%x chrn=(%d, %d, %d, %d) eot=%02x gpl=%02x dtl=%02x rate=%d\n",
			 tag(),
			 command[0] & 0x08 ? " deleted" : "",
			 command[0] & 0x80 ? " mt" : "",
			 command[0] & 0x40 ? " mfm" : "",
			 command[0] & 0x20 ? " sk" : "",
			 fifocfg & 0x40 ? " seek" : "",
			 command[0],
			 command[1],
			 command[2],
			 command[3],
			 command[4],
			 128 << (command[5] & 7),
			 command[6],
			 command[7],
			 command[8],
			 cur_rate);

	st0 = command[1] & 7;
	st1 = ST1_MA;
	st2 = 0x00;

	if(fi.dev)
		fi.dev->ss_w(command[1] & 4 ? 1 : 0);
	read_data_continue(fi);
}

void upd765_family_device::read_data_continue(floppy_info &fi)
{
	for(;;) {
		switch(fi.sub_state) {
		case HEAD_LOAD_DONE:
			if(fi.pcn == command[2] || !(fifocfg & 0x40)) {
				fi.sub_state = SEEK_DONE;
				break;
			}
			st0 |= ST0_SE;
			if(fi.dev) {
				fi.dev->dir_w(fi.pcn > command[2] ? 1 : 0);
				fi.dev->stp_w(0);
			}
			fi.sub_state = SEEK_WAIT_STEP_SIGNAL_TIME;
			fi.tm->adjust(attotime::from_nsec(2500));
			return;

		case SEEK_WAIT_STEP_SIGNAL_TIME:
			return;

		case SEEK_WAIT_STEP_SIGNAL_TIME_DONE:
			if(fi.dev)
				fi.dev->stp_w(1);

			fi.sub_state = SEEK_WAIT_STEP_TIME;
			delay_cycles(fi.tm, 500*(16-(spec >> 12)));
			return;

		case SEEK_WAIT_STEP_TIME:
			return;

		case SEEK_WAIT_STEP_TIME_DONE:
			if(fi.pcn > command[2])
				fi.pcn--;
			else
				fi.pcn++;
			fi.sub_state = HEAD_LOAD_DONE;
			break;

		case SEEK_DONE:
			fi.counter = 0;
			fi.sub_state = SCAN_ID;
			live_start(fi, command[0] & 0x40 ? SEARCH_ADDRESS_MARK_HEADER : SEARCH_ADDRESS_MARK_HEADER_FM);
			return;

		case SCAN_ID:
			if(cur_live.crc) {
				st0 |= ST0_FAIL;
				st1 |= ST1_DE|ST1_ND;
				fi.sub_state = COMMAND_DONE;
				break;
			}
			st1 &= ~ST1_MA;
			if(!sector_matches()) {
				if(cur_live.idbuf[0] != command[2]) {
					if(cur_live.idbuf[0] == 0xff)
						st2 |= ST2_WC|ST2_BC;
					else
						st2 |= ST2_WC;
					st0 |= ST0_FAIL;
					fi.sub_state = COMMAND_DONE;
					break;
				}
				live_start(fi, command[0] & 0x40 ? SEARCH_ADDRESS_MARK_HEADER : SEARCH_ADDRESS_MARK_HEADER_FM);
				return;
			}
			logerror("%s: reading sector %02x %02x %02x %02x\n",
					 tag(),
					 cur_live.idbuf[0],
					 cur_live.idbuf[1],
					 cur_live.idbuf[2],
					 cur_live.idbuf[3]);
			sector_size = calc_sector_size(cur_live.idbuf[3]);
			fifo_expect(sector_size, false);
			fi.sub_state = SECTOR_READ;
			live_start(fi, command[0] & 0x40 ? SEARCH_ADDRESS_MARK_DATA : SEARCH_ADDRESS_MARK_DATA_FM);
			return;

		case SCAN_ID_FAILED:
			st0 |= ST0_FAIL;
			st1 |= ST1_ND;
			fi.sub_state = COMMAND_DONE;
			break;

		case SECTOR_READ: {
			if(st2 & ST2_MD) {
				st0 |= ST0_FAIL;
				fi.sub_state = COMMAND_DONE;
				break;
			}
			if(cur_live.crc) {
				st0 |= ST0_FAIL;
				st1 |= ST1_DE;
				st2 |= ST2_CM;
				fi.sub_state = COMMAND_DONE;
				break;
			}
			bool done = tc_done;
			command[4]++;
			if(command[4] > command[6]) {
				command[4] = 1;
				if(command[0] & 0x80) {
					command[3] = command[3] ^ 1;
					if(fi.dev)
						fi.dev->ss_w(command[3] & 1);
				}
				if(!(command[0] & 0x80) || !(command[3] & 1)) {
					command[2]++;
					if(!tc_done) {
						st0 |= ST0_FAIL;
						st1 |= ST1_EN;
					}
					done = true;
				}
			}
			if(!done) {
				fi.sub_state = SEEK_DONE;
				break;
			}
			fi.sub_state = COMMAND_DONE;
			break;
		}

		case COMMAND_DONE:
			main_phase = PHASE_RESULT;
			result[0] = st0;
			result[1] = st1;
			result[2] = st2;
			result[3] = command[2];
			result[4] = command[3];
			result[5] = command[4];
			result[6] = command[5];
			result_pos = 7;
			command_end(fi, true);
			return;

		default:
			logerror("%s: read sector unknown sub-state %d\n", ttsn().cstr(), fi.sub_state);
			return;
		}
	}
}

void upd765_family_device::write_data_start(floppy_info &fi)
{
	fi.main_state = WRITE_DATA;
	fi.sub_state = HEAD_LOAD_DONE;
	logerror("%s: command write%s data%s%s cmd=%02x sel=%x chrn=(%d, %d, %d, %d) eot=%02x gpl=%02x dtl=%02x rate=%d\n",
			 tag(),
			 command[0] & 0x08 ? " deleted" : "",
			 command[0] & 0x80 ? " mt" : "",
			 command[0] & 0x40 ? " mfm" : "",
			 command[0],
			 command[1],
			 command[2],
			 command[3],
			 command[4],
			 128 << (command[5] & 7),
			 command[6],
			 command[7],
			 command[8],
			 cur_rate);

	if(fi.dev)
		fi.dev->ss_w(command[1] & 4 ? 1 : 0);

	st0 = command[1] & 7;
	st1 = ST1_MA;
	st2 = 0x00;

	write_data_continue(fi);
}

void upd765_family_device::write_data_continue(floppy_info &fi)
{
	for(;;) {
		switch(fi.sub_state) {
		case HEAD_LOAD_DONE:
			fi.counter = 0;
			fi.sub_state = SCAN_ID;
			live_start(fi, SEARCH_ADDRESS_MARK_HEADER);
			return;

		case SCAN_ID:
			if(!sector_matches()) {
				live_start(fi, SEARCH_ADDRESS_MARK_HEADER);
				return;
			}
			if(cur_live.crc) {
				fprintf(stderr, "Header CRC error\n");
				live_start(fi, SEARCH_ADDRESS_MARK_HEADER);
				return;
			}
			sector_size = calc_sector_size(cur_live.idbuf[3]);
			fifo_expect(sector_size, true);
			fi.sub_state = SECTOR_WRITTEN;
			live_start(fi, WRITE_SECTOR_SKIP_GAP2);
			return;

		case SCAN_ID_FAILED:
			st0 |= ST0_FAIL;
			st1 |= ST1_ND;
			fi.sub_state = COMMAND_DONE;
			break;

		case SECTOR_WRITTEN: {
			bool done = tc_done;
			command[4]++;
			if(command[4] > command[6]) {
				command[4] = 1;
				if(command[0] & 0x80) {
					command[3] = command[3] ^ 1;
					if(fi.dev)
						fi.dev->ss_w(command[3] & 1);
				}
				if(!(command[0] & 0x80) || !(command[3] & 1)) {
					command[2]++;
					if(!tc_done) {
						st0 |= ST0_FAIL;
						st1 |= ST1_EN;
					}
					done = true;
				}
			}
			if(!done) {
				fi.sub_state = HEAD_LOAD_DONE;
				break;
			}
			fi.sub_state = COMMAND_DONE;
			break;
		}

		case COMMAND_DONE:
			main_phase = PHASE_RESULT;
			result[0] = st0;
			result[1] = st1;
			result[2] = st2;
			result[3] = command[2];
			result[4] = command[3];
			result[5] = command[4];
			result[6] = command[5];
			result_pos = 7;
			command_end(fi, true);
			return;

		default:
			logerror("%s: write sector unknown sub-state %d\n", ttsn().cstr(), fi.sub_state);
			return;
		}
	}
}

void upd765_family_device::read_track_start(floppy_info &fi)
{
	fi.main_state = READ_TRACK;
	fi.sub_state = HEAD_LOAD_DONE;

	logerror("%s: command read track%s cmd=%02x sel=%x chrn=(%d, %d, %d, %d) eot=%02x gpl=%02x dtl=%02x rate=%d\n",
			 tag(),
			 command[0] & 0x40 ? " mfm" : "",
			 command[0],
			 command[1],
			 command[2],
			 command[3],
			 command[4],
			 128 << (command[5] & 7),
			 command[6],
			 command[7],
			 command[8],
			 cur_rate);

	if(fi.dev)
		fi.dev->ss_w(command[1] & 4 ? 1 : 0);
	read_track_continue(fi);
}

void upd765_family_device::read_track_continue(floppy_info &fi)
{
	for(;;) {
		switch(fi.sub_state) {
		case HEAD_LOAD_DONE:
			if(fi.pcn == command[2] || !(fifocfg & 0x40)) {
				fi.sub_state = SEEK_DONE;
				break;
			}
			if(fi.dev) {
				fi.dev->dir_w(fi.pcn > command[2] ? 1 : 0);
				fi.dev->stp_w(0);
			}
			fi.sub_state = SEEK_WAIT_STEP_SIGNAL_TIME;
			fi.tm->adjust(attotime::from_nsec(2500));
			return;

		case SEEK_WAIT_STEP_SIGNAL_TIME:
			return;

		case SEEK_WAIT_STEP_SIGNAL_TIME_DONE:
			if(fi.dev)
				fi.dev->stp_w(1);

			fi.sub_state = SEEK_WAIT_STEP_TIME;
			delay_cycles(fi.tm, 500*(16-(spec >> 12)));
			return;

		case SEEK_WAIT_STEP_TIME:
			return;

		case SEEK_WAIT_STEP_TIME_DONE:
			if(fi.pcn > command[2])
				fi.pcn--;
			else
				fi.pcn++;
			fi.sub_state = HEAD_LOAD_DONE;
			break;

		case SEEK_DONE:
			fi.counter = 0;
			fi.sub_state = SCAN_ID;
			live_start(fi, SEARCH_ADDRESS_MARK_HEADER);
			return;

		case SCAN_ID:
			if(cur_live.crc) {
				fprintf(stderr, "Header CRC error\n");
				live_start(fi, SEARCH_ADDRESS_MARK_HEADER);
				return;
			}
			sector_size = calc_sector_size(cur_live.idbuf[3]);
			fifo_expect(sector_size, false);
			fi.sub_state = SECTOR_READ;
			live_start(fi, SEARCH_ADDRESS_MARK_DATA);
			return;

		case SCAN_ID_FAILED:
			fprintf(stderr, "RNF\n");
			//          command_end(fi, true, 1);
			return;

		case SECTOR_READ:
			if(cur_live.crc) {
				fprintf(stderr, "CRC error\n");
			}
			if(command[4] < command[6]) {
				command[4]++;
				fi.sub_state = HEAD_LOAD_DONE;
				break;
			}

			main_phase = PHASE_RESULT;
			result[0] = 0x40 | (fi.dev->ss_r() << 2) | fi.id;
			result[1] = 0;
			result[2] = 0;
			result[3] = command[2];
			result[4] = command[3];
			result[5] = command[4];
			result[6] = command[5];
			result_pos = 7;
			//          command_end(fi, true, 0);
			return;

		default:
			logerror("%s: read track unknown sub-state %d\n", ttsn().cstr(), fi.sub_state);
			return;
		}
	}
}

int upd765_family_device::calc_sector_size(UINT8 size)
{
	return size > 7 ? 16384 : 128 << size;
}

void upd765_family_device::format_track_start(floppy_info &fi)
{
	fi.main_state = FORMAT_TRACK;
	fi.sub_state = HEAD_LOAD_DONE;

	logerror("%s: command format track %s h=%02x n=%02x sc=%02x gpl=%02x d=%02x\n",
			 tag(),
			 command[0] & 0x40 ? "mfm" : "fm",
			 command[1], command[2], command[3], command[4], command[5]);

	if(fi.dev)
		fi.dev->ss_w(command[1] & 4 ? 1 : 0);
	sector_size = calc_sector_size(command[2]);

	format_track_continue(fi);
}

void upd765_family_device::format_track_continue(floppy_info &fi)
{
	for(;;) {
		switch(fi.sub_state) {
		case HEAD_LOAD_DONE:
			fi.sub_state = WAIT_INDEX;
			break;

		case WAIT_INDEX:
			return;

		case WAIT_INDEX_DONE:
			logerror("%s: index found, writing track\n", tag());
			fi.sub_state = TRACK_DONE;
			cur_live.pll.start_writing(machine().time());
			live_start(fi, WRITE_TRACK_PRE_SECTORS);
			return;

		case TRACK_DONE:
			main_phase = PHASE_RESULT;
			result[0] = (fi.dev->ss_r() << 2) | fi.id;
			result[1] = 0;
			result[2] = 0;
			result[3] = 0;
			result[4] = 0;
			result[5] = 0;
			result[6] = 0;
			result_pos = 7;
			command_end(fi, true);
			return;

		default:
			logerror("%s: format track unknown sub-state %d\n", ttsn().cstr(), fi.sub_state);
			return;
		}
	}
}

void upd765_family_device::read_id_start(floppy_info &fi)
{
	fi.main_state = READ_ID;
	fi.sub_state = HEAD_LOAD_DONE;

	logerror("%s: command read id%s, rate=%d\n",
			 tag(),
			 command[0] & 0x40 ? " mfm" : "",
			 cur_rate);

	if(fi.dev)
		fi.dev->ss_w(command[1] & 4 ? 1 : 0);

	st0 = command[1] & 7;
	st1 = 0x00;
	st2 = 0x00;

	for(int i=0; i<4; i++)
		cur_live.idbuf[i] = 0x00;

	read_id_continue(fi);
}

void upd765_family_device::read_id_continue(floppy_info &fi)
{
	for(;;) {
		switch(fi.sub_state) {
		case HEAD_LOAD_DONE:
			fi.counter = 0;
			fi.sub_state = SCAN_ID;
			live_start(fi, SEARCH_ADDRESS_MARK_HEADER);
			return;

		case SCAN_ID:
			if(cur_live.crc) {
				st0 |= ST0_FAIL;
				st1 |= ST1_MA|ST1_DE|ST1_ND;
			}
			fi.sub_state = COMMAND_DONE;
			break;

		case SCAN_ID_FAILED:
			st0 |= ST0_FAIL;
			st1 |= ST1_ND|ST1_MA;
			fi.sub_state = COMMAND_DONE;
			break;

		case COMMAND_DONE:
			main_phase = PHASE_RESULT;
			result[0] = st0;
			result[1] = st1;
			result[2] = st2;
			result[3] = cur_live.idbuf[0];
			result[4] = cur_live.idbuf[1];
			result[5] = cur_live.idbuf[2];
			result[6] = cur_live.idbuf[3];
			result_pos = 7;
			command_end(fi, true);
			return;

		default:
			logerror("%s: read id unknown sub-state %d\n", ttsn().cstr(), fi.sub_state);
			return;
		}
	}
}

void upd765_family_device::check_irq()
{
	bool old_irq = cur_irq;
	cur_irq = data_irq || polled_irq || internal_drq;
	for(int i=0; i<4; i++)
		cur_irq = cur_irq || flopi[i].irq == floppy_info::IRQ_SEEK;
	cur_irq = cur_irq && (dor & 4) && (dor & 8);
	if(cur_irq != old_irq && !intrq_cb.isnull()) {
		logerror("%s: irq = %d\n", tag(), cur_irq);
		intrq_cb(cur_irq);
	}
}

bool upd765_family_device::get_irq() const
{
	return cur_irq;
}

astring upd765_family_device::tts(attotime t)
{
	char buf[256];
	const char *sign = "";
	if(t.seconds < 0) {
		t = attotime::zero-t;
		sign = "-";
	}
	int nsec = t.attoseconds / ATTOSECONDS_PER_NANOSECOND;
	sprintf(buf, "%s%04d.%03d,%03d,%03d", sign, int(t.seconds), nsec/1000000, (nsec/1000)%1000, nsec % 1000);
	return buf;
}

astring upd765_family_device::ttsn()
{
	return tts(machine().time());
}

void upd765_family_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	if(id == TIMER_DRIVE_READY_POLLING) {
		run_drive_ready_polling();
		return;
	}

	live_sync();

	floppy_info &fi = flopi[id];
	switch(fi.sub_state) {
	case SEEK_WAIT_STEP_SIGNAL_TIME:
		fi.sub_state = SEEK_WAIT_STEP_SIGNAL_TIME_DONE;
		break;
	case SEEK_WAIT_STEP_TIME:
		fi.sub_state = SEEK_WAIT_STEP_TIME_DONE;
		break;
	}

	general_continue(fi);
}

void upd765_family_device::run_drive_ready_polling()
{
	if(main_phase != PHASE_CMD || (fifocfg & FIF_POLL))
		return;

	bool changed = false;
	for(int fid=0; fid<4; fid++) {
		bool ready = get_ready(fid);
		if(ready != flopi[fid].ready) {
			logerror("%s: polled %d : %d -> %d\n", tag(), fid, flopi[fid].ready, ready);
			flopi[fid].ready = ready;
			if(flopi[fid].irq == floppy_info::IRQ_NONE) {
				flopi[fid].irq = floppy_info::IRQ_POLLED;
				polled_irq = true;
				changed = true;
			}
		}
	}
	if(changed)
		check_irq();
}

void upd765_family_device::index_callback(floppy_image_device *floppy, int state)
{
	for(int fid=0; fid<4; fid++) {
		floppy_info &fi = flopi[fid];
		if(fi.dev != floppy)
			continue;

		if(fi.live)
			live_sync();
		fi.index = state;

		if(!state) {
			general_continue(fi);
			continue;
		}

		switch(fi.sub_state) {
		case IDLE:
		case SEEK_MOVE:
		case SEEK_WAIT_STEP_SIGNAL_TIME:
		case SEEK_WAIT_STEP_SIGNAL_TIME_DONE:
		case SEEK_WAIT_STEP_TIME:
		case SEEK_WAIT_STEP_TIME_DONE:
		case HEAD_LOAD_DONE:
		case SCAN_ID_FAILED:
		case SECTOR_READ:
			break;

		case WAIT_INDEX:
			fi.sub_state = WAIT_INDEX_DONE;
			break;

		case SCAN_ID:
			fi.counter++;
			if(fi.counter == 2) {
				fi.sub_state = SCAN_ID_FAILED;
				live_abort();
			}
			break;

		case TRACK_DONE:
			live_abort();
			break;

		default:
			logerror("%s: Index pulse on unknown sub-state %d\n", ttsn().cstr(), fi.sub_state);
			break;
		}

		general_continue(fi);
	}
}


void upd765_family_device::general_continue(floppy_info &fi)
{
	if(fi.live && cur_live.state != IDLE) {
		live_run();
		if(cur_live.state != IDLE)
			return;
	}

	switch(fi.main_state) {
	case IDLE:
		break;

	case RECALIBRATE:
	case SEEK:
		seek_continue(fi);
		break;

	case READ_DATA:
		read_data_continue(fi);
		break;

	case WRITE_DATA:
		write_data_continue(fi);
		break;

	case READ_TRACK:
		read_track_continue(fi);
		break;

	case FORMAT_TRACK:
		format_track_continue(fi);
		break;

	case READ_ID:
		read_id_continue(fi);
		break;

	default:
		logerror("%s: general_continue on unknown main-state %d\n", ttsn().cstr(), fi.main_state);
		break;
	}
}

bool upd765_family_device::read_one_bit(attotime limit)
{
	int bit = cur_live.pll.get_next_bit(cur_live.tm, cur_live.fi->dev, limit);
	if(bit < 0)
		return true;
	cur_live.shift_reg = (cur_live.shift_reg << 1) | bit;
	cur_live.bit_counter++;
	if(cur_live.data_separator_phase) {
		cur_live.data_reg = (cur_live.data_reg << 1) | bit;
		if((cur_live.crc ^ (bit ? 0x8000 : 0x0000)) & 0x8000)
			cur_live.crc = (cur_live.crc << 1) ^ 0x1021;
		else
			cur_live.crc = cur_live.crc << 1;
	}
	cur_live.data_separator_phase = !cur_live.data_separator_phase;
	return false;
}

bool upd765_family_device::write_one_bit(attotime limit)
{
	bool bit = cur_live.shift_reg & 0x8000;
	if(cur_live.pll.write_next_bit(bit, cur_live.tm, cur_live.fi->dev, limit))
		return true;
	if(cur_live.bit_counter & 1) {
		if((cur_live.crc ^ (bit ? 0x8000 : 0x0000)) & 0x8000)
			cur_live.crc = (cur_live.crc << 1) ^ 0x1021;
		else
			cur_live.crc = cur_live.crc << 1;
	}
	cur_live.shift_reg = cur_live.shift_reg << 1;
	cur_live.bit_counter--;
	return false;
}

void upd765_family_device::live_write_raw(UINT16 raw)
{
	//  logerror("write %04x %04x\n", raw, cur_live.crc);
	cur_live.shift_reg = raw;
	cur_live.data_bit_context = raw & 1;
}

void upd765_family_device::live_write_mfm(UINT8 mfm)
{
	bool context = cur_live.data_bit_context;
	UINT16 raw = 0;
	for(int i=0; i<8; i++) {
		bool bit = mfm & (0x80 >> i);
		if(!(bit || context))
			raw |= 0x8000 >> (2*i);
		if(bit)
			raw |= 0x4000 >> (2*i);
		context = bit;
	}
	cur_live.data_reg = mfm;
	cur_live.shift_reg = raw;
	cur_live.data_bit_context = context;
	//  logerror("write %02x   %04x %04x\n", mfm, cur_live.crc, raw);
}

void upd765_family_device::live_write_fm(UINT8 fm)
{
	bool context = cur_live.data_bit_context;
	UINT16 raw = 0;
	for(int i=0; i<8; i++) {
		bool bit = fm & (0x80 >> i);
		raw |= 0x8000 >> (2*i);
		if(bit)
			raw |= 0x4000 >> (2*i);
		context = bit;
	}
	cur_live.data_reg = fm;
	cur_live.shift_reg = raw;
	cur_live.data_bit_context = context;
	//  logerror("write %02x   %04x %04x\n", fm, cur_live.crc, raw);
}

bool upd765_family_device::sector_matches() const
{
	return
		cur_live.idbuf[0] == command[2] &&
		cur_live.idbuf[1] == command[3] &&
		cur_live.idbuf[2] == command[4] &&
		cur_live.idbuf[3] == command[5];
}

void upd765_family_device::pll_t::set_clock(attotime _period)
{
	period = _period;
	period_adjust_base = period * 0.05;
	min_period = period * 0.75;
	max_period = period * 1.25;
}

void upd765_family_device::pll_t::reset(attotime when)
{
	ctime = when;
	phase_adjust = attotime::zero;
	freq_hist = 0;
	write_position = 0;
	write_start_time = attotime::never;
}

void upd765_family_device::pll_t::start_writing(attotime tm)
{
	write_start_time = tm;
	write_position = 0;
}

void upd765_family_device::pll_t::stop_writing(floppy_image_device *floppy, attotime tm)
{
	commit(floppy, tm);
	write_start_time = attotime::never;
}

void upd765_family_device::pll_t::commit(floppy_image_device *floppy, attotime tm)
{
	if(write_start_time.is_never() || tm == write_start_time)
		return;

	if(floppy)
		floppy->write_flux(write_start_time, tm, write_position, write_buffer);
	write_start_time = tm;
	write_position = 0;
}

int upd765_family_device::pll_t::get_next_bit(attotime &tm, floppy_image_device *floppy, attotime limit)
{
	attotime edge = floppy ? floppy->get_next_transition(ctime) : attotime::never;

	attotime next = ctime + period + phase_adjust;

#if 0
	if(!edge.is_never())
		fprintf(stderr, "ctime=%s, transition_time=%s, next=%s, pha=%s\n", tts(ctime).cstr(), tts(edge).cstr(), tts(next).cstr(), tts(phase_adjust).cstr());
#endif

	if(next > limit)
		return -1;

	ctime = next;
	tm = next;

	if(edge.is_never() || edge >= next) {
		// No transition in the window means 0 and pll in free run mode
		phase_adjust = attotime::zero;
		return 0;
	}

	// Transition in the window means 1, and the pll is adjusted

	attotime delta = edge - (next - period/2);

	if(delta.seconds < 0)
		phase_adjust = attotime::zero - ((attotime::zero - delta)*65)/100;
	else
		phase_adjust = (delta*65)/100;

	if(delta < attotime::zero) {
		if(freq_hist < 0)
			freq_hist--;
		else
			freq_hist = -1;
	} else if(delta > attotime::zero) {
		if(freq_hist > 0)
			freq_hist++;
		else
			freq_hist = 1;
	} else
		freq_hist = 0;

	if(freq_hist) {
		int afh = freq_hist < 0 ? -freq_hist : freq_hist;
		if(afh > 1) {
			attotime aper = attotime::from_double(period_adjust_base.as_double()*delta.as_double()/period.as_double());
			period += aper;

			if(period < min_period)
				period = min_period;
			else if(period > max_period)
				period = max_period;
		}
	}

	return 1;
}

bool upd765_family_device::pll_t::write_next_bit(bool bit, attotime &tm, floppy_image_device *floppy, attotime limit)
{
	if(write_start_time.is_never()) {
		write_start_time = ctime;
		write_position = 0;
	}

	attotime etime = ctime + period;
	if(etime > limit)
		return true;

	if(bit && write_position < ARRAY_LENGTH(write_buffer))
		write_buffer[write_position++] = ctime + period/2;

	tm = etime;
	ctime = etime;
	return false;
}

upd765a_device::upd765a_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) : upd765_family_device(mconfig, UPD765A, "UPD765A", tag, owner, clock)
{
	m_shortname = "upd765a";
	dor_reset = 0x0c;
}

upd765b_device::upd765b_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) : upd765_family_device(mconfig, UPD765B, "UPD765B", tag, owner, clock)
{
	m_shortname = "upd765b";
	dor_reset = 0x0c;
}

i8272a_device::i8272a_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) : upd765_family_device(mconfig, I8272A, "I8272A", tag, owner, clock)
{
	m_shortname = "i8272a";
	dor_reset = 0x0c;
}

upd72065_device::upd72065_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) : upd765_family_device(mconfig, UPD72065, "UPD72065", tag, owner, clock)
{
	m_shortname = "upd72065";
	dor_reset = 0x0c;
}

smc37c78_device::smc37c78_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) : upd765_family_device(mconfig, SMC37C78, "SMC37C78", tag, owner, clock)
{
	m_shortname = "smc37c78";
	ready_connected = false;
	select_connected = true;
}

n82077aa_device::n82077aa_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) : upd765_family_device(mconfig, N82077AA, "N82077AA", tag, owner, clock)
{
	m_shortname = "n82077aa";
	ready_connected = false;
	select_connected = true;
}

pc_fdc_superio_device::pc_fdc_superio_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) : upd765_family_device(mconfig, PC_FDC_SUPERIO, "PC FDC SUPERIO", tag, owner, clock)
{
	m_shortname = "pc_fdc_superio";
	ready_polled = false;
	ready_connected = false;
	select_connected = true;
}
