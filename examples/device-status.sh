#!/usr/bin/env bash
# API-Smoke-Test. Benötigt produktive HTTPS-Domain und zuvor angelegtes Gerät.
set -euo pipefail
: "${PANNE_PRO_URL:?HTTPS-Basisadresse setzen, z.B. https://panne-pro.example.de}"
: "${PANNE_PRO_TOKEN:?Geräte-API-Schlüssel setzen}"
: "${PANNE_PRO_DEVICE_ID:?Gerätekennung setzen}"
curl --fail-with-body --silent --show-error --request POST "${PANNE_PRO_URL%/}/api/v1/devices/status" \
 -H "Authorization: Bearer ${PANNE_PRO_TOKEN}" \
 -H 'Content-Type: application/json' \
 --data "{\"device_id\":\"${PANNE_PRO_DEVICE_ID}\",\"status\":\"available\",\"capacity\":1}"
printf '\n'
