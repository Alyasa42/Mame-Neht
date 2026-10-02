/***************************************************************************

    dsp16.h

    WE|AT&T DSP16 series emulator.

***************************************************************************/

#pragma once

#ifndef __DSP16_H__
#define __DSP16_H__

class dsp16_device : public cpu_device
{
public:
	dsp16_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	UINT8 psel_r() const { return m_pdx0; } 
	UINT8 ose_r() const  { return (m_sioc & 0x0004) ? 1 : 0; } 
	UINT8 old_r() const  { return (m_sioc & 0x0010) ? 1 : 0; } 
	UINT8 do_r() const   { return (m_sdx & 0x8000) ? 1 : 0; }  

	typedef void (*ock_cb_func)(device_t *device, int state);
	typedef void (*pio_w_cb_func)(device_t *device, offs_t offset, UINT16 data);
	typedef UINT16 (*pio_r_cb_func)(device_t *device);

	void set_ock_cb(ock_cb_func cb) { m_ock_cb = cb; }
	void set_pio_w_cb(pio_w_cb_func cb) { m_pio_w_cb = cb; }
	void set_pio_r_cb(pio_r_cb_func cb) { m_pio_r_cb = cb; }

	ock_cb_func    m_ock_cb;
	pio_w_cb_func  m_pio_w_cb;
	pio_r_cb_func  m_pio_r_cb;

protected:
	virtual void device_start();
	virtual void device_reset();

	virtual UINT64 execute_clocks_to_cycles(UINT64 clocks) const { return (clocks + 2 - 1) / 2; }
	virtual UINT64 execute_cycles_to_clocks(UINT64 cycles) const { return (cycles * 2); }
	virtual UINT32 execute_min_cycles() const;
	virtual UINT32 execute_max_cycles() const;
	virtual UINT32 execute_input_lines() const;
	virtual void execute_run();
	virtual void execute_set_input(int inputnum, int state);

	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const;

	virtual void state_string_export(const device_state_entry &entry, astring &string);

	virtual UINT32 disasm_min_opcode_bytes() const;
	virtual UINT32 disasm_max_opcode_bytes() const;
	virtual offs_t disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options);

	const address_space_config m_program_config;
	const address_space_config m_data_config;

	UINT16 m_i;     
	UINT16 m_pc;
	UINT16 m_pt;
	UINT16 m_pr;
	UINT16 m_pi;

	UINT16 m_j;     
	UINT16 m_k;     
	UINT16 m_rb;
	UINT16 m_re;
	UINT16 m_r0;
	UINT16 m_r1;
	UINT16 m_r2;
	UINT16 m_r3;

	UINT16 m_x;
	UINT32 m_y;
	UINT32 m_p;
	UINT64 m_a0;    
	UINT64 m_a1;    
	UINT8 m_auc;    
	UINT16 m_psw;
	UINT8 m_c0;
	UINT8 m_c1;
	UINT8 m_c2;

	UINT16 m_sioc;
	UINT16 m_srta;
	UINT16 m_sdx;
	UINT16 m_pioc;
	UINT16 m_pdx0;  
	UINT16 m_pdx1;  

	UINT16 m_ppc;

	UINT16 m_cacheStart;
	UINT16 m_cacheEnd;
	UINT16 m_cacheRedoNextPC;
	UINT16 m_cacheIterations;
	static const UINT16 CACHE_INVALID = 0xffff;

	inline UINT32 data_read(const UINT16& addr);
	inline void data_write(const UINT16& addr, const UINT16& data);
	inline UINT32 opcode_read(const UINT8 pcOffset=0);

	address_space* m_program;
	address_space* m_data;
	direct_read_data* m_direct;

	int m_icount;

	void execute_one(const UINT16& op, UINT8& cycles, UINT8& pcAdvance);
	void* registerFromRImmediateField(const UINT8& R);
	void* registerFromRTable(const UINT8& R);
	UINT16* registerFromYFieldUpper(const UINT8& Y);

	void executeF1Field(const UINT8& F1, const UINT8& D, const UINT8& S);
	void executeYFieldPost(const UINT8& Y);
	void executeZFieldPartOne(const UINT8& Z, UINT16* rN);
	void executeZFieldPartTwo(const UINT8& Z, UINT16* rN);

	void* addressYL();
	void writeRegister(void* reg, const UINT16& value);
	bool conditionTest(const UINT8& CON);

	bool lmi();
	bool leq();
	bool llv();
	bool lmv();
};

extern const device_type DSP16;

enum
{
	DSP16_I,        
	DSP16_PC,
	DSP16_PT,
	DSP16_PR,
	DSP16_PI,
	DSP16_J,        
	DSP16_K,
	DSP16_RB,
	DSP16_RE,
	DSP16_R0,
	DSP16_R1,
	DSP16_R2,
	DSP16_R3,
	DSP16_X,        
	DSP16_Y,
	DSP16_P,
	DSP16_A0,
	DSP16_A1,
	DSP16_AUC,
	DSP16_PSW,
	DSP16_C0,
	DSP16_C1,
	DSP16_C2,
	DSP16_SIOC,
	DSP16_SRTA,
	DSP16_SDX,
	DSP16_PIOC,
	DSP16_PDX0,
	DSP16_PDX1
};

#endif /* __DSP16_H__ */
