/***************************************************************************

  Capcom System QSound(tm) - Version Hybride pour MAME 0.160 Fix

***************************************************************************/

#include "emu.h"
#include "qsound.h"

const device_type QSOUND = &device_creator<qsound_device>;

static ADDRESS_MAP_START( dsp16_io_map, AS_IO, 16, qsound_device )
	AM_RANGE(0x0000, 0x7fff) AM_MIRROR(0x8000) AM_READ(dsp_sample_r)
ADDRESS_MAP_END

static MACHINE_CONFIG_FRAGMENT( qsound )
	MCFG_CPU_ADD("qsound_dsp", DSP16, QSOUND_CLOCK) 
	MCFG_CPU_IO_MAP(dsp16_io_map)
MACHINE_CONFIG_END

ROM_START( qsound )
	ROM_REGION( 0x2000, "qsound_dsp", 0 )
	ROM_LOAD16_WORD_SWAP( "dl-1425.bin", 0x0000, 0x2000, CRC(d6cf5ef5) SHA1(555f50fe5cdf127619da7d854c03f4a244a0c501) )
ROM_END

qsound_device::qsound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, QSOUND, "Q-Sound (Hybride)", tag, owner, clock, "qsound", __FILE__),
		device_sound_interface(mconfig, *this),
		m_cpu(*this, "qsound_dsp"),
		m_sample_rom(*this, DEVICE_SELF),
		m_stream(NULL)
{
}

const rom_entry *qsound_device::device_rom_region() const
{
	return ROM_NAME( qsound );
}

machine_config_constructor qsound_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( qsound );
}

void qsound_device::device_start()
{
	// FIX DÉFINITIF : Diviseur calé à 2490 (60000000 / 2490 = 24096.38 Hz d'origine)
	m_stream = stream_alloc(0, 2, clock() / 2490);

	for (int i = 0; i < 33; i++)
		m_pan_table[i] = (int)((256 / sqrt(32.0)) * sqrt((double)i));

	memset(m_channel, 0, sizeof(m_channel));

	for (int adr = 0x7f; adr >= 0; adr--)
		write_data(adr, 0);
	for (int adr = 0x80; adr < 0x90; adr++)
		write_data(adr, 0x120);

	for (int i = 0; i < 16; i++)
	{
		save_item(NAME(m_channel[i].bank), i);
		save_item(NAME(m_channel[i].address), i);
		save_item(NAME(m_channel[i].freq), i);
		save_item(NAME(m_channel[i].loop), i);
		save_item(NAME(m_channel[i].end), i);
		save_item(NAME(m_channel[i].vol), i);
		save_item(NAME(m_channel[i].enabled), i);
		save_item(NAME(m_channel[i].lvol), i);
		save_item(NAME(m_channel[i].rvol), i);
		save_item(NAME(m_channel[i].step_ptr), i);
	}

	m_cpu->set_ock_cb(qsound_device::static_dsp_ock_w);
	m_cpu->set_pio_r_cb(qsound_device::static_dsp_pio_r);
	m_cpu->set_pio_w_cb(qsound_device::static_dsp_pio_w);
}

void qsound_device::device_reset()
{
	m_cmd_pending = 0;
	m_dsp_ready = 1;
	m_cpu->set_input_line(0, CLEAR_LINE); 
}

void qsound_device::sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	// FIX : Indexation correcte des buffers de sortie [0] Gauche et [1] Droite
	memset(outputs[0], 0, samples * sizeof(*outputs[0]));
	memset(outputs[1], 0, samples * sizeof(*outputs[1]));

	for (int ch = 0; ch < 16; ch++)
	{
		if (m_channel[ch].enabled)
		{
			stream_sample_t *lmix = outputs[0];
			stream_sample_t *rmix = outputs[1];

			for (int i = 0; i < samples; i++)
			{
				m_channel[ch].address += (m_channel[ch].step_ptr >> 12);
				m_channel[ch].step_ptr &= 0xfff;
				m_channel[ch].step_ptr += m_channel[ch].freq;

				if (m_channel[ch].address >= m_channel[ch].end)
				{
					if (m_channel[ch].loop)
					{
						m_channel[ch].address -= m_channel[ch].loop;
						if (m_channel[ch].address >= m_channel[ch].end)
							m_channel[ch].address = m_channel[ch].end - m_channel[ch].loop;
						m_channel[ch].address &= 0xffff;
					}
					else
					{
						m_channel[ch].enabled = false;
						break;
					}
				}

				UINT32 rom_addr = m_channel[ch].bank | m_channel[ch].address;
				INT8 sample = (INT8)m_sample_rom[rom_addr & m_sample_rom.mask()];

				*lmix++ += ((sample * m_channel[ch].lvol * m_channel[ch].vol) >> 14);
				*rmix++ += ((sample * m_channel[ch].rvol * m_channel[ch].vol) >> 14);
			}
		}
	}
}

WRITE8_MEMBER(qsound_device::qsound_w)
{
	switch (offset)
	{
		case 0:
			m_new_data = (m_new_data & 0x00ff) | (data << 8);
			break;
		case 1:
			m_new_data = (m_new_data & 0xff00) | data;
			break;
		case 2:
			m_stream->update();
			write_data(data, m_new_data); 
			break;
		default:
			break;
	}
}

READ8_MEMBER(qsound_device::qsound_r)
{
	return 0x80; 
}

void qsound_device::write_data(UINT8 address, UINT16 data)
{
	int ch = 0, reg = 0;

	if (address < 0x80)
	{
		ch = address >> 3;
		reg = address & 7;
	}
	else if (address < 0x90)
	{
		ch = address & 0xf;
		reg = 8;
	}
	else if (address >= 0xba && address < 0xca)
	{
		ch = address - 0xba;
		reg = 9;
	}
	else
	{
		reg = address;
	}

	switch (reg)
	{
		case 0:
			ch = (ch + 1) & 0xf; 
			m_channel[ch].bank = data << 16;
			break;
		case 1:
			m_channel[ch].address = data;
			break;
		case 2:
			m_channel[ch].freq = data;
			if (data == 0)
				m_channel[ch].enabled = false;
			break;
		case 3:
			m_channel[ch].enabled = true;
			m_channel[ch].step_ptr = 0;
			break;
		case 4:
			m_channel[ch].loop = data;
			break;
		case 5:
			m_channel[ch].end = data;
			break;
		case 6:
			m_channel[ch].vol = data;
			break;
		case 8:
		{
			int pan = (data & 0x3f) - 0x10;
			if (pan > 0x20) pan = 0x20;
			if (pan < 0) pan = 0;

			m_channel[ch].rvol = m_pan_table[pan];
			m_channel[ch].lvol = m_pan_table[0x20 - pan];
			break;
		}
		default:
			break;
	}
}

void qsound_device::set_cmd(void *ptr, INT32 param) { }
void qsound_device::set_dsp_ready(void *ptr, INT32 param) { }

READ16_MEMBER(qsound_device::dsp_sample_r) { return 0; }
void qsound_device::dsp_ock_w(int state) { }
void qsound_device::dsp_pio_w(offs_t offset, UINT16 data) { }
UINT16 qsound_device::dsp_pio_r() { return 0; }

void qsound_device::static_dsp_ock_w(device_t *device, int state) { }
void qsound_device::static_dsp_pio_w(device_t *device, offs_t offset, UINT16 data) { }
UINT16 qsound_device::static_dsp_pio_r(device_t *device) { return 0; }
