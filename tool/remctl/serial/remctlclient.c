/* client-side Linux (eventually Windows?) */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

/* for connecting to localhost (DOSBox null modem emulation) */
#include <netinet/in.h>
#include <netinet/ip.h>

/* for connecting to serial port */
#include <termios.h>

#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "proto.h"

#ifndef O_BINARY
#define O_BINARY (0)
#endif

static int              repeat = -1;
static int              baud_rate = -1;
static int              localhost_port = -1;
static char*            serial_tty = NULL;
static int              localhost = 0;
static int              waitconn = 0;
static char*            command = NULL;
static int              debug = 0;
static int              stop_bits = 1;
static int              ioport = -1;
static unsigned long    memaddr = 0;
static signed long      memsz = -1;
static unsigned long    data = 0;
static char*            memstr = NULL;
static char*            input_file = NULL;
static char*            output_file = NULL;
static unsigned char    enterkey = 0;

static int              conn_fd = -1;

static void help(void) {
    fprintf(stderr,"remctlclient [options]\n");
    fprintf(stderr,"Remote control client for RS-232 control of a DOS system.\n");
    fprintf(stderr,"  -h --help         Show this help\n");
    fprintf(stderr,"  -l                Connect to localhost (DOSBox VM)\n");
    fprintf(stderr,"  -p <N>            localhost port number (default 23)\n");
    fprintf(stderr,"  -s <dev>          Connect to serial port device\n");
    fprintf(stderr,"  -w                Wait for connection\n");
    fprintf(stderr,"  -c <command>      Command to execute (default: ping)\n");
    fprintf(stderr,"  -d                Debug (dump packets)\n");
    fprintf(stderr,"  -r <n>            Repeat command N times\n");
    fprintf(stderr,"  -b <rate>         Baud rate\n");
    fprintf(stderr,"  -sb <n>           Stop bits (1 or 2)\n");
    fprintf(stderr,"  -ioport <n>       I/O port\n");
    fprintf(stderr,"  -maddr <n>        Memory address\n");
    fprintf(stderr,"  -msz <n>          Memory size (how much)\n");
    fprintf(stderr,"  -mstr <n>         Memory string to write\n");
    fprintf(stderr,"  -data <n>         data value\n");
    fprintf(stderr,"  -o <file>         Output file\n");
    fprintf(stderr,"  -i <file>         Input file\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"Commands are:\n");
    fprintf(stderr,"   ping             Ping the server (test connection)\n");
    fprintf(stderr,"   halt             Halt the system\n");
    fprintf(stderr,"   unhalt           Un-halt the system\n");
    fprintf(stderr,"   inp -ioport <n>  I/O port read (byte)\n");
    fprintf(stderr,"   inpw -ioport <n> I/O port read (word)\n");
    fprintf(stderr,"   inpd -ioport <n> I/O port read (dword)\n");
    fprintf(stderr,"                    CAUTION: Do not use if server is not 386 or higher\n");
    fprintf(stderr,"   outp -ioport <n> -data <n>  I/O port write (byte)\n");
    fprintf(stderr,"   outpw -ioport <n> -data <n> I/O port write (word)\n");
    fprintf(stderr,"   outpd -ioport <n> -data <n> I/O port write (dword)\n");
    fprintf(stderr,"                    CAUTION: Do not use if server is not 386 or higher\n");
    fprintf(stderr,"   memread -msz <n> -maddr <n> Read server memory\n");
    fprintf(stderr,"   memwrite -msz <n> -maddr <n> -data <n> Write server memory\n");
    fprintf(stderr,"   memwrite -maddr <n> -mstr <x> Write server memory with string\n");
    fprintf(stderr,"   memdump -msz <n> -maddr <n> -o <file> Dump memory starting at -maddr\n");
    fprintf(stderr,"   dos_lol          Report MS-DOS List of Lists location\n");
    fprintf(stderr,"   indos            Report MS-DOS InDOS flag\n");
    fprintf(stderr,"   pwd              Report current working path\n");
    fprintf(stderr,"   chdir -mstr <path> Set current working path (and drive)\n");
    fprintf(stderr,"   mkdir -mstr <path> Set current working path (and drive)\n");
    fprintf(stderr,"   rmdir -mstr <path> Set current working path (and drive)\n");
    fprintf(stderr,"   dir -mstr <path> Enumerate directory contents\n");
    fprintf(stderr,"   open -mstr <path> Open file (fail if exists)\n");
    fprintf(stderr,"   create -mstr <path> Create file (truncate if exists)\n");
    fprintf(stderr,"   close            Close currently open file\n");
    fprintf(stderr,"   seek -data <off> Seek file pointer\n");
    fprintf(stderr,"   seekcur -data <off> Seek file pointer from cur\n");
    fprintf(stderr,"   seekend -data <off> Seek file pointer from end\n");
    fprintf(stderr,"   read -msz <count> Read count bytes\n");
    fprintf(stderr,"   truncate          Truncate at file pointer\n");
    fprintf(stderr,"   upload -mstr <path> -i <path> Send file from -i to -mstr path\n");
    fprintf(stderr,"   download -mstr <path> -o <path> Download file from -mstr to -o locally\n");
    fprintf(stderr,"   stuffkey -data <code> Stuff code into BIOS keyboard buffer\n");
    fprintf(stderr,"   stuffkey -mstr <string> Stuff ASCII codes into BIOS keyboard buffer\n");
    fprintf(stderr,"   stuffkey -mstr <string> -enter Stuff ASCII codes, then send ENTER key\n");
}

static int parse_argv(int argc,char **argv) {
    char *a;
    int i=1;

    while (i < argc) {
        a = argv[i++];

        if (*a == '-') {
            do { a++; } while (*a == '-');

            if (!strcmp(a,"h") || !strcmp(a,"help")) {
                help();
                return 1;
            }
            else if (!strcmp(a,"l")) {
                localhost = 1;
            }
            else if (!strcmp(a,"r")) {
                a = argv[i++];
                if (a == NULL) return 1;
                repeat = atoi(a);
            }
            else if (!strcmp(a,"enter")) {
                enterkey = 1;
            }
            else if (!strcmp(a,"mstr")) {
                a = argv[i++];
                if (a == NULL) return 1;
                memstr = a;
            }
            else if (!strcmp(a,"i")) {
                a = argv[i++];
                if (a == NULL) return 1;
                input_file = a;
            }
            else if (!strcmp(a,"o")) {
                a = argv[i++];
                if (a == NULL) return 1;
                output_file = a;
            }
            else if (!strcmp(a,"b")) {
                a = argv[i++];
                if (a == NULL) return 1;
                baud_rate = strtoul(a,NULL,0);
            }
            else if (!strcmp(a,"data")) {
                a = argv[i++];
                if (a == NULL) return 1;
                data = strtoul(a,NULL,0);
            }
            else if (!strcmp(a,"ioport")) {
                a = argv[i++];
                if (a == NULL) return 1;
                ioport = strtoul(a,NULL,0);
            }
            else if (!strcmp(a,"maddr")) {
                a = argv[i++];
                if (a == NULL) return 1;
                memaddr = strtoul(a,NULL,0);
            }
            else if (!strcmp(a,"msz")) {
                a = argv[i++];
                if (a == NULL) return 1;
                memsz = strtoul(a,NULL,0);
            }
            else if (!strcmp(a,"p")) {
                a = argv[i++];
                if (a == NULL) return 1;
                localhost_port = atoi(a);
                localhost = 1;
            }
            else if (!strcmp(a,"c")) {
                a = argv[i++];
                if (a == NULL) return 1;
                command = a;
            }
            else if (!strcmp(a,"s")) {
                a = argv[i++];
                if (a == NULL) return 1;
                serial_tty = a;
            }
            else if (!strcmp(a,"sb")) {
                a = argv[i++];
                if (a == NULL) return 1;
                stop_bits = atoi(a);
                if (stop_bits < 1) stop_bits = 1;
                else if (stop_bits > 2) stop_bits = 2;
            }
            else if (!strcmp(a,"w")) {
                waitconn = 1;
            }
            else if (!strcmp(a,"d")) {
                debug = 1;
            }
            else {
                fprintf(stderr,"Unknown switch %s\n",a);
                return 1;
            }
        }
        else {
            fprintf(stderr,"Unexpected argv\n");
            return 1;
        }
    }

    if (command == NULL)
        command = "ping";

    if (baud_rate < 0)
        baud_rate = 115200;

    if (serial_tty != NULL) {
        /* Motherboard RS-232 ports are usually /dev/ttyS0, /dev/ttyS1, etc.
         * USB RS-232 ports are usually /dev/ttyUSB0, /dev/ttyUSB1, etc. */
        if (!strncmp(serial_tty,"/dev/tty",8)) {
            /* good */
        }
        else {
            return 1;
        }
    }
    else if (localhost) {
        if (localhost_port < 0)
            localhost_port = 23;

        if (localhost_port < 1 || localhost_port > 65534) {
            fprintf(stderr,"Invalid localhost port\n");
            return 1;
        }
    }
    else {
        help();
        return 1;
    }

    return 0;
}

/* make a file descriptor non-blocking */
int nonblocking_fd(const int fd) {
    int x,r;

    x = fcntl(fd,F_GETFL);
    if (x < 0) return x;

    x |= O_NONBLOCK;

    if ((r=fcntl(fd,F_SETFL,x)) < 0)
        return r;

    return 0;
}

void drop_connection(void) {
    if (conn_fd >= 0) {
        close(conn_fd);
        conn_fd = -1;
    }
}

int do_connection(void) {
    if (conn_fd >= 0)
        return 0;

    if (localhost) {
        struct sockaddr_in sin;

        conn_fd = socket(AF_INET,SOCK_STREAM,PF_UNSPEC);
        if (conn_fd < 0) {
            fprintf(stderr,"Socket() failed, %s\n",strerror(errno));
            drop_connection();
            return -1;
        }

        memset(&sin,0,sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(localhost_port);
        sin.sin_addr.s_addr = htonl(0x7F000001UL); /* 127.0.0.1 */
        if (connect(conn_fd,(const struct sockaddr*)(&sin),sizeof(sin)) < 0) {
            fprintf(stderr,"Connect failed, %s\n",strerror(errno));
            drop_connection();
            return -1;
        }

        if (nonblocking_fd(conn_fd) < 0)
            fprintf(stderr,"WARNING: failed to make socket non-blocking\n");
    }
    else if (serial_tty != NULL) {
        struct termios tios;

        conn_fd = open(serial_tty,O_RDWR);
        if (conn_fd < 0) {
            fprintf(stderr,"open() failed, %s\n",strerror(errno));
            drop_connection();
            return -1;
        }

        if (tcgetattr(conn_fd,&tios) < 0) {
            fprintf(stderr,"Failed to get termios info, %s\n",strerror(errno));
            drop_connection();
            return -1;
        }
        tcflush(conn_fd,TCIOFLUSH);
        cfmakeraw(&tios);
        if (cfsetspeed(&tios,baud_rate) < 0) {
            fprintf(stderr,"Failed to set baud rate, %s\n",strerror(errno));
            drop_connection();
            return -1;
        }

        tios.c_cflag &= ~CSTOPB;
        if (stop_bits == 2) tios.c_cflag |= CSTOPB;
        tios.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL|ECHOPRT|ECHOKE);
        tios.c_cflag &= ~(CSIZE);
        tios.c_cflag |= CS8;

        if (tcsetattr(conn_fd,TCSANOW,&tios) < 0) {
            fprintf(stderr,"Failed to set termios info, %s\n",strerror(errno));
            drop_connection();
            return -1;
        }

        if (nonblocking_fd(conn_fd) < 0)
            fprintf(stderr,"WARNING: failed to make socket non-blocking\n");
    }
    else {
        return -1;
    }

    return 0;
}

int do_connect(void) {
    while (conn_fd < 0) {
        if (do_connection()) {
            if (!waitconn)
                return -1;
            else
                usleep(250);
        }
    }

    return 0;
}

int do_disconnect(void) {
    drop_connection();
    return 0;
}

struct remctl_serial_packet         cur_pkt;
unsigned char                       cur_pkt_seq=0xFF;
unsigned char                       cur_pkt_recv_seq=0xFF;

void remctl_serial_packet_begin(struct remctl_serial_packet * const pkt,const unsigned char type) {
    pkt->hdr.mark = REMCTL_SERIAL_MARK;
    pkt->hdr.length = 0;
    pkt->hdr.sequence = cur_pkt_seq;
    pkt->hdr.type = type;
    pkt->hdr.chksum = 0;

    if (cur_pkt_seq == 0xFF)
        cur_pkt_seq = 0;
    else
        cur_pkt_seq = (cur_pkt_seq+1)&0x7F;
}

void remctl_serial_packet_end(struct remctl_serial_packet * const pkt) {
    unsigned char sum = 0;
    unsigned int i;

    for (i=0;i < pkt->hdr.length;i++)
        sum += pkt->data[i];

    pkt->hdr.chksum = 0x100 - sum;
}

void dump_packet(const struct remctl_serial_packet * const pkt) {
    unsigned int i;

    fprintf(stderr,"   mark=0x%02x length=%u sequence=%u type=0x%02x('%c') chk=0x%02x\n",
        pkt->hdr.mark,
        pkt->hdr.length,
        pkt->hdr.sequence,
        pkt->hdr.type,
        pkt->hdr.type,
        pkt->hdr.chksum);

    if (pkt->hdr.length != 0) {
        fprintf(stderr,"        data: ");
        for (i=0;i < pkt->hdr.length;i++) fprintf(stderr,"%02x ",pkt->data[i]);
        fprintf(stderr,"\n");
    }
}

int write_persistent(const int fd,const void *p,int sz);

int do_send_packet(const struct remctl_serial_packet * const pkt) {
    if (conn_fd < 0)
        return -1;

    if (debug) {
        fprintf(stderr,"Sending packet:\n");
        dump_packet(pkt);
    }

    if (write_persistent(conn_fd,(const void*)(&(pkt->hdr)),sizeof(pkt->hdr)) != sizeof(pkt->hdr)) {
        fprintf(stderr,"Send failed: header\n");
        drop_connection();
        return -1;
    }

    if (pkt->hdr.length != 0 && write_persistent(conn_fd,pkt->data,pkt->hdr.length) != pkt->hdr.length) {
        fprintf(stderr,"Send failed: data\n");
        drop_connection();
        return -1;
    }

    return 0;
}

int read_persistent(const int fd,void *p,int sz) {
    int patience = 5000; /* 5000 x 1ms = 5 seconds */
    int rd = 0;
    int rt;

    while (sz > 0) {
        rt = read(fd,p,sz);
        if (rt < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (--patience < 0) {
                    fprintf(stderr,"Read timeout\n");
                    if (rd != 0)
                        break;
                    else
                        return -1;
                }

                /* non-blocking handling */
                usleep(1000); /* 1ms */
                continue;
            }
            return rt;
        }
        if (rt == 0) break; /* EOF */
        assert(rt <= sz);
        p = (void*)((char*)p + rt);
        sz -= rt;
        rd += rt;
    }

    return rd;
}

