set -e

if [ ! -f "disc/wa-tor.d81" ]; then
  mkdir -p disc
  c1541 -format wa-tor,sk d81 disc/wa-tor.d81
fi

c1541 <<EOF
attach disc/wa-tor.d81
delete wa-tor.prg
write bin/wa-tor.prg
EOF
