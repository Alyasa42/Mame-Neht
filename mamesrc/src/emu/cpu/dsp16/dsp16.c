/***************************************************************************

    dsp16.c

    WE|AT&T DSP16 series emulator.

***************************************************************************/

#include "emu.h"
#include "debugger.h"
#include "dsp16.h"

const device_type DSP16 = &device_creator<dsp16_device>;

dsp16_device::dsp16_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: cpu_device(mconfig, DSP16, "DSP16", tag, owner, clock, "dsp16", __FILE__),
		m_program_config("program", ENDIANNESS_LITTLE, 16, 16, -1),
		m_data_config("data", ENDIANNESS_LITTLE, 16, 16, -1),
		m_i(0), m_pc(0), m_pt(0), m_pr(0), m_pi(0),
		m_j(0), m_k(0), m_rb(0), m_re(0),
		m_r0(0), m_r1(0), m_r2(0), m_r3(0),
		m_x(0), m_y(0), m_p(0), m_a0(0), m_a1(0),
		m_auc(0), m_psw(0), m_c0(0), m_c1(0), m_c2(0),
		m_sioc(0), m_srta(0), m_sdx(0), m_pioc(0), m_pdx0(0), m_pdx1(0),
		m_ppc(0),
		m_cacheStart(CACHE_INVALID),
		m_cacheEnd(CACHE_INVALID),
		m_cacheRedoNextPC(CACHE_INVALID),
		m_cacheIterations(0),
		m_program(NULL),
		m_data(NULL),
		m_direct(NULL),
		m_icount(0)
{
	m_ock_cb = NULL;
	m_pio_w_cb = NULL;
	m_pio_r_cb = NULL;
}

void dsp16_device::device_start()
{
	state_add(STATE_GENPC,    "GENPC",     m_pc).noshow();
	state_add(STATE_GENFLAGS, "GENFLAGS",  m_psw).callimport().callexport().formatstr("%10s").noshow();
	state_add(DSP16_PC,       "PC",        m_pc);
	state_add(DSP16_I,        "I",         m_i);
	state_add(DSP16_PT,       "PT",        m_pt);
	state_add(DSP16_PR,       "PR",        m_pr);
	state_add(DSP16_PI,       "PI",        m_pi);
	state_add(DSP16_J,        "J",         m_j);
	state_add(DSP16_K,        "K",         m_k);
	state_add(DSP16_RB,       "RB",        m_rb);
	state_add(DSP16_RE,       "RE",        m_re);
	state_add(DSP16_R0,       "R0",        m_r0);
	state_add(DSP16_R1,       "R1",        m_r1);
	state_add(DSP16_R2,       "R2",        m_r2);
	state_add(DSP16_R3,       "R3",        m_r3);
	state_add(DSP16_X,        "X",         m_x);
	state_add(DSP16_Y,        "Y",         m_y);
	state_add(DSP16_P,        "P",         m_p);
	state_add(DSP16_A0,       "A0",        m_a0).mask(U64(0xfffffffff));
	state_add(DSP16_A1,       "A1",        m_a1).mask(U64(0xfffffffff));
	state_add(DSP16_AUC,      "AUC",       m_auc).formatstr("%8s");
	state_add(DSP16_PSW,      "PSW",       m_psw).formatstr("%16s");
	state_add(DSP16_C0,       "C0",        m_c0);
	state_add(DSP16_C1,       "C1",        m_c1);
	state_add(DSP16_C2,       "C2",        m_c2);
	state_add(DSP16_SIOC,     "SIOC",      m_sioc).formatstr("%10s");
	state_add(DSP16_SRTA,     "SRTA",      m_srta);
	state_add(DSP16_SDX,      "SDX",       m_sdx);
	state_add(DSP16_PIOC,     "PIOC",      m_pioc).formatstr("%16s");
	state_add(DSP16_PDX0,     "PDX0",      m_pdx0);
	state_add(DSP16_PDX1,     "PDX1",      m_pdx1);

	save_item(NAME(m_i));
	save_item(NAME(m_pc));
	save_item(NAME(m_pt));
	save_item(NAME(m_pr));
	save_item(NAME(m_pi));
	save_item(NAME(m_j));
	save_item(NAME(m_k));
	save_item(NAME(m_rb));
	save_item(NAME(m_re));
	save_item(NAME(m_r0));
	save_item(NAME(m_r1));
	save_item(NAME(m_r2));
	save_item(NAME(m_r3));
	save_item(NAME(m_x));
	save_item(NAME(m_y));
	save_item(NAME(m_p));
	save_item(NAME(m_a0));
	save_item(NAME(m_a1));
	save_item(NAME(m_auc));
	save_item(NAME(m_psw));
	save_item(NAME(m_c0));
	save_item(NAME(m_c1));
	save_item(NAME(m_c2));
	save_item(NAME(m_sioc));
	save_item(NAME(m_srta));
	save_item(NAME(m_sdx));
	save_item(NAME(m_pioc));
	save_item(NAME(m_pdx0));
	save_item(NAME(m_pdx1));
	save_item(NAME(m_ppc));
	save_item(NAME(m_cacheStart));
	save_item(NAME(m_cacheEnd));
	save_item(NAME(m_cacheRedoNextPC));
	save_item(NAME(m_cacheIterations));

	m_program = &space(AS_PROGRAM);
	m_data = &space(AS_DATA);
	m_direct = &m_program->direct();

	m_icountptr = &m_icount;
}