int write_persistent(const int fd,const void *p,int sz) {
    int rd = 0;
    int rt;

    while (sz > 0) {
        rt = write(fd,p,sz);
        if (rt < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* non-blocking handling */
                usleep(1000);
                continue;
            }
            return rt;
        }
        if (rt == 0) break; /* EOF */
        assert(rt <= sz);
        p = (const void*)((const char*)p + rt);
        sz -= rt;
        rd += rt;
    }

    return rd;
}

void reset_packet_io(void) {
    char tmp[300];

    cur_pkt_recv_seq = 0xFF;
    cur_pkt_seq=0xFF;

    /* it's likely no response came back because the DOS machine missed a byte
     * and is waiting for the packet to complete. so to force reset it, we have
     * to flood it with bytes */
    /* TODO: We need to do this in a way that deliberately makes sure the partial
     *       packet checksum does not work, so that it is discarded. */
    if (conn_fd >= 0) {
        memset(tmp,0xFF,sizeof(tmp));
        write_persistent(conn_fd,tmp,sizeof(tmp));
    }
}

int do_recv_packet(struct remctl_serial_packet * const pkt) {
    if (conn_fd < 0)
        return -1;

    do {
        if (read_persistent(conn_fd,&(pkt->hdr.mark),1) != 1) {
            reset_packet_io();
            drop_connection();
            return -1;
        }
    } while (pkt->hdr.mark != REMCTL_SERIAL_MARK);

    /* length is the first field after mark */
    if (read_persistent(conn_fd,&(pkt->hdr.length),sizeof(pkt->hdr)-offsetof(struct remctl_serial_packet_header,length)) !=
            (sizeof(pkt->hdr)-offsetof(struct remctl_serial_packet_header,length))) {
        reset_packet_io();
        drop_connection();
        return -1;
    }

    if (pkt->hdr.length != 0) {
        if (read_persistent(conn_fd,pkt->data,pkt->hdr.length) != pkt->hdr.length) {
            reset_packet_io();
            drop_connection();
            return -1;
        }
    }

    if (debug) {
        fprintf(stderr,"Received packet:\n");
        dump_packet(pkt);
    }

    {
        unsigned char sum = pkt->hdr.chksum;
        unsigned int i;

        for (i=0;i < pkt->hdr.length;i++)
            sum += pkt->data[i];

        if (sum != 0) {
            fprintf(stderr,"Checksum failed\n");
            drop_connection();
            return -1;
        }
    }

    if (pkt->hdr.sequence == 0xFF)
        cur_pkt_recv_seq = 0xFF;

    if (cur_pkt_recv_seq != 0xFF) {
        if (pkt->hdr.sequence != cur_pkt_recv_seq) {
            fprintf(stderr,"Sequence failure\n");
            drop_connection();
            return -1;
        }

        cur_pkt_recv_seq = (cur_pkt_recv_seq+1)&0x7F;
    }
    else {
        cur_pkt_recv_seq = (pkt->hdr.sequence+1)&0x7F;
    }

    return 0;
}

