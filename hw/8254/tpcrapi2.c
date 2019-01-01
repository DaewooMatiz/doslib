
#include <stdio.h>
#include <conio.h> /* this is where Open Watcom hides the outp() etc. functions */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dos.h>

#include <hw/8254/8254.h>

#include <hw/dos/dos.h>
#include <hw/dos/doswin.h>
#include <hw/dos/dosbox.h>
#include <hw/dos/dosntvdm.h>

#ifdef TARGET_WINDOWS
# define WINFCON_STOCK_WIN_MAIN
# include <hw/dos/winfcon.h>
#endif

#pragma pack(push,1)
struct pcspk_ent {
    uint8_t     freq;       // 0 == stop, else upper 8 bits of 16-bit count
    uint8_t     delay;      // 0 = end of list, else timer ticks
};
#pragma pack(pop)

const struct pcspk_ent      duke_entry[] = {
    //freq  delay
    {0x01,  0x1F},
    {0x02,  0x1F},
    {0x03,  0x1F},
    {0x04,  0x1F},
    {0x05,  0x1F},
    {0x06,  0x1F},
    {0x07,  0x1F},
    {0x08,  0x1F},
    {0x09,  0x1F},
    {0x0A,  0x1F},
    {0x0B,  0x1F},
    {0x0C,  0x1F},
    {0x0D,  0x1F},
    {0x0E,  0x1F},
    {0x0F,  0x1F},
    {0x10,  0x1F},
    {0x11,  0x1F},
    {0x12,  0x1F},
    {0x13,  0x1F},
    {0x14,  0x1F},
    {0x15,  0x1F},
    {0x16,  0x1F},
    {0x17,  0x1F},
    {0x18,  0x1F},
    {0x19,  0x1F},
    {0x1A,  0x1F},
    {0x1B,  0x1F},
    {0x1C,  0x1F},
    {0x1D,  0x1F},
    {0x1E,  0x1F},
    {0x1F,  0x1F},
    {0,     0}              // END
};

void play_sfx(const struct pcspk_ent *p) {
    t8254_time_t pc,cc;
    t8254_time_t delta;
    int32_t tim = 0;

    cc = read_8254(0);
    while (p->delay != 0) {
        if (p->freq != 0) {
            write_8254_pc_speaker(p->freq << 8u);
            t8254_pc_speaker_set_gate(PC_SPEAKER_GATE_ON);
        }
        else {
            t8254_pc_speaker_set_gate(PC_SPEAKER_GATE_OFF);
        }

        tim += (int32_t)((unsigned int)p->delay << 8u);
        while (tim >= 0l) {
            pc = cc;
            cc = read_8254(0);
            delta = (pc - cc) & 0xFFFFul; /* counts DOWN */
            tim -= delta;
        }

        p++;
    }

    t8254_pc_speaker_set_gate(PC_SPEAKER_GATE_OFF);
}

int main() {
#if defined(TARGET_WINDOWS) && TARGET_WINDOWS == 32 && !defined(WIN386)
    /* As a 32-bit Windows application, we *can* talk directly to the 8254 but only if running under Windows 3.1/95/98/ME.
     * Windows NT would never permit us to directly talk to IO.
     *
     * However if we're a Win16 program or Watcom 386 application, Windows NT will trap and emulate the 8254 through NTVDM.EXE */
    detect_windows();
    if (!(windows_mode == WINDOWS_REAL || windows_mode == WINDOWS_STANDARD || windows_mode == WINDOWS_ENHANCED)) {
        printf("This test program is only applicable to Windows 3.x/9x\n");
        return 1;
    }
#endif

	printf("8254 library test program\n");
	if (!probe_8254()) {
		printf("Chip not present. Your computer might be 2010-era hardware that dropped support for it.\n");
		return 1;
	}

    printf("This program tests the ability for emulation to handle\n");
    printf("rapid changes to PC speaker frequency.\n");
    printf("Apogee/Duke Nukem style sound effects.\n");

	write_8254_system_timer(0); /* restore normal function to prevent BIOS from going crazy */

    play_sfx(duke_entry);

	write_8254_system_timer(0); /* restore normal function to prevent BIOS from going crazy */
	return 0;
}