void dsp16_device::device_reset()
{
	m_pc = 0x0000;
	m_pi = 0x0000;
	m_sioc = 0x0000;    
	m_pioc = 0x0008;
	m_rb = 0x0000;
	m_re = 0x0000;
	m_ppc = m_pc;

	m_cacheStart = CACHE_INVALID;
	m_cacheEnd = CACHE_INVALID;
	m_cacheRedoNextPC = CACHE_INVALID;
	m_cacheIterations = 0;
}

const address_space_config *dsp16_device::memory_space_config(address_spacenum spacenum) const
{
	return (spacenum == AS_PROGRAM) ? &m_program_config :
			(spacenum == AS_DATA) ? &m_data_config :
			NULL;
}

void dsp16_device::state_string_export(const device_state_entry &entry, astring &string)
{
	switch (entry.index())
	{
		case STATE_GENFLAGS:
			string.printf("(below)");
			break;

		case DSP16_AUC:
		{
			astring alignString;
			const UINT8 align = m_auc & 0x03;
			switch (align)
			{
				case 0x00: alignString.printf("xy"); break;
				case 0x01: alignString.printf("/4"); break;
				case 0x02: alignString.printf("x4"); break;
				case 0x03: alignString.printf(",,"); break;
			}
			string.printf("%c%c%c%c%c%s",
							m_auc & 0x40 ? 'Y':'.', m_auc & 0x20 ? '1':'.',
							m_auc & 0x10 ? '0':'.', m_auc & 0x08 ? '1':'.',
							m_auc & 0x04 ? '0':'.', alignString.cstr());
			break;
		}

		case DSP16_PSW:
			string.printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
							m_psw & 0x8000 ? 'M':'.', m_psw & 0x4000 ? 'E':'.',
							m_psw & 0x2000 ? 'L':'.', m_psw & 0x1000 ? 'V':'.',
							',', ',',
							m_psw & 0x0200 ? 'O':'.', m_psw & 0x0100 ? '1':'.',
							m_psw & 0x0080 ? '1':'.', m_psw & 0x0040 ? '1':'.',
							m_psw & 0x0020 ? '1':'.', m_psw & 0x0010 ? 'O':'.',
							m_psw & 0x0008 ? '1':'.', m_psw & 0x0004 ? '1':'.',
							m_psw & 0x0002 ? '1':'.', m_psw & 0x0001 ? '1':'.');
			break;

		case DSP16_PIOC:
		{
			astring strobeString;
			const UINT8 strobe = (m_pioc & 0x6000) >> 13;
			switch (strobe)
			{
				case 0x00: strobeString.printf("1T"); break;
				case 0x01: strobeString.printf("2T"); break;
				case 0x02: strobeString.printf("3T"); break;
				case 0x03: strobeString.printf("4T"); break;
			}
			string.printf("%c%s%c%c%c%c%c%c%c%c%c%c%c%c%c",
							m_pioc & 0x8000 ? 'I':'.', strobeString.cstr(),
							m_pioc & 0x1000 ? 'O':'I', m_pioc & 0x0800 ? 'O':'I',
							m_pioc & 0x0400 ? 'S':'.', m_pioc & 0x0200 ? 'I':'.',
							m_pioc & 0x0100 ? 'O':'.', m_pioc & 0x0080 ? 'P':'.',
							m_pioc & 0x0040 ? 'P':'.', m_pioc & 0x0020 ? 'I':'.',
							m_pioc & 0x0010 ? 'I':'.', m_pioc & 0x0008 ? 'O':'.',
							m_pioc & 0x0004 ? 'P':'.', m_pioc & 0x0002 ? 'P':'.',
							m_pioc & 0x0001 ? 'I':'.');
			break;
		}

		case DSP16_SIOC:
		{
			astring clkString;
			const UINT8 clk = (m_sioc & 0x0180) >> 7;
			switch (clk)
			{
				case 0x00: clkString.printf("/4"); break;
				case 0x01: clkString.printf("12"); break;
				case 0x02: clkString.printf("16"); break;
				case 0x03: clkString.printf("20"); break;
			}
			string.printf("%c%s%c%c%c%c%c%c%c",
							m_sioc & 0x0200 ? 'I':'O', clkString.cstr(),
							m_sioc & 0x0040 ? 'L':'M', m_sioc & 0x0020 ? 'I':'O',
							m_sioc & 0x0010 ? 'I':'O', m_sioc & 0x0008 ? 'I':'O',
							m_sioc & 0x0004 ? 'I':'O', m_sioc & 0x0002 ? '2':'1',
							m_sioc & 0x0001 ? '2':'1');
			break;
		}
	}
}

UINT32 dsp16_device::disasm_min_opcode_bytes() const { return 2; }
UINT32 dsp16_device::disasm_max_opcode_bytes() const { return 4; }

offs_t dsp16_device::disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options)
{
	extern CPU_DISASSEMBLE( dsp16a );
	return CPU_DISASSEMBLE_NAME(dsp16a)(this, buffer, pc, oprom, opram, options);
}

inline UINT32 dsp16_device::data_read(const UINT16& addr) { return m_data->read_word(addr << 1); }
inline void dsp16_device::data_write(const UINT16& addr, const UINT16& data) { m_data->write_word(addr << 1, data & 0xffff); }
inline UINT32 dsp16_device::opcode_read(const UINT8 pcOffset)
{
	const UINT16 readPC = m_pc + pcOffset;
	return m_direct->read_decrypted_dword(readPC << 1);
}

UINT32 dsp16_device::execute_min_cycles() const { return 1; }
UINT32 dsp16_device::execute_max_cycles() const { return 1; }
UINT32 dsp16_device::execute_input_lines() const { return 1; }
void dsp16_device::execute_set_input(int inputnum, int state) { }

void dsp16_device::execute_run()
{
	m_icount = 0;
	return;
}

#include "dsp16ops.inc"