int do_ping(void) {
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_PING);

    strncpy((char*)(cur_pkt.data+cur_pkt.hdr.length),"PING",4);
    cur_pkt.hdr.length += 4;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_PING ||
        memcmp(cur_pkt.data,"PING",4) != 0) {
        fprintf(stderr,"Ping failed, malformed response\n");
        return -1;
    }

    return 0;
}

int do_halt(const unsigned char flag) {
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_HALT);

    cur_pkt.data[cur_pkt.hdr.length] = flag;
    cur_pkt.hdr.length += 1;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_HALT ||
        cur_pkt.data[0] != flag) {
        fprintf(stderr,"Halt failed\n");
        return -1;
    }

    return 0;
}

int do_inp(const unsigned char sz,unsigned long * const data) {
    unsigned int i;

    if (sz == 0 || sz > 4)
        return -1;

    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_INPORT);

    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)(ioport & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)((ioport >> 8) & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = sz;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_INPORT ||
        cur_pkt.data[2] != sz) {
        fprintf(stderr,"I/O read failed\n");
        return -1;
    }

    *data = cur_pkt.data[3];
    for (i=1;i < sz;i++)
        *data += ((unsigned long)(cur_pkt.data[3+i])) << ((unsigned long)i * 8U);

    return 0;
}

