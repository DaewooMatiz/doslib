#!/usr/bin/perl
#
# Given the path of WIN386.EXE, list the W3 directory.
# (C) 2017 Jonathan Campbell
#
# Someday we'll even offer to decompress them...
my $win386 = shift @ARGV;
die "need win386 path" unless -f $win386;

open(W3,"<",$win386) || die;
binmode(W3);

my $raw;
my $w3ofs;
seek(W3,0x3C,0);
read(W3,$raw,4);
$w3ofs = unpack("V",$raw);

print "W3 offset at $w3ofs (".sprintf("0x%04x",$w3ofs).")\n";

seek(W3,$w3ofs,0);
read(W3,$raw,16);
die unless substr($raw,0,2) eq "W3";

# appears to take the form:
#
# +0x00   char[2]    "W3"
# +0x02   uint16_t   Windows version (0x0300 = 3.0 or 0x030A = 3.10)
# +0x04   uint16_t   Number of directory elements

my $winver = unpack("v",substr($raw,2,2));
print "This is Windows kernel ".sprintf("%u.%u",$winver >> 8,$winver & 0xFF)."\n";

# must be kernel 3.x
die "Unsupported Windows kernel" unless ($winver >> 8) == 3;

my $wincount = unpack("v",substr($raw,4,2));
print "There are $wincount files here\n";

die "Unsupported file count" unless ($wincount > 0 && $wincount < 256); # Windows 3.0 and Windows 3.1 have somewhere between 20 and 30 items

print "NOTE: Be aware the extracted files are raw LE header without MS-DOS stub!\n";

# read! Each entry is 16 bytes
$fofs = $w3ofs + 16;
for ($i=0;$i < $wincount;$i++,$fofs += 16) {
# +0x00   char[8]    Name of file
# +0x08   uint32_t   offset
# +0x0C   uint32_t   length
    seek(W3,$fofs,0);
    read(W3,$raw,16);

    my $name,$ofs,$len;
    $name = substr($raw,0,8);
    ($ofs,$len) = unpack("VV",substr($raw,8));

    $name =~ s/ +$//;
    print "    \"$name\" at @".sprintf("0x%08x",$ofs).", $len bytes\n";

    next if $len < 64;
    next if $len > (4 * 1024 * 1024); # avoid funny sizes
    next if $ofs < $w3ofs;

    # name should only be alphanumeric
    next unless $name =~ m/^[a-z0-9]+$/i;

    open(OUT,">$name.386") || die;
    binmode(OUT);

    seek(W3,$ofs,0);
    read(W3,$raw,$len);
    print OUT $raw;
    close(OUT);
}

close(W3);

