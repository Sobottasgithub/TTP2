#!/usr/bin/env bash
set -e

cd lib/ttp2/src/
echo "Build asn1 packets..."
asn1Parser packets.asn1