int do_outp(const unsigned char sz,const unsigned long data) {
    unsigned int i;

    if (sz == 0 || sz > 4)
        return -1;

    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_OUTPORT);

    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)(ioport & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)((ioport >> 8) & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = sz;
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)data;
    for (i=1;i < sz;i++)
        cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)(data >> (i * 8UL));

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_OUTPORT ||
        cur_pkt.data[2] != sz) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

const unsigned char *do_memread(const unsigned char sz,const unsigned long addr) {
    if (sz == 0 || sz > 192)
        return NULL;

    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_MEMREAD);

    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)( addr & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)((addr >> 8UL) & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)((addr >> 16UL) & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)((addr >> 24UL) & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = sz;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return NULL;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return NULL;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_MEMREAD ||
        cur_pkt.data[4] != sz) {
        fprintf(stderr,"I/O write failed\n");
        return NULL;
    }

    /* data starts at byte 5 */
    return &(cur_pkt.data[5]);
}

int do_memwrite(const unsigned char sz,const unsigned long addr,const unsigned char *str) {
    if (sz == 0 || sz > 192)
        return -1;

    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_MEMWRITE);

    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)( addr & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)((addr >> 8UL) & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)((addr >> 16UL) & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)((addr >> 24UL) & 0xFF);
    cur_pkt.data[cur_pkt.hdr.length++] = sz;

    memcpy(cur_pkt.data+cur_pkt.hdr.length,str,sz);
    cur_pkt.hdr.length += sz;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_MEMWRITE ||
        cur_pkt.data[4] != sz) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

int do_get_dos_lol(unsigned int * const sv,unsigned int * const ov) {
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_DOS);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_DOS_LOL;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_DOS ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_DOS_LOL) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    *ov =   ((unsigned int)cur_pkt.data[1]      ) +
            ((unsigned int)cur_pkt.data[2] << 8U);
    *sv =   ((unsigned int)cur_pkt.data[3]      ) +
            ((unsigned int)cur_pkt.data[4] << 8U);
    return 0;
}

int do_get_indos(unsigned int * const sv,unsigned int * const ov) {
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_DOS);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_DOS_INDOS;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_DOS ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_DOS_INDOS) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    *ov =   ((unsigned int)cur_pkt.data[1]      ) +
            ((unsigned int)cur_pkt.data[2] << 8U);
    *sv =   ((unsigned int)cur_pkt.data[3]      ) +
            ((unsigned int)cur_pkt.data[4] << 8U);
    return 0;
}

const unsigned char *do_get_pwd() {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_PWD;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return NULL;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return NULL;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_PWD) {
        fprintf(stderr,"I/O write failed\n");
        return NULL;
    }
    cur_pkt.data[cur_pkt.hdr.length] = 0;
    cur_pkt.data[sizeof(cur_pkt.data)-1] = 0;
    return (const unsigned char*)(cur_pkt.data+1);;
}

