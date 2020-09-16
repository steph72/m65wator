set -e

if [ ! -f "disc/wa-tor.d81" ]; then
  mkdir -p disc
  c1541 -format wa-tor,sk d81 disc/wa-tor.d81
fi

cat utils/wrapper.prg bin/wa-tor.prg > bin/wrapped.prg

c1541 <<EOF
attach disc/wa-tor.d81
delete m65wator.prg
write bin/wrapped.prg m65wator.prg
EOF
