The return of a classic.

After more than a decade of silence, MAME NEHT is back in active development! This project aims to revive and modernize the NEHT custom builds by bridging the gap between historical sources and modern MAME engineering.

This project is based on MAME 0.160 source code to ensure compatibility with specific ROM hacks that no longer work on later official MAME releases.

//CHANGE LOG//

drivers: Major updates for CPS1, CPS2 and Neo-Geo
- [NEOGEO] Added functional driver init for King of Fighters '98 - Dream Match Never Ends (Combo, Ivex's KOF '98 Hack, 2018/02/28).
- [NEOGEO] Added functional driver init for King of Fighters '98 - Dream Match Never Ends (Combo, Ivex's KOF '98 Hack, 2018/03/01).
- [CPS2] Integrated [sf2prime] Street Fighter II': Prime (v0.80) clone set into the cps2 driver.
- [CPS1 CPS2] Updated QSound core with modern ROM compatibility (dl-1425.bin) and custom hybrid HLE driver calibrated at 24096.38 Hz for MAME 0.160.

Integrate HBMAME KOF99 sets into neogeo driver
- Added kof99s111 (Anniversary Edition 2016-04-19)
- Added kof99s102 (Anniversary Edition 2020-03-24)
- Added kof99s185 (Anniversary Edition Original 2020-04-07)
- Added kof99s190 (Anniversary Edition LC+SK 2025-03-09)
- Fixed 'ymsnd' region mapping syntax for 0.160 compatibility

Added neogeo bios files from recent bios
-Added BIOS US MVS (U4)
-Added BIOS US MVS (U3)
-Added BIOS Japan MVS (J3, alt)
-Added BIOS Japan NEO-MVH MV1C