int do_set_chdir(const char * const str) {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_CHDIR;

    {
        size_t l = strlen(str);
        if (l > 190) return -1;
        memcpy(cur_pkt.data+cur_pkt.hdr.length,str,l);
        cur_pkt.hdr.length += l;
    }

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_CHDIR) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

int do_set_mkdir(const char * const str) {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_MKDIR;

    {
        size_t l = strlen(str);
        if (l > 190) return -1;
        memcpy(cur_pkt.data+cur_pkt.hdr.length,str,l);
        cur_pkt.hdr.length += l;
    }

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_MKDIR) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

int do_set_rmdir(const char * const str) {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_RMDIR;

    {
        size_t l = strlen(str);
        if (l > 190) return -1;
        memcpy(cur_pkt.data+cur_pkt.hdr.length,str,l);
        cur_pkt.hdr.length += l;
    }

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_RMDIR) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

int do_begin_dir(const char * const str) {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_FIND;

    {
        size_t l = strlen(str);
        if (l > 190) return -1;
        memcpy(cur_pkt.data+cur_pkt.hdr.length,str,l);
        cur_pkt.hdr.length += l;
    }

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_FINISHED) {
        return 0;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_FIND) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    /* got result */
    return 1;
}

int do_next_dir() {
    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy... at unexpected time\n");
        return -1;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_FINISHED) {
        return 0;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_FIND) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    /* got result */
    return 1;
}

int do_stuff_key(const unsigned short code) {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_DOS);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_DOS_STUFF_BIOS_KEYBOARD;
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)(code >> 0U);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)(code >> 8U);

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_DOS &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    /* the keyboard buffer could be full */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_DOS &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_FULL) {
        fprintf(stderr,"Keyboard buffer is full...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_DOS ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_DOS_STUFF_BIOS_KEYBOARD) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

int do_file_create(const char * const str) {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_CREATE;

    {
        size_t l = strlen(str);
        if (l > 190) return -1;
        memcpy(cur_pkt.data+cur_pkt.hdr.length,str,l);
        cur_pkt.hdr.length += l;
    }

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_CREATE) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

int do_file_open(const char * const str) {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_OPEN;

    {
        size_t l = strlen(str);
        if (l > 190) return -1;
        memcpy(cur_pkt.data+cur_pkt.hdr.length,str,l);
        cur_pkt.hdr.length += l;
    }

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_OPEN) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

int do_file_close(void) {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_CLOSE;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_CLOSE) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

int do_file_seek(unsigned long * const noff,const unsigned long roff,const unsigned char how) {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_SEEK;
    cur_pkt.data[cur_pkt.hdr.length++] = how;
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)(roff >> 0UL);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)(roff >> 8UL);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)(roff >> 16UL);
    cur_pkt.data[cur_pkt.hdr.length++] = (unsigned char)(roff >> 24UL);

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_SEEK) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    *noff =
        ((unsigned long)cur_pkt.data[2] << 0UL) +
        ((unsigned long)cur_pkt.data[3] << 8UL) +
        ((unsigned long)cur_pkt.data[4] << 16UL) +
        ((unsigned long)cur_pkt.data[5] << 24UL);
    return 0;
}

unsigned char *do_file_read(int * const got_rd,const int do_rd) {
    if (do_rd > 188)
        return NULL;

retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_READ;
    cur_pkt.data[cur_pkt.hdr.length++] = do_rd;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return NULL;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return NULL;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return NULL;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_READ) {
        fprintf(stderr,"I/O write failed\n");
        return NULL;
    }

    if (got_rd != NULL)
        *got_rd = cur_pkt.data[1];

    return (unsigned char*)(cur_pkt.data + 2);
}

int do_file_write(const unsigned char * const data,const unsigned int len) {
    if (len > 188)
        return -1;

retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_WRITE;
    cur_pkt.data[cur_pkt.hdr.length++] = len;

    if (len != 0) {
        memcpy(cur_pkt.data+cur_pkt.hdr.length,data,len);
        cur_pkt.hdr.length += len;
    }

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_WRITE) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return cur_pkt.data[1];
}

int do_file_truncate() {
retry:
    remctl_serial_packet_begin(&cur_pkt,REMCTL_SERIAL_TYPE_FILE);

    cur_pkt.data[cur_pkt.hdr.length++] = REMCTL_SERIAL_TYPE_FILE_TRUNCATE;

    remctl_serial_packet_end(&cur_pkt);

    if (do_send_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to send packet\n");
        return -1;
    }

    if (do_recv_packet(&cur_pkt) < 0) {
        fprintf(stderr,"Failed to recv packet\n");
        return -1;
    }

    /* MS-DOS might be busy at this point... */
    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_IS_BUSY) {
        fprintf(stderr,"MS-DOS is busy...\n");
        usleep(10000);
        goto retry;
    }

    if (cur_pkt.hdr.type == REMCTL_SERIAL_TYPE_FILE &&
        cur_pkt.data[0] == REMCTL_SERIAL_TYPE_FILE_MSDOS_ERROR) {
        fprintf(stderr,"MS-DOS returned an error\n");
        return -1;
    }

    if (cur_pkt.hdr.type != REMCTL_SERIAL_TYPE_FILE ||
        cur_pkt.data[0] != REMCTL_SERIAL_TYPE_FILE_TRUNCATE) {
        fprintf(stderr,"I/O write failed\n");
        return -1;
    }

    return 0;
}

