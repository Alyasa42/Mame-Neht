/*********************************************************

    Capcom Q-Sound system - Version Hybride 0.160 Fix

*********************************************************/

#pragma once

#ifndef __QSOUND_H__
#define __QSOUND_H__

#include "cpu/dsp16/dsp16.h"

#define QSOUND_CLOCK 60000000    

#define MCFG_QSOUND_ADD(_tag, _clock) \
	MCFG_DEVICE_ADD(_tag, QSOUND, _clock)
#define MCFG_QSOUND_REPLACE(_tag, _clock) \
	MCFG_DEVICE_REPLACE(_tag, QSOUND, _clock)

class qsound_device : public device_t,
						public device_sound_interface
{
public:
	qsound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~qsound_device() { }

	DECLARE_WRITE8_MEMBER(qsound_w);
	DECLARE_READ8_MEMBER(qsound_r);

	DECLARE_READ16_MEMBER(dsp_sample_r);

	void dsp_ock_w(int state);
	void dsp_pio_w(offs_t offset, UINT16 data);
	UINT16 dsp_pio_r();

	static void static_dsp_ock_w(device_t *device, int state);
	static void static_dsp_pio_w(device_t *device, offs_t offset, UINT16 data);
	static UINT16 static_dsp_pio_r(device_t *device);

	void set_cmd(void *ptr, INT32 param);
	void set_dsp_ready(void *ptr, INT32 param);

protected:
	const rom_entry *device_rom_region() const;
	machine_config_constructor device_mconfig_additions() const;
	virtual void device_start();
	virtual void device_reset();

	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);

private:
	void write_data(UINT8 address, UINT16 data);

	struct qsound_channel
	{
		UINT32 bank;        
		UINT32 address;     
		UINT16 loop;        
		UINT16 end;         
		UINT32 freq;        
		UINT16 vol;         

		bool enabled;       
		int lvol;           
		int rvol;           
		UINT32 step_ptr;    
	} m_channel[16]; // FIX: Tableau de 16 canaux restauré

	required_device<dsp16_device> m_cpu;
	required_region_ptr<UINT8> m_sample_rom;
	sound_stream *m_stream;

	int m_pan_table[33]; // FIX: Tableau de 33 entrées restauré   
	UINT16 m_rom_bank;
	UINT16 m_rom_offset;
	UINT16 m_cmd_addr;
	UINT16 m_cmd_data;
	UINT16 m_new_data;
	UINT8  m_cmd_pending;
	UINT8  m_dsp_ready;

	UINT16 m_samples[2]; // FIX: Tableau Gauche/Droite restauré
	UINT16 m_sr;         
	UINT16 m_fsr;        
	int    m_ock;        
	int    m_old;        
	int    m_ready;      
	UINT32 m_channel_num; 
};

extern const device_type QSOUND;

#endif /* __QSOUND_H__ */