void do_print_dir(void) {
    /* the contents are a MS-DOS FileInfoRec */
    unsigned char *p = cur_pkt.data + 1;
    unsigned char bAttr = p[0];
    unsigned short rTime = ((unsigned short)p[1] + ((unsigned short)p[2] << 8U));
    unsigned short rDate = ((unsigned short)p[3] + ((unsigned short)p[4] << 8U));
    unsigned long lSize = ((unsigned long)p[5] + ((unsigned long)p[6] << 8UL) +
        ((unsigned long)p[7] << 16UL) + ((unsigned long)p[8] << 24UL));
    char *szFilespec = (char*)(p + 9);
    cur_pkt.data[cur_pkt.hdr.length] = 0;

    if ((bAttr & 0xF) == 0x0F)
        return;

    if (bAttr & 0x8)
        printf("  <LABEL> ");
    else if (bAttr & 0x10)
        printf("  <DIR>   ");
    else
        printf("  <FILE>  ");

    printf("%c%c%c%c ",
        (bAttr & 0x01) ? 'R' : '-',
        (bAttr & 0x02) ? 'H' : '-',
        (bAttr & 0x04) ? 'S' : '-',
        (bAttr & 0x20) ? 'A' : '-');

    printf("%-13s ",szFilespec);

    printf("%-10lub ",lSize);

    printf("%02u:%02u:%02u ",
         (rTime >> 11U) & 0x1FU,
         (rTime >>  5U) & 0x3FU,
        ((rTime >>  0U) & 0x1FU) * 2U);

    printf("%04u/%02u/%02u ",
        ((rDate >>  9U) & 0x7FU) + 1980,
         (rDate >>  5U) & 0x0FU,
         (rDate >>  0U) & 0x1FU);

    printf("\n");
}

int main(int argc,char **argv) {
    unsigned int i;

    if (parse_argv(argc,argv))
        return 1;

    if (do_connect())
        return 1;

    if (!strcmp(command,"ping")) {
        int count = 1;

        if (repeat > 1)
            count = repeat;

        while (count-- > 0) {
            if (do_ping() < 0)
                break;

            printf("Ping OK\n");
        }
    }
    else if (!strcmp(command,"halt")) {
        if (do_halt(1) < 0)
            return 1;

        printf("Halt OK\n");
    }
    else if (!strcmp(command,"unhalt")) {
        if (do_halt(0) < 0)
            return 1;

        printf("Un-halt OK\n");
    }
    else if (!strcmp(command,"inp")) {
        if (ioport < 0 || ioport > 65535) {
            fprintf(stderr,"I/O port out of range\n");
            return 1;
        }

        if (do_inp(1,&data) < 0)
            return 1;

        printf("I/O port 0x%X: got 0x%02lx\n",ioport,data);
    }
    else if (!strcmp(command,"inpw")) {
        if (ioport < 0 || ioport > 65535) {
            fprintf(stderr,"I/O port out of range\n");
            return 1;
        }

        if (do_inp(2,&data) < 0)
            return 1;

        printf("I/O port 0x%X: got 0x%04lx\n",ioport,data);
    }
    else if (!strcmp(command,"inpd")) {
         if (ioport < 0 || ioport > 65535) {
            fprintf(stderr,"I/O port out of range\n");
            return 1;
        }

        if (do_inp(4,&data) < 0)
            return 1;

        printf("I/O port 0x%X: got 0x%08lx\n",ioport,data);
    }
    else if (!strcmp(command,"outp")) {
        if (ioport < 0 || ioport > 65535) {
            fprintf(stderr,"I/O port out of range\n");
            return 1;
        }

        if (do_outp(1,data) < 0)
            return 1;

        printf("I/O write OK\n");
    }
    else if (!strcmp(command,"outpw")) {
        if (ioport < 0 || ioport > 65535) {
            fprintf(stderr,"I/O port out of range\n");
            return 1;
        }

        if (do_outp(2,data) < 0)
            return 1;

        printf("I/O write OK\n");
    }
     else if (!strcmp(command,"outpd")) {
        if (ioport < 0 || ioport > 65535) {
            fprintf(stderr,"I/O port out of range\n");
            return 1;
        }

        if (do_outp(4,data) < 0)
            return 1;

        printf("I/O write OK\n");
    }
    else if (!strcmp(command,"memread")) {
        const unsigned char *ptr;

        if (memsz < 0L)
            memsz = 1L;

        if (memsz > 192L)
            memsz = 192L;

        if ((ptr=do_memread(memsz,memaddr)) == NULL)
            return 1;

        printf("Read OK: %lu bytes\n",memsz);
        for (i=0;i < (unsigned int)memsz;i++) printf("%02X ",ptr[i]);
        printf("\n");
    }
    else if (!strcmp(command,"memwrite")) {
        if (memstr != NULL) {
            size_t l = strlen(memstr);
            if (memsz <= 0L || memsz > (long)l) memsz = l;
            if (memsz > 192L) memsz = 192L;

            if (do_memwrite(memsz,memaddr,(unsigned char*)memstr) < 0)
                return 1;
        }
        else {
            unsigned char tmp[4];

            if (memsz <= 0L) memsz = 1L;
            if (memsz > 4L) memsz = 4L;
            tmp[0] = (unsigned char)data;
            tmp[1] = (unsigned char)(data >> 8UL);
            tmp[2] = (unsigned char)(data >> 16UL);
            tmp[3] = (unsigned char)(data >> 24UL);

            if (do_memwrite(memsz,memaddr,tmp) < 0)
                return 1;
        }

        printf("Wrtie OK: %lu bytes\n",memsz);
    }
    else if (!strcmp(command,"memdump")) {
        const unsigned char *ptr;
        unsigned long addr;
        int do_read;
        int fd;

        if (output_file == NULL)
            return 1;
        if (memsz <= 0)
            return 1;

        fd = open(output_file,O_BINARY|O_CREAT|O_TRUNC|O_WRONLY,0644);
        if (fd < 0) {
            fprintf(stderr,"Cannot open output file %s, %s\n",output_file,strerror(errno));
            return 1;
        }

        addr = memaddr;
        while (memsz > 0) {
            if (memsz > 192)
                do_read = 192;
            else
                do_read = (int)memsz;

            printf("\x0D" "Reading 0x%08lX + %u bytes",addr,do_read);
            fflush(stdout);

            if ((ptr=do_memread(do_read,addr)) == NULL) {
                fprintf(stderr,"\nReconnecting...\n");
                do_connect();
                continue;
            }
            if (write(fd,ptr,do_read) != do_read)
                break;

            memsz -= do_read;
            addr += do_read;
        }
        printf("\n");

        close(fd);
    }
    else if (!strcmp(command,"dos_lol")) {
        unsigned int sv,ov;

        if (do_get_dos_lol(&sv,&ov) < 0)
            return 1;

        printf("MS-DOS List of Lists is at %04x:%04x\n",sv,ov);
    }
    else if (!strcmp(command,"indos")) {
        unsigned int sv,ov;

        if (do_get_indos(&sv,&ov) < 0)
            return 1;

        printf("MS-DOS InDOS flag is at %04x:%04x\n",sv,ov);
    }
    else if (!strcmp(command,"pwd")) {
        const unsigned char *str;
        int count = 1;

        if (repeat > 1)
            count = repeat;

        while (count-- > 0) {
            if ((str=do_get_pwd()) == NULL)
                return 1;

            printf("Current working directory: '%s'\n",str);
        }
    }
    else if (!strcmp(command,"chdir")) {
        if (memstr == NULL) {
            fprintf(stderr,"need -mstr to contain path\n");
            return 1;
        }

        if (do_set_chdir(memstr) < 0)
            return 1;

        printf("CHDIR OK\n");
    }
    else if (!strcmp(command,"mkdir")) {
        if (memstr == NULL) {
            fprintf(stderr,"need -mstr to contain path\n");
            return 1;
        }

        if (do_set_mkdir(memstr) < 0)
            return 1;

        printf("CHDIR OK\n");
    }
    else if (!strcmp(command,"rmdir")) {
        if (memstr == NULL) {
            fprintf(stderr,"need -mstr to contain path\n");
            return 1;
        }

        if (do_set_rmdir(memstr) < 0)
            return 1;

        printf("CHDIR OK\n");
    }
    else if (!strcmp(command,"dir")) {
        int r;

        if (memstr == NULL) {
            fprintf(stderr,"need -mstr to contain path (use *.* for current dir)\n");
            return 1;
        }

        if ((r=do_begin_dir(memstr)) <= 0)
            return 1;

        do {
            do_print_dir();
        } while ((r=do_next_dir()) > 0);

        printf("* done\n");
    }
    else if (!strcmp(command,"open")) {
        if (memstr == NULL) {
            fprintf(stderr,"need -mstr to contain path\n");
            return 1;
        }

        if (do_file_open(memstr) < 0)
            return 1;

        printf("OPEN OK\n");
    }
    else if (!strcmp(command,"create")) {
        if (memstr == NULL) {
            fprintf(stderr,"need -mstr to contain path\n");
            return 1;
        }

        if (do_file_create(memstr) < 0)
            return 1;

        printf("CREATE OK\n");
    }
    else if (!strcmp(command,"close")) {
        if (do_file_close() < 0)
            return 1;

        printf("CLOSE OK\n");
    }
    else if (!strcmp(command,"seek")) {
        if (do_file_seek(&data,data,0/*SEEK_SET*/) < 0)
            return 1;

        printf("SEEK OK, position is %lu\n",data);
    }
    else if (!strcmp(command,"seekcur")) {
        if (do_file_seek(&data,data,1/*SEEK_CUR*/) < 0)
            return 1;

        printf("SEEK OK, position is %lu\n",data);
    }
    else if (!strcmp(command,"seekend")) {
        if (do_file_seek(&data,data,2/*SEEK_END*/) < 0)
            return 1;

        printf("SEEK OK, position is %lu\n",data);
    }
    else if (!strcmp(command,"read")) {
        const unsigned char *str;
        int count = 1;
        int rd,i;

        if (memsz > 188)
            memsz = 188;

        if (repeat > 1)
            count = repeat;

        while (count-- > 0) {
            if ((str=do_file_read(&rd,memsz)) == NULL)
                return 1;

            printf("READ result %u/%u bytes\n",rd,(int)memsz);
            if (rd != 0) {
                for (i=0;i < rd;i++) printf("%02X ",str[i]);
                printf("\n");
            }

            if (rd == 0)
                break;
        }
    }
    else if (!strcmp(command,"write")) {
        int count = 1;
        int rd;

        if (memstr == NULL) {
            fprintf(stderr,"need -mstr to contain data\n");
            return 1;
        }

        {
            size_t l = strlen(memstr);
            if (memsz <= 0L || memsz > (long)l) memsz = l;
            if (memsz > 192L) memsz = 192L;
        }

        if (repeat > 1)
            count = repeat;

        while (count-- > 0) {
            if ((rd=do_file_write((const unsigned char*)memstr,memsz)) < 0)
                return 1;

            printf("WRITE result %u/%u bytes\n",rd,(int)memsz);
            if (rd == 0)
                break;
        }
    }
    else if (!strcmp(command,"truncate")) {
        int count = 1;

        if (repeat > 1)
            count = repeat;

        while (count-- > 0) {
            if (do_file_truncate() < 0)
                return 1;

            printf("TRUNCATE OK\n");
        }
    }
    else if (!strcmp(command,"upload")) {
        unsigned char tmp[256];
        long file_size,count;
        int ifd,rwd,wd,doc;
        time_t now,next;

        next = 0;

        if (memstr == NULL) {
            fprintf(stderr,"need -mstr to contain path to upload to\n");
            return 1;
        }
        if (input_file == NULL) {
            fprintf(stderr,"need -i to specify source file\n");
            return 1;
        }

        ifd = open(input_file,O_RDONLY|O_BINARY);
        if (ifd < 0) {
            fprintf(stderr,"Failed to open source file\n");
            return 1;
        }
        file_size = lseek(ifd,0,SEEK_END);
        if (file_size < 0L) {
            fprintf(stderr,"Cannot determine source file size\n");
            return 1;
        }
        lseek(ifd,0,SEEK_SET);

        if (do_file_create(memstr) < 0)
            return 1;

        count = 0L;
        while (count < file_size) {
            now = time(NULL);
            if (now >= next) {
                unsigned long percent;

                percent = (count >> 7UL) * 100UL;
                percent /= ((file_size + 127UL) >> 7UL);

                next = now + 1;
                printf("\x0D" "Uploading, %lu%% %lu / %lu... ",
                    percent,count,file_size);
                fflush(stdout);
            }

            if ((file_size - count) > 188)
                doc = 188;
            else
                doc = (int)(file_size - count);

            assert(doc != 0);
            wd = read(ifd,tmp,doc);
            if (wd == 0) break;

            if ((rwd=do_file_write(tmp,wd)) < 0) {
                fprintf(stderr,"\nReconnecting...\n");
                do_connect(); /* retry */

                /* now, where were we...? */
                if (lseek(ifd,count,SEEK_SET) != count)
                    return 1;
                if (do_file_seek(&data,count,0/*SEEK_SET*/) < 0 || (unsigned long)data != (unsigned long)count)
                    return 1;

                continue;
            }

            if (rwd < wd) {
                fprintf(stderr,"Remote end incomplete write\n");
                break;
            }

            count += wd;
        }

        if (do_file_close() < 0)
            return 1;

        if (count != file_size) {
            printf("Upload incomplete\n");
        }
        else {
            printf("Upload OK\n");
        }

        close(ifd);
    }
    else if (!strcmp(command,"download")) {
        long file_size,count;
        unsigned char *str;
        int ofd,rwd,wd,doc;
        time_t now,next;

        next = 0;

        if (memstr == NULL) {
            fprintf(stderr,"need -mstr to contain path to upload from\n");
            return 1;
        }
        if (output_file == NULL) {
            fprintf(stderr,"need -o to specify local target file\n");
            return 1;
        }

        ofd = open(output_file,O_WRONLY|O_BINARY|O_CREAT|O_TRUNC,0644);
        if (ofd < 0) {
            fprintf(stderr,"Failed to open target file\n");
            return 1;
        }

        if (do_file_open(memstr) < 0)
            return 1;

        if (do_file_seek(&data,0,2/*SEEK_END*/) < 0)
            return 1;

        count = 0L;
        file_size = data;

        if (do_file_seek(&data,0,0/*SEEK_SET*/) < 0)
            return 1;

        while (count < file_size) {
            now = time(NULL);
            if (now >= next) {
                unsigned long percent;

                percent = (count >> 7UL) * 100UL;
                percent /= ((file_size + 127UL) >> 7UL);

                next = now + 1;
                printf("\x0D" "Download, %lu%% %lu / %lu... ",
                    percent,count,file_size);
                fflush(stdout);
            }

            if ((file_size - count) > 188)
                doc = 188;
            else
                doc = (int)(file_size - count);

            assert(doc != 0);
            if ((str=do_file_read(&wd,doc)) == NULL) {
                fprintf(stderr,"\nReconnecting...\n");
                do_connect(); /* retry */

                /* now, where were we...? */
                if (do_file_seek(&data,count,0/*SEEK_SET*/) < 0 || (unsigned long)data != (unsigned long)count)
                    return 1;

                continue;
            }

            if (wd == 0) {
                fprintf(stderr,"Unexpected end of file\n");
                break;
            }
            assert(wd <= doc);

            rwd = write(ofd,str,wd);
            if (rwd == 0) break;

            if (rwd < wd) {
                fprintf(stderr,"Remote end incomplete read\n");
                break;
            }

            count += wd;
        }

        if (do_file_close() < 0)
            return 1;

        if (count != file_size) {
            printf("Download incomplete\n");
        }
        else {
            printf("Download OK\n");
        }

        close(ofd);
    }
    else if (!strcmp(command,"stuffkey")) {
        if (memstr != NULL) {
            unsigned int i;

            for (i=0;memstr[i] != 0;i++) {
                if (do_stuff_key(memstr[i] & 0xFF) < 0)
                    return 1;
            }

            if (enterkey) {
                if (do_stuff_key(13/*ENTER*/) < 0)
                    return 1;
            }

            printf("STUFF OK\n");
        }
        else if (data != 0) {
            if (do_stuff_key(data) < 0)
                return 1;

            if (enterkey) {
                if (do_stuff_key(13/*ENTER*/) < 0)
                    return 1;
            }

            printf("STUFF OK\n");
        }
        else {
            fprintf(stderr,"stuffkey needs -data or -mstr\n");
            return 1;
        }
    }
    else {
        fprintf(stderr,"Unknown command\n");
        return 1;
    }

    do_disconnect();
    return 0;
}

